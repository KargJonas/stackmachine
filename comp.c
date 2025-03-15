#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>

#define MAX_WORD_LENGTH 32
#define BUFFER_GROWTH_FACTOR 2
#define byte uint8_t

#define N_INSTR 11
char* instr[N_INSTR] = {
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

typedef struct table_entry {
  char*  name;              // label name
  size_t addr;              // destination pc
  bool   is_defined;        // indicates if addr is fixed already
  struct table_entry* next; // reference to the next label
  byte*  refs;              // places where this label is used
  size_t n_refs;            // number of places where label is used  
} table_entry;

FILE* infile;
FILE* outfile;

struct table_entry* sym_tab = NULL;
int ch, nch, line = 1, col = 0;

byte* program;
size_t pc = 0;
size_t prog_buf_size = 32;

void error(const char* fmt, ...) {
  fprintf(stderr, "Error on line %d col %d:\n  ", line, col);
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
  exit(1);
}

void mem_error(char* name) {
  fprintf(stderr, "Memory related error:\n  %s", name);
  exit(1);
}

// Adds a value to the program buffer (instruction or constant).
// Makes the program buffer larger, if necessary.
void prog_add(byte value) {
  if (program == NULL) mem_error("Program buffer expansion");

  if (pc == prog_buf_size - 1) {
    prog_buf_size *= BUFFER_GROWTH_FACTOR;
    printf("Program buffer growth event.\n");
    program = realloc(program, prog_buf_size);
  }

  program[pc++] = value;
}

// Finds the index of an instruction by it's name.
// A trie-based implementation would be better but also overkill.
byte get_instr(char* name) {
  for (byte i = 0; i < N_INSTR; i++) {
    if (strcmp(instr[i], name) == 0) return i;
  }

  return 255;
}

// Finds a entry from the symbol table by it's name.
struct table_entry* find_symbol(char* name) {
  struct table_entry* current = sym_tab;

  while (current != NULL) {
    if (strcmp(current->name, name) == 0) break;
    current = current->next;
  }

  return current;
}

// Prints the symbol table to the console.
void print_sym_tab() {
  struct table_entry* current = sym_tab;
  printf("\n[Symbol Table]\nName\tAddress\n---------------\n");
  while (current != NULL) {
    printf("%s\t%ld\n", current->name, current->addr);
    current = current->next;
  }
}

// Adds a label to the symbol table.
void add_symbol(char* name, size_t addr, bool is_defined) {
  struct table_entry* entry = find_symbol(name);

  if (entry != NULL) {
    // Address for this symbol has already been defined => Duplicate definition.
    if (entry->is_defined) {
      error("Redeclaration of label \"%s\".\n", name);
      exit(1);
    }
    
    // Concrete address for previously used symbol found.
    if (is_defined) {
      entry->addr = addr;
      entry->is_defined = true;
      return;
    }
  }

  entry = malloc(sizeof(struct table_entry));
  if (!entry) mem_error("Add symbol table entry");

  entry->name = strdup(name);
  entry->addr = addr;
  entry->is_defined = is_defined;
  entry->refs = NULL;
  entry->n_refs = 0;

  if (sym_tab == NULL) {
    entry->next = NULL;
    sym_tab = entry;
    return;
  }

  // Prepend
  entry->next = sym_tab;
  sym_tab = entry;
}

// Adds a usage reference to a symbol table entry.
void add_reference(char* name, byte addr) {
  table_entry* entry = find_symbol(name);

  if (entry == NULL) {
    add_symbol(name, 0, false);
  }

  entry->n_refs++;
  entry->refs = realloc(entry->refs, sizeof(byte) * entry->n_refs);

  if (entry->refs == NULL) {
    mem_error("Add label reference");
  }

  entry->refs[entry->n_refs - 1] = addr;
  prog_add(pc);
}

// Reads a single character from the input file,
// makes that char uppercase and updates nch and ch
int scan() {
  ch = nch;
  nch = fgetc(infile);

  if (nch > 96 && nch < 123) nch -= 32;
  if (ch == '\n') { line++; col = 0; }
  else col++;

  return ch;
}

// Skips an entire line
void comment() {
  while (scan() != '\n' && ch != EOF);
}

// Reads all characters until a non-alphabetical char is encountered.
char* word() {
  char* wrd = malloc(MAX_WORD_LENGTH);
  int i = 0;

  if (wrd == NULL) mem_error("Allocate memory for word/label.");
  wrd[i++] = ch;
  
  while (nch > 64 && nch < 91) {
    scan();
    if (ch > 64 && ch < 91) wrd[i++] = ch; 
    if (i == MAX_WORD_LENGTH - 2) error("Label may not exceed %d characters.", MAX_WORD_LENGTH);
  }

  if (i == 0) error("Label must be at least one char.");

  wrd[i++] = 0;
  wrd = realloc(wrd, i);
  return wrd;
}

void label() {
  scan(); // scan "."
  char* name = word();
  add_symbol(name, pc, true);
}

void use_label() {
  char* name = word();
  add_reference(name, pc);
}

// Reads a char constant of the form 'a' or '\n'
byte chr() {
  byte val;
  scan(); // consume '
  if (ch == '\\') switch(scan()) {
    case '\\': val = '\\'; break;
    case '\'': val = '\''; break;
    case 'n':  val = '\n'; break;
    case 't':  val = '\t'; break;
    case 'r':  val = '\r'; break;
    case 'b':  val = '\b'; break;
    case 'f':  val = '\f'; break;
    case 'v':  val = '\v'; break;
    case '0':  val = '\0'; break;
    default:
      printf("Error on line %d col %d:\n  Unsupported escape code '\\%c'.\n", line, col, ch);
      exit(1);
  }
  else val = scan();
  scan(); // consume "'"
  return val;
}

// Reads numbers between 0 and 255
byte num() {
  char strval[4] = "\0\0\0\0";

  for (int i = 0 ;; i++) {
    if (i > 2) {
      error("Numbers may not exceed 255.");
      exit(1);
    }

    strval[i] = scan();
    if (nch == ' ' || nch == '\n') break;
  }

  errno = 0;
  long numval = strtol(strval, NULL, 10);
  if (errno == EINVAL) error("Invalid number \"%s\"", strval);
  if (numval > 255) error("Numbers may not exceed 255.");

  return (byte)numval;
}

// Reads an instructions with or without operands and
// adds the appropriate opcodes and constants to the program buffer
void instruction() {
  char* name = word();
  byte instr = get_instr(name);
  prog_add(instr);

  if (instr == 255) error("Invalid operation \"%s\".\n", name);

  if (strcmp(name, "CONST") == 0) {
    int val;

    if (nch != ' ' && nch != '\t') error("Expected space or tab followed by char or int literal after CONST. Found: %d", nch);
    while (nch == ' ' || nch == '\t') scan();

    switch(nch) {
      case '\'':
        val = chr();
        break;

      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        val = num();
        break;

      default:
        error("Expected space or tab followed by char or int literal after CONST. Found: %d", nch);
    }

    prog_add(val);
  } else if (strcmp(name, "JMP") == 0 || strcmp(name, "BNZ") == 0) {
    if (nch != ' ' && nch != '\t') fprintf(stderr, "Expected space or tab followed by a label after %s. Found: %d", name, nch);
    while (nch == ' ' || nch == '\t') scan();
    scan();
    use_label();
  }
}

// Updates all places in the code where labels were used
// with concrete jump addresses
void update_refs() {
  struct table_entry* current = sym_tab;
  while (current != NULL) {
    if (!current->is_defined) {
      fprintf(stderr, "Error: Label \"%s\" is used but not declared.", current->name);
      exit(1);
    }

    for (int i = 0; i < current->n_refs; i++) {
      fprintf(stderr, "\nInserted a reference to addr %ld at pc %d for label %s\n\n", current->addr, current->refs[i], current->name);
      program[current->refs[i]] = current->addr;
    }

    current = current->next;
  }
}

int main(int argc, char** argv) {
  if (argc != 3) printf("Usage:  comp <infile> <outfile>\n");
  
  infile  = fopen(argv[1], "r");
  outfile = fopen(argv[2], "wb");

  if (!infile)  { fprintf(stderr, "Error:\n  Failed to open input file \"%s\".\n", argv[1]); return 1; }
  if (!outfile) { fprintf(stderr, "Error:\n  Failed to open output file \"%s\".\n", argv[1]); return 1; }

  // Initialize program buffer
  program = malloc(prog_buf_size);

  scan();

  while (scan() != EOF) {
    if (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t') continue; 
    if (ch == '#') comment();
    else if (ch == '.') label();
    else if (nch > 64 && nch < 91) instruction();
    else error("Unexpected char '%c' (code: %d).\n", ch, ch);
  }

  print_sym_tab();
  update_refs();

  fwrite(program, sizeof(byte), pc - 1, outfile);

  fclose(infile);
  fclose(outfile);

  return 0;
}
