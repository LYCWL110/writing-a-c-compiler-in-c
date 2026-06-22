#ifndef AST_H
#define AST_H

/* === C Abstract Syntax Tree definitions ===
 * Chapter 1 supports only:
 *   program = Program(function_definition)
 *   function_definition = Function(identifier name, statement body)
 *   statement = Return(exp)
 *   exp = Constant(int)
 */

typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_STATEMENT_RETURN,
    AST_EXP_CONSTANT
} AstNodeType;

/* Forward declarations */
typedef struct Exp Exp;
typedef struct Statement Statement;
typedef struct FunctionDef FunctionDef;
typedef struct Program Program;

/* Expression: only integer constants in Chapter 1 */
struct Exp {
    AstNodeType type;   /* AST_EXP_CONSTANT */
    int value;
};

/* Statement: only return statement in Chapter 1 */
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
Statement *make_statement_return(Exp *return_val);
FunctionDef *make_function_def(char *name, Statement *body);
Program *make_program(FunctionDef *function);

/* Pretty-print AST for debugging */
void ast_print(Program *prog);

/* Free AST memory */
void ast_free(Program *prog);

#endif /* AST_H */
