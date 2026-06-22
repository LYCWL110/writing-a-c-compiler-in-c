#ifndef EMIT_H
#define EMIT_H

#include "asm_ast.h"

/* Write assembly AST to the given output file stream (e.g., stdout or a file).
 * Uses AT&T x64 syntax. */
void emit(FILE *out, AsmProgram *prog);

#endif  EMIT_H 
