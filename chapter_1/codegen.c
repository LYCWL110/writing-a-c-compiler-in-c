#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

/* ================================================================
 * Stage A: TACKY → Assembly AST (with pseudoregisters)
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

static AsmInstruction *tacky_inst_to_asm(TackyInstruction *inst) {
    if (inst->type == TACKY_INST_RETURN) {
        AsmInstruction *mov = make_inst_mov(val_to_operand(inst->val),
                                             make_operand_reg(REG_AX));
        mov->next = make_inst_ret();
        return mov;
    }

    if (inst->type == TACKY_INST_UNARY) {
        AsmInstruction *mov = make_inst_mov(val_to_operand(inst->src),
                                             val_to_operand(inst->dst));
        mov->next = make_inst_unary(convert_unary_op(inst->unary_op),
                                     val_to_operand(inst->dst));
        return mov;
    }

    if (inst->type == TACKY_INST_BINARY) {
        if (inst->binary_op == BINARY_DIVIDE || inst->binary_op == BINARY_REMAINDER) {
            AsmInstruction *mov = make_inst_mov(val_to_operand(inst->src),
                                                  make_operand_reg(REG_AX));
            AsmInstruction *cdq = make_inst_cdq();
            AsmInstruction *idiv = make_inst_idiv(val_to_operand(inst->src2));
            RegId result_reg = (inst->binary_op == BINARY_DIVIDE) ? REG_AX : REG_DX;
            AsmInstruction *mov2 = make_inst_mov(make_operand_reg(result_reg),
                                                   val_to_operand(inst->dst));
            mov->next = cdq;
            cdq->next = idiv;
            idiv->next = mov2;
            return mov;
        }
        /* Add, Subtract, Multiply */
        AsmInstruction *mov = make_inst_mov(val_to_operand(inst->src),
                                             val_to_operand(inst->dst));
        mov->next = make_inst_binary(convert_binary_op(inst->binary_op),
                                      val_to_operand(inst->src2),
                                      val_to_operand(inst->dst));
        return mov;
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
 * Stage B: Replace pseudoregisters with stack addresses
 * ================================================================ */

#define MAX_PSEUDOS 64
static struct { char *name; int offset; } pseudo_map[MAX_PSEUDOS];
static int pseudo_count = 0;

static int get_or_alloc_offset(const char *name) {
    for (int i = 0; i < pseudo_count; i++)
        if (strcmp(pseudo_map[i].name, name) == 0) return pseudo_map[i].offset;
    int offset = (pseudo_count == 0) ? -4 : pseudo_map[pseudo_count - 1].offset - 4;
    pseudo_map[pseudo_count].name = strdup(name);
    pseudo_map[pseudo_count].offset = offset;
    pseudo_count++;
    return offset;
}

static void clear_pseudo_map(void) {
    for (int i = 0; i < pseudo_count; i++) free(pseudo_map[i].name);
    pseudo_count = 0;
}

static int replace_pseudos(AsmInstruction *inst) {
    int max_offset = 0;
    while (inst) {
        if (inst->src && inst->src->type == ASM_OPERAND_PSEUDO) {
            int off = get_or_alloc_offset(inst->src->pseudo_name);
            free(inst->src->pseudo_name);
            inst->src->type = ASM_OPERAND_STACK;
            inst->src->stack_offset = off;
            if (-off > max_offset) max_offset = -off;
        }
        if (inst->dst && inst->dst->type == ASM_OPERAND_PSEUDO) {
            int off = get_or_alloc_offset(inst->dst->pseudo_name);
            free(inst->dst->pseudo_name);
            inst->dst->type = ASM_OPERAND_STACK;
            inst->dst->stack_offset = off;
            if (-off > max_offset) max_offset = -off;
        }
        if (inst->operand && inst->operand->type == ASM_OPERAND_PSEUDO) {
            int off = get_or_alloc_offset(inst->operand->pseudo_name);
            free(inst->operand->pseudo_name);
            inst->operand->type = ASM_OPERAND_STACK;
            inst->operand->stack_offset = off;
            if (-off > max_offset) max_offset = -off;
        }
        inst = inst->next;
    }
    return max_offset;
}

/* ================================================================
 * Stage C: Fix invalid instructions (build a new list approach)
 * ================================================================ */

static void emit_fixed(AsmInstruction *node, AsmInstruction **head, AsmInstruction **tail) {
    /* Helper: append a single node (with next=NULL) to the output list */
    AsmInstruction *n = node;
    n->next = NULL;
    if (!*head) { *head = *tail = n; }
    else        { (*tail)->next = n; *tail = n; }
}

static AsmInstruction *fix_instructions(AsmInstruction *old_list, int stack_size) {
    AsmInstruction *new_head = NULL, *new_tail = NULL;

    if (stack_size > 0)
        emit_fixed(make_inst_allocate_stack(stack_size), &new_head, &new_tail);

    AsmInstruction *cur = old_list;
    while (cur) {
        AsmInstruction *next = cur->next;

        /* Mov(Stack,Stack) → split through R10 */
        if (cur->type == ASM_INST_MOV
            && cur->src->type == ASM_OPERAND_STACK
            && cur->dst->type == ASM_OPERAND_STACK) {
            int dst_off = cur->dst->stack_offset;
            cur->dst = make_operand_reg(REG_R10);
            emit_fixed(cur, &new_head, &new_tail);
            emit_fixed(make_inst_mov(make_operand_reg(REG_R10),
                                      make_operand_stack(dst_off)),
                       &new_head, &new_tail);
            cur = next; continue;
        }

        /* Idiv(Imm) → movl $n, %r10d then idivl %r10d */
        if (cur->type == ASM_INST_IDIV
            && cur->src->type == ASM_OPERAND_IMM) {
            int val = cur->src->imm;
            emit_fixed(make_inst_mov(make_operand_imm(val),
                                      make_operand_reg(REG_R10)),
                       &new_head, &new_tail);
            cur->src = make_operand_reg(REG_R10);
            emit_fixed(cur, &new_head, &new_tail);
            cur = next; continue;
        }

        /* Add/Sub(Stack,Stack) → movl src, %r10d then addl/subl %r10d, dst */
        if (cur->type == ASM_INST_BINARY
            && (cur->binary_op == ASM_BINARY_ADD || cur->binary_op == ASM_BINARY_SUB)
            && cur->src->type == ASM_OPERAND_STACK
            && cur->dst->type == ASM_OPERAND_STACK) {
            int src_off = cur->src->stack_offset;
            emit_fixed(make_inst_mov(make_operand_stack(src_off),
                                      make_operand_reg(REG_R10)),
                       &new_head, &new_tail);
            cur->src = make_operand_reg(REG_R10);
            emit_fixed(cur, &new_head, &new_tail);
            cur = next; continue;
        }

        /* Imul(*,Stack) → movl dst,%r11d ; imull src,%r11d ; movl %r11d,dst */
        if (cur->type == ASM_INST_BINARY && cur->binary_op == ASM_BINARY_MULT
            && cur->dst->type == ASM_OPERAND_STACK) {
            int dst_off = cur->dst->stack_offset;
            emit_fixed(make_inst_mov(make_operand_stack(dst_off),
                                      make_operand_reg(REG_R11)),
                       &new_head, &new_tail);
            cur->dst = make_operand_reg(REG_R11);
            emit_fixed(cur, &new_head, &new_tail);
            emit_fixed(make_inst_mov(make_operand_reg(REG_R11),
                                      make_operand_stack(dst_off)),
                       &new_head, &new_tail);
            cur = next; continue;
        }

        /* Default: pass through unchanged */
        emit_fixed(cur, &new_head, &new_tail);
        cur = next;
    }
    return new_head;
}

/* ================================================================
 * Public entry
 * ================================================================ */

AsmProgram *codegen(TackyProgram *tacky) {
    if (!tacky || !tacky->function) { fprintf(stderr, "Codegen: empty\n"); return NULL; }
    AsmFunction *asm_func = tacky_func_to_asm(tacky->function);
    if (!asm_func) return NULL;
    int size = replace_pseudos(asm_func->instructions);
    clear_pseudo_map();
    asm_func->instructions = fix_instructions(asm_func->instructions, size);
    return make_asm_program(asm_func);
}
