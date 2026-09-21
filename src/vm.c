/* vm.c — see vm.h */
#include "vm.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const struct { const char *name; opcode op; int has_operand; } OPCODES[] = {
    {"PUSH", OP_PUSH, 1}, {"ADD", OP_ADD, 0}, {"SUB", OP_SUB, 0},
    {"MUL", OP_MUL, 0}, {"DIV", OP_DIV, 0}, {"MOD", OP_MOD, 0},
    {"EQ", OP_EQ, 0}, {"LT", OP_LT, 0}, {"GT", OP_GT, 0},
    {"NOT", OP_NOT, 0}, {"PRINT", OP_PRINT, 0}, {"POP", OP_POP, 0},
    {"LOAD", OP_LOAD, 1}, {"STORE", OP_STORE, 1},
    {"JUMP", OP_JUMP, 1}, {"JUMP_IF_FALSE", OP_JUMP_IF_FALSE, 1},
    {"HALT", OP_HALT, 0},
};

const char *vm_opcode_name(opcode op) {
    for (size_t i = 0; i < sizeof OPCODES / sizeof OPCODES[0]; i++)
        if (OPCODES[i].op == op) return OPCODES[i].name;
    return "?";
}

static int opcode_by_name(const char *name, opcode *out) {
    for (size_t i = 0; i < sizeof OPCODES / sizeof OPCODES[0]; i++)
        if (strcmp(OPCODES[i].name, name) == 0) { *out = OPCODES[i].op; return 1; }
    return 0;
}

static int has_operand(opcode op) {
    return op == OP_PUSH || op == OP_LOAD || op == OP_STORE ||
           op == OP_JUMP || op == OP_JUMP_IF_FALSE;
}

/* --- assembler --- */

int vm_assemble(const char *source, program *out, char *err, size_t errcap) {
    memset(out, 0, sizeof *out);
    size_t code_cap = 256;
    out->code = malloc(code_cap);
    double consts[256];
    size_t nconst = 0;

    char line[256];
    int line_no = 0;
    const char *cursor = source;
    while (*cursor) {
        const char *nl = strchr(cursor, '\n');
        size_t len = nl ? (size_t)(nl - cursor) : strlen(cursor);
        if (len >= sizeof line) len = sizeof line - 1;
        memcpy(line, cursor, len);
        line[len] = 0;
        cursor += len ? len : 1;
        if (nl) cursor = nl + 1; else if (!*cursor) { /* last line */ }
        line_no++;

        /* strip comments (#) and trim */
        char *hash = strchr(line, '#');
        if (hash) *hash = 0;
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        char *end = p + strlen(p);
        while (end > p && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r')) *--end = 0;
        if (!*p) continue;

        /* token = first word */
        char word[64] = {0};
        int w = 0;
        while (*p && *p != ' ' && *p != '\t' && w < 63) word[w++] = *p++;
        word[w] = 0;
        while (*p == ' ' || *p == '\t') p++;
        char *operand_str = p;

        opcode op;
        if (!opcode_by_name(word, &op)) {
            snprintf(err, errcap, "line %d: unknown instruction %s", line_no, word);
            return -1;
        }
        if (out->codelen + 1 >= code_cap) {
            code_cap *= 2;
            out->code = realloc(out->code, code_cap);
        }
        out->code[out->codelen++] = (uint8_t)op;
        if (has_operand(op)) {
            if (!*operand_str) {
                snprintf(err, errcap, "line %d: %s needs an operand", line_no, word);
                return -1;
            }
            if (op == OP_PUSH) {
                double value = strtod(operand_str, NULL);
                size_t found = nconst;
                for (size_t i = 0; i < nconst; i++)
                    if (consts[i] == value) { found = i; break; }
                if (found == nconst && nconst < 256) consts[nconst++] = value;
                out->code[out->codelen++] = (uint8_t)found;
            } else {
                long slot = strtol(operand_str, NULL, 10);
                if (slot < 0 || slot > 255) {
                    snprintf(err, errcap, "line %d: slot out of range", line_no);
                    return -1;
                }
                out->code[out->codelen++] = (uint8_t)slot;
            }
        }
        if (!*cursor) break;
    }

    out->constants = malloc((nconst ? nconst : 1) * sizeof *out->constants);
    memcpy(out->constants, consts, nconst * sizeof *out->constants);
    out->nconstants = nconst;
    return 0;
}

/* --- disassembler --- */

int vm_disassemble(const program *p, char *out, size_t cap) {
    size_t pos = 0;
    size_t ip = 0;
    while (ip < p->codelen) {
        uint8_t byte = p->code[ip];
        if (byte >= OP_COUNT) return -1;
        const char *name = vm_opcode_name((opcode)byte);
        if (has_operand((opcode)byte)) {
            if (ip + 1 >= p->codelen) return -1;
            int operand = p->code[ip + 1];
            const char *fmt = (byte == (uint8_t)OP_PUSH)
                ? "%04zu %-16s %d (%g)\n"
                : "%04zu %-16s %d\n";
            pos += (size_t)snprintf(out + pos, cap - pos, fmt,
                                    ip, name, operand,
                                    byte == (uint8_t)OP_PUSH ? p->constants[operand] : operand);
            ip += 2;
        } else {
            pos += (size_t)snprintf(out + pos, cap - pos, "%04zu %s\n", ip, name);
            ip += 1;
        }
        if (pos >= cap) return -1;
    }
    return 0;
}

/* --- interpreter --- */

#define STACK_SIZE 256

int vm_run(const program *p, size_t max_steps) {
    double stack[STACK_SIZE];
    size_t sp = 0;
    double slots[256] = {0};
    size_t ip = 0;
    size_t steps = 0;

    while (ip < p->codelen) {
        if (++steps > max_steps)
            return 3; /* step limit exceeded (infinite loop) */
        uint8_t op = p->code[ip++];
        switch (op) {
        case OP_PUSH:
            if (sp >= STACK_SIZE) return 2;
            stack[sp++] = p->constants[p->code[ip++]];
            break;
        case OP_LOAD: {
            if (sp >= STACK_SIZE) return 2;
            stack[sp++] = slots[p->code[ip++]];
            break;
        }
        case OP_STORE: {
            if (sp == 0) return 2;
            uint8_t slot = p->code[ip++];
            slots[slot] = stack[--sp];
            break;
        }
        case OP_ADD: case OP_SUB: case OP_MUL: case OP_DIV: case OP_MOD: {
            if (sp < 2) return 2;
            double b = stack[--sp], a = stack[--sp];
            double r = 0;
            if (op == OP_ADD) r = a + b;
            else if (op == OP_SUB) r = a - b;
            else if (op == OP_MUL) r = a * b;
            else if (op == OP_DIV) { if (b == 0) return 2; r = a / b; }
            else r = fmod(a, b);
            stack[sp++] = r;
            break;
        }
        case OP_EQ: {
            if (sp < 2) return 2;
            double b = stack[--sp], a = stack[--sp];
            stack[sp++] = (a == b) ? 1 : 0;
            break;
        }
        case OP_LT: {
            if (sp < 2) return 2;
            double b = stack[--sp], a = stack[--sp];
            stack[sp++] = (a < b) ? 1 : 0;
            break;
        }
        case OP_GT: {
            if (sp < 2) return 2;
            double b = stack[--sp], a = stack[--sp];
            stack[sp++] = (a > b) ? 1 : 0;
            break;
        }
        case OP_NOT: {
            if (sp == 0) return 2;
            stack[sp - 1] = stack[sp - 1] == 0 ? 1 : 0;
            break;
        }
        case OP_PRINT: {
            if (sp == 0) return 2;
            printf("%g\n", stack[--sp]);
            break;
        }
        case OP_POP:
            if (sp == 0) return 2;
            sp--;
            break;
        case OP_JUMP:
            if (ip >= p->codelen) return 2;
            ip = p->code[ip];
            break;
        case OP_JUMP_IF_FALSE: {
            if (ip >= p->codelen) return 2;
            uint8_t target = p->code[ip++];
            if (sp == 0) return 2;
            double cond = stack[--sp];
            if (cond == 0) ip = target;
            break;
        }
        case OP_HALT:
            return 0;
        default:
            return 2;
        }
    }
    return 0;
}

void vm_program_free(program *p) {
    free(p->constants);
    free(p->code);
    memset(p, 0, sizeof *p);
}

#include <math.h>
