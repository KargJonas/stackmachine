#ifndef INSTR_H
#define INSTR_H

#define N_INSTR 12

// Opcode definitions
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
#define MUL   11

// Operation name definitions
char* instr_names[N_INSTR] = {
  "HALT",   // 0
  "CONST",  // 1
  "DUP",    // 2
  "DROP",   // 3
  "READ",   // 4
  "PRINT",  // 5
  "JMP",    // 6
  "BNZ",    // 7
  "LSS",    // 8
  "ADD",    // 9
  "SUB",    // 10
  "MUL"     // 11
};

// Operation behavior definitions
#define OP_CASES \
  case HALT:   return 0; \
  case CONST:  stack[++sp] = progmem[++pc]; break; \
  case DUP:    stack[++sp] = stack[sp]; break; \
  case DROP:   sp--; break; \
  case READ:   stack[++sp] = getchar(); break; \
  case PRINT:  putchar(stack[sp--]); break; \
  case JMP:    pc = stack[sp--] - 1; break; \
  case BNZ:    pc = stack[sp] != 0 ? progmem[pc + 1] - 1 : pc + 1; sp--; break; \
  case LSS:    stack[sp - 1] = stack[sp - 1] < stack[sp]; sp--; break; \
  case ADD:    stack[sp - 1] += stack[sp]; sp--; break; \
  case SUB:    stack[sp - 1] -= stack[sp]; sp--; break; \
  case MUL:    stack[sp - 1] *= stack[sp]; sp--; break;

#endif // INSTR_H
