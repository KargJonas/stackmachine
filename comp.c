#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

#define MAX_WORD_LENGTH 32
#define byte uint8_t

#define N_INSTR 11
char* instr[N_INSTR] = { "HALT", "CONST", "DUP", "DROP", "READ", "PRINT", "JMP", "BNZ", "LSS", "ADD", "SUB" };

typedef struct table_entry {
  char*  name;              // label name
  size_t addr;              // destination pc
  bool   is_defined;        // indicates if addr is fixed already
  struct table_entry* next; // reference to the next label
  byte*  refs;              // places where this label is used
  size_t n_refs;            // number of places where label is used  
} table_entry;

struct table_entry* sym_tab = NULL;
FILE* infile;
FILE* outfile;
int ch, nch, cur_addr = 0, line = 0, col = 0;

byte* program;
size_t pc = 0;
size_t prog_buf_size = 32;

void prog_add(byte value) {
  if (program == NULL) { printf("Program buffer error."); exit(1); }

  if (pc == prog_buf_size - 1) {
    prog_buf_size *= 1.5;
    printf("Program buffer growth event.\n");
    program = realloc(program, prog_buf_size);
  }

  program[pc++] = value;
}

// If possible, a slim trie implementation would be better
byte get_instr(char* name) {
  for (byte i = 0; i < N_INSTR; i++) {
    if (strcmp(instr[i], name) == 0) return i;
  }

  return 255;
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

void add_symbol(char* name, size_t addr, bool is_defined) {
  struct table_entry* entry = find_symbol(name);

  if (entry != NULL) {
    if (entry->is_defined) {
      // Address for this symbol has already been defined
      // => Duplicate definition
      printf("Error on line %d col %d:\n  Redeclaration of label \"%s\".\n", line, col, name);
      exit(1);
    } else {
      // Address for previously used symbol found
      entry->addr = pc;
      is_defined = true;
      return;
    }
  }

  entry = malloc(sizeof(struct table_entry));

  if (!entry) {
    printf("Failed to allocate memory for symbol table entry.");
    exit(1);
  }

  entry->name = name;
  entry->addr = addr;
  entry->is_defined = is_defined;

  if (sym_tab == NULL) {
    entry->next = NULL;
    sym_tab = entry;
    return;
  }

  // Prepend
  entry->next = sym_tab;
  sym_tab = entry;

  print_sym_tab();
}

// Adds a reference to the symbol table that indicates
// at which location in the program a label was used
// as a jump destination
void add_reference(char* name, byte addr) {
  table_entry* entry = find_symbol(name);

  if (entry == NULL) {
    add_symbol(name, 0, false);
  }

  entry->n_refs++;
  entry->refs = realloc(entry->refs, entry->n_refs);

  if (entry->refs == NULL) {
    printf("Error on line %d col %d:\n  Failed to reallocate memory for lable references.", line, col);
    exit(1);
  }

  entry->refs[entry->n_refs - 1] = addr;
  prog_add(pc);
}

int scan() {
  ch = nch;
  nch = fgetc(infile);

  if   (nch > 96 && nch < 123) { nch -= 32;       }  // Make uppercase
  if   (ch == '\n'           ) { line++; col = 0; }
  else                         { col++;           }

  printf("%c", ch);

  return ch;
}

void comment() {
  while (scan() != '\n' && ch != EOF);
}

char* word() {
  char* wrd = malloc(MAX_WORD_LENGTH);
  int i = 0;

  if (wrd == NULL) { printf("Failed to allocate memory for label."); exit(1); }

  wrd[i++] = ch;
  
  while (nch > 64 && nch < 91) {
    scan();
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
  scan(); // scan "."
  char* name = word();
  add_symbol(name, cur_addr, true);
}

void use_label() {
  char* name = word();
  add_reference(name, pc);
}

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
  scan(); // consume '
  return val;
}

byte num() {
  char strval[4] = "\0\0\0\0";

  for (int i = 0 ;; i++) {
    if (i > 2) {
      printf("Error on line %d col %d:\n  Numbers may not exceed 255.", line, col);
      exit(1);
    }

    strval[i] = scan();
    if (nch == ' ' || nch == '\n') break;
  }

  errno = 0;
  long numval = strtol(strval, NULL, 10);
  if (errno == EINVAL) {
    printf("Error on line %d col %d:\n  Invalid number \"%s\"", line, col, strval);
  }

  return (byte)numval;
}

void command() {
  char* name = word();
  byte instr = get_instr(name);
  prog_add(instr);

  if (instr == 255) {
    printf("Error on line %d col %d:\n  Invalid operation \"%s\".\n", line, col, name);
    exit(1);
  }

  if (strcmp(name, "CONST") == 0) {
    int val;

    if (nch != ' ' && nch != '\t') {
      printf("Error on line %d col %d:\n  Expected space or tab followed by char or int literal after CONST. Found: %d", line, col, nch);
      exit(1);
    }

    while (nch == ' ' || nch == '\t') scan();

    switch(nch) {
      case '\'':
        val = chr();
        printf("Line %d col %d: Char val %c", line, col, val);
        break;

      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        val = num();
        printf("Line %d col %d: Num val %d", line, col, val);
        break;

      default:
        printf("Error on line %d col %d:\n  Expected space or tab followed by char or int literal after CONST.", line, col);
        exit(1);
    }

    prog_add(val);
  } else if (strcmp(name, "JMP") == 0 || strcmp(name, "BNZ") == 0) {
    if (nch != ' ' && nch != '\t') {
      printf("Error on line %d col %d:\n  Expected space or tab followed by a label after %s. Found: %d", line, col, name, nch);
      exit(1);
    }

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
    for (int i = 0; i < current->n_refs; i++) {
      program[current->refs[i]] = current->addr;
    }

    current = current->next;
  }
}

int main(int argc, char** argv) {
  if (argc != 3) printf("Usage:  comp <infile> <outfile>\n");
  
  infile  = fopen(argv[1], "r");
  outfile = fopen(argv[2], "wb");

  if (!infile)  { printf("Error:\n  Failed to open input file \"%s\".\n", argv[1]); return 1; }
  if (!outfile) { printf("Error:\n  Failed to open output file \"%s\".\n", argv[1]); return 1; }

  // Initialize program buffer
  program = malloc(prog_buf_size);

  scan();

  while (scan() != EOF) {
    if (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t') continue; 
    if (ch == '#') comment();
    else if (ch == '.') label();
    else if (nch > 64 && nch < 91) command();
    else {
      printf("Error on line %d col %d:\n  Unexpected char '%c' (code: %d).\n", line, col, ch, ch);
      return 1;
    }
  }

  update_refs();
  fwrite(program, sizeof(byte), pc - 1, outfile);

  fclose(infile);
  fclose(outfile);

  return 0;
}
