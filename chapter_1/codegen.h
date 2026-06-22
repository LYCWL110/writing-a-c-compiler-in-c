#ifndef CODEGEN_H
#define CODEGEN_H

#include "tacky.h"
#include "asm_ast.h"

/* Convert TACKY IR to Assembly AST through three sub-stages:
 *   (A) TACKY → Assembly (with pseudoregisters)
 *   (B) Replace pseudos with stack addresses
 *   (C) Fix invalid instructions (insert AllocateStack, rewrite mem-to-mem movs)
 */
AsmProgram *codegen(TackyProgram *tacky);

#endif /* CODEGEN_H */
