#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define byte uint8_t

#define PROGMEM_SIZE 128

#define INITIAL_STACK_SIZE   2
#define STACK_GROWTH_FACTOR  1.5

#define HALT  0
#define CONST 1
#define DUP   2
#define DROP  3
#define READ  4
#define PRINT 5
#define JMP   6
#define BNZ   7
#define LSS   8
#define ADD   9
#define SUB   10

char* instr_names[] = {
  "HALT", "CONST", "DUP", "DROP", "READ", "PRINT", "JMP", "BNZ", "LSS", "ADD", "SUB"
};

byte progmem[PROGMEM_SIZE] = {

  // Loop counter
  CONST, 0,

  // Increment counter
  CONST, 1,  // .loop
  ADD,

  // Print "Hello\n"
  CONST, 72,
  PRINT,
  CONST, 69,
  PRINT,
  CONST, 76,
  PRINT,
  CONST, 76,
  PRINT,
  CONST, 79,
  PRINT,
  CONST, 10,
  PRINT,

  // Loop condition + branching
  DUP,
  CONST, 5,
  LSS,
  BNZ, 1,
  HALT
};

int main() {
  char* debug_str = getenv("DEBUG");
  int debug = debug_str == NULL ? 0 : atoi(debug_str);

  byte* stack       = malloc(INITIAL_STACK_SIZE);
  size_t stack_size = INITIAL_STACK_SIZE;
  int pc = 0, sp = -1;

  if (debug > 0) printf("\nAddr\tInstr\tOut\tStack\n------------------------------");

  while (pc < PROGMEM_SIZE - 1) {

    byte instr = progmem[pc];
    if (debug > 0) printf("\n%d\t%s\t", pc, instr_names[instr]);

    switch (instr) {
      case HALT:   return 0;
      case CONST:  stack[++sp] = progmem[++pc]; break;
      case DUP:    stack[++sp] = stack[sp]; break;
      case DROP:   sp--; break;
      case READ:   stack[++sp] = getchar(); break;
      case PRINT:  putchar(stack[sp--]); break;
      case JMP:    pc = stack[sp--] - 1; break;
      case BNZ:    pc = stack[sp] != 0 ? progmem[pc + 1] - 1 : pc; sp--; pc++; break;
      case LSS:    stack[sp - 1] = stack[sp - 1] < stack[sp]; sp--; break;
      case ADD:    stack[sp - 1] += stack[sp]; sp--; break;
      case SUB:    stack[sp - 1] -= stack[sp]; sp--; break; 
    }

    if (debug > 0) {
      putchar('\t');
      for (int i = 0; i <= sp; i++) printf("[%d]", stack[i]);
      putchar('\n');
    }

    pc++;

    // Handle stack underflow and growth
    if (sp < 0) { printf("Stack underflow"); return 1; }
    else if (sp == stack_size - 1) {
      stack = realloc(stack, stack_size *= STACK_GROWTH_FACTOR);
      if (debug > 0) printf("\n  [Stack growth] New size: %ld\n", stack_size);
      if (stack == NULL) { printf("Stack growth failed."); return 1; }
    }
  }

  printf("Unexpected program exit");
  return 1;
}

