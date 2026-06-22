#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "ast.h"

/* Track current position in token list */
typedef struct {
    Token *current;
} ParserState;

/* Helper: expect a token type and advance. Print error and exit if not found. */
static int expect(ParserState *state, TokenType expected, const char *ctx) {
    if (state->current->type != expected) {
        const char *type_names[] = {
            "identifier", "constant", "'int'", "'void'", "'return'",
            "'('", "')'", "'{'", "'}'", "';'", "'-'", "'~'", "'--'", "EOF"
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
static Exp *parse_exp(ParserState *state);
static Statement *parse_statement(ParserState *state);
static FunctionDef *parse_function(ParserState *state);

/* <exp> ::= <int> | <unop> <exp> | "(" <exp> ")" */
static Exp *parse_exp(ParserState *state) {
    TokenType t = state->current->type;

    /* Constant integer */
    if (t == TOK_CONSTANT) {
        int val = state->current->value;
        state->current = state->current->next;
        return make_exp_constant(val);
    }

    /* Unary operator */
    if (t == TOK_MINUS || t == TOK_COMPLEMENT) {
        UnaryOperator op = (t == TOK_MINUS) ? UNARY_NEGATE : UNARY_COMPLEMENT;
        state->current = state->current->next;  /* consume the operator */
        Exp *inner = parse_exp(state);
        if (!inner) return NULL;
        return make_exp_unary(op, inner);
    }

    /* Parenthesized expression */
    if (t == TOK_LPAREN) {
        state->current = state->current->next;  /* consume '(' */
        Exp *inner = parse_exp(state);
        if (!inner) return NULL;
        if (!expect(state, TOK_RPAREN, "parenthesized expression")) {
            ast_free_exp(inner);
            return NULL;
        }
        return inner;
    }

    /* Decrement operator — not supported yet */
    if (t == TOK_DECREMENT) {
        fprintf(stderr, "Parse error at line %d, col %d: '--' is not supported\n",
                state->current->line, state->current->col);
        return NULL;
    }

    /* Unknown token */
    fprintf(stderr, "Parse error at line %d, col %d: Expected expression but found '%s'\n",
            state->current->line, state->current->col, state->current->lexeme);
    return NULL;
}

/* <statement> ::= "return" <exp> ";" */
static Statement *parse_statement(ParserState *state) {
    if (!expect(state, TOK_RETURN, "statement")) return NULL;

    Exp *return_val = parse_exp(state);
    if (!return_val) return NULL;

    if (!expect(state, TOK_SEMICOLON, "statement")) {
        ast_free_exp(return_val);
        return NULL;
    }

    return make_statement_return(return_val);
}

/* <function> ::= "int" <identifier> "(" "void" ")" "{" <statement> "}" */
static FunctionDef *parse_function(ParserState *state) {
    if (!expect(state, TOK_INT, "function")) return NULL;

    /* function name must be an identifier */
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

/* <program> ::= <function> */
Program *parse(Token *tokens) {
    if (!tokens) return NULL;

    ParserState state;
    state.current = tokens;

    FunctionDef *func = parse_function(&state);
    if (!func) return NULL;

    /* Ensure no extra tokens after the function */
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
