#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

/* === Constructors === */

Exp *make_exp_constant(int value) {
    Exp *e = malloc(sizeof(Exp));
    if (!e) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
    e->type = AST_EXP_CONSTANT;
    e->value = value;
    return e;
}

Statement *make_statement_return(Exp *return_val) {
    Statement *s = malloc(sizeof(Statement));
    if (!s) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
    s->type = AST_STATEMENT_RETURN;
    s->return_val = return_val;
    return s;
}

FunctionDef *make_function_def(char *name, Statement *body) {
    FunctionDef *f = malloc(sizeof(FunctionDef));
    if (!f) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
    f->type = AST_FUNCTION;
    f->name = strdup(name);
    f->body = body;
    return f;
}

Program *make_program(FunctionDef *function) {
    Program *p = malloc(sizeof(Program));
    if (!p) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
    p->type = AST_PROGRAM;
    p->function = function;
    return p;
}

/* === Pretty-print === */

static void ast_print_exp(Exp *e, int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
    printf("Constant(%d)\n", e->value);
}

static void ast_print_statement(Statement *s, int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
    printf("Return(\n");
    ast_print_exp(s->return_val, indent + 1);
    for (int i = 0; i < indent; i++) printf("  ");
    printf(")\n");
}

void ast_print(Program *prog) {
    printf("Program(\n");
    printf("  Function(\n");
    printf("    name=\"%s\",\n", prog->function->name);
    printf("    body=");
    ast_print_statement(prog->function->body, 2);
    printf("  )\n");
    printf(")\n");
}

/* === Free === */

static void ast_free_exp(Exp *e) {
    if (e) free(e);
}

static void ast_free_statement(Statement *s) {
    if (s) {
        ast_free_exp(s->return_val);
        free(s);
    }
}

static void ast_free_function(FunctionDef *f) {
    if (f) {
        free(f->name);
        ast_free_statement(f->body);
        free(f);
    }
}

void ast_free(Program *prog) {
    if (prog) {
        ast_free_function(prog->function);
        free(prog);
    }
}
