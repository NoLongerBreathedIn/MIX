/*
 * mix_io.c
 *
 *  Created on: Sep 1, 2015
 *      Author: eyal
 */


#include "mix_io.h"
#include "atomic_queue.h"
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
// Ugly hack to get Eclipse to handle _Atomic correctly.
#ifdef __CDT_PARSER__
#define _Atomic
typedef char atomic_flag;
#define ATOMIC_FLAG_INIT 1;
char atomic_flag_test_and_set(char *c) {
  char w = *c;
  *c = 1;
  return(w);
}
#else
#include <stdatomic.h>
#endif

#define P(x) while(atomic_flag_test_and_set(x)) pthread_yield()
#define V atomic_flag_clear

#define PRINTER_PAGE_LEN 22

static FILE *input_file[21], *output_file[21];
static void (*reader[21])(FILE *f, mix_word *buf,
			  mix_word posn, int blocksize);
static void (*writer[21])(FILE *f, mix_word *buf,
			  mix_word posn, int blocksize);
static void (*control[21])(FILE *f, mix_word operand, int blocksize);
static atomic_flag running[21];
static atomic_schar overwork, todo_l[21];
int bsize[21];
bool isdisk[21];
static atomic_queue *ioq;
static atomic_flag tcreate = ATOMIC_FLAG_INIT, work_guard = ATOMIC_FLAG_INIT;

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
 * 6. Set input structure's read to full.
 * 7. Create a new thread running the handler.
 * 8. p(read)
 */

typedef struct {
  void (*func)(FILE *f, mix_word *buf, mix_word posn, int blocksize);
  FILE *f;
  mix_word posn;
  int blocksize;
  mix_word *buf;
} rw;

typedef struct {
  void (*func)(FILE *f, mix_word operand, int blocksize);
  FILE *f;
  mix_word operand;
  int blocksize;
} ctl;

typedef struct {
  atomic_flag *sem;
  atomic_flag begun;
  atomic_char *cnt;
  bool is_rw;
  union {
    ctl cs;
    rw rws;
  } dat;
} todo;

#define inc_todo(cnt) if(atomic_fetch_add(cnt, 1) == 0) \
    atomic_fetch_add(&overwork, 1);
#define dec_todo(cnt) if(atomic_fetch_sub(cnt, 1) == 1) \
    atomic_fetch_sub(&overwork, 1);

/* The next two functions work together.
   The todo structure contains stuff that is unimportant for synchronization,
   as well as three important variables:
   begun -- a semaphore to do with whether the operation has begun
   sem -- points to the semaphore for this particular device
   cnt -- points to the task count for this device.
   There are a few other important variables:
   ioq -- a queue of work items, each associated with a semaphore
   work_guard -- a semaphore about the work queue operations
   overwork -- keeps track of how many more devices are busy
   than there are worker threads

   The functions dec_todo and inc_todo decrement/increment the appropriate
   counter of operations on the device and, if appropriate,
   also modify overwork.

   worker does as follows:
   1) p(work_guard)
   2) decrement overwork
   3) Try to get a queue item, call it t; if it's null, go to 
   4) v(work_guard)
   5) v(begun)
   6) 

*/


static void *worker(void *unused) {
  todo *t;
  P(work_guard);
  atomic_fetch_sub(&overwork, 1);
  while((t = (todo *)atomic_dequeue(ioq)) != NULL) {
    atomic_char *cnt;
    V(work_guard)
    V(&t->begun);
    if(t->is_rw)
      t->dat.rws.func(t->rws.f, t->rws.buf, t->rws.posn, t->rws.blocksize);
    else
      t->dat.cs.func(t->cs.f, t->cs.operand, t->cs.blocksize);
    V(t->sem);
    cnt = t->cnt;
    free(t);
    P(work_guard);
    dec_todo(cnt);
  }
  atomic_fetch_sub(&overwork, 1);
  V(work_guard);
  return(NULL);
}

static void submit(todo *stuff, char device) {
  pthread_t ignore;
  atomic_flag_test_and_set(&(stuff->begun = ATOMIC_FLAG_INIT));
  atomic_enqueue(ioq, *stuff, stuff->sem = running + device);
  P(work_guard);
  inc_todo(stuff->cnt = todo_l + device);
  if(atomic_load(overwork) > 0)
    pthread_create(&ignore, NULL, worker, NULL);
  V(work_guard);
  P(&stuff->begun);
}

void input(mix_word *mem, mix_word posn, char device) {
  todo *stuff;
  if((unsigned)device > 20)
    return;
  stuff = malloc(sizeof(todo));
  stuff->dat.rws.func = reader[(int)device];
  stuff->dat.rws.f = input_file[(int)device];
  stuff->dat.rws.buf = mem;
  stuff->dat.rws.blocksize = bsize[(int)device];
  stuff->dat.rws.posn = posn;
  stuff->is_rw = true;
  submit(stuff, device);
}

void output(mix_word *mem, mix_word posn, char device) {
  todo *stuff;
  if((unsigned)device > 20)
    return;
  stuff = malloc(sizeof(todo));
  stuff->dat.rws.func = writer[(int)device];
  stuff->dat.rws.f = output_file[(int)device];
  stuff->dat.rws.buf = mem;
  stuff->dat.rws.blocksize = bsize[(int)device];
  stuff->dat.rws.posn = posn;
  stuff->is_rw = true;
  submit(stuff, device);
}

void cntrl(mix_word argument, char device) {
  todo *stuff;
  if((unsigned)device > 20)
    return;
  stuff = malloc(sizeof(todo));
  stuff->dat.cs.func = control[(int)device];
  stuff->dat.cs.f = output_file[(int)device];
  stuff->dat.cs.blocksize = bsize[(int)device];
  stuff->dat.cs.argument = argument;
  stuff->is_rw = false;
  submit(stuff, device);
}

bool ready(char device) {
  return(!atomic_load(todo_l + device));
}

static void trivial_control(FILE *f, mix_word operand, int blocksize) {}

static void trivialRW(FILE *f, mix_word *buf, mix_word posn, int blocksize) {}

static void read_binary(FILE *f, mix_word *buf, mix_word posn, int blocksize);
static void read_char(FILE *f, mix_word *buf, mix_word posn, int blocksize);
static void write_binary(FILE *f, mix_word *buf, mix_word posn, int blocksize);
static void write_char(FILE *f, mix_word *buf, mix_word posn, int blocksize);
static void tape_control(FILE *f, mix_word operand, int blocksize);
static void disk_control(FILE *f, mix_word operand, int blocksize);
static void write_printer(FILE *f, mix_word *buf,
			  mix_word posn, int blocksize);
static void printer_control(FILE *f, mix_word operand, int blocksize);

static void read_disk(FILE *f, mix_word *buf, mix_word posn, int blocksize) {
  disk_control(f, posn, blocksize);
  read_binary(f, buf, posn, blocksize);
}
static void write_disk(FILE *f, mix_word *buf, mix_word posn, int blocksize) {
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
  for(i = 0; i < 21; i++) {
    running[i] = ATOMIC_FLAG_INIT;
    atomic_init(todo_l + i, 0);
  }
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
  ioq = create_atomic_queue();
  atomic_init(&workers, 0);
  atomic_init(&todos, 0);
}

static void read_char(FILE *f, mix_word *buf, mix_word posn, int blocksize) {
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

static void write_char(FILE *f, mix_word *buf, mix_word posn, int blocksize) {
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

static void read_binary(FILE *f, mix_word *buf, mix_word posn, int blocksize) {
  size_t wr = fread(buf, sizeof(mix_word), blocksize, f);
  if(wr == blocksize)
    return;
  buf += wr;
  while(wr++ < blocksize)
    *buf++ = 0;
}

static void write_binary(FILE *f, mix_word *buf,
			 mix_word posn, int blocksize) {
  fwrite(buf, sizeof(mix_word), blocksize, f);
}

static void tape_control(FILE *f, mix_word operand, int blocksize) {
  int32_t o = mix_word_magnitude(operand);
  if(o == 0)
    rewind(f);
  if(mix_word_is_negative(operand))
    o = -o;
  if(fseek(f, o * blocksize * sizeof(mix_word), SEEK_CUR)
      && errno == EINVAL)
    rewind(f);
}

static void disk_control(FILE *f, mix_word operand, int blocksize) {
  int32_t o = mix_word_magnitude(operand);
  fseek(f, o * blocksize * sizeof(mix_word), SEEK_SET);
}

static int lines_left = PRINTER_PAGE_LEN;
static void write_printer(FILE *f, mix_word *buf,
			  mix_word posn, int blocksize) {
  lines_left--;
  if(lines_left == 0)
    lines_left = PRINTER_PAGE_LEN;
  write_char(f, buf, posn, blocksize);
}

static void printer_control(FILE *f, mix_word operand, int blocksize) {
  while(lines_left--)
    putc('\n', f);
  lines_left = PRINTER_PAGE_LEN;
}
