#ifndef TACKY_H
#define TACKY_H

#include "ast.h"

/* === TACKY Intermediate Representation ===
 * Three-address code for bridging high-level AST to low-level assembly.
 *
 *   program = Program(function_definition)
 *   function_definition = Function(identifier, instruction* body)
 *   instruction = Return(val) | Unary(unary_operator, val src, val dst)
 *   val = Constant(int) | Var(identifier)
 */

typedef enum {
    TACKY_VAL_CONSTANT,
    TACKY_VAL_VAR
} TackyValType;

typedef struct TackyVal {
    TackyValType type;
    union {
        int value;          /* TACKY_VAL_CONSTANT */
        char *name;         /* TACKY_VAL_VAR — temporary variable name */
    };
} TackyVal;

typedef enum {
    TACKY_INST_RETURN,
    TACKY_INST_UNARY,
    TACKY_INST_BINARY
} TackyInstType;

typedef struct TackyInstruction TackyInstruction;

struct TackyInstruction {
    TackyInstType type;
    TackyInstruction *next;   /* linked list */

    /* For TACKY_INST_RETURN */
    TackyVal *val;

    /* For TACKY_INST_UNARY */
    UnaryOperator unary_op;
    TackyVal *src;
    TackyVal *dst;

    /* For TACKY_INST_BINARY */
    BinaryOperator binary_op;
    TackyVal *src2;    /* second source operand */
};

typedef struct {
    char *name;
    TackyInstruction *body;   /* instruction list head */
} TackyFunction;

typedef struct {
    TackyFunction *function;
} TackyProgram;

/* Constructors */
TackyVal *tacky_val_constant(int value);
TackyVal *tacky_val_var(const char *name);
TackyInstruction *tacky_inst_return(TackyVal *val);
TackyInstruction *tacky_inst_unary(UnaryOperator op, TackyVal *src, TackyVal *dst);
TackyInstruction *tacky_inst_binary(BinaryOperator op, TackyVal *src1, TackyVal *src2, TackyVal *dst);
TackyInstruction *tacky_append_instruction(TackyInstruction *head, TackyInstruction *inst);
TackyFunction *tacky_function(const char *name, TackyInstruction *body);
TackyProgram *tacky_program(TackyFunction *function);

/* Pretty-print for debugging */
void tacky_print(TackyProgram *prog);
void tacky_print_val(TackyVal *v);

/* Free memory */
void tacky_free_val(TackyVal *v);
void tacky_free_instructions(TackyInstruction *inst);
void tacky_free(TackyProgram *prog);

#endif /* TACKY_H */
