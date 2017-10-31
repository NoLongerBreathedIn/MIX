/*
 * mix_float.h
 *
 *  Created on: Aug 18, 2015
 *      Author: eyal
 */

#ifndef MIX_FLOAT_H_
#define MIX_FLOAT_H_
#include "mix_types.h"

extern bool oflow;

typedef uint64_t mix_double;

extern mix_double fmul(mix_double x, mix_double y);
extern mix_double fdiv(mix_double x, mix_double y);
extern mix_double fadd(mix_double x, mix_double y);
#define fsub(x, y) fadd((x), (y) ^ 1L << 62)

extern mix_comp_ind fcmp(mix_double y, mix_double z); //can't overflow

extern mix_double fix(mix_double x); //put in rAX
extern mix_double flot(mix_double x); //can't overflow
extern mix_word shorten(mix_double x);
extern mix_double lengthen(mix_word x);

#define combine_to_double(h, l) ((mix_double)(h) << 32 | (l))
extern void split_double(mix_double d, mix_word *h, mix_word *l);

#endif /* MIX_FLOAT_H_ */
