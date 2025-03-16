#ifndef INSTR_H
#define INSTR_H

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

#define N_INSTR 11
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
  "SUB"     // 10
};

#endif // INSTR_H
