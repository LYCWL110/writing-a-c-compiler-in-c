#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

/* Convert a C AST expression to an assembly operand */
static AsmOperand *codegen_exp(Exp *e) {
    if (e->type != AST_EXP_CONSTANT) {
        fprintf(stderr, "Codegen error: unsupported expression type\n");
        return NULL;
    }
    /* Constant(int) → Imm(int) */
    return make_operand_imm(e->value);
}

/* Convert a C AST statement to a list of assembly instructions */
static AsmInstruction *codegen_statement(Statement *s) {
    if (s->type != AST_STATEMENT_RETURN) {
        fprintf(stderr, "Codegen error: unsupported statement type\n");
        return NULL;
    }

    /* Return(exp) → Mov(src, %eax) then Ret */
    AsmOperand *src = codegen_exp(s->return_val);
    if (!src) return NULL;

    AsmOperand *dst = make_operand_reg("eax");

    AsmInstruction *instructions = NULL;
    AsmInstruction *mov = make_inst_mov(src, dst);
    AsmInstruction *ret = make_inst_ret();
    instructions = append_instruction(instructions, mov);
    instructions = append_instruction(instructions, ret);

    return instructions;
}

/* Convert a C AST function definition to an assembly function */
static AsmFunction *codegen_function(FunctionDef *func) {
    AsmInstruction *instructions = codegen_statement(func->body);
    if (!instructions) return NULL;
    return make_asm_function(func->name, instructions);
}

/* Convert C AST program to assembly AST program */
AsmProgram *codegen(Program *prog) {
    if (!prog || !prog->function) {
        fprintf(stderr, "Codegen error: empty program\n");
        return NULL;
    }
    AsmFunction *asm_func = codegen_function(prog->function);
    if (!asm_func) return NULL;
    return make_asm_program(asm_func);
}
