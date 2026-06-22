#include <stdio.h>
#include <stdlib.h>
#include "tacky_gen.h"

/* Global counter for generating unique temporary variable names */
static int tmp_counter = 0;

static char *make_temporary(void) {
    char *name = malloc(32);
    snprintf(name, 32, "tmp.%d", tmp_counter++);
    return name;
}

/* Convert an AST expression to TACKY — returns the TackyVal for the result
 * and appends instructions to the instruction list.
 *
 * Ownership rules: the returned TackyVal is owned by the CALLER.
 * Instructions take ownership of their own copies of src/dst. */
static TackyVal *emit_tacky_exp(Exp *e, TackyInstruction **instructions) {
    if (e->type == AST_EXP_CONSTANT) {
        return tacky_val_constant(e->value);
    }

    if (e->type == AST_EXP_UNARY) {
        /* Recursively flatten the inner expression */
        TackyVal *src = emit_tacky_exp(e->unary.operand, instructions);

        /* Create a destination temporary variable */
        char *tmp_name = make_temporary();
        TackyVal *dst = tacky_val_var(tmp_name);
        free(tmp_name);

        /* Create the Unary instruction. The instruction takes ownership
         * of src and dst. Return a fresh copy of dst for the caller. */
        TackyInstruction *inst = tacky_inst_unary(e->unary.operator, src, dst);
        *instructions = tacky_append_instruction(*instructions, inst);

        /* Return a new Val referencing the same variable name.
         * This prevents the same pointer from being in both
         * an instruction's dst and an outer instruction's src. */
        TackyVal *result = tacky_val_var(dst->name);
        return result;
    }

    fprintf(stderr, "TACKY generation error: unsupported expression type\n");
    return NULL;
}

TackyProgram *gen_tacky(Program *prog) {
    if (!prog || !prog->function) return NULL;

    /* Reset temporary counter */
    tmp_counter = 0;

    TackyInstruction *instructions = NULL;

    /* Process the return statement */
    Statement *stmt = prog->function->body;
    if (stmt->type != AST_STATEMENT_RETURN) {
        fprintf(stderr, "TACKY generation error: unsupported statement\n");
        return NULL;
    }

    TackyVal *return_val = emit_tacky_exp(stmt->return_val, &instructions);

    /* The Return instruction takes ownership of return_val */
    TackyInstruction *ret_inst = tacky_inst_return(return_val);
    instructions = tacky_append_instruction(instructions, ret_inst);

    TackyFunction *func = tacky_function(prog->function->name, instructions);
    return tacky_program(func);
}
