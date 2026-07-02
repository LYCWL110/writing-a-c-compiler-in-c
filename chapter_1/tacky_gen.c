#include <stdio.h>
#include <stdlib.h>
#include "tacky_gen.h"

static int tmp_counter = 0;
static int label_counter = 0;

static char *make_temporary(void) {
    char *n = malloc(32); snprintf(n, 32, "tmp.%d", tmp_counter++); return n;
}
static char *make_label(const char *pref) {
    char *n = malloc(64); snprintf(n, 64, "%s.%d", pref, label_counter++); return n;
}

static TackyVal *emit_tacky_exp(Exp *e, TackyInstruction **instructions);

static TackyVal *emit_tacky_and(Exp *l, Exp *r, TackyInstruction **ins) {
    char *fl = make_label("and_false"), *el = make_label("and_end");
    TackyVal *v1 = emit_tacky_exp(l, ins);
    *ins = tacky_append_instruction(*ins, tacky_inst_jump_if_zero(v1, fl));
    TackyVal *v2 = emit_tacky_exp(r, ins);
    *ins = tacky_append_instruction(*ins, tacky_inst_jump_if_zero(v2, fl));
    char *tn = make_temporary(); TackyVal *d = tacky_val_var(tn);
    *ins = tacky_append_instruction(*ins, tacky_inst_copy(tacky_val_constant(1), d));
    *ins = tacky_append_instruction(*ins, tacky_inst_jump(el));
    *ins = tacky_append_instruction(*ins, tacky_inst_label(fl));
    TackyVal *d2 = tacky_val_var(tn);
    *ins = tacky_append_instruction(*ins, tacky_inst_copy(tacky_val_constant(0), d2));
    *ins = tacky_append_instruction(*ins, tacky_inst_label(el));
    free(fl); free(el); free(tn);
    return tacky_val_var(d->name);
}

static TackyVal *emit_tacky_or(Exp *l, Exp *r, TackyInstruction **ins) {
    char *tl = make_label("or_true"), *el = make_label("or_end");
    TackyVal *v1 = emit_tacky_exp(l, ins);
    *ins = tacky_append_instruction(*ins, tacky_inst_jump_if_not_zero(v1, tl));
    TackyVal *v2 = emit_tacky_exp(r, ins);
    *ins = tacky_append_instruction(*ins, tacky_inst_jump_if_not_zero(v2, tl));
    char *tn = make_temporary(); TackyVal *d = tacky_val_var(tn);
    *ins = tacky_append_instruction(*ins, tacky_inst_copy(tacky_val_constant(0), d));
    *ins = tacky_append_instruction(*ins, tacky_inst_jump(el));
    *ins = tacky_append_instruction(*ins, tacky_inst_label(tl));
    TackyVal *d2 = tacky_val_var(tn);
    *ins = tacky_append_instruction(*ins, tacky_inst_copy(tacky_val_constant(1), d2));
    *ins = tacky_append_instruction(*ins, tacky_inst_label(el));
    free(tl); free(el); free(tn);
    return tacky_val_var(d->name);
}

static TackyVal *emit_tacky_exp(Exp *e, TackyInstruction **instructions) {
    if (!e) return NULL;
    switch (e->type) {
        case AST_EXP_CONSTANT: return tacky_val_constant(e->value);
        case AST_EXP_VAR:      return tacky_val_var(e->var_name);
        case AST_EXP_ASSIGNMENT: {
            TackyVal *result = emit_tacky_exp(e->assign.right, instructions);
            if (!result) return NULL;
            if (e->assign.left->type != AST_EXP_VAR) {
                fprintf(stderr, "TACKY gen: invalid lvalue\n"); return NULL;
            }
            TackyVal *lv = tacky_val_var(e->assign.left->var_name);
            *instructions = tacky_append_instruction(*instructions, tacky_inst_copy(result, lv));
            return tacky_val_var(lv->name);
        }
        case AST_EXP_UNARY: {
            TackyVal *src = emit_tacky_exp(e->unary.operand, instructions);
            char *tn = make_temporary(); TackyVal *dst = tacky_val_var(tn); free(tn);
            *instructions = tacky_append_instruction(*instructions,
                tacky_inst_unary(e->unary.operator, src, dst));
            return tacky_val_var(dst->name);
        }
        case AST_EXP_BINARY:
            if (e->binary.operator == BINARY_AND)
                return emit_tacky_and(e->binary.left, e->binary.right, instructions);
            if (e->binary.operator == BINARY_OR)
                return emit_tacky_or(e->binary.left, e->binary.right, instructions);
            {
                TackyVal *v1 = emit_tacky_exp(e->binary.left, instructions);
                TackyVal *v2 = emit_tacky_exp(e->binary.right, instructions);
                char *tn = make_temporary(); TackyVal *dst = tacky_val_var(tn); free(tn);
                *instructions = tacky_append_instruction(*instructions,
                    tacky_inst_binary(e->binary.operator, v1, v2, dst));
                return tacky_val_var(dst->name);
            }
        default: fprintf(stderr, "TACKY gen: unsupported exp\n"); return NULL;
    }
}

/* Generate TACKY for one block item */
static TackyInstruction *gen_block_item(BlockItem *b, TackyInstruction **tail) {
    if (!b) return NULL;
    if (b->is_declaration) {
        /* Declaration without init: nothing. With init: like assignment */
        if (b->decl->init) {
            TackyVal *result = emit_tacky_exp(b->decl->init, tail);
            TackyVal *lv = tacky_val_var(b->decl->name);
            return tacky_inst_copy(result, lv);
        }
        return NULL;
    }
    /* Statement */
    Statement *s = b->stmt;
    if (s->type == AST_STATEMENT_NULL) return NULL;
    if (s->type == AST_STATEMENT_EXPRESSION) {
        TackyVal *v = emit_tacky_exp(s->expr, tail);
        tacky_free_val(v);
        return NULL;
    }
    /* Return */
    if (s->type == AST_STATEMENT_RETURN) {
        TackyVal *val = emit_tacky_exp(s->return_val, tail);
        return tacky_inst_return(val);
    }
    return NULL;
}

TackyProgram *gen_tacky(Program *prog) {
    if (!prog || !prog->function) return NULL;
    tmp_counter = 0; label_counter = 0;
    TackyInstruction *instructions = NULL;
    FunctionDef *f = prog->function;
    int has_return = 0;
    for (int i = 0; i < f->body_count; i++) {
        TackyInstruction *new_inst = gen_block_item(f->body[i], &instructions);
        if (new_inst) {
            if (new_inst->type == TACKY_INST_RETURN) has_return = 1;
            instructions = tacky_append_instruction(instructions, new_inst);
        }
    }
    /* Add implicit Return(0) at end of function if no return */
    if (!has_return) {
        TackyInstruction *ret = tacky_inst_return(tacky_val_constant(0));
        instructions = tacky_append_instruction(instructions, ret);
    }
    TackyFunction *func = tacky_function(f->name, instructions);
    return tacky_program(func);
}
