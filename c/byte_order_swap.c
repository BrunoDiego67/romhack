#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

char *exec;

struct Arguments {
  char *filename;
  char *byte_ordering;
};

int print_usage() {
  fprintf(stderr, "Swap byte order of a file.\n");
  fprintf(stderr, "Usefull to switch between big-endiand and little-endian.\n\n");
  fprintf(stderr, "usage:\n");
  fprintf(stderr, "    %s <filename> <swap_order>\n", exec);
  fprintf(stderr, "\n");
  fprintf(stderr, "example:\n");
  fprintf(stderr, "   cat my_file.bin\n");
  fprintf(stderr, "   # 0123456789abcdef\n\n");
  fprintf(stderr, "   %s my_file.bin 1032\n", exec);
  fprintf(stderr, "   # 1023546798abdcef\n");
  fprintf(stderr, "   #  - swaped byte order from 0123 to 1032\n");
  fprintf(stderr, "\n");
  return(0);
}

char* left_shift_mask(char *byte_ordering) {
  if (strlen(byte_ordering) != 4) {
    printf("swap order must be a 4 digit number\n\n");
    print_usage();
    exit(1);
  }

  static char mask[4];

  for (int i = 0; i < 4; i++) {
    char c = byte_ordering[i];
    if (c < '0' || c > '9' ) {
      printf("Invalid byte ordering number: %c in %s\n", c, byte_ordering);
      exit(1);
    }
    int cbyte = c - '0';
    int bit_offset = (i - cbyte) * 8;
    mask[i] = bit_offset;
  }

  return mask;
}

struct Arguments *parse_arguments(int argc, char *argv[]) {
  if (argc != 3) {
    print_usage();
    exit(1);
  }
  struct Arguments *arguments = (struct Arguments*)malloc(sizeof(struct Arguments));
  arguments->filename = argv[1];
  arguments->byte_ordering = argv[2];

  return(arguments);
}

int validate_file_size(char *filename) {
  struct stat st;

  if (stat(filename, &st) == 0) {
    if (st.st_size % 4 == 0) {
      return 0;
    }
    fprintf(stderr, "Filesize of %s must be multiple of 4 bytes but it has %lu bytes\n",
      filename, (unsigned long)st.st_size);
    exit(1);
  }

  fprintf(stderr, "Cannot determine size of %s: %s\n", filename, strerror(errno));
  exit(1);
}

int main(int argc, char *argv[]) {
  exec = argv[0];
  struct Arguments *args = parse_arguments(argc, argv);

  validate_file_size(argv[1]);
  char *mask = left_shift_mask(args->byte_ordering);
  FILE *input_filep = fopen(args->filename, "rb");
  if (input_filep == NULL) {
    fprintf(stderr, "Unable to open file %s\n", args->filename);
    print_usage();
    exit(1);
  }
  unsigned long *chunk = (unsigned long*)malloc(4);
  // unsigned char* chunk_c;
  unsigned long swapped;
  unsigned char* swapped_c;
  unsigned long total_read = 0;
  size_t eread;
  // fprintf(stderr, "%i %i %i %i\n", mask[0], mask[1], mask[2], mask[3]);
  while(!feof(input_filep)) {
    while((eread = fread(chunk, 4, 1, input_filep)) != 0) {
      // chunk_c = (unsigned char*)chunk;
      // fprintf(stderr, "%c%c%c%c %3i %3i %3i %3i\n", chunk_c[0], chunk_c[1], chunk_c[2], chunk_c[3], chunk_c[0], chunk_c[1], chunk_c[2], chunk_c[3]);
      swapped = (
        ((                                  *chunk >> -mask[0]) & 0x000000ff) |
        ((mask[1] > 0 ? *chunk << mask[1] : *chunk >> -mask[1]) & 0x0000ff00) |
        ((mask[2] > 0 ? *chunk << mask[2] : *chunk >> -mask[2]) & 0x00ff0000) |
        ((              *chunk << mask[3]                     ) & 0xff000000)
      );
      swapped_c = (unsigned char*)&swapped;
      // fprintf(stderr, " - %c%c%c%c %3i %3i %3i %3i\n", swapped_c[0], swapped_c[1], swapped_c[2], swapped_c[3], swapped_c[0], swapped_c[1], swapped_c[2], swapped_c[3]);
      fwrite(swapped_c, 4, 1, stdout);
    }
  }
  fflush(stdout);
  fclose(input_filep);
  return(0);
}