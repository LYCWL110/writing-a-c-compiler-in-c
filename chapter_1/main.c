#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/wait.h>
#include "lexer.h"
#include "parser.h"
#include "tacky_gen.h"
#include "semantic.h"
#include "codegen.h"
#include "emit.h"
#include "ast.h"
#include "asm_ast.h"

/* Read entire file into a string. Returns NULL on error. */
static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc(size + 1);
    if (!buf) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(f);
        return NULL;
    }
    size_t n = fread(buf, 1, size, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

/* Build path by replacing extension: "foo.c" → "foo.i" or "foo.s" or "foo" */
static char *replace_ext(const char *path, const char *new_ext) {
    char *cpy = strdup(path);
    char *dot = strrchr(cpy, '.');
    if (dot) *dot = '\0';

    size_t len = strlen(cpy) + strlen(new_ext) + 2;
    char *result = malloc(len);
    if (new_ext[0] == '\0') {
        snprintf(result, len, "%s", cpy);
    } else {
        snprintf(result, len, "%s.%s", cpy, new_ext);
    }
    free(cpy);
    return result;
}

/* Run a command and check its return code */
static int run_cmd(const char *cmd) {
    int ret = system(cmd);
    if (ret != 0) {
        if (WIFEXITED(ret)) {
            int exit_code = WEXITSTATUS(ret);
            if (exit_code != 0) {
                fprintf(stderr, "Error: Command failed with exit code %d: %s\n", exit_code, cmd);
                return exit_code;
            }
        } else {
            fprintf(stderr, "Error: Command terminated abnormally: %s\n", cmd);
            return 1;
        }
    }
    return 0;
}

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options] <source.c>\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  --lex      Stop after lexing\n");
    fprintf(stderr, "  --parse    Stop after parsing\n");
    fprintf(stderr, "  --validate Stop after semantic analysis\n");
    fprintf(stderr, "  --tacky    Stop after TACKY generation\n");
    fprintf(stderr, "  --codegen  Stop after code generation\n");
    fprintf(stderr, "  -S         Output assembly file only, don't assemble or link\n");
}

int main(int argc, char **argv) {
    int stop_lex = 0;
    int stop_parse = 0;
    int stop_validate = 0;
    int stop_tacky = 0;
    int stop_codegen = 0;
    int emit_asm_only = 0;
    char *source_file = NULL;

    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--lex") == 0) {
            stop_lex = 1;
        } else if (strcmp(argv[i], "--parse") == 0) {
            stop_parse = 1;
        } else if (strcmp(argv[i], "--validate") == 0) {
            stop_validate = 1;
        } else if (strcmp(argv[i], "--tacky") == 0) {
            stop_tacky = 1;
        } else if (strcmp(argv[i], "--codegen") == 0) {
            stop_codegen = 1;
        } else if (strcmp(argv[i], "-S") == 0) {
            emit_asm_only = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        } else {
            source_file = argv[i];
        }
    }

    if (!source_file) {
        fprintf(stderr, "Error: No source file specified\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Step 1: Preprocess with GCC */
    char *preprocessed_file = replace_ext(source_file, "i");
    {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "gcc -E -P '%s' -o '%s'", source_file, preprocessed_file);
        if (run_cmd(cmd) != 0) {
            free(preprocessed_file);
            return 1;
        }
    }

    /* Read preprocessed source */
    char *source = read_file(preprocessed_file);
    if (!source) {
        free(preprocessed_file);
        return 1;
    }

    /* Step 2: Compile pipeline: Lex → Parse → TACKY → Codegen → Emit */

    /* Lex */
    Token *tokens = lex(source);
    if (!tokens) { free(source); free(preprocessed_file); return 1; }

    if (stop_lex) {
        tokens_free(tokens); free(source); remove(preprocessed_file);
        free(preprocessed_file); return 0;
    }

    /* Parse */
    Program *ast = parse(tokens);
    if (!ast) {
        tokens_free(tokens); free(source); remove(preprocessed_file);
        free(preprocessed_file); return 1;
    }

    if (stop_parse) {
        ast_free(ast); tokens_free(tokens); free(source);
        remove(preprocessed_file); free(preprocessed_file); return 0;
    }

    /* Semantic analysis */
    ast = semantic_analysis(ast);
    if (!ast) {
        tokens_free(tokens); free(source); remove(preprocessed_file);
        free(preprocessed_file); return 1;
    }

    if (stop_validate) {
        ast_free(ast); tokens_free(tokens); free(source);
        remove(preprocessed_file); free(preprocessed_file); return 0;
    }

    /* TACKY generation: C AST → TACKY IR */
    TackyProgram *tacky = gen_tacky(ast);
    if (!tacky) {
        ast_free(ast); tokens_free(tokens); free(source);
        remove(preprocessed_file); free(preprocessed_file); return 1;
    }

    if (stop_tacky) {
        tacky_free(tacky); ast_free(ast); tokens_free(tokens);
        free(source); remove(preprocessed_file); free(preprocessed_file);
        return 0;
    }

    /* Code generation: TACKY → Assembly AST (3 sub-stages) */
    AsmProgram *asm_prog = codegen(tacky);
    if (!asm_prog) {
        tacky_free(tacky); ast_free(ast); tokens_free(tokens);
        free(source); remove(preprocessed_file); free(preprocessed_file);
        return 1;
    }

    if (stop_codegen) {
        asm_ast_free(asm_prog); tacky_free(tacky); ast_free(ast);
        tokens_free(tokens); free(source); remove(preprocessed_file);
        free(preprocessed_file); return 0;
    }

    /* Emit: Assembly AST → .s file */
    char *asm_file = replace_ext(source_file, "s");
    FILE *asm_out = fopen(asm_file, "w");
    if (!asm_out) {
        fprintf(stderr, "Error: Cannot write to '%s'\n", asm_file);
        asm_ast_free(asm_prog); tacky_free(tacky); ast_free(ast);
        tokens_free(tokens); free(source); remove(preprocessed_file);
        free(preprocessed_file); free(asm_file); return 1;
    }
    emit(asm_out, asm_prog);
    fclose(asm_out);

    /* Cleanup */
    asm_ast_free(asm_prog); tacky_free(tacky); ast_free(ast);
    tokens_free(tokens); free(source);
    remove(preprocessed_file); free(preprocessed_file);

    /* If -S flag, stop here, keep .s file */
    if (emit_asm_only) { free(asm_file); return 0; }

    /* Step 3: Assemble and link */
    char *exe_file = replace_ext(source_file, "");
    {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "gcc '%s' -o '%s'", asm_file, exe_file);
        int ret = run_cmd(cmd);
        if (ret != 0) { free(asm_file); free(exe_file); return ret; }
    }

    remove(asm_file);
    free(asm_file); free(exe_file);
    return 0;
}
