#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

/* Track current position in token list */
typedef struct {
    Token *current;
} ParserState;

/* Helper: expect a token type and advance. Print error and exit if not found. */
static int expect(ParserState *state, TokenType expected, const char *ctx) {
    if (state->current->type != expected) {
        const char *type_names[] = {
            "identifier", "constant", "'int'", "'void'", "'return'",
            "'('", "')'", "'{'", "'}'", "';'", "EOF"
        };
        fprintf(stderr, "Parse error at line %d, col %d: Expected %s but found '%s'\n",
                state->current->line, state->current->col,
                type_names[expected], state->current->lexeme);
        return 0;
    }
    state->current = state->current->next;
    return 1;
}

/* Forward declarations */
static Exp *parse_exp(ParserState *state);
static Statement *parse_statement(ParserState *state);
static FunctionDef *parse_function(ParserState *state);

/* <exp> ::= <int> */
static Exp *parse_exp(ParserState *state) {
    if (state->current->type != TOK_CONSTANT) {
        fprintf(stderr, "Parse error at line %d, col %d: Expected constant but found '%s'\n",
                state->current->line, state->current->col, state->current->lexeme);
        return NULL;
    }
    int val = state->current->value;
    state->current = state->current->next;
    return make_exp_constant(val);
}

/* <statement> ::= "return" <exp> ";" */
static Statement *parse_statement(ParserState *state) {
    if (!expect(state, TOK_RETURN, "statement")) return NULL;

    Exp *return_val = parse_exp(state);
    if (!return_val) return NULL;

    if (!expect(state, TOK_SEMICOLON, "statement")) {
        /* free the expression we already parsed */
        free(return_val);
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
        free(body->return_val);
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

    /* Skip leading comments/whitespace — already handled by lexer */

    FunctionDef *func = parse_function(&state);
    if (!func) return NULL;

    /* Ensure no extra tokens after the function */
    if (state.current->type != TOK_EOF) {
        fprintf(stderr, "Parse error at line %d, col %d: Unexpected token '%s' after end of program\n",
                state.current->line, state.current->col, state.current->lexeme);
        /* free AST */
        if (func->body) {
            free(func->body->return_val);
            free(func->body);
        }
        free(func->name);
        free(func);
        return NULL;
    }

    return make_program(func);
}
