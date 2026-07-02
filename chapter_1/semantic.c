#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantic.h"

/* Simple name→name map (linear scan, fine for small programs) */
#define MAX_VARS 256
static struct { char *orig; char *uniq; } var_map[MAX_VARS];
static int var_count = 0;
static int unique_counter = 0;

static char *make_unique(const char *name) {
    char *u = malloc(strlen(name) + 16);
    snprintf(u, strlen(name)+16, "%s.%d", name, unique_counter++);
    return u;
}

static const char *resolve_var(const char *name) {
    for (int i = 0; i < var_count; i++)
        if (strcmp(var_map[i].orig, name) == 0) return var_map[i].uniq;
    return NULL;
}

static void add_var(const char *orig, const char *uniq) {
    var_map[var_count].orig = strdup(orig);
    var_map[var_count].uniq = strdup(uniq);
    var_count++;
}

static void clear_var_map(void) {
    for (int i = 0; i < var_count; i++) { free(var_map[i].orig); free(var_map[i].uniq); }
    var_count = 0;
}

/* Forward */
static Exp *resolve_exp(Exp *e);
static Statement *resolve_statement(Statement *s);
static Declaration *resolve_declaration(Declaration *d);
static BlockItem *resolve_block_item(BlockItem *b);

static Exp *resolve_exp(Exp *e) {
    if (!e) return NULL;
    switch (e->type) {
        case AST_EXP_CONSTANT: return e;
        case AST_EXP_VAR: {
            const char *uniq = resolve_var(e->var_name);
            if (!uniq) {
                fprintf(stderr, "Semantic error: undeclared variable '%s'\n", e->var_name);
                return NULL;
            }
            free(e->var_name);
            e->var_name = strdup(uniq);
            return e;
        }
        case AST_EXP_ASSIGNMENT: {
            if (e->assign.left->type != AST_EXP_VAR) {
                fprintf(stderr, "Semantic error: invalid lvalue in assignment\n");
                return NULL;
            }
            if (!resolve_exp(e->assign.left)) return NULL;
            if (!resolve_exp(e->assign.right)) return NULL;
            return e;
        }
        case AST_EXP_UNARY:
            if (!resolve_exp(e->unary.operand)) return NULL;
            return e;
        case AST_EXP_BINARY:
            if (!resolve_exp(e->binary.left)) return NULL;
            if (!resolve_exp(e->binary.right)) return NULL;
            return e;
        default: return e;
    }
}

static Statement *resolve_statement(Statement *s) {
    if (!s) return NULL;
    if (s->type == AST_STATEMENT_RETURN) {
        if (!resolve_exp(s->return_val)) return NULL;
    } else if (s->type == AST_STATEMENT_EXPRESSION) {
        if (!resolve_exp(s->expr)) return NULL;
    }
    return s;
}

static Declaration *resolve_declaration(Declaration *d) {
    if (resolve_var(d->name)) {
        fprintf(stderr, "Semantic error: duplicate variable declaration '%s'\n", d->name);
        return NULL;
    }
    char *uniq = make_unique(d->name);
    add_var(d->name, uniq);
    if (d->init && !resolve_exp(d->init)) return NULL;
    free(d->name);
    d->name = uniq; /* transfer ownership */
    return d;
}

static BlockItem *resolve_block_item(BlockItem *b) {
    if (b->is_declaration) {
        if (!resolve_declaration(b->decl)) return NULL;
    } else {
        if (!resolve_statement(b->stmt)) return NULL;
    }
    return b;
}

Program *semantic_analysis(Program *prog) {
    if (!prog || !prog->function) return NULL;
    var_count = 0;
    FunctionDef *f = prog->function;
    for (int i = 0; i < f->body_count; i++) {
        if (!resolve_block_item(f->body[i])) {
            clear_var_map(); return NULL;
        }
    }
    return prog; /* modified in-place */
}
