# bytecode-vm

**A stack bytecode VM in pure C — assembler, disassembler, interpreter, 17 opcodes. Zero dependencies.**

[![CI](https://github.com/efealtiparmakoglu/bytecode-vm/actions/workflows/ci.yml/badge.svg)](https://github.com/efealtiparmakoglu/bytecode-vm/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C11](https://img.shields.io/badge/standard-C11-blue.svg)]()

```
$ cat demo.vm
PUSH 0
STORE 0
LOAD 0
PUSH 3
LT
JUMP_IF_FALSE 23
LOAD 0
PRINT
LOAD 0
PUSH 1
ADD
STORE 0
JUMP 4
HALT

$ bytecode-vm run demo.vm
0
1
2
```

A complete stack machine: a line-based assembler (text → bytecode), a
disassembler (bytecode → text), and an interpreter with a 256-slot value
stack, 256 variable slots, jumps, and a step limit that catches infinite
loops.

## Opcodes

`PUSH n` `ADD` `SUB` `MUL` `DIV` `MOD` `EQ` `LT` `GT` `NOT` `PRINT` `POP`
`LOAD slot` `STORE slot` `JUMP target` `JUMP_IF_FALSE target` `HALT`

`JUMP`/`JUMP_IF_FALSE` targets are byte offsets. Division and modulo by
zero are runtime errors (exit 1); infinite loops are caught by the step
limit (also exit 1).

## Usage

```sh
bytecode-vm run demo.vm
bytecode-vm dis demo.vm
```

## Development

```sh
make test
```

Tests cover arithmetic, variables, loops (byte-exact jump targets),
division by zero, step limits, assembler error reporting, and
disassembler output.

## License

MIT — see [LICENSE](LICENSE).
