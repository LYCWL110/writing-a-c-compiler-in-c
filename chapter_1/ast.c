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

Exp *make_exp_unary(UnaryOperator op, Exp *operand) {
    Exp *e = malloc(sizeof(Exp));
    if (!e) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
    e->type = AST_EXP_UNARY;
    e->unary.operator = op;
    e->unary.operand = operand;
    return e;
}

Exp *make_exp_binary(BinaryOperator op, Exp *left, Exp *right) {
    Exp *e = malloc(sizeof(Exp));
    if (!e) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
    e->type = AST_EXP_BINARY;
    e->binary.operator = op;
    e->binary.left = left;
    e->binary.right = right;
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
        case BINARY_ADD:          return "Add";
        case BINARY_SUBTRACT:     return "Subtract";
        case BINARY_MULTIPLY:     return "Multiply";
        case BINARY_DIVIDE:       return "Divide";
        case BINARY_REMAINDER:    return "Remainder";
        case BINARY_AND:          return "And";
        case BINARY_OR:           return "Or";
        case BINARY_EQUAL:        return "Equal";
        case BINARY_NOT_EQUAL:    return "NotEqual";
        case BINARY_LESS:         return "LessThan";
        case BINARY_LESS_EQ:      return "LessOrEqual";
        case BINARY_GREATER:      return "GreaterThan";
        case BINARY_GREATER_EQ:   return "GreaterOrEqual";
        default:                  return "?";
    }
}

static void ast_print_exp(Exp *e, int indent) {
    for (int i = 0; i < indent; i++) printf("  ");
    if (e->type == AST_EXP_CONSTANT) {
        printf("Constant(%d)\n", e->value);
    } else if (e->type == AST_EXP_UNARY) {
        printf("Unary(%s,\n", unary_op_name(e->unary.operator));
        ast_print_exp(e->unary.operand, indent + 1);
        for (int i = 0; i < indent; i++) printf("  ");
        printf(")\n");
    } else {
        printf("Binary(%s,\n", binary_op_name(e->binary.operator));
        ast_print_exp(e->binary.left, indent + 1);
        ast_print_exp(e->binary.right, indent + 1);
        for (int i = 0; i < indent; i++) printf("  ");
        printf(")\n");
    }
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

void ast_free_exp(Exp *e) {
    if (!e) return;
    if (e->type == AST_EXP_UNARY) {
        ast_free_exp(e->unary.operand);
    } else if (e->type == AST_EXP_BINARY) {
        ast_free_exp(e->binary.left);
        ast_free_exp(e->binary.right);
    }
    free(e);
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
