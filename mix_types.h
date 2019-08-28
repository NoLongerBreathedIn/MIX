/*
 * mix_types.h
 *
 *  Created on: Aug 5, 2015
 *      Author: eyal
 */

#ifndef MIX_TYPES_H_
#define MIX_TYPES_H_

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

extern void mix_init_types(void);
typedef uint8_t mix_byte;
#define MIX_BYTE_MAX ((mix_byte)63)
#define MIX_BYTE_ZERO ((mix_byte)0)
#define mix_byte_new(x) ((mix_byte)((x) & MIX_BYTE_MAX))
#define mix_byte_add(x, y) mix_byte_new((x) + (y))
#define mix_byte_sub(x, y) mix_byte_new((x) - (y))
#define mix_byte_mul(x, y) mix_byte_new((x) * (y))
#define mix_byte_div(x, y) mix_byte_new((x) / (y))

typedef uint8_t mix_char;

#define MIX_CHAR_MAX 63
#define mix_char_new(l) (((l) < MIX_CHAR_MAX)?(l):MIX_CHAR_MAX)
#define mix_char_to_byte(mc) mix_byte_new(mc)
#define mix_byte_to_char(mb) mix_char_new(mb)

extern mix_char ascii_to_mixcii(char c);
extern char mixcii_to_ascii(mix_char mc);

typedef int32_t mix_word;
#define MIX_WORD_MAX ((1 << 30) - 1)
#define MIX_WORD_SIGN_BIT (1 << 30)
#define MIX_WORD_ZERO ((mix_word)0)
#define MIX_WORD_NEG_ZERO (MIX_WORD_ZERO | MIX_WORD_SIGN_BIT)
#define mix_word_new(x) ((x) < 0? \
		(MIX_WORD_SIGN_BIT | (mix_word)(-(x) & MIX_WORD_MAX)):\
		(mix_word)(x & MIX_WORD_MAX))
#define mix_word_pack(b1, b2, b3, b4, b5) \
	((mix_word)(mix_byte_new(b5) | mix_byte_new(b4) << 6 \
			| mix_byte_new(b3) << 12 | mix_byte_new(b2) << 18\
			| mix_byte_new(b1) << 24))
#define mix_word_packn(b1, b2, b3, b4, b5) \
		((mix_word)(mix_byte_new(b5) | mix_byte_new(b4) << 6 \
				| mix_byte_new(b3) << 12 | mix_byte_new(b2) << 18\
				| mix_byte_new(b1) << 24 | MIX_WORD_SIGN_BIT))
extern mix_byte mix_word_get_byte(mix_word w, int idx);
extern void mix_word_set_byte(mix_word *w, int idx, mix_byte mb);
#define mix_word_negate(mw) ((mw) ^ MIX_WORD_SIGN_BIT)
#define mix_word_reverse_sign(mw) ((mw) ^= MIX_WORD_SIGN_BIT)
#define mix_word_sign(mw) ((mw) & MIX_WORD_SIGN_BIT)
#define mix_word_magnitude(mw) ((mw) & MIX_WORD_MAX)
#define mix_word_is_positive(mw) (!mix_word_sign(mw))
#define mix_word_is_negative(mw) (mix_word_sign(mw) >> 30)
extern mix_word mix_word_add(mix_word x, mix_word y);
#define mix_word_sub(x, y) (mix_word_add(x, mix_word_negate(y)))
extern bool mix_word_add_and_carry(mix_word x, mix_word y, mix_word *h,
		mix_word *l);
extern void mix_word_mul(mix_word x, mix_word y, mix_word *h, mix_word *l);
extern bool mix_word_div(mix_word nh, mix_word nl, mix_word d,
		mix_word *q, mix_word *r);
extern void mix_word_sl(mix_word A, mix_word X, unsigned long count,
		mix_word *pA, mix_word *pX);
extern void mix_word_sr(mix_word A, mix_word X, unsigned long count,
		mix_word *pA, mix_word *pX);
extern void mix_word_slc(mix_word A, mix_word X, unsigned long count,
		mix_word *pA, mix_word *pX);
#define mix_word_src(A, X, c, pA, pX) mix_word_slc(A, X, 10 - c % 10, pA, pX)

typedef uint8_t mix_fspec;
#define mix_fspec_left(f) ((f) >> 3 & 7)
#define mix_fspec_right(f) ((f) & 7)
#define mix_fspec_new(l, r) mix_byte_new((l) << 3 | (r))
extern bool fspec_is_valid(mix_fspec f);

extern mix_word mix_word_get_field(mix_fspec f, mix_word w);
extern mix_word mix_word_set_field(mix_fspec f, mix_word from, mix_word to);
#define mix_word_extract_field(f, w) mix_word_set_field(f, w, MIX_WORD_ZERO)
extern mix_word mix_word_store_field(mix_fspec f, mix_word from, mix_word to);
#define mix_word_print(w, m) mix_word_print_to_file(w, m, stdout)
extern void mix_word_print_to_file(mix_word w, const char *message, FILE *f);
extern void mix_word_print_to_buffer(mix_word w, char *buf);

typedef uint16_t mix_short;
#define MIX_SHORT_MAX ((1 << 12) - 1)
#define MIX_SHORT_SIGN_BIT ((mix_short)(1 << 12))
#define MIX_SHORT_ZERO ((mix_short)0)
#define MIX_SHORT_MINUS_ZERO (MIX_SHORT_ZERO | MIX_SHORT_SIGN_BIT)

#define mix_short_negate(ms) ((ms) ^ MIX_SHORT_SIGN_BIT)
#define mix_short_sign(ms) ((ms) & MIX_SHORT_SIGN_BIT)
#define mix_short_magnitude(ms) ((ms) & MIX_SHORT_MAX)
#define mix_short_is_negative(ms) (mix_short_sign(ms) >> 12)
#define mix_short_is_positive(ms) (!mix_short_sign(ms))
#define mix_short_reverse_sign(ms) ((ms) ^= MIX_SHORT_SIGN_BIT)
#define mix_short_new(val) \
	((val) >= 0? (val) & MIX_SHORT_MAX : mix_short_negate(-(val)))
#define mix_short_pack(b1, b2) \
	((mix_short)(mix_byte_new(b1) << 6 | mix_byte_new(b2)))
#define mix_short_packn(b1, b2) ((mix_short)(MIX_SHORT_SIGN_BIT \
		| mix_byte_new(b1) << 6 | mix_byte_new(b2)))
extern mix_short mix_word_to_short(mix_word w);
extern mix_word mix_short_to_word(mix_short ms);
#define mix_word_to_short_fast(w) (mix_word_is_negative(w)?\
		(w) & MIX_SHORT_MAX | MIX_SHORT_SIGN_BIT : (w) & MIX_SHORT_MAX)
#define mix_short_to_word_fast(ms) (mix_short_is_negative(ms)?\
		(mix_word)(mix_short_magnitude(ms)|MIX_WORD_SIGN_BIT) : (mix_word)(ms))
extern mix_short mix_short_add(mix_short x, mix_short y);
#define mix_short_sub(x, y) mix_short_add(x, mix_short_negate(y))


typedef enum {LESS, ZERO, GREATER, NCMP} mix_comp_ind;

extern mix_comp_ind compare(mix_word x, mix_word y);

#endif /* MIX_TYPES_H_ */
