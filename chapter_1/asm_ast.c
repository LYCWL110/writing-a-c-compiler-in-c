#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asm_ast.h"

/* === Constructors === */

AsmOperand *make_operand_imm(int value) {
    AsmOperand *op = malloc(sizeof(AsmOperand));
    if (!op) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
    op->type = ASM_OPERAND_IMM;
    op->imm = value;
    return op;
}

AsmOperand *make_operand_reg(char *reg_name) {
    AsmOperand *op = malloc(sizeof(AsmOperand));
    if (!op) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
    op->type = ASM_OPERAND_REG;
    op->reg_name = strdup(reg_name);
    return op;
}

AsmInstruction *make_inst_mov(AsmOperand *src, AsmOperand *dst) {
    AsmInstruction *inst = malloc(sizeof(AsmInstruction));
    if (!inst) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
    inst->type = ASM_INST_MOV;
    inst->src = src;
    inst->dst = dst;
    inst->next = NULL;
    return inst;
}

AsmInstruction *make_inst_ret(void) {
    AsmInstruction *inst = malloc(sizeof(AsmInstruction));
    if (!inst) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
    inst->type = ASM_INST_RET;
    inst->src = NULL;
    inst->dst = NULL;
    inst->next = NULL;
    return inst;
}

AsmInstruction *append_instruction(AsmInstruction *head, AsmInstruction *new_inst) {
    if (head == NULL) {
        return new_inst;
    }
    AsmInstruction *cur = head;
    while (cur->next != NULL) {
        cur = cur->next;
    }
    cur->next = new_inst;
    return head;
}

AsmFunction *make_asm_function(char *name, AsmInstruction *instructions) {
    AsmFunction *f = malloc(sizeof(AsmFunction));
    if (!f) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
    f->type = ASM_FUNCTION;
    f->name = strdup(name);
    f->instructions = instructions;
    return f;
}

AsmProgram *make_asm_program(AsmFunction *function) {
    AsmProgram *p = malloc(sizeof(AsmProgram));
    if (!p) {
        fprintf(stderr, "Memory allocation error\n");
        exit(1);
    }
    p->type = ASM_PROGRAM;
    p->function = function;
    return p;
}

/* === Pretty-print === */

static void asm_ast_print_operand(AsmOperand *op) {
    if (op->type == ASM_OPERAND_IMM) {
        printf("Imm(%d)", op->imm);
    } else {
        printf("Register(%s)", op->reg_name);
    }
}

void asm_ast_print(AsmProgram *prog) {
    if (!prog || !prog->function) return;
    printf("Program(\n");
    printf("  Function(\n");
    printf("    name=\"%s\",\n", prog->function->name);
    printf("    instructions=[\n");
    AsmInstruction *inst = prog->function->instructions;
    while (inst) {
        if (inst->type == ASM_INST_MOV) {
            printf("      Mov(\n        src=");
            asm_ast_print_operand(inst->src);
            printf(",\n        dst=");
            asm_ast_print_operand(inst->dst);
            printf("\n      )\n");
        } else {
            printf("      Ret\n");
        }
        inst = inst->next;
    }
    printf("    ]\n");
    printf("  )\n");
    printf(")\n");
}

/* === Free === */

static void asm_ast_free_operand(AsmOperand *op) {
    if (op) {
        if (op->type == ASM_OPERAND_REG) {
            free(op->reg_name);
        }
        free(op);
    }
}

void asm_ast_free(AsmProgram *prog) {
    if (prog) {
        if (prog->function) {
            free(prog->function->name);
            AsmInstruction *inst = prog->function->instructions;
            while (inst) {
                AsmInstruction *next = inst->next;
                asm_ast_free_operand(inst->src);
                asm_ast_free_operand(inst->dst);
                free(inst);
                inst = next;
            }
            free(prog->function);
        }
        free(prog);
    }
}
