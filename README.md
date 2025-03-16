# Micro Stack Machine Language
#### A super simple (<500 lines) compiler and runtime for an assembly-like, stack machine-based language.

```bash
# Line count
$ wc -l *.c *.h
  349 comp.c
   88 vm.c
   51 instr.h
  488 total
```

This project is a little exercise in C programming and a bit of experimentation with stack machines.
It contains a compiler that can convert source files written in an assembly-like format into
bytecode, as well as a very simple runtime that can run the bytecode.

One sample program is provided in `hello.usml`.

It can be compiled / run like so:

```bash
$ gcc comp.c -o comp       # Build compiler
$ ./comp hello.usml hello  # Use compiler on hello.usml

$ gcc vm.c -o vm           # Build runtime
$ ./vm hello               # Use runtime to run the compiled hello world program
```

### The USML language
The Micro Stack Machine Language has 12 different instructions:
`HALT`, `CONST`, `DUP`, `DROP`, `READ`, `PRINT`, `JMP`, `BNZ`, `LSS`, `ADD`, `SUB` and `MUL`.

It is *very easy* to add new instructions by simply editing `instr.h`, where the behaviors, names and opcodes of all instructions are defined.
You can also read `instr_definition.txt`, where it's explained how each instruction operates.

It is an 8-bit architecture, so only numbers and addresses in the range 0 to 256 are supported.

You can use labels, characters and comments.

```asm
# This program prints "Hello" five times

# Index variable i
CONST 0

.loop
  # Increment counter
  CONST 1
  ADD

  CONST 'H'
  PRINT
  CONST 'E'
  PRINT
  CONST 'L'
  PRINT
  CONST 'L'
  PRINT
  CONST 'O'
  PRINT
  CONST '\n'
  PRINT

  # Loop condition + branching
  DUP
  CONST 5
  LSS
  BNZ loop
  HALT
```
