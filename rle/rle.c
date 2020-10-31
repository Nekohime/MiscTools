/*
 * rle.c -- by David Henry | Released Dec. 28, 2004
 * this code is free. | original http://tfc.duke.free.fr/coding/src/rle.c
 * cc -Wall -ansi rle.c -o $@
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char ubyte;

// enc_size() - calculate size of output file when it will be encoded.
long enc_size(ubyte *in, long sizein) {
  int packet_size = 0;
  long sizeout = 0;
  ubyte *pin = in;

  while (pin < in + sizein) {
    if ((pin[0] == pin[1]) && (packet_size < (0x7f - 1))) {
      packet_size++;
      pin++;
    } else {
      if (packet_size > 0) {
        sizeout += 2;
        pin++;
      }

      packet_size = 0;

      while (pin[0] != pin[1] &&
             !((pin >= in + sizein) || (-packet_size > (0x7f - 1)))) {
        packet_size--;
        pin++;
      }

      sizeout += (-packet_size) + 1;
      packet_size = 0;
    }
  }

  return sizeout;
}

// dec_size() - calculate size of output file when it will be decoded.
long dec_size(ubyte *in, long sizein) {
  ubyte *pin = in;
  ubyte packet_header;
  long sizeout = 0;
  int size;

  while (pin < in + sizein) {
    packet_header = *(pin++);
    size = packet_header & 0x7f;

    if (packet_header & 0x80) {
      sizeout += size;
      pin++;
    } else {
      sizeout += size;
      pin += size;
    }
  }

  return sizeout;
}

// rle() - encode data from *in and store it in *out buffer. return
long rle(ubyte *in, ubyte *out, long sizein) {
  ubyte *pin = in;
  ubyte *pout = out;
  ubyte *ptmp;
  int packet_size = 0;
  int i;

  while (pin < in + sizein) {
    // look for rle packet
    if ((pin[0] == pin[1]) && (packet_size < (0x7f - 1))) {
      packet_size++;
      pin++;
    } else {
      if (packet_size > 0) {
        // write rle header and packet
        *(pout++) = (1 << 7) + ((packet_size + 1) & 0x7f);
        *(pout++) = *(pin++);
      }

      packet_size = 0;
      ptmp = pin;

      // look for next rle packet
      while (pin[0] != pin[1]) {
        // don't overflow buffer
        if ((pin >= in + sizein) || (-packet_size > (0x7f - 1)))
          break;

        // skip byte...
        packet_size--;
        pin++;
      }

      // write non-rle header
      *(pout++) = (-packet_size) & 0x7f;

      // write non-rle packet
      for (i = 0; i < (-packet_size); i++, pout++, ptmp++)
        *pout = *ptmp;

      packet_size = 0;
    }
  }

  return (pout - out);
}

/*
 * unrle() - decode data from *in and store it in *out buffer. return
 * the size of *out array.
 */

long unrle(ubyte *in, ubyte *out, long sizein) {
  ubyte *pin = in;
  ubyte *pout = out;
  ubyte packet_header;
  int i, size;

  while (pin < in + sizein) {
    // read first byte
    packet_header = *(pin++);
    size = packet_header & 0x7f;

    if (packet_header & 0x80) {
      // run-length packet
      for (i = 0; i < size; i++, pout++)
        *pout = *pin;

      pin++;
    } else {
      // non run-length packet
      for (i = 0; i < size; i++, pin++, pout++)
        *pout = *pin;
    }
  }

  return (pout - out);
}

/*
 * main() - take two parameters: -e (encode) or -d (decode) and filename
 * Encode or decode a file with RLE algorithm. It will produce a file
 * with ".rle" extension if encoding, ".unrle" if decoding.
 */

int main(int argc, char *argv[]) {
  FILE *fin, *fout;
  char fileout[256];
  ubyte *bufin, *bufout;
  long sizein, sizeout;
  long (*func)(ubyte *, ubyte *, long) = NULL;

  if (argc < 3) {
    printf("usage: rle -[e/d] filein\n");
    return 1;
  }

  // read filein
  fin = fopen(argv[2], "rb");
  if (!fin) {
    printf("error: could not open \"%s\"\n", argv[2]);
    return 1;
  }

  fseek(fin, 0, SEEK_END);
  sizein = ftell(fin);
  fseek(fin, 0, SEEK_SET);

  bufin = (ubyte *)malloc(sizeof(ubyte) * sizein);
  if (!bufin) {
    fclose(fin);
    printf("error: malloc failed!\n");
    return 1;
  }

  fread(bufin, sizeof(ubyte), sizein, fin);
  fclose(fin);

  if (strstr(argv[1], "-e")) {
    // encode
    strcpy(fileout, argv[2]);
    strcat(fileout, ".rle");
    func = rle;
    sizeout = enc_size(bufin, sizein);

    if (sizeout > sizein) {
      printf("warning: output file will be bigger than input file!\n");
    }
  } else if (strstr(argv[1], "-d")) {
    // decode
    strcpy(fileout, argv[2]);
    fileout[strlen(fileout) - 4] = '\0';
    strcat(fileout, ".unrle");
    func = unrle;
    sizeout = dec_size(bufin, sizein);
  } else {
    // error
    printf("unknown \"%s\" parameter.\n", argv[1]);
    free(bufin);
    return 1;
  }

  bufout = (ubyte *)malloc(sizeof(ubyte) * sizeout);
  if (!bufout) {
    free(bufin);
    printf("error: malloc failed!\n");
    return 1;
  }

  // encode or decode
  sizeout = func(bufin, bufout, sizein);

  // write fileout
  fout = fopen(fileout, "w+b");
  fwrite(bufout, sizeof(ubyte), sizeout, fout);
  fclose(fout);

  free(bufin);
  free(bufout);

  printf("writing \"%s\" file... (%ld : %ld)\n", fileout, sizein, sizeout);

  return 0;
}
