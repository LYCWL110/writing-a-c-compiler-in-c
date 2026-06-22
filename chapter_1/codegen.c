#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

/* ================================================================
 * Stage A: TACKY → Assembly AST (with pseudoregisters)
 * ================================================================ */

/* Convert TACKY value to assembly operand */
static AsmOperand *val_to_operand(TackyVal *v) {
    if (v->type == TACKY_VAL_CONSTANT) {
        return make_operand_imm(v->value);
    } else {
        return make_operand_pseudo(v->name);
    }
}

/* Convert TACKY unary operator to assembly unary operator */
static AsmUnaryOp convert_unary_op(UnaryOperator op) {
    return (op == UNARY_COMPLEMENT) ? ASM_UNARY_NOT : ASM_UNARY_NEG;
}

/* Convert a single TACKY instruction to assembly instructions */
static AsmInstruction *tacky_inst_to_asm(TackyInstruction *inst) {
    if (inst->type == TACKY_INST_RETURN) {
        /* Return(val) → Mov(val, Reg(AX)) + Ret */
        AsmOperand *src = val_to_operand(inst->val);
        AsmOperand *dst = make_operand_reg(REG_AX);
        AsmInstruction *mov = make_inst_mov(src, dst);
        AsmInstruction *ret = make_inst_ret();
        mov->next = ret;
        return mov;
    }

    if (inst->type == TACKY_INST_UNARY) {
        /* Unary(op, src, dst) → Mov(src, dst) + Unary(asm_op, dst) */
        AsmOperand *mov_src = val_to_operand(inst->src);
        AsmOperand *mov_dst = val_to_operand(inst->dst);
        AsmInstruction *mov = make_inst_mov(mov_src, mov_dst);

        AsmUnaryOp asm_op = convert_unary_op(inst->unary_op);
        /* dst operand is the same as mov_dst — need a copy since mov consumes it */
        AsmOperand *unary_operand = val_to_operand(inst->dst);
        AsmInstruction *unary = make_inst_unary(asm_op, unary_operand);

        mov->next = unary;
        return mov;
    }

    fprintf(stderr, "Codegen error: unsupported TACKY instruction\n");
    return NULL;
}

/* Convert entire TACKY function to assembly */
static AsmFunction *tacky_func_to_asm(TackyFunction *func) {
    AsmInstruction *asm_insts = NULL;
    TackyInstruction *t_inst = func->body;
    while (t_inst) {
        AsmInstruction *a = tacky_inst_to_asm(t_inst);
        if (!a) return NULL;
        asm_insts = append_instruction(asm_insts, a);
        t_inst = t_inst->next;
    }
    return make_asm_function(func->name, asm_insts);
}

/* ================================================================
 * Stage B: Replace pseudoregisters with stack addresses
 * ================================================================ */

/* Simple string→int mapping (linear scan, fine for Chapter 2) */
#define MAX_PSEUDOS 64
static struct { char *name; int offset; } pseudo_map[MAX_PSEUDOS];
static int pseudo_count = 0;

static int get_or_alloc_offset(const char *name) {
    for (int i = 0; i < pseudo_count; i++) {
        if (strcmp(pseudo_map[i].name, name) == 0)
            return pseudo_map[i].offset;
    }
    /* New pseudo: assign next stack slot (grows downward) */
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

/* Replace Pseudos with Stack operands. Returns the absolute stack frame size needed */
static int replace_pseudos_in_instructions(AsmInstruction *inst) {
    int max_offset = 0;
    while (inst) {
        /* Check src operand */
        if (inst->src && inst->src->type == ASM_OPERAND_PSEUDO) {
            int off = get_or_alloc_offset(inst->src->pseudo_name);
            free(inst->src->pseudo_name);
            inst->src->type = ASM_OPERAND_STACK;
            inst->src->stack_offset = off;
            if (-off > max_offset) max_offset = -off;
        }
        /* Check dst operand */
        if (inst->dst && inst->dst->type == ASM_OPERAND_PSEUDO) {
            int off = get_or_alloc_offset(inst->dst->pseudo_name);
            free(inst->dst->pseudo_name);
            inst->dst->type = ASM_OPERAND_STACK;
            inst->dst->stack_offset = off;
            if (-off > max_offset) max_offset = -off;
        }
        /* Check operand (for Unary) */
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
 * Stage C: Fix invalid instructions
 * ================================================================ */

/* Insert AllocateStack at beginning and fix Mov(Stack, Stack) */
static AsmInstruction *fix_instructions(AsmInstruction *inst, int stack_size) {
    /* Insert AllocateStack at the beginning */
    if (stack_size > 0) {
        inst = prepend_instruction(inst, make_inst_allocate_stack(stack_size));
    }

    /* Fix Mov(Stack(src), Stack(dst)) — can't have both as memory operands */
    AsmInstruction *cur = inst;
    while (cur) {
        if (cur->type == ASM_INST_MOV
            && cur->src && cur->src->type == ASM_OPERAND_STACK
            && cur->dst && cur->dst->type == ASM_OPERAND_STACK) {

            /* Rewrite:
             *   movl Stack(src), Stack(dst)
             * to:
             *   movl Stack(src), %r10d
             *   movl %r10d, Stack(dst)
             */
            AsmOperand *src_copy = make_operand_stack(cur->src->stack_offset);
            AsmOperand *dst_copy = make_operand_stack(cur->dst->stack_offset);

            /* First instruction: movl src, %r10d */
            cur->type = ASM_INST_MOV;
            /* src stays as-is */
            cur->dst->type = ASM_OPERAND_REG;
            cur->dst->reg = REG_R10;

            /* Second instruction: movl %r10d, dst */
            AsmInstruction *new_mov = make_inst_mov(
                make_operand_reg(REG_R10),
                dst_copy
            );
            new_mov->next = cur->next;
            cur->next = new_mov;

            /* Don't free src_copy — it was just a copy of the original we kept */
            free(src_copy);
        }
        cur = cur->next;
    }
    return inst;
}

/* ================================================================
 * Public entry: run all three stages
 * ================================================================ */

AsmProgram *codegen(TackyProgram *tacky) {
    if (!tacky || !tacky->function) {
        fprintf(stderr, "Codegen error: empty TACKY program\n");
        return NULL;
    }

    /* Stage A: TACKY → Assembly */
    AsmFunction *asm_func = tacky_func_to_asm(tacky->function);
    if (!asm_func) return NULL;

    /* Stage B: Replace pseudos with stack addresses */
    int stack_size = replace_pseudos_in_instructions(asm_func->instructions);
    clear_pseudo_map();

    /* Stage C: Fix invalid instructions */
    asm_func->instructions = fix_instructions(asm_func->instructions, stack_size);

    return make_asm_program(asm_func);
}
