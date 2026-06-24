#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

/* ================================================================
 * Stage A: TACKY → Assembly AST
 * ================================================================ */

static AsmOperand *val_to_operand(TackyVal *v) {
    return (v->type == TACKY_VAL_CONSTANT) ? make_operand_imm(v->value)
                                            : make_operand_pseudo(v->name);
}

static AsmUnaryOp convert_unary_op(UnaryOperator op) {
    return (op == UNARY_COMPLEMENT) ? ASM_UNARY_NOT : ASM_UNARY_NEG;
}

static AsmBinaryOp convert_binary_op(BinaryOperator op) {
    switch (op) {
        case BINARY_ADD:      return ASM_BINARY_ADD;
        case BINARY_SUBTRACT: return ASM_BINARY_SUB;
        case BINARY_MULTIPLY: return ASM_BINARY_MULT;
        default:             return ASM_BINARY_ADD;
    }
}

static CondCode relational_cc(BinaryOperator op) {
    switch (op) {
        case BINARY_EQUAL:        return CC_E;
        case BINARY_NOT_EQUAL:    return CC_NE;
        case BINARY_LESS:         return CC_L;
        case BINARY_LESS_EQ:      return CC_LE;
        case BINARY_GREATER:      return CC_G;
        case BINARY_GREATER_EQ:   return CC_GE;
        default:                 return CC_E;
    }
}

static AsmInstruction *tacky_inst_to_asm(TackyInstruction *inst) {
    if (inst->type == TACKY_INST_RETURN) {
        AsmInstruction *mov = make_inst_mov(val_to_operand(inst->val),
                                             make_operand_reg(REG_AX));
        mov->next = make_inst_ret();
        return mov;
    }

    if (inst->type == TACKY_INST_UNARY) {
        if (inst->unary_op == UNARY_NOT) {
            /* Not(src, dst) → Cmp(Imm(0), src) + Mov(Imm(0), dst) + SetCC(E, dst) */
            AsmInstruction *cmp = make_inst_cmp(make_operand_imm(0),
                                                  val_to_operand(inst->src));
            AsmInstruction *mov = make_inst_mov(make_operand_imm(0),
                                                  val_to_operand(inst->dst));
            AsmInstruction *set = make_inst_setcc(CC_E, val_to_operand(inst->dst));
            cmp->next = mov; mov->next = set;
            return cmp;
        }
        AsmInstruction *mov = make_inst_mov(val_to_operand(inst->src),
                                             val_to_operand(inst->dst));
        mov->next = make_inst_unary(convert_unary_op(inst->unary_op),
                                     val_to_operand(inst->dst));
        return mov;
    }

    if (inst->type == TACKY_INST_BINARY) {
        /* Relational: Cmp(src2, src1) + Mov(Imm(0), dst) + SetCC(cc, dst) */
        if (inst->binary_op >= BINARY_EQUAL && inst->binary_op <= BINARY_GREATER_EQ) {
            AsmInstruction *cmp = make_inst_cmp(val_to_operand(inst->src2),
                                                  val_to_operand(inst->src));
            AsmInstruction *mov = make_inst_mov(make_operand_imm(0),
                                                  val_to_operand(inst->dst));
            AsmInstruction *set = make_inst_setcc(relational_cc(inst->binary_op),
                                                    val_to_operand(inst->dst));
            cmp->next = mov; mov->next = set;
            return cmp;
        }
        /* Division / Remainder (existing) */
        if (inst->binary_op == BINARY_DIVIDE || inst->binary_op == BINARY_REMAINDER) {
            AsmInstruction *mov = make_inst_mov(val_to_operand(inst->src),
                                                  make_operand_reg(REG_AX));
            AsmInstruction *cdq = make_inst_cdq();
            AsmInstruction *idiv = make_inst_idiv(val_to_operand(inst->src2));
            RegId r = (inst->binary_op == BINARY_DIVIDE) ? REG_AX : REG_DX;
            AsmInstruction *mov2 = make_inst_mov(make_operand_reg(r),
                                                   val_to_operand(inst->dst));
            mov->next = cdq; cdq->next = idiv; idiv->next = mov2;
            return mov;
        }
        /* Add/Sub/Mult */
        AsmInstruction *mov = make_inst_mov(val_to_operand(inst->src),
                                             val_to_operand(inst->dst));
        mov->next = make_inst_binary(convert_binary_op(inst->binary_op),
                                      val_to_operand(inst->src2),
                                      val_to_operand(inst->dst));
        return mov;
    }

    if (inst->type == TACKY_INST_COPY) {
        return make_inst_mov(val_to_operand(inst->src), val_to_operand(inst->dst));
    }
    if (inst->type == TACKY_INST_JUMP) {
        return make_inst_jmp(inst->target);
    }
    if (inst->type == TACKY_INST_JUMP_IF_ZERO) {
        AsmInstruction *cmp = make_inst_cmp(make_operand_imm(0),
                                              val_to_operand(inst->val));
        cmp->next = make_inst_jmpcc(CC_E, inst->target);
        return cmp;
    }
    if (inst->type == TACKY_INST_JUMP_IF_NOT_ZERO) {
        AsmInstruction *cmp = make_inst_cmp(make_operand_imm(0),
                                              val_to_operand(inst->val));
        cmp->next = make_inst_jmpcc(CC_NE, inst->target);
        return cmp;
    }
    if (inst->type == TACKY_INST_LABEL) {
        return make_inst_label(inst->target);
    }

    fprintf(stderr, "Codegen error: unsupported TACKY instruction\n");
    return NULL;
}

static AsmFunction *tacky_func_to_asm(TackyFunction *func) {
    AsmInstruction *asm_insts = NULL;
    for (TackyInstruction *t = func->body; t; t = t->next) {
        AsmInstruction *a = tacky_inst_to_asm(t);
        if (!a) return NULL;
        asm_insts = append_instruction(asm_insts, a);
    }
    return make_asm_function(func->name, asm_insts);
}

/* ================================================================
 * Stage B: Replace pseudos
 * ================================================================ */
#define MAX_PSEUDOS 64
static struct { char *name; int offset; } pseudo_map[MAX_PSEUDOS];
static int pseudo_count = 0;

static int get_or_alloc_offset(const char *name) {
    for (int i = 0; i < pseudo_count; i++)
        if (strcmp(pseudo_map[i].name, name) == 0) return pseudo_map[i].offset;
    int off = (pseudo_count == 0) ? -4 : pseudo_map[pseudo_count - 1].offset - 4;
    pseudo_map[pseudo_count].name = strdup(name);
    pseudo_map[pseudo_count].offset = off; pseudo_count++;
    return off;
}
static void clear_pseudo_map(void) {
    for (int i = 0; i < pseudo_count; i++) free(pseudo_map[i].name);
    pseudo_count = 0;
}
static void replace_op(AsmOperand *op, int *max_offset) {
    if (!op || op->type != ASM_OPERAND_PSEUDO) return;
    int off = get_or_alloc_offset(op->pseudo_name);
    free(op->pseudo_name);
    op->type = ASM_OPERAND_STACK;
    op->stack_offset = off;
    if (-off > *max_offset) *max_offset = -off;
}

static int replace_pseudos(AsmInstruction *inst) {
    int m = 0;
    while (inst) {
        replace_op(inst->src, &m);
        replace_op(inst->dst, &m);
        replace_op(inst->operand, &m);
        inst = inst->next;
    }
    return m;
}

/* ================================================================
 * Stage C: Fix invalid instructions
 * ================================================================ */
static void emit_fixed(AsmInstruction *n, AsmInstruction **h, AsmInstruction **t) {
    n->next = NULL;
    if (!*h) *h = *t = n; else { (*t)->next = n; *t = n; }
}

static AsmInstruction *fix_instructions(AsmInstruction *old, int stk) {
    AsmInstruction *nh = NULL, *nt = NULL;
    if (stk > 0) emit_fixed(make_inst_allocate_stack(stk), &nh, &nt);
    AsmInstruction *cur = old;
    while (cur) {
        AsmInstruction *nx = cur->next;

        /* Mov(Stack,Stack) */
        if (cur->type == ASM_INST_MOV
            && cur->src->type == ASM_OPERAND_STACK
            && cur->dst->type == ASM_OPERAND_STACK) {
            int dst_off = cur->dst->stack_offset;
            cur->dst = make_operand_reg(REG_R10);
            emit_fixed(cur, &nh, &nt);
            emit_fixed(make_inst_mov(make_operand_reg(REG_R10), make_operand_stack(dst_off)), &nh, &nt);
            cur = nx; continue;
        }
        /* Idiv(Imm) */
        if (cur->type == ASM_INST_IDIV && cur->src->type == ASM_OPERAND_IMM) {
            int v = cur->src->imm;
            emit_fixed(make_inst_mov(make_operand_imm(v), make_operand_reg(REG_R10)), &nh, &nt);
            cur->src = make_operand_reg(REG_R10);
            emit_fixed(cur, &nh, &nt);
            cur = nx; continue;
        }
        /* Add/Sub(Stack,Stack) */
        if (cur->type == ASM_INST_BINARY
            && (cur->binary_op == ASM_BINARY_ADD || cur->binary_op == ASM_BINARY_SUB)
            && cur->src->type == ASM_OPERAND_STACK
            && cur->dst->type == ASM_OPERAND_STACK) {
            int src_off = cur->src->stack_offset;
            emit_fixed(make_inst_mov(make_operand_stack(src_off), make_operand_reg(REG_R10)), &nh, &nt);
            cur->src = make_operand_reg(REG_R10);
            emit_fixed(cur, &nh, &nt);
            cur = nx; continue;
        }
        /* Imul(*,Stack) */
        if (cur->type == ASM_INST_BINARY && cur->binary_op == ASM_BINARY_MULT
            && cur->dst->type == ASM_OPERAND_STACK) {
            int dst_off = cur->dst->stack_offset;
            emit_fixed(make_inst_mov(make_operand_stack(dst_off), make_operand_reg(REG_R11)), &nh, &nt);
            cur->dst = make_operand_reg(REG_R11);
            emit_fixed(cur, &nh, &nt);
            emit_fixed(make_inst_mov(make_operand_reg(REG_R11), make_operand_stack(dst_off)), &nh, &nt);
            cur = nx; continue;
        }
        /* Cmp(Stack,Stack) → split through R10 (fix 1st operand) */
        if (cur->type == ASM_INST_CMP
            && cur->src->type == ASM_OPERAND_STACK
            && cur->dst->type == ASM_OPERAND_STACK) {
            int src_off = cur->src->stack_offset;
            emit_fixed(make_inst_mov(make_operand_stack(src_off), make_operand_reg(REG_R10)), &nh, &nt);
            cur->src = make_operand_reg(REG_R10);
            emit_fixed(cur, &nh, &nt);
            cur = nx; continue;
        }
        /* Cmp(*, Imm) → fix 2nd operand through R11 */
        if (cur->type == ASM_INST_CMP && cur->dst->type == ASM_OPERAND_IMM) {
            int d_val = cur->dst->imm;
            emit_fixed(make_inst_mov(make_operand_imm(d_val), make_operand_reg(REG_R11)), &nh, &nt);
            cur->dst = make_operand_reg(REG_R11);
            emit_fixed(cur, &nh, &nt);
            cur = nx; continue;
        }
        /* Default: pass through */
        emit_fixed(cur, &nh, &nt);
        cur = nx;
    }
    return nh;
}

/* ================================================================ */
AsmProgram *codegen(TackyProgram *tacky) {
    if (!tacky || !tacky->function) { fprintf(stderr, "Codegen: empty\n"); return NULL; }
    AsmFunction *af = tacky_func_to_asm(tacky->function);
    if (!af) return NULL;
    int sz = replace_pseudos(af->instructions);
    clear_pseudo_map();
    af->instructions = fix_instructions(af->instructions, sz);
    return make_asm_program(af);
}
