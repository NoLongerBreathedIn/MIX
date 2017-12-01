/*
 * mix_types.c
 *
 *  Created on: Aug 5, 2015
 *      Author: eyal
 */
#include "mix_types.h"
#include <ctype.h>
#include <assert.h>
static bool is_bad_field[MIX_BYTE_MAX + 1];
static unsigned shift[MIX_BYTE_MAX + 1];
static long mask[MIX_BYTE_MAX + 1];
#define NONP_ '?'

static mix_char ascii_to_mixcii_[128];
static char mixcii_to_ascii_[MIX_CHAR_MAX + 1] = {
    ' ', 'A',  'B', 'C', 'D', 'E', 'F', 'G',
    'H', 'I',  '~', 'J', 'K', 'L', 'M', 'N',
    'O', 'P',  'Q', 'R', '[', ']', 'S', 'T',
    'U', 'V',  'W', 'X', 'Y', 'Z', '0', '1',
    '2', '3',  '4', '5', '6', '7', '8', '9',
    '.', ',',  '(', ')', '+', '-', '*', '/',
    '=', '$',  '<', '>', '@', ';', ':', '\'',
    '`', '\\', '"', '&', '{', '}', '|', '^'
};

void mix_init_types(void) {
  int l, r;
  for(l = 0; l < 8; l++)
    for(r = 0; r < 8; r++) {
      int F = l << 3 | r;
      if((is_bad_field[F] = r < l || 5 < r))
	shift[F] = mask[F] = 0;
      else {
	int width = r - ((l == 0)? 1 : l) + 1;
	mask[F] = ((1 << 6 * width) - 1) << (shift[F] = 6 * (5 - r));
      }
    }
  for(l = 0; l < 128; l++)
    ascii_to_mixcii_[l] = NONP_;
  for(l = 0; l <= MIX_CHAR_MAX; l++)
    ascii_to_mixcii_[(int)mixcii_to_ascii_[l]] = l;
  for(l = 0; l < 26; l++)
    ascii_to_mixcii_['a' + l] = ascii_to_mixcii_['A' + l];
}

mix_char ascii_to_mixcii(char c) {
  return(ascii_to_mixcii_[toupper(c)]);
}

char mixcii_to_ascii(mix_char c) {
  return(c > MIX_CHAR_MAX? NONP_ : mixcii_to_ascii_[c]);
}

mix_word mix_bytes_to_word(mix_byte *mbs, unsigned byteno) {
  mix_word result = 0;
  unsigned k;
  if(mbs == NULL || byteno == 0) return(MIX_WORD_ZERO);
  if(byteno > 5) byteno = 5;
  for(k = 0; k < byteno; k++)
    result |= (long)mbs[k] << 6 * (byteno - k - 1);
  return(result);
}

mix_word mix_word_get_field(mix_fspec f, mix_word w) {
  mix_word result;
  if(is_bad_field[f]) return(MIX_WORD_ZERO);
  result = (w & mask[f]) >> shift[f];
  if(f < 8)
    result |= mix_word_sign(w);
  return(result);
}

mix_word mix_word_set_field(mix_fspec f, mix_word from, mix_word to) {
  mix_word result;
  int m = mask[f];
  if(is_bad_field[f]) return(to);
  if(f < 8)
    result = (to & ~m & ~MIX_WORD_SIGN_BIT) | (from & m) | mix_word_sign(from);
  else
    result = (to & ~m) | (from & m);
  return(result);
}

mix_word mix_word_store_field(mix_fspec f, mix_word from, mix_word to) {
  int m = mask[f];
  if(is_bad_field[f]) return(to);
  if(f < 8)
    return((to & ~m & ~MIX_WORD_SIGN_BIT) |
	(from << shift[f] & m) | mix_word_sign(from));
  return((to & ~m) | (from << shift[f] & m));
}

bool mix_fspec_is_valid(mix_fspec f) {
  return(!is_bad_field[f]);
}

mix_byte mix_word_get_byte(mix_word w, int idx) {
  mix_fspec fspec = mix_fspec_new(idx, idx);
  if(idx < 1 || idx > 5) return(MIX_BYTE_ZERO);
  return(mix_byte_new(mix_word_get_field(fspec, w)));
}

void mix_word_set_byte(mix_word *w, int idx, mix_byte mb) {
  mix_fspec f = mix_fspec_new(idx, idx);
  if(w == NULL || idx < 1 || idx > 5) return;
  *w = mix_word_set_field(f, (mix_word)mb << shift[f], *w);
}

mix_word mix_word_add(mix_word x, mix_word y) {
  static const unsigned MIX_WORD_MAX_SIM = 2 * MIX_WORD_MAX + 1;
  int res;
  if(mix_word_sign(x) == mix_word_sign(y)) {
    res = mix_word_magnitude(x) + mix_word_magnitude(y);
    if(res > MIX_WORD_MAX) {
      res = MIX_WORD_MAX_SIM - res;
      res |= mix_word_sign(mix_word_negate(x));
    } else
      res |= mix_word_sign(x);
  } else {
    res = mix_word_magnitude(x) - mix_word_magnitude(y);
    if(res < 0)
      res = -res | mix_word_sign(y);
    else
      res |= mix_word_sign(x);
  }
  assert(res >= 0);
  return((mix_word)res);
}

bool mix_word_add_and_carry(mix_word x, mix_word y, mix_word *h, mix_word *l) {
  bool res;
  if(mix_word_sign(x) == mix_word_sign(y)) {
    unsigned sum = mix_word_magnitude(x) + mix_word_magnitude(y);
    if(sum > MIX_WORD_MAX) {
      res = true;
      if(l != NULL) {
	*l = sum - MIX_WORD_MAX - 1;
	*l |= mix_word_sign(x);
      }
      if(h != NULL)
	*h = sum >> 30 | mix_word_sign(x);
    } else {
      res = false;
      if(h != NULL) *h = 0;
      if(l != NULL)
	*l = sum | mix_word_sign(x);
    }
  } else {
    res = false;
    if(h != NULL) *h = 0;
    if(l != NULL) {
      int dif = mix_word_magnitude(x) - mix_word_magnitude(y);
      if(dif < 0)
	*l = (-dif) | mix_word_sign(y);
      else
	*l = dif | mix_word_sign(x);
    }
  }
  return(res);
}

void mix_word_mul(mix_word x, mix_word y, mix_word *h, mix_word *l) {
  mix_word s = mix_word_sign(x ^ y);
  unsigned long p = (unsigned long)mix_word_magnitude(x) *
      (unsigned long)mix_word_magnitude(y);
  if(h != NULL)
    *h = s | mix_word_new(p >> 30);
  if(l != NULL)
    *l = s | mix_word_new(p);
}

bool mix_word_div(mix_word nh, mix_word nl, mix_word d,
    mix_word *q, mix_word *r) {
  if(mix_word_magnitude(d) <= mix_word_magnitude(nh))
    return(true);
  long num = (long)mix_word_magnitude(nh) << 30 | mix_word_magnitude(nl);
  long den = mix_word_magnitude(d);
  long quot = num / den;
  long rem = num % den;
  if(q != NULL)
    *q = mix_word_new(quot) | mix_word_sign(nh ^ d);
  if(r != NULL)
    *r = mix_word_new(rem) | mix_word_sign(nh);
  return(false);
}

void mix_word_sl(mix_word A, mix_word X, unsigned long count,
    mix_word *pA, mix_word *pX) {
  if(pX != NULL) *pX = mix_word_sign(X);
  if(pA != NULL) *pA = mix_word_sign(A);
  if(count < 5) {
    if(pX != NULL)
      *pX |= MIX_WORD_MAX & X << 6 * count;
    if(pA != NULL)
      *pA |= MIX_WORD_MAX & (A << 6 * count |
	  mix_word_magnitude(X) >> (30 - 6 * count));
  } else if(count < 10 && pA != NULL)
    *pA |= MIX_WORD_MAX & X << (6 * count - 30);
}

void mix_word_sr(mix_word A, mix_word X, unsigned long count,
    mix_word *pA, mix_word *pX) {
  if(pX != NULL) *pX = mix_word_sign(X);
  if(pA != NULL) *pA = mix_word_sign(A);
  if(count < 5) {
    if(pA != NULL)
      *pA |= mix_word_magnitude(A) >> 6 * count;
    if(pX != NULL)
      *pX |= MIX_WORD_MAX & (mix_word_magnitude(X) >> 6 * count
	  | A << (30 - 6 * count));
  } else if(count < 10 && pX != NULL) {
    *pX |= mix_word_magnitude(A) >> (6 * count - 30);
  }
}

void mix_word_slc(mix_word A, mix_word X, unsigned long count,
    mix_word *pA, mix_word *pX) {
  count %= 10;
  if(pX != NULL) *pX = mix_word_sign(X);
  if(pA != NULL) *pA = mix_word_sign(A);
  if(count > 5) {
    count -= 5;
    A ^= X;
    X ^= A;
    A ^= X;
  }
  count *= 6;
  if(pX != NULL) {
    *pX |= MIX_WORD_MAX & X << count;
    *pX |= mix_word_magnitude(A) >> (30 - count);
  }
  if(pA != NULL) {
    *pA |= MIX_WORD_MAX & A << count;
    *pA |= mix_word_magnitude(X) >> (30 - count);
  }
}

void mix_word_print_to_buffer(mix_word w, char *buf) {
  if(buf == NULL) return;
  sprintf(buf, "%c%011o",
      mix_word_is_negative(w)? '-' : '+', mix_word_magnitude(w));
}

mix_short mix_word_to_short(mix_word w) {
  if(mix_word_is_negative(w))
    return((MIX_SHORT_MAX & w) | MIX_SHORT_SIGN_BIT);
  return(MIX_SHORT_MAX & w);
}

mix_word mix_short_to_word(mix_short ms) {
  if(mix_short_is_negative(ms))
    return((mix_word)(mix_short_magnitude(ms) | MIX_WORD_SIGN_BIT));
  return((mix_word)ms);
}

mix_short mix_short_add(mix_short x, mix_short y) {
  mix_short xm = mix_short_magnitude(x), ym = mix_short_magnitude(y);
  if(mix_short_is_negative(x) ^ mix_short_is_negative(y))
    return(mix_short_sign(x) ^ mix_short_new((int16_t)(xm - ym)));
  return(mix_short_sign(x) ^ mix_short_new(xm + ym));
}

void mix_short_fprint(FILE *f, mix_short ms, const char *message) {
  if(message) fprintf(f, "%s ", message);
  fprintf(f, "%c%05o", mix_short_is_negative(ms)? '-' : '+',
      mix_short_magnitude(ms));
}

mix_comp_ind compare(mix_word x, mix_word y) {
  if(mix_word_magnitude(x) == 0 && mix_word_magnitude(y) == 0)
    return(ZERO);
  if(mix_word_sign(x) ^ mix_word_sign(y))
    return(mix_word_sign(x)? LESS : GREATER);
  if(mix_word_magnitude(x) == mix_word_magnitude(y))
    return(ZERO);
  return(mix_word_sign(x) ^ (mix_word_magnitude(x) < mix_word_magnitude(y))?
      LESS : GREATER);
}
