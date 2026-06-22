#ifndef TACKY_GEN_H
#define TACKY_GEN_H

#include "ast.h"
#include "tacky.h"

/* Convert C AST to TACKY IR */
TackyProgram *gen_tacky(Program *prog);

#endif /* TACKY_GEN_H */
