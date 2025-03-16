#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

#include "./instr.h"

#define BUFFER_GROWTH_FACTOR 2
#define byte uint8_t

size_t stack_size = 32;
size_t program_size;

byte* stack;
byte* progmem;

void error(const char* fmt, ...) {
  fprintf(stderr, "Error: \n  ");
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
  exit(1);
}

void read(char* filename) {
  FILE* infile = fopen(filename, "rb");
  if (!infile) error("Failed to open input file \"%s\".\n", filename);

  fseek(infile, 0, SEEK_END);
  program_size = ftell(infile);
  progmem      = malloc(program_size);
  if (!progmem) error("Failed to allocate memory for program buffer.");

  rewind(infile);
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

  byte* stack = malloc(stack_size);
  int pc = 0;

  // This is a consequence of pointing to the last element
  // and not the first free location on the stack
  int sp = -1;

  while (pc < program_size) {
    byte instr = progmem[pc];

    // Print debug info
    if (debug > 0) {
      printf("\n%-8d%-6s", pc, instr_names[instr]);
      if (instr == CONST || instr == BNZ || instr == JMP) printf("(%d)\t", progmem[pc + 1]);
      else printf("     \t");
      for (int i = 0; i <= sp; i++) printf("[%d]", stack[i]);
      printf("  \t");
    }

    switch(instr) {
      OP_CASES;
      default: error("Unexpected opcode: %d\n", instr);
    }

    pc++;

    if (sp < 0) error("Stack underflow\n");
    else if (sp >= stack_size) {
      stack_size *= BUFFER_GROWTH_FACTOR;
      stack = realloc(stack, stack_size);
      if (stack == NULL) error("Failed to grow stack.\n");
    }
  }

  error("Unexpected program exit with stack pointer %d and prog size %ld.\n", sp, program_size);
  return 1;
}
