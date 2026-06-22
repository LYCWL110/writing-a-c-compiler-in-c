#ifndef AST_H
#define AST_H

/* === C Abstract Syntax Tree definitions ===
 * Chapter 2 adds:
 *   exp = Constant(int) | Unary(unary_operator, exp)
 *   unary_operator = Complement | Negate
 */

typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_STATEMENT_RETURN,
    AST_EXP_CONSTANT,
    AST_EXP_UNARY
} AstNodeType;

typedef enum {
    UNARY_COMPLEMENT,   /* ~ */
    UNARY_NEGATE        /* - */
} UnaryOperator;

/* Forward declarations */
typedef struct Exp Exp;
typedef struct Statement Statement;
typedef struct FunctionDef FunctionDef;
typedef struct Program Program;

/* Expression: constant or unary operation */
struct Exp {
    AstNodeType type;
    union {
        int value;                     /* AST_EXP_CONSTANT */
        struct {
            UnaryOperator operator;    /* AST_EXP_UNARY */
            struct Exp *operand;
        } unary;
    };
};

/* Statement: only return statement */
struct Statement {
    AstNodeType type;   /* AST_STATEMENT_RETURN */
    Exp *return_val;
};

/* Function definition */
struct FunctionDef {
    AstNodeType type;   /* AST_FUNCTION */
    char *name;         /* function name (e.g., "main") */
    Statement *body;
};

/* Program: root node */
struct Program {
    AstNodeType type;   /* AST_PROGRAM */
    FunctionDef *function;
};

/* Constructor functions */
Exp  *make_exp_constant(int value);
Exp  *make_exp_unary(UnaryOperator op, Exp *operand);
Statement *make_statement_return(Exp *return_val);
FunctionDef *make_function_def(char *name, Statement *body);
Program *make_program(FunctionDef *function);

/* Pretty-print AST for debugging */
void ast_print(Program *prog);

/* Free AST memory */
void ast_free_exp(Exp *e);
void ast_free(Program *prog);

#endif /* AST_H */
