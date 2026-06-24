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
    switch (op->type) {
        case ASM_OPERAND_IMM:
            fprintf(out, "$%d", op->imm);
            break;
        case ASM_OPERAND_REG: {
            const char *names[] = {"eax", "edx", "r10d", "r11d"};
            fprintf(out, "%%%s", names[op->reg]);
            break;
        }
        case ASM_OPERAND_STACK:
            fprintf(out, "%d(%%rbp)", op->stack_offset);
            break;
        case ASM_OPERAND_PSEUDO:
            /* Pseudos should have been replaced by now — emit an error marker */
            fprintf(out, "<pseudo:%s>", op->pseudo_name);
            break;
    }
}

/* Helper to emit a single instruction */
static void emit_instruction(FILE *out, AsmInstruction *inst) {
    switch (inst->type) {
        case ASM_INST_MOV:
            fprintf(out, "    movl    ");
            emit_operand(out, inst->src);
            fprintf(out, ", ");
            emit_operand(out, inst->dst);
            fprintf(out, "\n");
            break;
        case ASM_INST_UNARY:
            fprintf(out, "    %s    ",
                    inst->unary_op == ASM_UNARY_NEG ? "negl" : "notl");
            emit_operand(out, inst->operand);
            fprintf(out, "\n");
            break;
        case ASM_INST_BINARY: {
            const char *mn[] = {"addl", "subl", "imull"};
            fprintf(out, "    %s    ", mn[inst->binary_op]);
            emit_operand(out, inst->src);
            fprintf(out, ", ");
            emit_operand(out, inst->dst);
            fprintf(out, "\n");
            break;
        }
        case ASM_INST_IDIV:
            fprintf(out, "    idivl   ");
            emit_operand(out, inst->src);
            fprintf(out, "\n");
            break;
        case ASM_INST_CDQ:
            fprintf(out, "    cdq\n");
            break;
        case ASM_INST_ALLOCATE_STACK:
            fprintf(out, "    subq    $%d, %%rsp\n", inst->src ? inst->src->imm : 0);
            break;
        case ASM_INST_RET:
            /* Function epilogue */
            fprintf(out, "    movq    %%rbp, %%rsp\n");
            fprintf(out, "    popq    %%rbp\n");
            fprintf(out, "    ret\n");
            break;
        default:
            break;
    }
}

void emit(FILE *out, AsmProgram *prog) {
    if (!prog || !prog->function) {
        fprintf(stderr, "Emit error: empty program\n");
        return;
    }

    AsmFunction *func = prog->function;

    /* .globl <name> */
    fprintf(out, ".globl %s%s\n", FUNC_PREFIX, func->name);

    /* <name>: (function label) */
    fprintf(out, "%s%s:\n", FUNC_PREFIX, func->name);

    /* Function prologue */
    fprintf(out, "    pushq   %%rbp\n");
    fprintf(out, "    movq    %%rsp, %%rbp\n");

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
