#include <stdio.h>
#include <stdlib.h>
#include "tacky_gen.h"

static int tmp_counter = 0;
static int label_counter = 0;

static char *make_temporary(void) {
    char *name = malloc(32);
    snprintf(name, 32, "tmp.%d", tmp_counter++);
    return name;
}

static char *make_label(const char *prefix) {
    char *name = malloc(64);
    snprintf(name, 64, "%s.%d", prefix, label_counter++);
    return name;
}

static TackyVal *emit_tacky_exp(Exp *e, TackyInstruction **instructions);

/* ================================================================
 * Short-circuit logic for && and ||
 * ================================================================ */

static TackyVal *emit_tacky_and(Exp *left, Exp *right,
                                 TackyInstruction **instructions) {
    char *false_label = make_label("and_false");
    char *end_label   = make_label("and_end");

    TackyVal *v1 = emit_tacky_exp(left, instructions);
    *instructions = tacky_append_instruction(*instructions,
        tacky_inst_jump_if_zero(v1, false_label));

    TackyVal *v2 = emit_tacky_exp(right, instructions);
    *instructions = tacky_append_instruction(*instructions,
        tacky_inst_jump_if_zero(v2, false_label));

    /* Both true: result = 1 */
    char *tmp_name = make_temporary();
    TackyVal *dst = tacky_val_var(tmp_name);
    *instructions = tacky_append_instruction(*instructions,
        tacky_inst_copy(tacky_val_constant(1), dst));
    *instructions = tacky_append_instruction(*instructions,
        tacky_inst_jump(end_label));

    /* false_label: result = 0 */
    *instructions = tacky_append_instruction(*instructions,
        tacky_inst_label(false_label));
    TackyVal *dst2 = tacky_val_var(tmp_name); /* fresh copy for second Copy */
    *instructions = tacky_append_instruction(*instructions,
        tacky_inst_copy(tacky_val_constant(0), dst2));

    /* end_label: */
    *instructions = tacky_append_instruction(*instructions,
        tacky_inst_label(end_label));

    free(false_label); free(end_label);
    TackyVal *result = tacky_val_var(tmp_name);
    free(tmp_name);
    return result;
}

static TackyVal *emit_tacky_or(Exp *left, Exp *right,
                                TackyInstruction **instructions) {
    char *true_label = make_label("or_true");
    char *end_label  = make_label("or_end");

    TackyVal *v1 = emit_tacky_exp(left, instructions);
    *instructions = tacky_append_instruction(*instructions,
        tacky_inst_jump_if_not_zero(v1, true_label));

    TackyVal *v2 = emit_tacky_exp(right, instructions);
    *instructions = tacky_append_instruction(*instructions,
        tacky_inst_jump_if_not_zero(v2, true_label));

    /* Both false: result = 0 */
    char *tmp_name = make_temporary();
    TackyVal *dst = tacky_val_var(tmp_name);
    *instructions = tacky_append_instruction(*instructions,
        tacky_inst_copy(tacky_val_constant(0), dst));
    *instructions = tacky_append_instruction(*instructions,
        tacky_inst_jump(end_label));

    /* true_label: result = 1 */
    *instructions = tacky_append_instruction(*instructions,
        tacky_inst_label(true_label));
    TackyVal *dst2 = tacky_val_var(tmp_name);
    *instructions = tacky_append_instruction(*instructions,
        tacky_inst_copy(tacky_val_constant(1), dst2));

    /* end_label: */
    *instructions = tacky_append_instruction(*instructions,
        tacky_inst_label(end_label));

    free(true_label); free(end_label);
    TackyVal *result = tacky_val_var(tmp_name);
    free(tmp_name);
    return result;
}

/* ================================================================
 * Main expression emitter
 * ================================================================ */

static TackyVal *emit_tacky_exp(Exp *e, TackyInstruction **instructions) {
    if (e->type == AST_EXP_CONSTANT) {
        return tacky_val_constant(e->value);
    }

    if (e->type == AST_EXP_UNARY) {
        TackyVal *src = emit_tacky_exp(e->unary.operand, instructions);
        char *tmp_name = make_temporary();
        TackyVal *dst = tacky_val_var(tmp_name);
        free(tmp_name);
        TackyInstruction *inst = tacky_inst_unary(e->unary.operator, src, dst);
        *instructions = tacky_append_instruction(*instructions, inst);
        TackyVal *result = tacky_val_var(dst->name);
        return result;
    }

    if (e->type == AST_EXP_BINARY) {
        /* Short-circuit && and || */
        if (e->binary.operator == BINARY_AND)
            return emit_tacky_and(e->binary.left, e->binary.right, instructions);
        if (e->binary.operator == BINARY_OR)
            return emit_tacky_or(e->binary.left, e->binary.right, instructions);

        /* Regular binary: relational, arithmetic */
        TackyVal *v1 = emit_tacky_exp(e->binary.left, instructions);
        TackyVal *v2 = emit_tacky_exp(e->binary.right, instructions);
        char *tmp_name = make_temporary();
        TackyVal *dst = tacky_val_var(tmp_name);
        free(tmp_name);
        TackyInstruction *inst = tacky_inst_binary(e->binary.operator, v1, v2, dst);
        *instructions = tacky_append_instruction(*instructions, inst);
        TackyVal *result = tacky_val_var(dst->name);
        return result;
    }

    fprintf(stderr, "TACKY generation error: unsupported expression type\n");
    return NULL;
}

/* ================================================================
 * Entry point
 * ================================================================ */

TackyProgram *gen_tacky(Program *prog) {
    if (!prog || !prog->function) return NULL;
    tmp_counter = 0;
    label_counter = 0;

    TackyInstruction *instructions = NULL;
    Statement *stmt = prog->function->body;
    if (stmt->type != AST_STATEMENT_RETURN) {
        fprintf(stderr, "TACKY generation error: unsupported statement\n");
        return NULL;
    }
    TackyVal *return_val = emit_tacky_exp(stmt->return_val, &instructions);
    TackyInstruction *ret_inst = tacky_inst_return(return_val);
    instructions = tacky_append_instruction(instructions, ret_inst);
    TackyFunction *func = tacky_function(prog->function->name, instructions);
    return tacky_program(func);
}
