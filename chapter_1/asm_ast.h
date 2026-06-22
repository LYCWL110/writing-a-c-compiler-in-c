#ifndef ASM_AST_H
#define ASM_AST_H

/* === Assembly AST definitions ===
 * Chapter 1 supports only:
 *   program_asm = Program(function_definition)
 *   function_definition_asm = Function(identifier name, instruction* instructions)
 *   instruction = Mov(operand src, operand dst) | Ret
 *   operand = Imm(int) | Register
 */

typedef enum {
    ASM_PROGRAM,
    ASM_FUNCTION,
    ASM_INST_MOV,
    ASM_INST_RET
} AsmInstType;

typedef enum {
    ASM_OPERAND_IMM,
    ASM_OPERAND_REG
} AsmOperandType;

typedef struct AsmOperand AsmOperand;
typedef struct AsmInstruction AsmInstruction;
typedef struct AsmFunction AsmFunction;
typedef struct AsmProgram AsmProgram;

/* Operand: immediate integer or register */
struct AsmOperand {
    AsmOperandType type;
    union {
        int imm;             /* for ASM_OPERAND_IMM */
        char *reg_name;      /* for ASM_OPERAND_REG, e.g., "eax" */
    };
};

/* Instruction: Mov or Ret */
struct AsmInstruction {
    AsmInstType type;
    AsmInstruction *next;   /* linked list of instructions */
    AsmOperand *src;        /* for Mov */
    AsmOperand *dst;        /* for Mov */
};

/* Function: name + instruction list */
struct AsmFunction {
    AsmInstType type;       /* ASM_FUNCTION */
    char *name;
    AsmInstruction *instructions; /* linked list */
};

/* Program: root node */
struct AsmProgram {
    AsmInstType type;       /* ASM_PROGRAM */
    AsmFunction *function;
};

/* Constructors */
AsmOperand *make_operand_imm(int value);
AsmOperand *make_operand_reg(char *reg_name);
AsmInstruction *make_inst_mov(AsmOperand *src, AsmOperand *dst);
AsmInstruction *make_inst_ret(void);
AsmInstruction *append_instruction(AsmInstruction *head, AsmInstruction *new_inst);
AsmFunction *make_asm_function(char *name, AsmInstruction *instructions);
AsmProgram *make_asm_program(AsmFunction *function);

/* Pretty-print assembly AST for debugging */
void asm_ast_print(AsmProgram *prog);

/* Free assembly AST memory */
void asm_ast_free(AsmProgram *prog);

#endif /* ASM_AST_H */
