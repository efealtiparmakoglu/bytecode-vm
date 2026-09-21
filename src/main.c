/* main.c — bytecode-vm CLI: assemble, run, disassemble. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"

static void usage(void) {
    fprintf(stderr,
        "bytecode-vm 1.0.0 - stack bytecode VM\n\n"
        "USAGE:\n"
        "    bytecode-vm run FILE.vm      assemble + execute\n"
        "    bytecode-vm dis FILE.vm      disassemble\n"
        "    bytecode-vm --help\n");
}

int main(int argc, char **argv) {
    if (argc < 2) { usage(); return 2; }
    if (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) { usage(); return 0; }

    const char *mode = argv[1];
    int disas = strcmp(mode, "dis") == 0;
    if (strcmp(mode, "run") != 0 && !disas) { usage(); return 2; }
    if (argc < 3) { fprintf(stderr, "bytecode-vm: %s needs a file\n", mode); return 2; }

    FILE *fh = fopen(argv[2], "r");
    if (!fh) { perror("bytecode-vm"); return 1; }
    fseek(fh, 0, SEEK_END);
    long size = ftell(fh);
    fseek(fh, 0, SEEK_SET);
    char *source = malloc((size_t)size + 1);
    fread(source, 1, (size_t)size, fh);
    source[size] = 0;
    fclose(fh);

    program prog;
    char err[256] = {0};
    if (vm_assemble(source, &prog, err, sizeof err) != 0) {
        fprintf(stderr, "bytecode-vm: assemble failed: %s\n", err);
        return 2;
    }

    if (disas) {
        char *out = malloc(1 << 16);
        if (vm_disassemble(&prog, out, 1 << 16) != 0) {
            fprintf(stderr, "bytecode-vm: disassemble failed\n");
            return 1;
        }
        fputs(out, stdout);
        free(out);
    } else {
        int code = vm_run(&prog, 1000000);
        if (code == 3) {
            fprintf(stderr, "bytecode-vm: step limit exceeded (infinite loop?)\n");
            vm_program_free(&prog);
            return 1;
        }
        if (code != 0) {
            fprintf(stderr, "bytecode-vm: runtime error %d\n", code);
            vm_program_free(&prog);
            return 1;
        }
    }
    vm_program_free(&prog);
    return 0;
}
