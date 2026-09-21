/* vm.h — stack bytecode VM: opcodes, assembler, disassembler, interpreter. */
#ifndef VM_H
#define VM_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    OP_PUSH,   /* operand: constant index */
    OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
    OP_EQ, OP_LT, OP_GT,
    OP_NOT,
    OP_PRINT,     /* pop & print */
    OP_POP,
    OP_LOAD,      /* operand: slot */
    OP_STORE,     /* operand: slot */
    OP_JUMP,      /* operand: target */
    OP_JUMP_IF_FALSE, /* operand: target */
    OP_HALT,
    OP_COUNT
} opcode;

typedef struct {
    double *constants;
    size_t nconstants;
    uint8_t *code;
    size_t codelen;
} program;

/* Assembles text like "PUSH 0\nPUSH 1\nADD\nPRINT\nHALT" into a program. */
int vm_assemble(const char *source, program *out, char *err, size_t errcap);

/* Disassembles into text (for tests and debugging). */
int vm_disassemble(const program *p, char *out, size_t cap);

/* Interprets the program. Returns exit code; prints on OP_PRINT.
 * max_steps guards against infinite loops. */
int vm_run(const program *p, size_t max_steps);

void vm_program_free(program *p);

/* exposed for tests */
extern const char *vm_opcode_name(opcode op);

#endif
