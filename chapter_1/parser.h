#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"

/* Parse a token list into an AST.
 * Returns NULL on error and prints an error message to stderr. */
Program *parse(Token *tokens);

#endif /* PARSER_H */
