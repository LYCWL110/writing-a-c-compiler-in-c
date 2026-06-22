#ifndef CODEGEN_H
#define CODEGEN_H

#include "ast.h"
#include "asm_ast.h"

/* Convert C AST to Assembly AST */
AsmProgram *codegen(Program *prog);

#endif /* CODEGEN_H */
