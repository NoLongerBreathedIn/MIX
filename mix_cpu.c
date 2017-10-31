/*
 * mix_cpu.c
 *
 *  Created on: Sep 2, 2015
 *      Author: eyal
 */

#include <stdlib.h>

#include "mix_types.h"
#include "mix_io.h"
#include "mix_float.h"
#include "mix_decode.h"

bool oflow;

mix_short rJ = 0, rI[6] = {0, 0, 0, 0, 0, 0};
mix_short ip = 0;
mix_comp_ind comp = ZERO;
mix_word rA = 0, rX = 0;
mix_word *memp, *memn;

mix_word *mem(mix_short addr);
void perform_interrupt(mix_short loc);
void return_from_interrupt(void);

int main(int argc, char **argv) {
  mix_word inst;
  int time = 0;
  bool running = true;
  bool in_control_last = false;
  bool interrupt_req[21];
  mix_init_types();
  setupIO();
  setup_decode();
  memn = calloc(8191, sizeof(mix_word));
  memp = memn + 4095;
  input(memp, 0, 16);
  while(!ready(16));
  while(running) {
    int i;
    mix_short M;
    internal_instr op;
    mix_fspec f;
    jump_type j;
    mix_comp_ind c = comp;
    arith_type a;
    inst = *mem(ip);
    mix_word t;
    M = inst >> 18;
    op = decode[mix_word_get_byte(inst, 5)];
    i = mix_word_get_byte(inst, 5) - 1 & 7;
    f = mix_word_get_byte(inst, 3) % 7;
    if(f)
      M = mix_short_add(M, rI[f - 1]);
    f = mix_word_get_byte(inst, 4);
    switch(op) {
    case LDA:
      rA = mix_word_get_field(f, *mem(M));
      if(mix_word_get_byte(inst, 5) & 16)
	mix_word_reverse_sign(rA);
      break;
    case LDX:
      rX = mix_word_get_field(f, *mem(M));
      if(mix_word_get_byte(inst, 5) & 16)
	mix_word_reverse_sign(rX);
      break;
    case LDi:
      rI[i] = mix_word_to_short(mix_word_get_field(f, *mem(M)));
      if(mix_word_get_byte(inst, 5) & 16)
	mix_short_reverse_sign(rI[i]);
      break;
    case STA:
      *mem(M) = mix_word_store_field(f, rA, *mem(M));
      break;
    case STX:
      *mem(M) = mix_word_store_field(f, rX, *mem(M));
      break;
    case STi:
      *mem(M) = mix_word_store_field(f,
				     mix_short_to_word(rI[i]), *mem(M));
      break;
    case STJ:
      *mem(M) = mix_word_store_field(f,
				     mix_short_to_word(rJ), *mem(M));
      break;
    case STZ:
      *mem(M) = mix_word_store_field(f, 0, *mem(M));
      break;
    case ADD:
      a = misc_style[f];
      switch(a) {
      case MIX_INT:
	oflow |= mix_word_add_and_carry(rA,
					mix_word_get_field(f, *mem(M)),
					NULL, &rA);
	break;
      case FLT:
	rA = shorten(fadd(lengthen(rA), lengthen(*mem(M))));
	break;
      case DBL:
	split_double(fadd(combine_to_double(rA, rX),
			  combine_to_double(*mem(M),
					    *mem(mix_short_add(M, 1)))),
		     &rA, &rX);
	break;
      default:;
      }
      break;
    case SUB:
      a = misc_style[f];
      switch(a) {
      case MIX_INT:
	oflow |= mix_word_add_and_carry(rA,
	           mix_word_negate(mix_word_get_field(f, *mem(M))),
					NULL, &rA);
	break;
      case FLT:
	rA = shorten(fsub(lengthen(rA), lengthen(*mem(M))));
	break;
      case DBL:
	split_double(fsub(combine_to_double(rA, rX),
			  combine_to_double(*mem(M),
					    *mem(mix_short_add(M, 1)))),
		     &rA, &rX);
	break;
      default:;
      }
      break;
    case MUL:
      a = misc_style[f];
      switch(a) {
      case MIX_INT:
	mix_word_mul(rA, mix_word_get_field(f, *mem(M)),
		     &rA, &rX);
	break;
      case FLT:
	rA = shorten(fmul(lengthen(rA), lengthen(*mem(M))));
	break;
      case DBL:
	split_double(fmul(combine_to_double(rA, rX),
			  combine_to_double(*mem(M),
					    *mem(mix_short_add(M, 1)))),
		     &rA, &rX);
	break;
      case MX:
	split_double(fmul(lengthen(rA), lengthen(*mem(M))),
		     &rA, &rX);
      default:;
      }
      break;
    case DIV:
      a = misc_style[f];
      switch(a) {
      case MIX_INT:
	mix_word_div(rA, rX, mix_word_get_field(f, *mem(M)),
		     &rA, &rX);
	break;
      case FLT:
	rA = shorten(fdiv(lengthen(rA), lengthen(*mem(M))));
	break;
      case DBL:
	split_double(fdiv(combine_to_double(rA, rX),
			  combine_to_double(*mem(M),
					    *mem(mix_short_add(M, 1)))),
		     &rA, &rX);
	break;
      case MX:
	rA = shorten(fdiv(combine_to_double(rA, rX),
			  lengthen(*mem(M))));
      default:;
      }
      break;
    case EDA:
      if(f & 1)
	mix_short_reverse_sign(M);
      if(f & 2)
	rA = mix_short_to_word(M);
      else
	oflow |= mix_word_add_and_carry(rA, mix_short_to_word(M),
					NULL, &rA);
      break;
    case EDX:
      if(f & 1)
	mix_short_reverse_sign(M);
      if(f & 2)
	rX = mix_short_to_word(M);
      else
	oflow |= mix_word_add_and_carry(rX, mix_short_to_word(M),
					NULL, &rX);
      break;
    case EDi:
      if(f & 1)
	mix_short_reverse_sign(M);
      if(f & 2)
	rI[i] = M;
      else
	rI[i] = mix_short_add(rI[i], M);
      break;
    case CMPA:
      a = misc_style[f];
      switch(a) {
      case MIX_INT:
	comp = compare(mix_word_extract_field(f, rA),
		       mix_word_extract_field(f, *mem(M)));
	break;
      case FLT:
	comp = fcmp(lengthen(rA), lengthen(*mem(M)));
	break;
      case DBL:
	comp = fcmp(combine_to_double(rA, rX),
		    combine_to_double(*mem(M), *mem(mix_short_add(M, 1))));
	break;
      default:;
      }
      break;
    case CMPX:
      a = misc_style[f];
      switch(a) {
      case MIX_INT:
	comp = compare(mix_word_extract_field(f, rX),
		       mix_word_extract_field(f, *mem(M)));
	break;
      case FLT:
	comp = fcmp(lengthen(rX), lengthen(*mem(M)));
	break;
      default:;
      }
      break;
    case CMPi:
      comp = compare(mix_word_extract_field(f,
					    mix_short_to_word(rI[i])),
		     mix_word_extract_field(f, *mem(M)));
      break;
    case JS:
      j = js_style[f];
    jmp:
      if(j == SVJ)
	ip = mix_short_sub(M, 1);
      else if(j == OV || j == NOV) {
	if((j != OV) ^ oflow) {
	  rJ = mix_short_add(ip, 1);
	  ip = mix_short_sub(M, 1);
	}
	oflow = false;
      } else if(j & 1 << c) {
	rJ = mix_short_add(ip, 1);
	ip = mix_short_sub(M, 1);
      }
      break;
    case JAS:
      j = jrs_style[f & 15];
      a = jas_style[f >> 3];
      switch(a) {
      case MIX_INT:
	c = compare(rA, 0);
	break;
      case FLT:
	c = fcmp(lengthen(rA), 0);
	break;
      case DBL:
	c = fcmp(combine_to_double(rA, rX), 0);
	break;
      default:;
      }
      goto jmp;
    case JXS:
      j = jrs_style[f & 15];
      a = jxs_style[f >> 3];
      switch(a) {
      case MIX_INT:
	c = compare(rX, 0);
	break;
      case FLT:
	c = fcmp(lengthen(rX), 0);
	break;
      default:;
      }
      goto jmp;
    case JiS:
      j = jrs_style[f];
      c = compare(mix_short_to_word(rI[i]), 0);
      goto jmp;
    case IN:
      input(mem(M), rX, f);
      interrupt_req[f] |= mix_short_is_negative(ip);
      break;
    case OUT:
      output(mem(M), rX, f);
      interrupt_req[f] |= mix_short_is_negative(ip);
      break;
    case IOC:
      if(isdisk[f])
	t = rX;
      else
	t = mix_short_to_word(M);
      cntrl(t, f);
      interrupt_req[f] |= mix_short_is_negative(ip);
      break;
    case JRED:
      if(ready(f)) {
	rJ = mix_short_add(ip, 1);
	ip = mix_short_sub(M, 1);
      }
      break;
    case JBUS:
      if(!ready(f)) {
	if(ip == M && mix_short_is_negative(ip))
	  interrupt_req[f] = false;
	rJ = mix_short_add(ip, 1);
	ip = mix_short_sub(M, 1);
      }
      break;
    case MISC:
      switch(f & 7) {
      case 0: //NUM
	inst = 0;
	for(i = 1; i <= 5; i++)
	  inst = inst * 10 + mix_word_get_byte(rA, i);
	for(i = 1; i <= 5; i++)
	  inst = inst * 10 + mix_word_get_byte(rX, i);
	oflow = mix_word_is_negative(inst);
	rA = mix_word_sign(rA) | mix_word_magnitude(inst);
	break;
      case 1: //CHAR
	inst = mix_word_magnitude(rA);
	for(i = 5; i >= 1; i--) {
	  mix_word_set_byte(&rX, i, inst % 10 + 30);
	  inst %= 10;
	}
	for(i = 5; i >= 1; i--) {
	  mix_word_set_byte(&rX, i, inst % 10 + 30);
	  inst %= 10;
	}
	break;
      case 2: //HLT
	running = false;
	break;
      case 3: //LEN
	split_double(lengthen(rA), &rA, &rX);
	break;
      case 4: //SHR
	rA = shorten(combine_to_double(rA, rX));
	break;
      case 5: //FIX
	split_double(fix(combine_to_double(rA, rX)), &rA, &rX);
	break;
      case 6: //FLOT
	split_double(flot(combine_to_double(rA, rX)), &rA, &rX);
	break;
      case 7: //INT
	if(mix_short_is_negative(ip))
	  return_from_interrupt();
	else
	  perform_interrupt(12);
      default:;
      }
      break;
    case SHFT:
      switch(f) {
      case 0: //SLA
	mix_word_sl(rA, 0, mix_word_magnitude(M), &rA, NULL);
	break;
      case 1: //SRA
	mix_word_sr(rA, 0, mix_word_magnitude(M), &rA, NULL);
	break;
      case 2: //SLAX
	mix_word_sl(rA, rX, mix_word_magnitude(M), &rA, &rX);
	break;
      case 3: //SRAX
	mix_word_sr(rA, rX, mix_word_magnitude(M), &rA, &rX);
	break;
      case 4: //SLC
	mix_word_slc(rA, rX, mix_word_magnitude(M), &rA, &rX);
	break;
      case 5: //SRC
	mix_word_src(rA, rX, mix_word_magnitude(M), &rA, &rX);
      default:;
      }
      break;
    case MOV:
      time += 2 * f;
      while(f--) {
	*mem(rI[0]) = *mem(M);
	rI[0] = mix_short_add(rI[0], 1);
	M = mix_short_add(M, 1);
      }
      break;
    case NOP:
      break;
    default:;
    }
    time += op_time[op];
    if(!in_control_last && mix_short_is_positive(ip)) {
      if(time >= 1000 && mix_word_is_positive(*mem(010012))) {
	*mem(010012) -= time / 1000; //010012 is -10
	time %= 1000;
	if(mix_word_is_negative(*mem(010012)))
	  *mem(010012) = 0;
	if(!*mem(mix_short_negate(10)))
	  perform_interrupt(11);
      }
      for(i = 0; i < 21; i++)
	if(interrupt_req[i] && ready(i)) {
	  interrupt_req[i] = false;
	  perform_interrupt(20 + i);
	  break;
	}
    }
    in_control_last = mix_short_is_negative(ip);
    ip = mix_short_add(ip, 1);
  }
}

mix_word *mem(mix_short addr) {
  if(mix_short_is_negative(addr) && mix_short_is_positive(ip))
    return(memp);
  if(!mix_short_magnitude(addr))
    return(memp);
  if(mix_short_is_negative(addr))
    return(memn + 4095 - mix_short_magnitude(addr));
  return(memp + addr);
}

mix_word *register_storage = 0;

void perform_interrupt(mix_short loc) {
  int i;
  if(!register_storage)
    register_storage = mem(mix_short_negate(9));
  register_storage[0] = rA;
  for(i = 0; i < 6; i++)
    register_storage[i + 1] = mix_short_to_word(rI[i]);
  register_storage[7] = rX;
  register_storage[8] = ip << 18 | (oflow << 2 | comp) << 12 | (rJ & 07777);
  ip = mix_short_negate(loc + 1);
}

void return_from_interrupt(void) {
  int i;
  rA = register_storage[0];
  for(i = 0; i < 6; i++)
    rI[i] = mix_word_to_short(register_storage[i + 1]);
  rX = register_storage[7];
  ip = mix_word_get_field(002, register_storage[8]);
  i = mix_word_get_byte(register_storage[8], 3);
  oflow = i & 4;
  comp = i & 3;
  rJ = mix_word_get_field(045, register_storage[8]);
}
