#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tacky.h"

/* === Constructors === */

TackyVal *tacky_val_constant(int value) {
    TackyVal *v = malloc(sizeof(TackyVal));
    if (!v) { fprintf(stderr, "Memory error\n"); exit(1); }
    v->type = TACKY_VAL_CONSTANT;
    v->value = value;
    return v;
}

TackyVal *tacky_val_var(const char *name) {
    TackyVal *v = malloc(sizeof(TackyVal));
    if (!v) { fprintf(stderr, "Memory error\n"); exit(1); }
    v->type = TACKY_VAL_VAR;
    v->name = strdup(name);
    return v;
}

TackyInstruction *tacky_inst_return(TackyVal *val) {
    TackyInstruction *inst = malloc(sizeof(TackyInstruction));
    if (!inst) { fprintf(stderr, "Memory error\n"); exit(1); }
    inst->type = TACKY_INST_RETURN;
    inst->next = NULL;
    inst->val = val;
    return inst;
}

TackyInstruction *tacky_inst_unary(UnaryOperator op, TackyVal *src, TackyVal *dst) {
    TackyInstruction *inst = malloc(sizeof(TackyInstruction));
    if (!inst) { fprintf(stderr, "Memory error\n"); exit(1); }
    inst->type = TACKY_INST_UNARY;
    inst->next = NULL;
    inst->unary_op = op;
    inst->src = src;
    inst->dst = dst;
    return inst;
}

TackyInstruction *tacky_inst_binary(BinaryOperator op, TackyVal *src1, TackyVal *src2, TackyVal *dst) {
    TackyInstruction *inst = malloc(sizeof(TackyInstruction));
    if (!inst) { fprintf(stderr, "Memory error\n"); exit(1); }
    inst->type = TACKY_INST_BINARY;
    inst->next = NULL;
    inst->binary_op = op;
    inst->src = src1;
    inst->src2 = src2;
    inst->dst = dst;
    return inst;
}

TackyInstruction *tacky_append_instruction(TackyInstruction *head, TackyInstruction *inst) {
    if (!head) return inst;
    TackyInstruction *cur = head;
    while (cur->next) cur = cur->next;
    cur->next = inst;
    return head;
}

TackyFunction *tacky_function(const char *name, TackyInstruction *body) {
    TackyFunction *f = malloc(sizeof(TackyFunction));
    if (!f) { fprintf(stderr, "Memory error\n"); exit(1); }
    f->name = strdup(name);
    f->body = body;
    return f;
}

TackyProgram *tacky_program(TackyFunction *function) {
    TackyProgram *p = malloc(sizeof(TackyProgram));
    if (!p) { fprintf(stderr, "Memory error\n"); exit(1); }
    p->function = function;
    return p;
}

/* === Pretty-print === */

static const char *unary_op_name(UnaryOperator op) {
    return op == UNARY_COMPLEMENT ? "Complement" : "Negate";
}

void tacky_print_val(TackyVal *v) {
    if (v->type == TACKY_VAL_CONSTANT) {
        printf("Constant(%d)", v->value);
    } else {
        printf("Var(\"%s\")", v->name);
    }
}

void tacky_print(TackyProgram *prog) {
    if (!prog || !prog->function) return;
    printf("Program(\n  Function(\n    name=\"%s\",\n    instructions=[\n",
           prog->function->name);
    TackyInstruction *inst = prog->function->body;
    while (inst) {
        if (inst->type == TACKY_INST_RETURN) {
            printf("      Return(");
            tacky_print_val(inst->val);
            printf(")\n");
        } else if (inst->type == TACKY_INST_UNARY) {
            printf("      Unary(%s, ", unary_op_name(inst->unary_op));
            tacky_print_val(inst->src);
            printf(", ");
            tacky_print_val(inst->dst);
            printf(")\n");
        } else {
            /* Use the binary op name from ast.h */
            static const char *bn[] = {"Add","Subtract","Multiply","Divide","Remainder"};
            printf("      Binary(%s, ", bn[inst->binary_op]);
            tacky_print_val(inst->src);
            printf(", ");
            tacky_print_val(inst->src2);
            printf(", ");
            tacky_print_val(inst->dst);
            printf(")\n");
        }
        inst = inst->next;
    }
    printf("    ]\n  )\n)\n");
}

/* === Free === */

void tacky_free_val(TackyVal *v) {
    if (!v) return;
    if (v->type == TACKY_VAL_VAR) free(v->name);
    free(v);
}

void tacky_free_instructions(TackyInstruction *inst) {
    while (inst) {
        TackyInstruction *next = inst->next;
        if (inst->type == TACKY_INST_UNARY) {
            tacky_free_val(inst->src);
            tacky_free_val(inst->dst);
        } else if (inst->type == TACKY_INST_BINARY) {
            tacky_free_val(inst->src);
            tacky_free_val(inst->src2);
            tacky_free_val(inst->dst);
        } else {
            tacky_free_val(inst->val);
        }
        free(inst);
        inst = next;
    }
}

void tacky_free(TackyProgram *prog) {
    if (prog) {
        if (prog->function) {
            free(prog->function->name);
            tacky_free_instructions(prog->function->body);
            free(prog->function);
        }
        free(prog);
    }
}
