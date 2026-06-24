#ifndef LEXER_H
#define LEXER_H

/* === Token definitions === */
typedef enum {
    TOK_IDENTIFIER,
    TOK_CONSTANT,
    TOK_INT,        /* keyword int */
    TOK_VOID,       /* keyword void */
    TOK_RETURN,     /* keyword return */
    TOK_LPAREN,     /* ( */
    TOK_RPAREN,     /* ) */
    TOK_LBRACE,     /* { */
    TOK_RBRACE,     /* } */
    TOK_SEMICOLON,  /* ; */
    TOK_MINUS,      /* - (negation/subtraction) */
    TOK_COMPLEMENT, /* ~ (bitwise complement) */
    TOK_DECREMENT,  /* -- (decrement, not supported yet) */
    TOK_PLUS,       /* + */
    TOK_STAR,       /* * */
    TOK_SLASH,      /* / */
    TOK_PERCENT,    /* % */
    TOK_NOT,        /* ! */
    TOK_AND,        /* && */
    TOK_OR,         /* || */
    TOK_EQ,         /* == */
    TOK_NEQ,        /* != */
    TOK_LT,         /* < */
    TOK_GT,         /* > */
    TOK_LEQ,        /* <= */
    TOK_GEQ,        /* >= */
    TOK_EOF
} TokenType;

typedef struct Token Token;

struct Token {
    TokenType type;
    Token *next;         /* next token in list */
    char *lexeme;        /* the actual text matched */
    int value;           /* for TOK_CONSTANT */
    int line;            /* line number */
    int col;             /* column number */
};

/* Lex the source string into a linked list of tokens.
 * Returns NULL on error and prints an error message to stderr. */
Token *lex(const char *source);

/* Pretty-print token list (for debugging) */
void tokens_print(Token *head);

/* Free the token list */
void tokens_free(Token *head);

#endif /* LEXER_H */
