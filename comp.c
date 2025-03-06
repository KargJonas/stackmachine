#include <stdio.h>


int main(int argc, char** argv) {
  if (argc != 3) printf("Usage:  comp <infile> <outfile>\n");
  
  FILE* infile  = fopen(argv[1], "r");
  FILE* outfile = fopen(argv[2], "wb");

  if (!infile) {
    printf("Error: Input file \"%s\" does not exist.\n", argv[1]);
    return 1;
  }

  int ch;
  while ((ch = fgetc(infile) != EOF)) {
    putchar(ch);

  }

  fclose(infile);
  fclose(outfile);
  return 0;
}
