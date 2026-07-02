#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "ast.h"

/* Variable resolution: renames variables to unique names,
 * detects duplicate declarations and undeclared variables,
 * validates assignment lvalues. */
Program *semantic_analysis(Program *prog);

#endif
