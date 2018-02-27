/*
 * mix_io.c
 *
 *  Created on: Sep 1, 2015
 *      Author: eyal
 */


#include "mix_io.h"
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
// Ugly hack to get Eclipse to handle _Atomic correctly.
#ifdef __CDT_PARSER__
#define _Atomic
typedef char atomic_flag;
#else
#include <stdatomic.h>
#endif
#define PRINTER_PAGE_LEN 22

FILE *input_file[21], *output_file[21];
void (*reader[21])(FILE *f, mix_word *buf, mix_word posn, int blocksize);
void (*writer[21])(FILE *f, mix_word *buf, mix_word posn, int blocksize);
void (*control[21])(FILE *f, mix_word operand, int blocksize);
_Atomic pthread_t threadID[21];
int bsize[21];
bool isdisk[21];

/*
 * All of the following five functions work together.
 * The handler functions do the following:
 * (The input structure contains several pointers to nonlocal storage
 * that may be ignored, as well as two important variables: tid and read.
 * read is a pointer to a semaphore, initially in state 0.
 * tid points to the pertinent member of threadID.)
 * 1. Copy input structure to this stack frame.
 * 2. Set *tid to the current thread ID.
 * 3. v(read).
 * 4. Carry out I/O.
 * 5. Set *tid to 0.
 *
 * The other three functions do the following:
 * 1. Create an input structure (in this function's stack frame).
 * 2. Copy pertinent threadID to this function's frame.
 * 3. If said thread ID is not 0, wait for that thread to finish.
 * 4. Copy data to input structure.
 * 5. Set input structure's threadID pointer.
 * 6. Set input structure's read to empty.
 * 7. Create a new thread running the handler, and set threadID correctly.
 * 8. p(read)
 */

typedef struct {
  void (*func)(FILE *f, mix_word *buf, mix_word posn, int blocksize);
  FILE *f;
  mix_word *buf;
  mix_word posn;
  int blocksize;
  _Atomic pthread_t *tid;
  atomic_flag *read;
} rw;

typedef struct {
  void (*func)(FILE *f, mix_word operand, int blocksize);
  FILE *f;
  mix_word operand;
  int blocksize;
  _Atomic pthread_t *tid;
  atomic_flag *read;
} ctl;

void *handle_rw(void *stuff) {
  rw in = atomic_load((_Atomic rw *)stuff);
  atomic_store(in.tid, pthread_self());
  atomic_flag_clear(in.read);
  in.func(in.f, in.buf, in.posn, in.blocksize);
  atomic_store(in.tid, 0);
  return(NULL);
}

void *handle_ctl(void *stuff) {
  ctl in = atomic_load((_Atomic ctl *)stuff);
  atomic_store(in.tid, pthread_self());
  atomic_flag_clear(in.read);
  in.func(in.f, in.operand, in.blocksize);
  atomic_store(in.tid, 0);
  return(NULL);
}

void input(mix_word *mem, mix_word posn, char device) {
  rw stuff;
  _Atomic rw foo;
  pthread_t temp;
  atomic_flag bar;
  if((unsigned)device > 20)
    return;
  temp = atomic_load(threadID + device);
  if(temp)
    pthread_join(temp, NULL);
  stuff.func = reader[(int)device];
  stuff.f = input_file[(int)device];
  stuff.buf = mem;
  stuff.blocksize = bsize[(int)device];
  stuff.posn = posn;
  atomic_flag_clear(stuff.read = &bar);
  stuff.tid = threadID + device;
  atomic_store(&foo, stuff);
  pthread_create(&temp, NULL, handle_rw, (void *)&foo);
  while(atomic_flag_test_and_set(stuff.read))
    sched_yield();
}

void output(mix_word *mem, mix_word posn, char device) {
  rw stuff;
  _Atomic rw foo;
  pthread_t temp;
  atomic_flag bar;
  if((unsigned)device > 20)
    return;
  temp = atomic_load(threadID + (int)device);
  if(temp)
    pthread_join(temp, NULL);
  stuff.func = writer[(int)device];
  stuff.f = output_file[(int)device];
  stuff.buf = mem;
  stuff.blocksize = bsize[(int)device];
  atomic_flag_clear(stuff.read = &bar);
  stuff.tid = threadID + device;
  atomic_store(&foo, stuff);
  pthread_create(&temp, NULL, handle_rw, (void *)&foo);
  while(atomic_flag_test_and_set(stuff.read))
    sched_yield();
}

void cntrl(mix_word argument, char device) {
  ctl stuff;
  _Atomic ctl foo;
  pthread_t temp;
  atomic_flag bar;
  if((unsigned)device > 20)
    return;
  temp = atomic_load(threadID + device);
  if(temp)
    pthread_join(temp, NULL);
  stuff.func = control[(int)device];
  stuff.f = output_file[(int)device];
  stuff.operand = argument;
  stuff.blocksize = bsize[(int)device];
  atomic_flag_clear(stuff.read = &bar);
  stuff.tid = threadID + device;
  atomic_store(&foo, stuff);
  pthread_create(&temp, NULL, handle_ctl, (void *)&foo);
  while(!stuff.read)
    sched_yield();
}

bool ready(char device) {
  return(!atomic_load(threadID + device));
}

void trivial_control(FILE *f, mix_word operand, int blocksize) {}

void trivialRW(FILE *f, mix_word *buf, mix_word posn, int blocksize) {}

void read_binary(FILE *f, mix_word *buf, mix_word posn, int blocksize);
void read_char(FILE *f, mix_word *buf, mix_word posn, int blocksize);
void write_binary(FILE *f, mix_word *buf, mix_word posn, int blocksize);
void write_char(FILE *f, mix_word *buf, mix_word posn, int blocksize);
void tape_control(FILE *f, mix_word operand, int blocksize);
void disk_control(FILE *f, mix_word operand, int blocksize);
void write_printer(FILE *f, mix_word *buf, mix_word posn, int blocksize);
void printer_control(FILE *f, mix_word operand, int blocksize);

void read_disk(FILE *f, mix_word *buf, mix_word posn, int blocksize) {
  disk_control(f, posn, blocksize);
  read_binary(f, buf, posn, blocksize);
}
void write_disk(FILE *f, mix_word *buf, mix_word posn, int blocksize) {
  disk_control(f, posn, blocksize);
  write_binary(f, buf, posn, blocksize);
}

void setupIO(void) {
  int i;
  FILE *disk[8], *tape[8];
  FILE *cardreader, *cardpunch, *printer, *papertape, *tapepunch;
  char *name = "disk0.dev";
  for(i = 0; i < 8; i++) {
    disk[i] = fdopen(open(name, O_RDWR | O_CREAT, 0666), "r+");
    name[4]++;
  }
  name = "tape0.dev";
  for(i = 0; i < 8; i++) {
    tape[i] = fdopen(open(name, O_RDWR | O_CREAT, 0666), "r+");
    name[4]++;
  }
  cardreader = fdopen(open("card.dev", O_RDONLY | O_CREAT, 0666), "r");
  cardpunch = fopen("punch.dev", "a");
  printer = fopen("printer.dev", "w");
  papertape = fdopen(open("paper.dev", O_RDONLY | O_CREAT, 0666), "r");
  tapepunch = fopen("tapep.dev", "w");
  for(i = 0; i < 21; i++)
    threadID[i] = NULL;
  for(i = 0; i < 8; i++) {
    input_file[i] = output_file[i] = tape[i];
    input_file[i + 8] = output_file[i + 8] = disk[i];
    reader[i] = read_binary;
    writer[i] = write_binary;
    reader[i + 8] = read_disk;
    writer[i + 8] = write_disk;
    control[i] = tape_control;
    control[i + 8] = disk_control;
    bsize[i] = bsize[i + 8] = 100;
    isdisk[i] = false;
    isdisk[i + 8] = true;
  }
  input_file[16] = cardreader;
  output_file[16] = NULL;
  reader[16] = read_char;
  writer[16] = trivialRW;
  bsize[16] = 16;
  input_file[17] = NULL;
  output_file[17] = cardpunch;
  reader[17] = trivialRW;
  writer[17] = write_char;
  bsize[17] = 16;
  input_file[18] = NULL;
  output_file[18] = printer;
  reader[18] = trivialRW;
  writer[18] = write_printer;
  bsize[18] = 24;
  input_file[19] = stdin;
  output_file[19] = stdout;
  reader[19] = read_char;
  writer[19] = write_char;
  bsize[19] = 14;
  input_file[20] = papertape;
  output_file[20] = tapepunch;
  reader[20] = read_char;
  writer[20] = write_char;
  bsize[20] = 14;
  for(i = 16; i < 20; i++) {
    control[i] = trivial_control;
    isdisk[i] = false;
  }
  control[18] = printer_control;
  control[20] = tape_control;
}

void read_char(FILE *f, mix_word *buf, mix_word posn, int blocksize) {
  char *line = calloc(blocksize * 5 + 1, sizeof(char));
  int len, i;
  fgets(line, blocksize * 5 + 1, f);
  len = strlen(line);
  if(line[len - 1] == '\n') {
    line[len - 1] = 0;
    len--;
  } else
    while(getc(f) != '\n' && !feof(f));
  for(i = 0; i < len; i++)
    line[i] = ascii_to_mixcii(line[i]);
  for(i = 0; i < blocksize * 5; i += 5)
    *buf++ = mix_word_pack(line[i], line[i + 1], line[i + 2],
	line[i + 3], line[i + 4]);
  free(line);
}

void write_char(FILE *f, mix_word *buf, mix_word posn, int blocksize) {
  char *line = calloc(blocksize * 5 + 1, sizeof(char));
  int i, len;
  for(i = 0; i < blocksize * 5; i += 5) {
    line[i] = mix_byte_new(*buf >> 24);
    line[i + 1] = mix_byte_new(*buf >> 18);
    line[i + 2] = mix_byte_new(*buf >> 12);
    line[i + 3] = mix_byte_new(*buf >> 6);
    line[i + 4] = mix_byte_new(*buf);
  }
  for(len = blocksize * 5; !line[len - 1]; len--);
  for(i = 0; i < len; i++)
    line[i] = mixcii_to_ascii(line[i]);
  fprintf(f, "%s\n", line);
  free(line);
}

void read_binary(FILE *f, mix_word *buf, mix_word posn, int blocksize) {
  size_t wr = fread(buf, sizeof(mix_word), blocksize, f);
  if(wr == blocksize)
    return;
  buf += wr;
  while(wr++ < blocksize)
    *buf++ = 0;
}

void write_binary(FILE *f, mix_word *buf, mix_word posn, int blocksize) {
  fwrite(buf, sizeof(mix_word), blocksize, f);
}

void tape_control(FILE *f, mix_word operand, int blocksize) {
  int32_t o = mix_word_magnitude(operand);
  if(o == 0)
    rewind(f);
  if(mix_word_is_negative(operand))
    o = -o;
  if(fseek(f, o * blocksize * sizeof(mix_word), SEEK_CUR)
      && errno == EINVAL)
    rewind(f);
}

void disk_control(FILE *f, mix_word operand, int blocksize) {
  int32_t o = mix_word_magnitude(operand);
  fseek(f, o * blocksize * sizeof(mix_word), SEEK_SET);
}

static int lines_left = PRINTER_PAGE_LEN;
void write_printer(FILE *f, mix_word *buf, mix_word posn, int blocksize) {
  lines_left--;
  if(lines_left == 0)
    lines_left = PRINTER_PAGE_LEN;
  write_char(f, buf, posn, blocksize);
}

void printer_control(FILE *f, mix_word operand, int blocksize) {
  while(lines_left--)
    putc('\n', f);
  lines_left = PRINTER_PAGE_LEN;
}
