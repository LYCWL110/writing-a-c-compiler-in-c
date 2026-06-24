#include <stdio.h>
#include <stdlib.h>
#include "asm_ast.h"

#if defined(__APPLE__) || defined(__MACH__)
#define FUNC_PREFIX "_"
#define LABEL_PREFIX "L"
#else
#define FUNC_PREFIX ""
#define LABEL_PREFIX ".L"
#endif

static const char *reg_name_4(RegId r) {
    const char *n[] = {"eax","edx","r10d","r11d"};
    return n[r];
}
static const char *reg_name_1(RegId r) {
    const char *n[] = {"al","dl","r10b","r11b"};
    return n[r];
}

static void emit_operand(FILE *out, AsmOperand *op, int size_byte) {
    switch (op->type) {
        case ASM_OPERAND_IMM:
            fprintf(out, "$%d", op->imm); break;
        case ASM_OPERAND_REG:
            fprintf(out, "%%%s", size_byte ? reg_name_1(op->reg) : reg_name_4(op->reg));
            break;
        case ASM_OPERAND_STACK:
            fprintf(out, "%d(%%rbp)", op->stack_offset); break;
        case ASM_OPERAND_PSEUDO:
            fprintf(out, "<pseudo:%s>", op->pseudo_name); break;
    }
}

static void emit_instruction(FILE *out, AsmInstruction *inst) {
    switch (inst->type) {
        case ASM_INST_MOV:
            fprintf(out, "    movl    "); emit_operand(out, inst->src, 0);
            fprintf(out, ", "); emit_operand(out, inst->dst, 0); fprintf(out, "\n"); break;
        case ASM_INST_UNARY: {
            const char *mn = (inst->unary_op == ASM_UNARY_NEG) ? "negl" : "notl";
            fprintf(out, "    %s    ", mn); emit_operand(out, inst->operand, 0); fprintf(out, "\n"); break;
        }
        case ASM_INST_BINARY: {
            const char *mn[] = {"addl","subl","imull"};
            fprintf(out, "    %s    ", mn[inst->binary_op]);
            emit_operand(out, inst->src, 0); fprintf(out, ", ");
            emit_operand(out, inst->dst, 0); fprintf(out, "\n"); break;
        }
        case ASM_INST_CMP:
            fprintf(out, "    cmpl    "); emit_operand(out, inst->src, 0);
            fprintf(out, ", "); emit_operand(out, inst->dst, 0); fprintf(out, "\n"); break;
        case ASM_INST_IDIV:
            fprintf(out, "    idivl   "); emit_operand(out, inst->src, 0); fprintf(out, "\n"); break;
        case ASM_INST_CDQ:
            fprintf(out, "    cdq\n"); break;
        case ASM_INST_JMP:
            fprintf(out, "    jmp     %s%s\n", LABEL_PREFIX, inst->label); break;
        case ASM_INST_JMPCC: {
            const char *cc[] = {"e","ne","g","ge","l","le"};
            fprintf(out, "    j%s     %s%s\n", cc[inst->cond_code], LABEL_PREFIX, inst->label);
            break;
        }
        case ASM_INST_SETCC: {
            const char *cc[] = {"e","ne","g","ge","l","le"};
            fprintf(out, "    set%s   ", cc[inst->cond_code]);
            emit_operand(out, inst->operand, 1); fprintf(out, "\n"); break;
        }
        case ASM_INST_LABEL:
            fprintf(out, "%s%s:\n", LABEL_PREFIX, inst->label); break;
        case ASM_INST_ALLOCATE_STACK:
            fprintf(out, "    subq    $%d, %%rsp\n", inst->src ? inst->src->imm : 0); break;
        case ASM_INST_RET:
            fprintf(out, "    movq    %%rbp, %%rsp\n    popq    %%rbp\n    ret\n"); break;
        default: break;
    }
}

void emit(FILE *out, AsmProgram *prog) {
    if (!prog || !prog->function) return;
    AsmFunction *func = prog->function;
    fprintf(out, ".globl %s%s\n", FUNC_PREFIX, func->name);
    fprintf(out, "%s%s:\n", FUNC_PREFIX, func->name);
    fprintf(out, "    pushq   %%rbp\n    movq    %%rsp, %%rbp\n");
    for (AsmInstruction *inst = func->instructions; inst; inst = inst->next)
        emit_instruction(out, inst);
#if !defined(__APPLE__) && !defined(__MACH__)
    fprintf(out, ".section .note.GNU-stack,\"\",@progbits\n");
#endif
}
