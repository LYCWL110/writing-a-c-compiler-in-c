

#include <stdio.h>
#include <stdlib.h>
#include "asm_ast.h"

/* Platform detection: macOS prefixes function names with '_' */
#if defined(__APPLE__) || defined(__MACH__)
#define FUNC_PREFIX "_"
#else
#define FUNC_PREFIX ""
#endif

/* Helper to emit an operand */
static void emit_operand(FILE *out, AsmOperand *op) {
    if (op->type == ASM_OPERAND_IMM) {
        fprintf(out, "$%d", op->imm);
    } else if (op->type == ASM_OPERAND_REG) {
        fprintf(out, "%%%s", op->reg_name);
    }
}

/* Helper to emit a single instruction */
static void emit_instruction(FILE *out, AsmInstruction *inst) {
    if (inst->type == ASM_INST_MOV) {
        fprintf(out, "    movl    ");
        emit_operand(out, inst->src);
        fprintf(out, ", ");
        emit_operand(out, inst->dst);
        fprintf(out, "\n");
    } else if (inst->type == ASM_INST_RET) {
        fprintf(out, "    ret\n");
    }
}

void emit(FILE *out, AsmProgram *prog) {
    if (!prog || !prog->function) {
        fprintf(stderr, "Emit error: empty program\n");
        return;
    }

    AsmFunction *func = prog->function;

    /* .globl <prefix><name> */
    fprintf(out, ".globl %s%s\n", FUNC_PREFIX, func->name);

    /* <prefix><name>: */
    fprintf(out, "%s%s:\n", FUNC_PREFIX, func->name);

    /* Instructions */
    AsmInstruction *inst = func->instructions;
    while (inst) {
        emit_instruction(out, inst);
        inst = inst->next;
    }

    /* Linux: mark stack non-executable */
#if !defined(__APPLE__) && !defined(__MACH__)
    fprintf(out, ".section .note.GNU-stack,\"\",@progbits\n");
#endif
}
