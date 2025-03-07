#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_WORD_LENGTH 32
#define byte uint8_t

#define N_INSTR 11
char* instr[N_INSTR] = { "HALT", "CONST", "DUP", "DROP", "READ", "PRINT", "JMP", "BNZ", "LSS", "ADD", "SUB" };

typedef struct table_entry {
  char* name;
  size_t addr;
  struct table_entry* next;
} table_entry;

struct table_entry* sym_tab = NULL;
FILE* infile;
FILE* outfile;
int ch, cur_addr = 0, line = 0, col = 0;

byte get_instr(char* name) {
  for (byte i = 0; i < N_INSTR; i++) {
    if (strcmp(instr[i], name) == 0) return i;
  }
}

struct table_entry* find_symbol(char* name) {
  struct table_entry* current = sym_tab;

  while (current != NULL) {
    if (strcmp(current->name, name) == 0) break;
    current = current->next;
  }

  return current;
}

void print_sym_tab() {
  struct table_entry* current = sym_tab;
  printf("Name\tAddress\n---------------\n");
  while (current != NULL) {
    printf("%s\t%ld\n", current->name, current->addr);
    current = current->next;
  }
}

void add_symbol(char* name, size_t addr) {
  if (find_symbol(name) != NULL) {
    printf("Error on line %d col %d:\n  Redeclaration of label \"%s\".\n", line, col, name);
    exit(1);
  }

  struct table_entry* new_entry = malloc(sizeof(struct table_entry));
  if (!new_entry) {
    printf("Failed to allocate memory for symbol table entry.");
    exit(1);
  }

  new_entry->name = name;
  new_entry->addr = addr;

  if (sym_tab == NULL) {
    new_entry->next = NULL;
    sym_tab = new_entry;
    return;
  }

  // Prepend
  new_entry->next = sym_tab;
  sym_tab = new_entry;

  print_sym_tab();
}

int scan() {
  ch = fgetc(infile);
  if (ch == '\n') { line++; col = 0; }
  else col++;

  if (ch > 96 && ch < 123) ch -= 32;  // Make uppercase
  return ch;
}

void comment() {
  while (scan() != '\n' && ch != EOF);
}

char* word() {
  char* wrd = malloc(MAX_WORD_LENGTH);
  int i = 0;

  if (wrd == NULL) { printf("Failed to allocate memory for label."); exit(1); }
  
  while (scan() != '\n') {
    if (ch > 64 && ch < 91) wrd[i++] = ch; 
    if (i == MAX_WORD_LENGTH - 2) {
      printf("Label may not exceed %d characters.", MAX_WORD_LENGTH);
      exit(1);
    }
  }

  if (i == 0) { printf("Label must be at least one char."); exit(1); }

  wrd[i++] = 0;
  wrd = realloc(wrd, i);
  return wrd;
}

void label() {
  char* name = word();
  add_symbol(name, cur_addr);
}

int main(int argc, char** argv) {
  if (argc != 3) printf("Usage:  comp <infile> <outfile>\n");
  
  infile  = fopen(argv[1], "r");
  outfile = fopen(argv[2], "wb");

  if (!infile)  { printf("Error:\n  Failed to open input file \"%s\".\n", argv[1]); return 1; }
  if (!outfile) { printf("Error:\n  Failed to open output file \"%s\".\n", argv[1]); return 1; }

  while (scan() != EOF) {
    if (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t') continue; 
    if (ch == '#') comment();
    else if (ch == '.') label();
    else {
      printf("Error on line %d col %d:\n  Unexpected char '%c' (code: %d).\n", line, col, ch, ch);
      return 1;
    }
  }

  fclose(infile);
  fclose(outfile);
  return 0;
}
