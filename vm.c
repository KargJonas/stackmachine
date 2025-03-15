#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define byte uint8_t

#define STACK_SIZE   16

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

int main(int argc, char** argv) {
  char* debug_str = getenv("DEBUG");
  int debug = debug_str == NULL ? 0 : atoi(debug_str);

  if (argc != 2) {
    printf("Usage:  vm <infile>\n");
    exit(1);
  }

  FILE* infile = fopen(argv[1], "rb");
  if (!infile) { fprintf(stderr, "Error:\n  Failed to open input file \"%s\".\n", argv[1]); return 1; }

  fseek(infile, 0, SEEK_END);
  size_t program_size = ftell(infile);
  rewind(infile);

  byte* progmem = malloc(program_size);
  if (!progmem) { fprintf(stderr, "Memory related error: Failed to allocate memory for program buffer."); }

  if (progmem) fread(progmem, 1, program_size, infile);
  fclose(infile);

  // byte* progmem = malloc(PROGMEM_SIZE);
  byte* stack = malloc(STACK_SIZE);
  int pc = 0;

  // This is a consequence of pointing to the last element
  // and not the first free location on the stack
  int sp = -1; 

  while (pc < program_size - 1) {

    byte instr = progmem[pc];
    if (debug > 0) printf("\n%d\t%s\t", pc, instr_names[instr]);

    switch (instr) {
      case HALT:   return 0;
      // Consumes a constant from the progmem, so we need to jump one address.
      case CONST:  stack[++sp] = progmem[++pc]; break;
      case DUP:    stack[++sp] = stack[sp]; break;
      case DROP:   sp--; break;
      case READ:   stack[++sp] = getchar(); break;
      case PRINT:  putchar(stack[sp--]); break;
      // Compensate -1 address for jmp/branch. This saves logic at pc increment.
      case JMP:    pc = stack[sp--] - 1; break;
      case BNZ:    pc = stack[sp] != 0 ? progmem[pc + 1] - 1 : pc; sp--; pc++; break;
      case LSS:    stack[sp - 1] = stack[sp - 1] < stack[sp]; sp--; break;
      case ADD:    stack[sp - 1] += stack[sp]; sp--; break;
      case SUB:    stack[sp - 1] -= stack[sp]; sp--; break; 
    }

    if (debug > 0) {
      putchar('\t');
      for (int i = 0; i <= sp; i++) {
        printf("[%d]", stack[i]);
      }
      putchar('\n');
    }

    pc++;

    if (sp < 0)                { printf("Stack underflow\n"); return 1; }
    else if (sp >= STACK_SIZE) { printf("Stack overflow\n");  return 1; }
  }

  printf("Unexpected program exit.\n");
  return 1;
}
