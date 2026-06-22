#ifndef ASM_AST_H
#define ASM_AST_H

/* === Assembly AST definitions ===
 * Chapter 2 adds:
 *   instruction = ... | Unary(unary_operator, operand) | AllocateStack(int)
 *   operand = ... | Reg(reg) | Pseudo(identifier) | Stack(int)
 */

typedef enum {
    ASM_PROGRAM,
    ASM_FUNCTION,
    ASM_INST_MOV,
    ASM_INST_UNARY,
    ASM_INST_ALLOCATE_STACK,
    ASM_INST_RET
} AsmInstType;

typedef enum {
    ASM_OPERAND_IMM,
    ASM_OPERAND_REG,
    ASM_OPERAND_PSEUDO,
    ASM_OPERAND_STACK
} AsmOperandType;

typedef enum {
    ASM_UNARY_NEG,
    ASM_UNARY_NOT
} AsmUnaryOp;

typedef enum {
    REG_AX,    /* %eax */
    REG_R10    /* %r10d */
} RegId;

typedef struct AsmOperand AsmOperand;
typedef struct AsmInstruction AsmInstruction;
typedef struct AsmFunction AsmFunction;
typedef struct AsmProgram AsmProgram;

struct AsmOperand {
    AsmOperandType type;
    union {
        int imm;             /* ASM_OPERAND_IMM */
        RegId reg;           /* ASM_OPERAND_REG */
        char *pseudo_name;   /* ASM_OPERAND_PSEUDO */
        int stack_offset;    /* ASM_OPERAND_STACK, e.g. -4 */
    };
};

struct AsmInstruction {
    AsmInstType type;
    AsmInstruction *next;    /* linked list of instructions */
    AsmOperand *src;         /* for Mov */
    AsmOperand *dst;         /* for Mov */
    AsmUnaryOp unary_op;     /* for Unary */
    AsmOperand *operand;     /* for Unary (src and dst are the same) */
};

struct AsmFunction {
    AsmInstType type;        /* ASM_FUNCTION */
    char *name;
    AsmInstruction *instructions;
};

struct AsmProgram {
    AsmInstType type;        /* ASM_PROGRAM */
    AsmFunction *function;
};

/* Constructors */
AsmOperand *make_operand_imm(int value);
AsmOperand *make_operand_reg(RegId reg);
AsmOperand *make_operand_pseudo(const char *name);
AsmOperand *make_operand_stack(int offset);
AsmInstruction *make_inst_mov(AsmOperand *src, AsmOperand *dst);
AsmInstruction *make_inst_unary(AsmUnaryOp op, AsmOperand *operand);
AsmInstruction *make_inst_allocate_stack(int size);
AsmInstruction *make_inst_ret(void);
AsmInstruction *append_instruction(AsmInstruction *head, AsmInstruction *new_inst);
AsmInstruction *prepend_instruction(AsmInstruction *head, AsmInstruction *new_inst);
AsmFunction *make_asm_function(char *name, AsmInstruction *instructions);
AsmProgram *make_asm_program(AsmFunction *function);

/* Pretty-print for debugging */
void asm_ast_print(AsmProgram *prog);

/* Free memory */
void asm_ast_free_operand(AsmOperand *op);
void asm_ast_free(AsmProgram *prog);

#endif /* ASM_AST_H */
