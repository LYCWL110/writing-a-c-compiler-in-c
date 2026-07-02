#ifndef AST_H
#define AST_H

typedef enum {
    AST_PROGRAM, AST_FUNCTION,
    AST_STATEMENT_RETURN, AST_STATEMENT_EXPRESSION, AST_STATEMENT_NULL,
    AST_EXP_CONSTANT, AST_EXP_UNARY, AST_EXP_BINARY, AST_EXP_VAR, AST_EXP_ASSIGNMENT,
    AST_DECLARATION, AST_BLOCK_ITEM
} AstNodeType;

typedef enum {
    UNARY_COMPLEMENT, UNARY_NEGATE, UNARY_NOT
} UnaryOperator;

typedef enum {
    BINARY_ADD, BINARY_SUBTRACT, BINARY_MULTIPLY, BINARY_DIVIDE, BINARY_REMAINDER,
    BINARY_AND, BINARY_OR, BINARY_EQUAL, BINARY_NOT_EQUAL,
    BINARY_LESS, BINARY_LESS_EQ, BINARY_GREATER, BINARY_GREATER_EQ
} BinaryOperator;

typedef struct Exp Exp;
typedef struct Statement Statement;
typedef struct Declaration Declaration;
typedef struct BlockItem BlockItem;
typedef struct FunctionDef FunctionDef;
typedef struct Program Program;

struct Exp {
    AstNodeType type;
    union {
        int value;                              /* AST_EXP_CONSTANT */
        struct { UnaryOperator operator; struct Exp *operand; } unary;  /* AST_EXP_UNARY */
        struct { BinaryOperator operator; struct Exp *left, *right; } binary; /* AST_EXP_BINARY */
        char *var_name;                         /* AST_EXP_VAR */
        struct { struct Exp *left, *right; } assign; /* AST_EXP_ASSIGNMENT */
    };
};

struct Statement {
    AstNodeType type;
    union {
        Exp *return_val;    /* AST_STATEMENT_RETURN */
        Exp *expr;          /* AST_STATEMENT_EXPRESSION */
    };
};

struct Declaration {
    AstNodeType type;       /* AST_DECLARATION */
    char *name;             /* unique name (after resolution) */
    Exp *init;              /* optional initializer */
};

struct BlockItem {
    AstNodeType type;       /* AST_BLOCK_ITEM */
    int is_declaration;     /* 1=declaration, 0=statement */
    union {
        Declaration *decl;
        Statement *stmt;
    };
};

struct FunctionDef {
    AstNodeType type;       /* AST_FUNCTION */
    char *name;
    BlockItem **body;       /* array of block items */
    int body_count;         /* number of items */
};

struct Program {
    AstNodeType type;       /* AST_PROGRAM */
    FunctionDef *function;
};

/* Constructors */
Exp  *make_exp_constant(int value);
Exp  *make_exp_unary(UnaryOperator op, Exp *operand);
Exp  *make_exp_binary(BinaryOperator op, Exp *left, Exp *right);
Exp  *make_exp_var(char *name);
Exp  *make_exp_assignment(Exp *left, Exp *right);
Statement *make_statement_return(Exp *return_val);
Statement *make_statement_expression(Exp *expr);
Statement *make_statement_null(void);
Declaration *make_declaration(char *name, Exp *init);
BlockItem *make_block_item_stmt(Statement *stmt);
BlockItem *make_block_item_decl(Declaration *decl);
FunctionDef *make_function_def(char *name, BlockItem **body, int count);
Program *make_program(FunctionDef *function);

void ast_print(Program *prog);
void ast_free_exp(Exp *e);
void ast_free(Program *prog);

#endif
