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

size_t program_size;
byte* progmem;

void read(char* filename) {
  FILE* infile = fopen(filename, "rb");

  if (!infile) {
    fprintf(stderr, "Error:\n  Failed to open input file \"%s\".\n", filename);
    exit(1);
  }

  fseek(infile, 0, SEEK_END);
  program_size = ftell(infile);
  progmem      = malloc(program_size);
  rewind(infile);

  printf("Prog size %ld\n", program_size);

  if (!progmem) {
    fprintf(stderr, "Failed to allocate memory for program buffer.");
    exit(1);
  }
   
  fread(progmem, sizeof(byte), program_size, infile);
  fclose(infile);
}

int main(int argc, char** argv) {
  char* debug_str = getenv("DEBUG");
  int debug = debug_str == NULL ? 0 : atoi(debug_str);

  if (argc != 2) {
    printf("Usage:  vm <infile>\n");
    exit(1);
  }

  read(argv[1]);

  byte* stack = malloc(STACK_SIZE);
  int pc = 0;

  // This is a consequence of pointing to the last element
  // and not the first free location on the stack
  int sp = -1;

  while (1) {
    byte instr = progmem[pc];

    // Print debug info
    if (debug > 0) {
      printf("\n%-8d%-6s", pc, instr_names[instr]);
      if (instr == CONST || instr == BNZ || instr == JMP) printf("(%d)\t", progmem[pc + 1]);
      else printf("     \t");
      for (int i = 0; i <= sp; i++) printf("[%d]", stack[i]);
      printf("  \t");
    }

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
      case BNZ:    pc = stack[sp] != 0 ? progmem[pc + 1] - 1 : pc; sp--; break;
      case LSS:    stack[sp - 1] = stack[sp - 1] < stack[sp]; sp--; break;
      case ADD:    stack[sp - 1] += stack[sp]; sp--; break;
      case SUB:    stack[sp - 1] -= stack[sp]; sp--; break; 
    }

    pc++;

    if (sp < 0)                { printf("Stack underflow\n"); return 1; }
    else if (sp >= STACK_SIZE) { printf("Stack overflow\n");  return 1; }
  }

  fprintf(stdout, "Unexpected program exit with stack pointer %d and prog size %ld.\n", sp, program_size);
  return 1;
}
