/*
 * lexparse.h
 *
 *  Created on: Sep 10, 2015
 *      Author: eyal
 */

#ifndef LEXPARSE_H_
#define LEXPARSE_H_

#include "mix_types.h"

typedef struct {
	char label[10];
	mix_word res;
} var;


extern mix_word compute_word(char *a, int nch, mix_short *fref,
		mix_short *bref, void *symtab, mix_short loc);
extern mix_word compute_expr(char *a, int nch, mix_short *fref,
		mix_short *bref, void *symtab, mix_short loc);

#endif /* LEXPARSE_H_ */
