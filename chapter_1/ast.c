#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

/* === Constructors === */

Exp *make_exp_constant(int value) {
    Exp *e = malloc(sizeof(Exp));
    e->type = AST_EXP_CONSTANT; e->value = value; return e;
}
Exp *make_exp_unary(UnaryOperator op, Exp *operand) {
    Exp *e = malloc(sizeof(Exp));
    e->type = AST_EXP_UNARY; e->unary.operator = op; e->unary.operand = operand; return e;
}
Exp *make_exp_binary(BinaryOperator op, Exp *left, Exp *right) {
    Exp *e = malloc(sizeof(Exp));
    e->type = AST_EXP_BINARY; e->binary.operator = op; e->binary.left = left; e->binary.right = right; return e;
}
Exp *make_exp_var(char *name) {
    Exp *e = malloc(sizeof(Exp));
    e->type = AST_EXP_VAR; e->var_name = strdup(name); return e;
}
Exp *make_exp_assignment(Exp *left, Exp *right) {
    Exp *e = malloc(sizeof(Exp));
    e->type = AST_EXP_ASSIGNMENT; e->assign.left = left; e->assign.right = right; return e;
}
Statement *make_statement_return(Exp *return_val) {
    Statement *s = malloc(sizeof(Statement));
    s->type = AST_STATEMENT_RETURN; s->return_val = return_val; return s;
}
Statement *make_statement_expression(Exp *expr) {
    Statement *s = malloc(sizeof(Statement));
    s->type = AST_STATEMENT_EXPRESSION; s->expr = expr; return s;
}
Statement *make_statement_null(void) {
    Statement *s = malloc(sizeof(Statement));
    s->type = AST_STATEMENT_NULL; return s;
}
Declaration *make_declaration(char *name, Exp *init) {
    Declaration *d = malloc(sizeof(Declaration));
    d->type = AST_DECLARATION; d->name = strdup(name); d->init = init; return d;
}
BlockItem *make_block_item_stmt(Statement *stmt) {
    BlockItem *b = malloc(sizeof(BlockItem));
    b->type = AST_BLOCK_ITEM; b->is_declaration = 0; b->stmt = stmt; return b;
}
BlockItem *make_block_item_decl(Declaration *decl) {
    BlockItem *b = malloc(sizeof(BlockItem));
    b->type = AST_BLOCK_ITEM; b->is_declaration = 1; b->decl = decl; return b;
}
FunctionDef *make_function_def(char *name, BlockItem **body, int count) {
    FunctionDef *f = malloc(sizeof(FunctionDef));
    f->type = AST_FUNCTION; f->name = strdup(name); f->body = body; f->body_count = count; return f;
}
Program *make_program(FunctionDef *function) {
    Program *p = malloc(sizeof(Program));
    p->type = AST_PROGRAM; p->function = function; return p;
}

/* === Pretty-print === */

static const char *unary_op_name(UnaryOperator op) {
    switch (op) {
        case UNARY_COMPLEMENT: return "Complement";
        case UNARY_NEGATE:     return "Negate";
        case UNARY_NOT:        return "Not";
        default:              return "?";
    }
}
static const char *binary_op_name(BinaryOperator op) {
    switch (op) {
        case BINARY_ADD: return "Add"; case BINARY_SUBTRACT: return "Subtract";
        case BINARY_MULTIPLY: return "Multiply"; case BINARY_DIVIDE: return "Divide";
        case BINARY_REMAINDER: return "Remainder"; case BINARY_AND: return "And";
        case BINARY_OR: return "Or"; case BINARY_EQUAL: return "Equal";
        case BINARY_NOT_EQUAL: return "NotEqual"; case BINARY_LESS: return "LessThan";
        case BINARY_LESS_EQ: return "LessOrEqual"; case BINARY_GREATER: return "GreaterThan";
        case BINARY_GREATER_EQ: return "GreaterOrEqual"; default: return "?";
    }
}

static void print_exp(Exp *e, int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
    switch (e->type) {
        case AST_EXP_CONSTANT: printf("Constant(%d)\n", e->value); break;
        case AST_EXP_VAR: printf("Var(\"%s\")\n", e->var_name); break;
        case AST_EXP_UNARY:
            printf("Unary(%s,\n", unary_op_name(e->unary.operator));
            print_exp(e->unary.operand, indent+1);
            for (int i = 0; i < indent; i++) printf("  "); printf(")\n"); break;
        case AST_EXP_BINARY:
            printf("Binary(%s,\n", binary_op_name(e->binary.operator));
            print_exp(e->binary.left, indent+1);
            print_exp(e->binary.right, indent+1);
            for (int i = 0; i < indent; i++) printf("  "); printf(")\n"); break;
        case AST_EXP_ASSIGNMENT:
            printf("Assignment(\n");
            print_exp(e->assign.left, indent+1);
            print_exp(e->assign.right, indent+1);
            for (int i = 0; i < indent; i++) printf("  "); printf(")\n"); break;
        default: break;
    }
}

static void print_statement(Statement *s, int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
    switch (s->type) {
        case AST_STATEMENT_RETURN: printf("Return(\n"); print_exp(s->return_val, indent+1);
            for (int i = 0; i < indent; i++) printf("  "); printf(")\n"); break;
        case AST_STATEMENT_EXPRESSION: printf("Expression(\n"); print_exp(s->expr, indent+1);
            for (int i = 0; i < indent; i++) printf("  "); printf(")\n"); break;
        case AST_STATEMENT_NULL: printf("Null\n"); break;
        default: break;
    }
}

void ast_print(Program *prog) {
    if (!prog || !prog->function) return;
    printf("Program(\n  Function(name=\"%s\",\n", prog->function->name);
    FunctionDef *f = prog->function;
    for (int i = 0; i < f->body_count; i++) {
        BlockItem *b = f->body[i];
        for (int j = 0; j < 2; j++) printf("  ");
        if (b->is_declaration) {
            printf("D(Declaration(\"%s\"", b->decl->name);
            if (b->decl->init) { printf(",\n"); print_exp(b->decl->init, 3); }
            printf("))\n");
        } else {
            printf("S("); print_statement(b->stmt, 2); printf(")");
        }
    }
    printf("  )\n)\n");
}

/* === Free === */

void ast_free_exp(Exp *e) {
    if (!e) return;
    if (e->type == AST_EXP_VAR) free(e->var_name);
    else if (e->type == AST_EXP_UNARY) ast_free_exp(e->unary.operand);
    else if (e->type == AST_EXP_BINARY) { ast_free_exp(e->binary.left); ast_free_exp(e->binary.right); }
    else if (e->type == AST_EXP_ASSIGNMENT) { ast_free_exp(e->assign.left); ast_free_exp(e->assign.right); }
    free(e);
}

void ast_free(Program *prog) {
    if (!prog) return;
    if (prog->function) {
        free(prog->function->name);
        for (int i = 0; i < prog->function->body_count; i++) {
            BlockItem *b = prog->function->body[i];
            if (b->is_declaration) {
                free(b->decl->name);
                if (b->decl->init) ast_free_exp(b->decl->init);
                free(b->decl);
            } else {
                if (b->stmt->type == AST_STATEMENT_RETURN) ast_free_exp(b->stmt->return_val);
                else if (b->stmt->type == AST_STATEMENT_EXPRESSION) ast_free_exp(b->stmt->expr);
                free(b->stmt);
            }
            free(b);
        }
        free(prog->function->body);
        free(prog->function);
    }
    free(prog);
}
