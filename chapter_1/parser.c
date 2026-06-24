#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "ast.h"

typedef struct { Token *current; } ParserState;

static int expect(ParserState *state, TokenType expected, const char *ctx) {
    if (state->current->type != expected) {
        const char *type_names[] = {
            "identifier", "constant", "'int'", "'void'", "'return'",
            "'('", "')'", "'{'", "'}'", "';'", "'-'", "'~'", "'--'",
            "'+'", "'*'", "'/'", "'%'", "EOF"
        };
        fprintf(stderr, "Parse error at line %d, col %d: Expected %s but found '%s'\n",
                state->current->line, state->current->col,
                type_names[expected], state->current->lexeme);
        (void)ctx;
        return 0;
    }
    state->current = state->current->next;
    return 1;
}

/* Forward declarations */
static Exp *parse_exp(ParserState *state, int min_prec);
static Exp *parse_factor(ParserState *state);
static Statement *parse_statement(ParserState *state);
static FunctionDef *parse_function(ParserState *state);

/* ================================================================
 * Precedence climbing helpers
 * ================================================================ */

static int precedence(TokenType t) {
    switch (t) {
        case TOK_STAR:    case TOK_SLASH: case TOK_PERCENT: return 50;
        case TOK_PLUS:    case TOK_MINUS:                    return 45;
        default:                                              return -1;
    }
}

static BinaryOperator token_to_binary_op(TokenType t) {
    switch (t) {
        case TOK_PLUS:     return BINARY_ADD;
        case TOK_MINUS:    return BINARY_SUBTRACT;
        case TOK_STAR:     return BINARY_MULTIPLY;
        case TOK_SLASH:    return BINARY_DIVIDE;
        case TOK_PERCENT:  return BINARY_REMAINDER;
        default:           return BINARY_ADD; /* unreachable */
    }
}

/* ================================================================
 * parse_factor — constants, unary ops, parenthesized expressions
 * ================================================================ */

static Exp *parse_factor(ParserState *state) {
    TokenType t = state->current->type;

    /* Constant */
    if (t == TOK_CONSTANT) {
        int val = state->current->value;
        state->current = state->current->next;
        return make_exp_constant(val);
    }

    /* Unary operator */
    if (t == TOK_MINUS || t == TOK_COMPLEMENT) {
        UnaryOperator op = (t == TOK_MINUS) ? UNARY_NEGATE : UNARY_COMPLEMENT;
        state->current = state->current->next;
        Exp *inner = parse_factor(state);
        if (!inner) return NULL;
        return make_exp_unary(op, inner);
    }

    /* Parenthesized expression (may contain binary ops) */
    if (t == TOK_LPAREN) {
        state->current = state->current->next;
        Exp *inner = parse_exp(state, 0);  /* reset min_prec to 0 */
        if (!inner) return NULL;
        if (!expect(state, TOK_RPAREN, "parenthesized expression")) {
            ast_free_exp(inner);
            return NULL;
        }
        return inner;
    }

    /* Decrement */
    if (t == TOK_DECREMENT) {
        fprintf(stderr, "Parse error at line %d, col %d: '--' is not supported\n",
                state->current->line, state->current->col);
        return NULL;
    }

    fprintf(stderr, "Parse error at line %d, col %d: Expected expression but found '%s'\n",
            state->current->line, state->current->col, state->current->lexeme);
    return NULL;
}

/* ================================================================
 * parse_exp — precedence climbing for binary expressions
 * ================================================================ */

static Exp *parse_exp(ParserState *state, int min_prec) {
    Exp *left = parse_factor(state);
    if (!left) return NULL;

    TokenType next = state->current->type;

    while (precedence(next) >= min_prec) {
        /* consume the operator */
        BinaryOperator binop = token_to_binary_op(next);
        state->current = state->current->next;

        /* parse right subexpression with higher minimum precedence */
        Exp *right = parse_exp(state, precedence(next) + 1);
        if (!right) {
            ast_free_exp(left);
            return NULL;
        }

        left = make_exp_binary(binop, left, right);
        next = state->current->type;
    }
    return left;
}

/* ================================================================
 * parse_statement — "return" <exp> ";"
 * ================================================================ */

static Statement *parse_statement(ParserState *state) {
    if (!expect(state, TOK_RETURN, "statement")) return NULL;
    Exp *return_val = parse_exp(state, 0);
    if (!return_val) return NULL;
    if (!expect(state, TOK_SEMICOLON, "statement")) {
        ast_free_exp(return_val);
        return NULL;
    }
    return make_statement_return(return_val);
}

/* ================================================================
 * parse_function — "int" <id> "(" "void" ")" "{" <statement> "}"
 * ================================================================ */

static FunctionDef *parse_function(ParserState *state) {
    if (!expect(state, TOK_INT, "function")) return NULL;
    if (state->current->type != TOK_IDENTIFIER) {
        fprintf(stderr, "Parse error at line %d, col %d: Expected function name but found '%s'\n",
                state->current->line, state->current->col, state->current->lexeme);
        return NULL;
    }
    char *func_name = strdup(state->current->lexeme);
    state->current = state->current->next;
    if (!expect(state, TOK_LPAREN, "function")) { free(func_name); return NULL; }
    if (!expect(state, TOK_VOID, "function")) { free(func_name); return NULL; }
    if (!expect(state, TOK_RPAREN, "function")) { free(func_name); return NULL; }
    if (!expect(state, TOK_LBRACE, "function")) { free(func_name); return NULL; }

    Statement *body = parse_statement(state);
    if (!body) { free(func_name); return NULL; }

    if (!expect(state, TOK_RBRACE, "function")) {
        free(func_name);
        ast_free_exp(body->return_val);
        free(body);
        return NULL;
    }
    FunctionDef *func = make_function_def(func_name, body);
    free(func_name);
    return func;
}

/* ================================================================
 * parse — entry point
 * ================================================================ */

Program *parse(Token *tokens) {
    if (!tokens) return NULL;
    ParserState state;
    state.current = tokens;
    FunctionDef *func = parse_function(&state);
    if (!func) return NULL;
    if (state.current->type != TOK_EOF) {
        fprintf(stderr, "Parse error at line %d, col %d: Unexpected token '%s' after end of program\n",
                state.current->line, state.current->col, state.current->lexeme);
        if (func->body) {
            ast_free_exp(func->body->return_val);
            free(func->body);
        }
        free(func->name);
        free(func);
        return NULL;
    }
    return make_program(func);
}
