/*
 * mix_decode.h
 *
 *  Created on: Sep 2, 2015
 *      Author: eyal
 */

#ifndef MIX_DECODE_H_
#define MIX_DECODE_H_

typedef enum {
	LDA, LDX, LDi,
	STA, STX, STi, STJ, STZ,
	ADD, SUB, MUL, DIV,
	EDA, EDX, EDi,
	CMPA, CMPX, CMPi,
	JS, JAS, JXS, JiS,
	IN, OUT, IOC, JRED, JBUS,
	MISC, SHFT, MOV, NOP
} internal_instr;

typedef enum {
	SVJ, LT, EQ, LE, GT, GL, GE, RE,
	NAN, NGE, NGL, NG, NLE, NE, NL, ALL,
	OV, NOV
} jump_type;

typedef enum {
	MIX_INT, FLT, DBL, MX
} arith_type;

extern internal_instr decode[];
extern jump_type js_style[], jrs_style[];
extern arith_type jas_style[], jxs_style[];
extern arith_type misc_style[];
extern int op_time[];

extern void setup_decode();

#endif /* MIX_DECODE_H_ */
