#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "ast.h"

typedef struct { Token *current; } ParserState;

static int expect(ParserState *state, TokenType expected, const char *ctx) {
    if (state->current->type != expected) {
        const char *type_names[] = {
            "identifier","constant","'int'","'void'","'return'",
            "'('","')'","'{'","'}'","';'","'-'","'~'","'--'",
            "'+'","'*'","'/'","'%'","'!'","'&&'","'||'","'=='","'!='",
            "'<'","'>'","'<='","'>='","'='","EOF"
        };
        fprintf(stderr, "Parse error at line %d, col %d: Expected %s but found '%s'\n",
                state->current->line, state->current->col,
                type_names[expected], state->current->lexeme);
        (void)ctx; return 0;
    }
    state->current = state->current->next; return 1;
}

static Exp *parse_exp(ParserState *state, int min_prec);
static Exp *parse_factor(ParserState *state);
static Statement *parse_statement(ParserState *state);
static Declaration *parse_declaration(ParserState *state);
static BlockItem *parse_block_item(ParserState *state);

static int precedence(TokenType t) {
    switch (t) {
        case TOK_STAR: case TOK_SLASH: case TOK_PERCENT: return 50;
        case TOK_PLUS: case TOK_MINUS: return 45;
        case TOK_LT: case TOK_GT: case TOK_LEQ: case TOK_GEQ: return 35;
        case TOK_EQ: case TOK_NEQ: return 30;
        case TOK_AND: return 10;
        case TOK_OR:  return 5;
        case TOK_ASSIGN: return 1;
        default: return -1;
    }
}

static BinaryOperator token_to_binary_op(TokenType t) {
    switch (t) {
        case TOK_PLUS: return BINARY_ADD; case TOK_MINUS: return BINARY_SUBTRACT;
        case TOK_STAR: return BINARY_MULTIPLY; case TOK_SLASH: return BINARY_DIVIDE;
        case TOK_PERCENT: return BINARY_REMAINDER; case TOK_AND: return BINARY_AND;
        case TOK_OR: return BINARY_OR; case TOK_EQ: return BINARY_EQUAL;
        case TOK_NEQ: return BINARY_NOT_EQUAL; case TOK_LT: return BINARY_LESS;
        case TOK_LEQ: return BINARY_LESS_EQ; case TOK_GT: return BINARY_GREATER;
        case TOK_GEQ: return BINARY_GREATER_EQ;
        default: return BINARY_ADD;
    }
}

/* ================================================================ */
static Exp *parse_factor(ParserState *state) {
    TokenType t = state->current->type;
    if (t == TOK_CONSTANT) {
        int val = state->current->value; state->current = state->current->next;
        return make_exp_constant(val);
    }
    if (t == TOK_IDENTIFIER) {
        char *lex = state->current->lexeme;
        state->current = state->current->next;
        return make_exp_var(lex); /* make_exp_var strdups internally */
    }
    if (t == TOK_MINUS || t == TOK_COMPLEMENT || t == TOK_NOT) {
        UnaryOperator op;
        if (t == TOK_MINUS) op = UNARY_NEGATE;
        else if (t == TOK_COMPLEMENT) op = UNARY_COMPLEMENT;
        else op = UNARY_NOT;
        state->current = state->current->next;
        Exp *inner = parse_factor(state);
        if (!inner) return NULL;
        return make_exp_unary(op, inner);
    }
    if (t == TOK_LPAREN) {
        state->current = state->current->next;
        Exp *inner = parse_exp(state, 0);
        if (!inner) return NULL;
        if (!expect(state, TOK_RPAREN, "factor")) { ast_free_exp(inner); return NULL; }
        return inner;
    }
    if (t == TOK_DECREMENT) {
        fprintf(stderr, "Parse error at line %d, col %d: '--' is not supported\n", state->current->line, state->current->col);
        return NULL;
    }
    fprintf(stderr, "Parse error at line %d, col %d: Expected expression but found '%s'\n", state->current->line, state->current->col, state->current->lexeme);
    return NULL;
}

/* Precedence climbing for binary + assignment (right-associative) */
static Exp *parse_exp(ParserState *state, int min_prec) {
    Exp *left = parse_factor(state);
    if (!left) return NULL;
    TokenType next = state->current->type;
    while (precedence(next) >= min_prec) {
        if (next == TOK_ASSIGN) {
            state->current = state->current->next;
            /* Right-associative: use same precedence as min for right side */
            Exp *right = parse_exp(state, precedence(next));
            if (!right) { ast_free_exp(left); return NULL; }
            left = make_exp_assignment(left, right);
        } else {
            BinaryOperator binop = token_to_binary_op(next);
            state->current = state->current->next;
            Exp *right = parse_exp(state, precedence(next) + 1);
            if (!right) { ast_free_exp(left); return NULL; }
            left = make_exp_binary(binop, left, right);
        }
        next = state->current->type;
    }
    return left;
}

/* ================================================================ */
static Statement *parse_statement(ParserState *state) {
    TokenType t = state->current->type;
    if (t == TOK_RETURN) {
        state->current = state->current->next;
        Exp *e = parse_exp(state, 0);
        if (!e) return NULL;
        if (!expect(state, TOK_SEMICOLON, "return")) { ast_free_exp(e); return NULL; }
        return make_statement_return(e);
    }
    if (t == TOK_SEMICOLON) {
        state->current = state->current->next;
        return make_statement_null();
    }
    /* Expression statement */
    Exp *e = parse_exp(state, 0);
    if (!e) return NULL;
    if (!expect(state, TOK_SEMICOLON, "expression statement")) { ast_free_exp(e); return NULL; }
    return make_statement_expression(e);
}

/* ================================================================ */
static Declaration *parse_declaration(ParserState *state) {
    if (!expect(state, TOK_INT, "declaration")) return NULL;
    if (state->current->type != TOK_IDENTIFIER) {
        fprintf(stderr, "Parse error at line %d, col %d: Expected variable name\n", state->current->line, state->current->col);
        return NULL;
    }
    char *lex = state->current->lexeme;
    state->current = state->current->next;
    Exp *init = NULL;
    if (state->current->type == TOK_ASSIGN) {
        state->current = state->current->next;
        init = parse_exp(state, 0);
        if (!init) return NULL;
    }
    if (!expect(state, TOK_SEMICOLON, "declaration")) { if (init) ast_free_exp(init); return NULL; }
    return make_declaration(lex, init); /* make_declaration strdups */
}

static BlockItem *parse_block_item(ParserState *state) {
    if (state->current->type == TOK_INT) {
        Declaration *d = parse_declaration(state);
        if (!d) return NULL;
        return make_block_item_decl(d);
    }
    Statement *s = parse_statement(state);
    if (!s) return NULL;
    return make_block_item_stmt(s);
}

/* ================================================================ */
static FunctionDef *parse_function(ParserState *state) {
    if (!expect(state, TOK_INT, "function")) return NULL;
    if (state->current->type != TOK_IDENTIFIER) {
        fprintf(stderr, "Parse error at line %d, col %d: Expected function name\n", state->current->line, state->current->col);
        return NULL;
    }
    char *func_name = strdup(state->current->lexeme);
    state->current = state->current->next;
    if (!expect(state, TOK_LPAREN, "function")) { free(func_name); return NULL; }
    if (!expect(state, TOK_VOID, "function")) { free(func_name); return NULL; }
    if (!expect(state, TOK_RPAREN, "function")) { free(func_name); return NULL; }
    if (!expect(state, TOK_LBRACE, "function")) { free(func_name); return NULL; }

    /* Parse block items until '}' */
    int cap = 8, count = 0;
    BlockItem **body = malloc(cap * sizeof(BlockItem*));
    while (state->current->type != TOK_RBRACE) {
        if (count >= cap) { cap *= 2; body = realloc(body, cap * sizeof(BlockItem*)); }
        BlockItem *bi = parse_block_item(state);
        if (!bi) { free(func_name); free(body); return NULL; }
        body[count++] = bi;
    }
    state->current = state->current->next; /* consume '}' */

    FunctionDef *f = make_function_def(func_name, body, count);
    free(func_name);
    return f;
}

Program *parse(Token *tokens) {
    if (!tokens) return NULL;
    ParserState state; state.current = tokens;
    FunctionDef *func = parse_function(&state);
    if (!func) return NULL;
    if (state.current->type != TOK_EOF) {
        fprintf(stderr, "Parse error at line %d, col %d: Unexpected token '%s' after end of program\n", state.current->line, state.current->col, state.current->lexeme);
        ast_free(make_program(func)); return NULL;
    }
    return make_program(func);
}
