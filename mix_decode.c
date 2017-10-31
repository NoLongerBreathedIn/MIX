/*
 * mix_decode.c
 *
 *  Created on: Sep 2, 2015
 *      Author: eyal
 */

#include "mix_decode.h"

internal_instr decode[64] = {
		NOP,  ADD,  SUB,  MUL,  DIV,  MISC, SHFT, MOV,
		LDA,  LDi,  LDi,  LDi,  LDi,  LDi,  LDi,  LDX,
		LDA,  LDi,  LDi,  LDi,  LDi,  LDi,  LDi,  LDX,
		STA,  STi,  STi,  STi,  STi,  STi,  STi,  STX,
		STJ,  STZ,  JBUS, IOC,  IN,   OUT,  JRED, JS,
		JAS,  JiS,  JiS,  JiS,  JiS,  JiS,  JiS,  JXS,
		EDA,  EDi,  EDi,  EDi,  EDi,  EDi,  EDi,  EDX,
		CMPA, CMPi, CMPi, CMPi, CMPi, CMPi, CMPi, CMPX
};

jump_type js_style[18] = {
		ALL, SVJ, OV, NOV, LT, EQ, GT, GE, NE, LE, RE,
		NGE, NGL, NLE, NL, GL, NG, NAN
};

jump_type jrs_style[16] = {
		LT, EQ, GT, NL, NE, NG, NAN, 0,
		NGE, NGL, NLE, GE, GL, LE, RE, 0
};

arith_type jas_style[5] = {
		MIX_INT, FLT, FLT, DBL, DBL
};

arith_type jxs_style[3] = {
		MIX_INT, FLT, FLT
};

arith_type misc_style[64];

int op_time[31];

void setup_decode() {
	int i;
	for(i = 0; i < 64; i++)
		misc_style[i] = MIX_INT;
	misc_style[6] = FLT;
	misc_style[7] = DBL;
	misc_style[14] = MX;
	op_time[LDA] = op_time[LDX] = op_time[LDi] = 2;
	op_time[STA] = op_time[STX] = op_time[STi] = 2;
	op_time[STJ] = op_time[STZ] = 2;
	op_time[NOP] = op_time[MISC] = 1;
	op_time[MOV] = 1;
	op_time[JRED] = op_time[JBUS] = 1;
	op_time[JS] = op_time[JAS] = op_time[JXS] = op_time[JiS] = 1;
	op_time[EDA] = op_time[EDX] = op_time[EDi] = 1;
	op_time[CMPA] = op_time[CMPX] = op_time[CMPi] = 2;
	op_time[IOC] = op_time[IN] = op_time[OUT] = 1;
	op_time[ADD] = op_time[SUB] = 2;
	op_time[MUL] = 10;
	op_time[DIV] = 12;
	op_time[SHFT] = 2;
}
