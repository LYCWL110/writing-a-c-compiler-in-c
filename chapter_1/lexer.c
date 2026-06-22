#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

/* Create a new token and append to list */
static Token *new_token(TokenType type, const char *lexeme, int line, int col) {
    Token *t = malloc(sizeof(Token));
    if (!t) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
    t->type = type;
    t->lexeme = strdup(lexeme);
    t->value = 0;
    t->line = line;
    t->col = col;
    t->next = NULL;
    return t;
}

/* Append token to end of list. head is a pointer to the list head pointer. */
static void append_token(Token **head, Token **tail, Token *t) {
    if (*head == NULL) {
        *head = t;
        *tail = t;
    } else {
        (*tail)->next = t;
        *tail = t;
    }
}

/* Check if a token type is a keyword and return the appropriate type.
 * Returns TOK_IDENTIFIER if not a keyword. */
static TokenType classify_keyword(const char *lexeme) {
    if (strcmp(lexeme, "int") == 0)    return TOK_INT;
    if (strcmp(lexeme, "void") == 0)   return TOK_VOID;
    if (strcmp(lexeme, "return") == 0) return TOK_RETURN;
    return TOK_IDENTIFIER;
}

/* Check if char is whitespace */
static int is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

/* Check if char can start an identifier */
static int is_ident_start(char c) {
    return isalpha(c) || c == '_';
}

/* Check if char can be part of an identifier */
static int is_ident_char(char c) {
    return isalnum(c) || c == '_';
}

/* Check if char is a digit */
static int is_digit_char(char c) {
    return isdigit(c);
}

Token *lex(const char *source) {
    Token *head = NULL;
    Token *tail = NULL;
    int pos = 0;
    int line = 1;
    int col = 1;

    while (source[pos] != '\0') {
        /* Skip whitespace */
        if (is_whitespace(source[pos])) {
            if (source[pos] == '\n') {
                line++;
                col = 1;
            } else {
                col++;
            }
            pos++;
            continue;
        }

        /* Skip single-line comments (//) */
        if (source[pos] == '/' && source[pos+1] == '/') {
            pos += 2;
            col += 2;
            while (source[pos] != '\0' && source[pos] != '\n') {
                pos++;
                col++;
            }
            continue;
        }

        /* Skip block comments (slash star ... star slash) */
        if (source[pos] == '/' && source[pos+1] == '*') {
            pos += 2;
            col += 2;
            while (source[pos] != '\0' && !(source[pos] == '*' && source[pos+1] == '/')) {
                if (source[pos] == '\n') {
                    line++;
                    col = 1;
                } else {
                    col++;
                }
                pos++;
            }
            if (source[pos] == '\0') {
                fprintf(stderr, "Lex error at line %d, col %d: unterminated block comment\n", line, col);
                tokens_free(head);
                return NULL;
            }
            /* skip star slash */
            pos += 2;
            col += 2;
            continue;
        }

        int tok_start_line = line;
        int tok_start_col = col;

        /* Two-char token: -- (decrement) — must be checked before single '-' */
        if (source[pos] == '-' && source[pos+1] == '-') {
            Token *t = new_token(TOK_DECREMENT, "--", tok_start_line, tok_start_col);
            append_token(&head, &tail, t);
            pos += 2;
            col += 2;
            continue;
        }

        /* Single-char tokens */
        switch (source[pos]) {
            case '(':
            case ')':
            case '{':
            case '}':
            case ';':
            case '~':
            case '-': {
                TokenType tt;
                switch (source[pos]) {
                    case '(': tt = TOK_LPAREN; break;
                    case ')': tt = TOK_RPAREN; break;
                    case '{': tt = TOK_LBRACE; break;
                    case '}': tt = TOK_RBRACE; break;
                    case ';': tt = TOK_SEMICOLON; break;
                    case '~': tt = TOK_COMPLEMENT; break;
                    case '-': tt = TOK_MINUS; break;
                    default:  tt = TOK_EOF; break; /* unreachable */
                }
                char lex[2] = {source[pos], '\0'};
                Token *t = new_token(tt, lex, tok_start_line, tok_start_col);
                append_token(&head, &tail, t);
                pos++;
                col++;
                continue;
            }
        }

        /* Identifier or keyword */
        if (is_ident_start(source[pos])) {
            int start = pos;
            while (source[pos] != '\0' && is_ident_char(source[pos])) {
                pos++;
                col++;
            }
            int len = pos - start;
            char *lexeme = malloc(len + 1);
            if (!lexeme) {
                fprintf(stderr, "Memory allocation error\n");
                tokens_free(head);
                return NULL;
            }
            memcpy(lexeme, &source[start], len);
            lexeme[len] = '\0';

            TokenType tt = classify_keyword(lexeme);
            Token *t = new_token(tt, lexeme, tok_start_line, tok_start_col);
            append_token(&head, &tail, t);
            free(lexeme);
            continue;
        }

        /* Integer constant */
        if (is_digit_char(source[pos])) {
            int start = pos;
            int value = 0;
            while (source[pos] != '\0' && is_digit_char(source[pos])) {
                value = value * 10 + (source[pos] - '0');
                pos++;
                col++;
            }

            /* Check word boundary: constant must not be followed by identifier chars.
             * e.g., "123foo" is invalid — the digits aren't a valid constant token. */
            if (source[pos] != '\0' && is_ident_char(source[pos])) {
                while (source[pos] != '\0' && is_ident_char(source[pos])) {
                    pos++;
                    col++;
                }
                int bad_len = pos - start;
                char *bad_lexeme = malloc(bad_len + 1);
                memcpy(bad_lexeme, &source[start], bad_len);
                bad_lexeme[bad_len] = '\0';
                fprintf(stderr, "Lex error at line %d, col %d: invalid token '%s'\n",
                        tok_start_line, tok_start_col, bad_lexeme);
                free(bad_lexeme);
                tokens_free(head);
                return NULL;
            }

            int len = pos - start;
            char *lexeme = malloc(len + 1);
            if (!lexeme) {
                fprintf(stderr, "Memory allocation error\n");
                tokens_free(head);
                return NULL;
            }
            memcpy(lexeme, &source[start], len);
            lexeme[len] = '\0';

            Token *t = new_token(TOK_CONSTANT, lexeme, tok_start_line, tok_start_col);
            t->value = value;
            append_token(&head, &tail, t);
            free(lexeme);
            continue;
        }

        /* Invalid token */
        fprintf(stderr, "Lex error at line %d, col %d: invalid token '%c' (ASCII %d)\n",
                tok_start_line, tok_start_col, source[pos], (int)source[pos]);
        tokens_free(head);
        return NULL;
    }

    /* Add EOF token */
    Token *eof = new_token(TOK_EOF, "EOF", line, col);
    append_token(&head, &tail, eof);

    return head;
}

void tokens_print(Token *head) {
    const char *type_names[] = {
        "IDENTIFIER", "CONSTANT", "INT", "VOID", "RETURN",
        "(", ")", "{", "}", ";", "-", "~", "--", "EOF"
    };
    Token *cur = head;
    while (cur) {
        printf("Token{type=%s, lexeme='%s'", type_names[cur->type], cur->lexeme);
        if (cur->type == TOK_CONSTANT) {
            printf(", value=%d", cur->value);
        }
        printf(", line=%d, col=%d}\n", cur->line, cur->col);
        cur = cur->next;
    }
}

void tokens_free(Token *head) {
    Token *cur = head;
    while (cur) {
        Token *next = cur->next;
        free(cur->lexeme);
        free(cur);
        cur = next;
    }
}
