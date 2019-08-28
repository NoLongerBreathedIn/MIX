/*
 * mix_io.h
 *
 *  Created on: Sep 1, 2015
 *      Author: eyal
 */

#ifndef MIX_IO_H_
#define MIX_IO_H_

#include "mix_types.h"

extern void input(mix_word *mem, mix_word posn, char device);
extern void output(mix_word *mem, mix_word posn, char device);
extern void cntrl(mix_word argument, char device);
extern bool ready(char device);

extern int bsize[];
extern bool isdisk[];

extern void setupIO(void);

#endif /* MIX_IO_H_ */
