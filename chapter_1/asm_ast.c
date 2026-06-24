#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "asm_ast.h"

/* === Constructors === */

AsmOperand *make_operand_imm(int value) {
    AsmOperand *op = malloc(sizeof(AsmOperand));
    if (!op) { fprintf(stderr, "Memory error\n"); exit(1); }
    op->type = ASM_OPERAND_IMM;
    op->imm = value;
    return op;
}

AsmOperand *make_operand_reg(RegId reg) {
    AsmOperand *op = malloc(sizeof(AsmOperand));
    if (!op) { fprintf(stderr, "Memory error\n"); exit(1); }
    op->type = ASM_OPERAND_REG;
    op->reg = reg;
    return op;
}

AsmOperand *make_operand_pseudo(const char *name) {
    AsmOperand *op = malloc(sizeof(AsmOperand));
    if (!op) { fprintf(stderr, "Memory error\n"); exit(1); }
    op->type = ASM_OPERAND_PSEUDO;
    op->pseudo_name = strdup(name);
    return op;
}

AsmOperand *make_operand_stack(int offset) {
    AsmOperand *op = malloc(sizeof(AsmOperand));
    if (!op) { fprintf(stderr, "Memory error\n"); exit(1); }
    op->type = ASM_OPERAND_STACK;
    op->stack_offset = offset;
    return op;
}

AsmInstruction *make_inst_mov(AsmOperand *src, AsmOperand *dst) {
    AsmInstruction *inst = calloc(1, sizeof(AsmInstruction));
    if (!inst) { fprintf(stderr, "Memory error\n"); exit(1); }
    inst->type = ASM_INST_MOV;
    inst->src = src;
    inst->dst = dst;
    return inst;
}

AsmInstruction *make_inst_binary(AsmBinaryOp op, AsmOperand *src, AsmOperand *dst) {
    AsmInstruction *inst = calloc(1, sizeof(AsmInstruction));
    if (!inst) { fprintf(stderr, "Memory error\n"); exit(1); }
    inst->type = ASM_INST_BINARY;
    inst->binary_op = op;
    inst->src = src;
    inst->dst = dst;
    return inst;
}

AsmInstruction *make_inst_idiv(AsmOperand *divisor) {
    AsmInstruction *inst = calloc(1, sizeof(AsmInstruction));
    if (!inst) { fprintf(stderr, "Memory error\n"); exit(1); }
    inst->type = ASM_INST_IDIV;
    inst->src = divisor;
    return inst;
}

AsmInstruction *make_inst_cdq(void) {
    AsmInstruction *inst = calloc(1, sizeof(AsmInstruction));
    if (!inst) { fprintf(stderr, "Memory error\n"); exit(1); }
    inst->type = ASM_INST_CDQ;
    return inst;
}

AsmInstruction *make_inst_cmp(AsmOperand *a, AsmOperand *b) {
    AsmInstruction *inst = calloc(1, sizeof(AsmInstruction));
    if (!inst) { fprintf(stderr, "Memory error\n"); exit(1); }
    inst->type = ASM_INST_CMP;
    inst->src = a;  /* a = cmpl src, dst does dst - src */
    inst->dst = b;
    return inst;
}

AsmInstruction *make_inst_jmp(const char *label) {
    AsmInstruction *inst = calloc(1, sizeof(AsmInstruction));
    if (!inst) { fprintf(stderr, "Memory error\n"); exit(1); }
    inst->type = ASM_INST_JMP;
    inst->label = strdup(label);
    return inst;
}

AsmInstruction *make_inst_jmpcc(CondCode cc, const char *label) {
    AsmInstruction *inst = calloc(1, sizeof(AsmInstruction));
    if (!inst) { fprintf(stderr, "Memory error\n"); exit(1); }
    inst->type = ASM_INST_JMPCC;
    inst->cond_code = cc;
    inst->label = strdup(label);
    return inst;
}

AsmInstruction *make_inst_setcc(CondCode cc, AsmOperand *dst) {
    AsmInstruction *inst = calloc(1, sizeof(AsmInstruction));
    if (!inst) { fprintf(stderr, "Memory error\n"); exit(1); }
    inst->type = ASM_INST_SETCC;
    inst->cond_code = cc;
    inst->operand = dst;
    return inst;
}

AsmInstruction *make_inst_label(const char *name) {
    AsmInstruction *inst = calloc(1, sizeof(AsmInstruction));
    if (!inst) { fprintf(stderr, "Memory error\n"); exit(1); }
    inst->type = ASM_INST_LABEL;
    inst->label = strdup(name);
    return inst;
}

AsmInstruction *make_inst_unary(AsmUnaryOp op, AsmOperand *operand) {
    AsmInstruction *inst = calloc(1, sizeof(AsmInstruction));
    if (!inst) { fprintf(stderr, "Memory error\n"); exit(1); }
    inst->type = ASM_INST_UNARY;
    inst->unary_op = op;
    inst->operand = operand;
    return inst;
}

AsmInstruction *make_inst_allocate_stack(int size) {
    AsmInstruction *inst = calloc(1, sizeof(AsmInstruction));
    if (!inst) { fprintf(stderr, "Memory error\n"); exit(1); }
    inst->type = ASM_INST_ALLOCATE_STACK;
    inst->src = make_operand_imm(size);  /* store size in src->imm */
    return inst;
}

AsmInstruction *make_inst_ret(void) {
    AsmInstruction *inst = calloc(1, sizeof(AsmInstruction));
    if (!inst) { fprintf(stderr, "Memory error\n"); exit(1); }
    inst->type = ASM_INST_RET;
    return inst;
}

AsmInstruction *append_instruction(AsmInstruction *head, AsmInstruction *new_inst) {
    if (!head) return new_inst;
    AsmInstruction *cur = head;
    while (cur->next) cur = cur->next;
    cur->next = new_inst;
    return head;
}

AsmInstruction *prepend_instruction(AsmInstruction *head, AsmInstruction *new_inst) {
    new_inst->next = head;
    return new_inst;
}

AsmFunction *make_asm_function(char *name, AsmInstruction *instructions) {
    AsmFunction *f = malloc(sizeof(AsmFunction));
    if (!f) { fprintf(stderr, "Memory error\n"); exit(1); }
    f->type = ASM_FUNCTION;
    f->name = strdup(name);
    f->instructions = instructions;
    return f;
}

AsmProgram *make_asm_program(AsmFunction *function) {
    AsmProgram *p = malloc(sizeof(AsmProgram));
    if (!p) { fprintf(stderr, "Memory error\n"); exit(1); }
    p->type = ASM_PROGRAM;
    p->function = function;
    return p;
}

/* === Pretty-print === */

static void asm_ast_print_operand(AsmOperand *op) {
    if (!op) { printf("NULL"); return; }
    switch (op->type) {
        case ASM_OPERAND_IMM:    printf("Imm(%d)", op->imm); break;
        case ASM_OPERAND_REG: {
            const char *rn[] = {"AX","DX","R10","R11"};
            printf("Reg(%s)", rn[op->reg]); break;
        }
        case ASM_OPERAND_PSEUDO: printf("Pseudo(%s)", op->pseudo_name); break;
        case ASM_OPERAND_STACK:  printf("Stack(%d)", op->stack_offset); break;
    }
}

void asm_ast_print(AsmProgram *prog) {
    if (!prog || !prog->function) return;
    printf("Program(\n  Function(\n    name=\"%s\",\n    instructions=[\n", prog->function->name);
    AsmInstruction *inst = prog->function->instructions;
    while (inst) {
        switch (inst->type) {
            case ASM_INST_MOV:
                printf("      Mov("); asm_ast_print_operand(inst->src);
                printf(", ");        asm_ast_print_operand(inst->dst);
                printf(")\n"); break;
            case ASM_INST_UNARY:
                printf("      Unary(%s, ", inst->unary_op == ASM_UNARY_NEG ? "Neg" : "Not");
                asm_ast_print_operand(inst->operand);
                printf(")\n"); break;
            case ASM_INST_BINARY: {
                const char *bn[] = {"Add","Sub","Mult"};
                printf("      Binary(%s, ", bn[inst->binary_op]);
                asm_ast_print_operand(inst->src);
                printf(", "); asm_ast_print_operand(inst->dst);
                printf(")\n"); break;
            }
            case ASM_INST_IDIV:
                printf("      Idiv("); asm_ast_print_operand(inst->src); printf(")\n"); break;
            case ASM_INST_CDQ:
                printf("      Cdq\n"); break;
            case ASM_INST_CMP:
                printf("      Cmp("); asm_ast_print_operand(inst->src);
                printf(", "); asm_ast_print_operand(inst->dst); printf(")\n"); break;
            case ASM_INST_JMP:
                printf("      Jmp(\"%s\")\n", inst->label); break;
            case ASM_INST_JMPCC: {
                const char *cn[] = {"E","NE","G","GE","L","LE"};
                printf("      JmpCC(%s, \"%s\")\n", cn[inst->cond_code], inst->label);
                break;
            }
            case ASM_INST_SETCC: {
                const char *cn[] = {"E","NE","G","GE","L","LE"};
                printf("      SetCC(%s, ", cn[inst->cond_code]);
                asm_ast_print_operand(inst->operand); printf(")\n"); break;
            }
            case ASM_INST_LABEL:
                printf("      Label(\"%s\")\n", inst->label); break;
            case ASM_INST_ALLOCATE_STACK:
                printf("      AllocateStack(%d)\n", inst->src ? inst->src->imm : 0); break;
            case ASM_INST_RET:
                printf("      Ret\n"); break;
            default: break;
        }
        inst = inst->next;
    }
    printf("    ]\n  )\n)\n");
}

/* === Free === */

void asm_ast_free_operand(AsmOperand *op) {
    if (!op) return;
    if (op->type == ASM_OPERAND_PSEUDO) free(op->pseudo_name);
    free(op);
}

void asm_ast_free(AsmProgram *prog) {
    if (!prog) return;
    if (prog->function) {
        free(prog->function->name);
        AsmInstruction *inst = prog->function->instructions;
        while (inst) {
            AsmInstruction *next = inst->next;
            asm_ast_free_operand(inst->src);
            asm_ast_free_operand(inst->dst);
            asm_ast_free_operand(inst->operand);
            if (inst->label) free(inst->label);
            free(inst);
            inst = next;
        }
        free(prog->function);
    }
    free(prog);
}
