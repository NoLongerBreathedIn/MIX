/*
 * mix_encode.c
 *
 *  Created on: Sep 9, 2015
 *      Author: eyal
 */

#ifndef MIX_ENCODE_C_
#define MIX_ENCODE_C_

#include "mix_encode.h"

#ifdef __CDT_PARSER__
#define MIX_INT 22
#endif

mix_byte default_f[CON] = {
	0, 005, 6, 7,
	005, 6, 7, 005, 6, 7, 14,
	005, 6, 7, 14, 0, 1, 2, 3, 4, 5, 6, 7,
	0, 1, 2, 3, 4, 5, 1,
	005, 005, 005, 005, 005, 005, 005, 005,
	005, 005, 005, 005, 005, 005, 005, 005,
	005, 005, 005, 005, 005, 005, 005, 005,
	002, 005, 0, 040, 0,
	0, 0,
	0, 040,
	0, 1, 2, 3,
	4, 5, 6, 7, 8, 9, 10,
	11, 12, 13, 14, 15, 16, 17,
	042, 043,
	044, 045, 046, 047, 050, 051, 052,
	053, 054, 055, 056, 057, 060, 061,
	000, 001, 002, 003, 004, 005,
	010, 011, 012, 013, 014, 015, 016,
	020, 021, 022, 023, 024, 025, 026,
	030, 031, 032, 033, 034, 035, 036,
	040, 041, 042, 043, 044, 045,
	050, 051, 052, 053, 054, 055, 056,
	060, 061, 062, 063, 064, 065, 066,
	070, 071, 072, 073, 074, 075, 076,
	000, 001, 002, 003, 004, 005,
	040, 041, 042, 043, 044, 045,
	000, 001, 002, 003, 004, 005,
	040, 041, 042, 043, 044, 045,
	000, 001, 002, 003, 004, 005,
	040, 041, 042, 043, 044, 045,
	000, 001, 002, 003, 004, 005,
	040, 041, 042, 043, 044, 045,
	000, 001, 002, 003, 004, 005,
	040, 041, 042, 043, 044, 045,
	000, 001, 002, 003, 004, 005,
	040, 041, 042, 043, 044, 045,
	000, 001, 002, 003, 004, 005,
	010, 011, 012, 013, 014, 015, 016,
	020, 021, 022, 023, 024, 025, 026,
	030, 031, 032, 033, 034, 035, 036,
	040, 041, 042, 043, 044, 045,
	050, 051, 052, 053, 054, 055, 056,
	060, 061, 062, 063, 064, 065, 066,
	070, 071, 072, 073, 074, 075, 076,
	0, 1, 2, 3, 0, 1, 2, 3,
	0, 1, 2, 3, 0, 1, 2, 3,
	0, 1, 2, 3, 0, 1, 2, 3,
	0, 1, 2, 3, 0, 1, 2, 3,
	005, 6, 7, 005, 005, 005,
	005, 005, 005, 005, 6
};

mix_byte opcode[CON] = {
	0, 1, 1, 1,
	2, 2, 2, 3, 3, 3, 3,
	4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 7,
	8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23,
	24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 34, 35,
	36, 37,
	38, 38,
	39, 39, 39, 39,
	39, 39, 39, 39, 39, 39, 39,
	39, 39, 39, 39, 39, 39, 39,
	39, 39,
	39, 39, 39, 39, 39, 39, 39,
	39, 39, 39, 39, 39, 39, 39,
	40, 40, 40, 40, 40, 40,
	40, 40, 40, 40, 40, 40, 40,
	40, 40, 40, 40, 40, 40, 40,
	40, 40, 40, 40, 40, 40, 40,
	40, 40, 40, 40, 40, 40,
	40, 40, 40, 40, 40, 40, 40,
	40, 40, 40, 40, 40, 40, 40,
	40, 40, 40, 40, 40, 40, 40,	
	41, 41, 41, 41, 41, 41,
	41, 41, 41, 41, 41, 41,
	42, 42, 42, 42, 42, 42,
	42, 42, 42, 42, 42, 42,
	43, 43, 43, 43, 43, 43,
	43, 43, 43, 43, 43, 43,
	44, 44, 44, 44, 44, 44,
	44, 44, 44, 44, 44, 44,
	45, 45, 45, 45, 45, 45,
	45, 45, 45, 45, 45, 45,
	46, 46, 46, 46, 46, 46,
	46, 46, 46, 46, 46, 46,
	47, 47, 47, 47, 47, 47,
	47, 47, 47, 47, 47, 47, 47,
	47, 47, 47, 47, 47, 47, 47,
	47, 47, 47, 47, 47, 47, 47,
	47, 47, 47, 47, 47, 47,
	47, 47, 47, 47, 47, 47, 47,
	47, 47, 47, 47, 47, 47, 47,
	47, 47, 47, 47, 47, 47, 47,
	48, 48, 48, 48, 49, 49, 49, 49,
	50, 50, 50, 50, 51, 51, 51, 51,
	52, 52, 52, 52, 53, 53, 53, 53,
	54, 54, 54, 54, 55, 55, 55, 55,
	56, 56, 56, 57, 58, 59,
	60, 61, 62, 63, 63
};

instruction instr_table[] = {
	MIX_ADD, ALF, B1N, B1NN, B1NP, B1NZ, B1P, B1Z,
	B2N, B2NN, B2NP, B2NZ, B2P, B2Z, B3N, B3NN,
	B3NP, B3NZ, B3P, B3Z, B4N, B4NN, B4NP, B4NZ,
	B4P, B4Z, B5N, B5NN, B5NP, B5NZ, B5P, B5Z,
	B6N, B6NN, B6NP, B6NZ, B6P, B6Z, BAN, BANN,
	BANP, BANZ, BAP, BAZ, BBUS, BCMP, BDIN, BDIP,
	BDIZ, BDN, BDNN, BDNP, BDNR, BDNZ, BDP, BDPN,
	BDPZ, BDRE, BDZ, BDZN, BE, BFIN, BFIP, BFIZ,
	BFN, BFNN, BFNP, BFNR, BFNZ, BFP, BFPN, BFPZ,
	BFRE, BFZ, BFZN, BG, BGE, BGIN, BGIP, BGIZ,
	BGL, BGN, BGNN, BGNP, BGNR, BGNZ, BGP, BGPN,
	BGPZ, BGRE, BGZ, BGZN, BLE, BN, BNC, BNE,
	BNG, BNGE, BNGL, BNL, BNLE, BNOV, BOV, BRED,
	BXN, BXNN, BXNP, BXNZ, BXP, BXZ, CHR, CHR,
	CMP1, CMP2, CMP3, CMP4, CMP5, CMP6, MIX_CMPA, MIX_CMPX,
	CON, DADD, MIX_DBL, DCMP, DDIV, DEC1, DEC2, DEC3,
	DEC4, DEC5, DEC6, DECA, DECX, DFDV, MIX_DIV, DMUL,
	DSUB, END, ENN1, ENN2, ENN3, ENN4, ENN5, ENN6,
	ENNA, ENNX, ENT1, ENT2, ENT3, ENT4, ENT5, ENT6,
	ENTA, ENTX, EQU, FADD, FCMP, FDIV, FDML, FIX,
	FLOT, MIX_FLT, FMUL, FSUB, GCMP, HLT, MIX_IN, INC1,
	INC2, INC3, INC4, INC5, INC6, INCA, INCX, MIX_INT,
	MIX_IOC, J1N, J1NN, J1NP, J1NZ, J1P, J1Z, J2N,
	J2NN, J2NP, J2NZ, J2P, J2Z, J3N, J3NN, J3NP,
	J3NZ, J3P, J3Z, J4N, J4NN, J4NP, J4NZ, J4P,
	J4Z, J5N, J5NN, J5NP, J5NZ, J5P, J5Z, J6N,
	J6NN, J6NP, J6NZ, J6P, J6Z, JAN, JANN, JANP,
	JANZ, JAP, JAZ, MIX_JBUS, JCMP, JDIN, JDIP, JDIZ,
	JDN, JDNN, JDNP, JDNR, JDNZ, JDP, JDPN, JDPZ,
	JDRE, JDZ, JDZN, JE, JFIN, JFIP, JFIZ, JFN,
	JFNN, JFNP, JFNR, JFNZ, JFP, JFPN, JFPZ, JFRE,
	JFZ, JFZN, JG, JGE, JGIN, JGIP, JGIZ, JGL,
	JGN, JGNN, JGNP, JGNR, JGNZ, JGP, JGPN, JGPZ,
	JGRE, JGZ, JGZN, JL, JLE, JMP, JNC, JNE,
	JNG, JNGE, JNGL, JNL, JNLE, JNOV, JOV, MIX_JRED,
	JSJ, JXN, JXNN, JXNP, JXNZ, JXP, JXZ, LD1,
	LD1N, LD2, LD2N, LD3, LD3N, LD4, LD4N, LD5,
	LD5N, LD6, LD6N, MIX_LDA, LDAN, MIX_LDX, LDXN, LEN,
	LONG, MIX_MOV, MIX_MUL, MIX_NOP, NUM, ORIG, MIX_OUT, SHR,
	SLA, SLAX, SLC, SRA, SRAX, SRC, ST1, ST2,
	ST3, ST4, ST5, ST6, MIX_STA, MIX_STJ, MIX_STX, MIX_STZ, MIX_SUB
};

char instr_name[][5] = {
	"ADD", "ALF", "B1N", "B1NN", "B1NP", "B1NZ", "B1P", "B1Z",
	"B2N", "B2NN", "B2NP", "B2NZ", "B2P", "B2Z", "B3N", "B3NN",
	"B3NP", "B3NZ", "B3P", "B3Z", "B4N", "B4NN", "B4NP", "B4NZ",
	"B4P", "B4Z", "B5N", "B5NN", "B5NP", "B5NZ", "B5P", "B5Z",
	"B6N", "B6NN", "B6NP", "B6NZ", "B6P", "B6Z", "BAN", "BANN",
	"BANP", "BANZ", "BAP", "BAZ", "BBUS", "BCMP", "BDIN", "BDIP",
	"BDIZ", "BDN", "BDNN", "BDNP", "BDNR", "BDNZ", "BDP", "BDPN",
	"BDPZ", "BDRE", "BDZ", "BDZN", "BE", "BFIN", "BFIP", "BFIZ",
	"BFN", "BFNN", "BFNP", "BFNR", "BFNZ", "BFP", "BFPN", "BFPZ",
	"BFRE", "BFZ", "BFZN", "BG", "BGE", "BGIN", "BGIP", "BGIZ",
	"BGL", "BGN", "BGNN", "BGNP", "BGNR", "BGNZ", "BGP", "BGPN",
	"BGPZ", "BGRE", "BGZ", "BGZN", "BLE", "BN", "BNC", "BNE",
	"BNG", "BNGE", "BNGL", "BNL", "BNLE", "BNOV", "BOV", "BRED",
	"BXN", "BXNN", "BXNP", "BXNZ", "BXP", "BXZ", "CHAR", "CHR",
	"CMP1", "CMP2", "CMP3", "CMP4", "CMP5", "CMP6", "CMPA", "CMPX",
	"CON", "DADD", "DBL", "DCMP", "DDIV", "DEC1", "DEC2", "DEC3",
	"DEC4", "DEC5", "DEC6", "DECA", "DECX", "DFDV", "DIV", "DMUL",
	"DSUB", "END", "ENN1", "ENN2", "ENN3", "ENN4", "ENN5", "ENN6",
	"ENNA", "ENNX", "ENT1", "ENT2", "ENT3", "ENT4", "ENT5", "ENT6",
	"ENTA", "ENTX", "EQU", "FADD", "FCMP", "FDIV", "FDML", "FIX",
	"FLOT", "FLT", "FMUL", "FSUB", "GCMP", "HLT", "IN", "INC1",
	"INC2", "INC3", "INC4", "INC5", "INC6", "INCA", "INCX", "INT",
	"IOC", "J1N", "J1NN", "J1NP", "J1NZ", "J1P", "J1Z", "J2N",
	"J2NN", "J2NP", "J2NZ", "J2P", "J2Z", "J3N", "J3NN", "J3NP",
	"J3NZ", "J3P", "J3Z", "J4N", "J4NN", "J4NP", "J4NZ", "J4P",
	"J4Z", "J5N", "J5NN", "J5NP", "J5NZ", "J5P", "J5Z", "J6N",
	"J6NN", "J6NP", "J6NZ", "J6P", "J6Z", "JAN", "JANN", "JANP",
	"JANZ", "JAP", "JAZ", "JBUS", "JCMP", "JDIN", "JDIP", "JDIZ",
	"JDN", "JDNN", "JDNP", "JDNR", "JDNZ", "JDP", "JDPN", "JDPZ",
	"JDRE", "JDZ", "JDZN", "JE", "JFIN", "JFIP", "JFIZ", "JFN",
	"JFNN", "JFNP", "JFNR", "JFNZ", "JFP", "JFPN", "JFPZ", "JFRE",
	"JFZ", "JFZN", "JG", "JGE", "JGIN", "JGIP", "JGIZ", "JGL",
	"JGN", "JGNN", "JGNP", "JGNR", "JGNZ", "JGP", "JGPN", "JGPZ",
	"JGRE", "JGZ", "JGZN", "JL", "JLE", "JMP", "JNC", "JNE",
	"JNG", "JNGE", "JNGL", "JNL", "JNLE", "JNOV", "JOV", "JRED",
	"JSJ", "JXN", "JXNN", "JXNP", "JXNZ", "JXP", "JXZ", "LD1",
	"LD1N", "LD2", "LD2N", "LD3", "LD3N", "LD4", "LD4N", "LD5",
	"LD5N", "LD6", "LD6N", "LDA", "LDAN", "LDX", "LDXN", "LEN",
	"LONG", "MOV", "MUL", "NOP", "NUM", "ORIG", "OUT", "SHR",
	"SLA", "SLAX", "SLC", "SRA", "SRAX", "SRC", "ST1", "ST2",
	"ST3", "ST4", "ST5", "ST6", "STA", "STJ", "STX", "STZ", "SUB"
};

int nnames = 329;
#endif /* MIX_ENCODE_C_ */
