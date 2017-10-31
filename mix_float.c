/*
 * mix_float.c
 *
 *  Created on: Aug 18, 2015
 *      Author: eyal
 */

#include "mix_float.h"
#include <stdlib.h>

uint64_t aux;
bool oflow;

uint64_t omult(uint64_t y, uint64_t z) {
	uint64_t yl, yh, zl, zh, rl, rm, rh;
	yh = y >> 32;
	yl = y & 0xffffffff;
	zh = z >> 32;
	zl = z & 0xffffffff;
	rh = yh * zh;
	rl = yl * zl;
	rm = (yh + yl) * (zh + zl) - rl - rh;
	rh += rm >> 32;
	rm &= 0xffffffff;
	if(rm & 1L << 32)
		rh++;
	rl &= 0xffffffff;
	rl |= rm << 32;
	aux = rh;
	return(rl);
}

uint64_t odiv(uint64_t y, uint64_t z) {
	uint64_t u[4], v[2], q[2];
	int j;
	u[0] = y >> 32;
	u[1] = y & (1L << 32) - 1;
	u[2] = u[3] = 0;
	v[0] = z >> 32;
	v[1] = z & (1L << 32) - 1;
	for(j = 0; j < 2; j++) {
		uint64_t qhat, temp, t2, vq[3];
		bool neg;
		int k;
		if(u[j] == v[0])
			qhat = (1L << 32) - 1;
		else
			qhat = (u[j] << 32 | u[j + 1]) / v[0];
		temp = v[1] * qhat;
		t2 = (u[j] << 32) + u[j + 1] - qhat * v[0];
		while(temp >> 32 > t2 ||
				(temp >> 32 == t2 && (temp & (1L << 32) - 1) < u[j + 2])) {
			qhat--;
			t2 -= v[0];
			temp -= v[1];
		}
		vq[1] = v[0] * qhat;
		vq[2] = v[1] * qhat;
		vq[1] += vq[2] >> 32;
		vq[0] = vq[1] >> 32;
		vq[1] &= (1L << 32) - 1;
		vq[2] &= (1L << 32) - 1;
		for(k = 0; k < 3; k++)
			u[j + k] -= vq[k];
		for(k = 2; k >= 0; k--) {
			neg = u[j + k] >> 63;
			if(k) u[j + k - 1] += u[j + k] >> 32;
			u[j + k] &= (1L << 32) - 1;
		}
		if(neg) {
			qhat--;
			u[j + 2] += v[1];
			u[j + 1] += v[0] + (u[j + 2] >> 32);
			u[j + 2] &= (1L << 32) - 1;
			u[j] += u[j + 1] >> 32;
			u[j + 1] &= (1L << 32) - 1;
			u[j] &= (1L << 32) - 1;
		}
		q[j] = qhat;
	}
	aux = u[2] << 32 | u[3];
	return(q[0] << 32 | q[1]);
}

#define INF_DOUBLE (07777000000L << 32)
#define INF_SINGLE 07700000000
#define STANDARD_NAN (07777400000L << 32)

/* OK. The format is as follows for all:
 * +---+---+---+---+---+
 * | a |       b       |
 * +---+---+---+---+---+
 * represents (b / 64^4 + 1) * 2^(a-31)
 * unless a=0, in which case it's b/64^4 * 2^-30,
 * or a = 63, in which case it's infinity/NaN.
 * +---+---+---+---+---+
 * |   a   |           |
 * +---+---+   b       +
 * |                   |
 * +---+---+---+---+---+
 * represents (b / 64^8 + 1) * 2^(a-2047)
 * unless a=0, in which case it's b/64^8 * 2^-2046,
 * or a=4095, in which case it's infinity/NaN.
 *
 * The inputs to dpack are f, e.
 * 2^50 <= f <= 2^51;
 * The output is f * 2^(e-2096).
 */

mix_word fpack(uint64_t f, int e, bool s);

mix_double dpack(uint64_t f, int e, bool s);

#define zero_exp (-1000)

typedef enum {
	zro, inf, nan, num
} ftype;

ftype funpack(mix_word x, uint64_t *f, int *e, bool *s);

ftype dunpack(mix_double x, uint64_t *f, int *e, bool *s);

#define NANCASES case 4 * nan + nan: if(!(y & 0400000L << 30)) oflow = true; \
  case 4 * zro + nan: case 4 * num + nan: case 4 * inf + nan:\
    if(!(z & 0400000L << 30)) oflow = true; return(z | 0400000L << 30);\
  case 4 * nan + zro: case 4 * nan + num: case 4 * nan + inf:\
    if(!(y & 0400000L << 30)) oflow = true; return(y | 0400000L << 30);

void split_double(mix_double d, mix_word *h, mix_word *l) {
	*h = (mix_word)(d >> 32);
	*l = (mix_word)(d & MIX_WORD_MAX);
}

mix_double fmul(mix_double y, mix_double z) {
	int xe, ye, ze;
	bool ys, zs;
	uint64_t xf, yf, zf;
	mix_double x;
	bool xs;
	ftype yt = dunpack(y, &yf, &ye, &ys);
	ftype zt = dunpack(z, &zf, &ze, &zs);
	xs = ys ^ zs;
	switch(4 * yt + zt) {
 	NANCASES
	case 4 * zro + zro: case 4 * zro + num: case 4 * num + zro: x = 0; break;
	case 4 * inf + inf: case 4 * inf + num: case 4 * num + inf:
	    x = INF_DOUBLE; break;
	case 4 * zro + inf: case 4 * inf + zro: x = STANDARD_NAN; break;
	case 4 * num + num:
	    xe = ye + ze - 03775;
	    x = omult(yf, zf << 13); // so now aux = yf * zf * 2^-51
	    if(aux >= 1L << 50) // and xe = ye + ze - 2045.
	    	xf = aux; // So x = yf * zf * 2^(ye + ze - 2096 - 2096)
	    else {
	    	xf = aux << 1;
	    	xe--;
	    }
	    if(x)
	    	xf |= 1;
	    return(dpack(xf, xe, xs));
	}
	if(xs)
		x |= 1L << 62 | 1 << 30;
	return(x);
}

mix_double fdiv(mix_double y, mix_double z) {
	bool ys, zs, xs;
	int ye, ze, xe;
	uint64_t xf, yf, zf;
	mix_double x;
	ftype yt = dunpack(y, &yf, &ye, &ys);
	ftype zt = dunpack(z, &zf, &ze, &zs);
	xs = ys ^ zs;
	switch(4 * yt + zt) {
	NANCASES
	case 4 * zro + inf: case 4 * zro + num: case 4 * num + inf: x = 0; break;
	case 4 * num + zro: oflow = true;
	// no break
	case 4 * inf + num: case 4 * inf + zro: x = INF_DOUBLE; break;
	case 4 * zro + zro: case 4 * inf + inf:
	    x = STANDARD_NAN; break;
	case 4 * num + num:
	    xe = ye - ze + 07775;
	    xf = odiv(yf, zf << 13); // so now xf = yf/zf * 2^51,
	    if(xf >= 1L << 51) { // and xe = ye - ze + 2045.
	    	aux |= xf & 1; // So x = yf/zf * 2^(ye - ze + 2096 - 2096)
	    	xf >>= 1;
	    	xe++;
	    }
	    if(aux) xf |= 1;
	    return(dpack(xf, xe, xs));
	}
	if(xs)
		x |= 1L << 62 | 1 << 30;
	return(x);
}

mix_double fadd(mix_double y, mix_double z) {
	bool ys, zs, xs;
	int ye, ze, xe;
	uint64_t xf, yf, zf;
	mix_double x;
	ftype yt = dunpack(y, &yf, &ye, &ys);
	ftype zt = dunpack(z, &zf, &ze, &zs);
	switch(4 * yt + zt) {
	NANCASES
	case 4 * zro + num: return(dpack(zf, ze, zs));
	case 4 * num + zro: return(dpack(yf, ye, ys));
	case 4 * inf + inf:
		if(ys ^ zs) {
			oflow = true;
			x = STANDARD_NAN;
			xs = zs;
			break;
		}
		/* no break */
	case 4 * num + inf: case 4 * zro + inf: x = INF_DOUBLE; xs = zs; break;
	case 4 * inf + num: case 4 * inf + zro: x = INF_DOUBLE; xs = ys; break;
	case 4 * num + num:
		if(y ^ z ^ ((uint64_t)MIX_WORD_SIGN_BIT << 32)) {
			uint64_t o, oo;
			if(ye < ze || (ye == ze && yf < zf)) {
				ye ^= ze; ze ^= ye; ye ^= ze;
				yf ^= zf; zf ^= yf; yf ^= zf;
				ys ^= zs; zs ^= ys; ys ^= zs;
			}
			int d = ye - ze;
			xs = ys;
			xe = ye;
			if(d) {
				if(d <= 2)
					zf >>= d;
				else if(d >= 49)
					zf = 1;
				else {
					if(ys ^= zs) {
						d--;
						xe--;
						yf <<= 1;
					}
					o = zf;
					zf >>= d;
					oo = zf << d;
					if(o != oo)
						zf |= 1;
				}
			}
			if(ys == zs) {
				xf = yf + zf;
				if(xf >= 1L << 51) {
					xe++;
					d = xf & 1;
					xf >>= 1;
					xf |= d;
				}
			} else {
				xf = yf - zf;
				if(xf >= 1L << 51) {
					xe++;
					d = xf & 1;
					xf >>= 1;
					xf |= d;
				} else while(xf < 1L << 50) {
					xe--;
					xf <<= 1;
				}
			}
			return(dpack(xf, xe, xs));
		}
		/* no break */
	case 4 * zro + zro: x = 0; xs = ys && zs;
	}
	if(xs)
		x |= 1L << 62 | 1 << 30;
	return(x);
}

mix_comp_ind fcmp(mix_double y, mix_double z) {
	bool ys, zs;
	int ye, ze;
	uint64_t yf, zf;
	ftype yt = dunpack(y, &yf, &ye, &ys);
	ftype zt = dunpack(z, &zf, &ze, &zs);
	switch(4 * yt + zt) {
	case 4 * nan + num: case 4 * nan + inf: case 4 * nan + zro:
	case 4 * num + nan: case 4 * inf + nan: case 4 * zro + nan:
	case 4 * nan + nan: return(NCMP);
	case 4 * zro + zro: return(ZERO);
	case 4 * inf + num: case 4 * inf + zro: return(ys? LESS : GREATER);
	case 4 * num + inf: case 4 * num + zro: return(zs? GREATER : LESS);
	case 4 * inf + inf: return(ys ^ zs? ys? LESS : GREATER : ZERO);
	case 4 * num + num:
		if(ys ^ zs) return(ys? LESS : GREATER);
		if(ye == ze && yf == zf) return(ZERO);
		return(ys ^ (ye < ze || (ye == ze && yf < zf))? LESS : GREATER);
	}
	fprintf(stderr, "Can\'t happen: unpack returned non-ftype.");
	exit(1);
}

mix_double integerize(mix_double x) {
	bool xs;
	uint64_t xf, rf;
	int xe;
	ftype xt = dunpack(x, &xf, &xe, &xs);
	if(xt == nan && !(x & 0400000L << 32)) {
		oflow = true;
		x |= 0400000L << 32;
	}
	if(xt != num)
		return(x);
	if(xe > 2094) // 2094 - 2096 = -2; the low-order two bits of xf are 0.
		return(dpack(xf, xe, xs));
	if(xe <= 2044) //2096 - 2044 = 52, so x <= .5
		rf = 1;
	else {
		uint64_t oo;
		rf = xf >> (2094 - xe);
		oo = rf << (2094 - xe);
		if(oo != xf)
			rf |= 1;
	}
	rf += rf & 4? 2 : 1;
	rf &= ~(uint64_t)3;
	if(xe >= 2046)
		return(fpack(rf << (2094 - xe), xe, xs));
	if(rf & 07777777777)
		rf = 03777000000L << 32;
	if(xs)
		rf |= 1L << 62 | 1 << 30;
	return(rf);
}

mix_double munge(uint64_t sm);
uint64_t unmunge(mix_double md);

mix_double fix(mix_double x) {
	int xe;
	bool xs;
	uint64_t xf, o;
	ftype xt = dunpack(x, &xf, &xe, &xs);
	if(xt == zro) {
		o = 0;
		goto END;
	}
	if(xt != num) {
		oflow = true;
		return(x);
	}
	if(dunpack(integerize(x), &xf, &xe, &xs) == zro) {
		o = 0;
		goto END;
	}
	if(xe <= 2096)
		o = xf >> (2096 - xe); // we need to shift right
	else {
		if(xe > 2109 || (xe == 2109 && xf >= 1L << 48))
			oflow = true; // we need to shift left
		if(xe > 2158) // 2156 - 2096 = 62, so all bits will be 0
			o = 0;
		else
			o = xf << (xe - 2096); // else shift left
	}
	END:
	if(xs)
		o ^= 1L << 63;
	return(munge(o));
}

mix_double flot(mix_double x) {
	int e, t;
	bool s;
	uint64_t xv;
	xv = unmunge(x);
	s = xv >> 63;
	xv &= ~(uint64_t)0 >> 2;
	if(!xv)
		return(x | x >> 32);
	e = 2096;
	while(xv < 1L << 50) {
		e--;
		xv <<= 1;
	}
	while(xv > 1L << 51) {
		e++;
		t = xv & 1;
		xv >>= 1;
		xv |= t;
	}
	return(dpack(xv, e, s));
}

mix_word shorten(mix_double x) {
	uint64_t f;
	int e;
	bool s;
	mix_word r;
	ftype t = dunpack(x, &f, &e, &s);
	switch(t) {
	case zro: r = 0; break;
	case num: return(fpack(f, e, s));
	case inf: r = INF_SINGLE; break;
	case nan:
		if(!(f & 1L << 49)) {
			f |= 1L << 49;
			oflow = true;
		}
		r = 07700000000 | (f >> 24 & 077777777);
	}
	if(s)
		mix_word_reverse_sign(r);
	return(r);
}

mix_double lengthen(mix_word x) {
	uint64_t f, r;
	int e;
	bool s;
	ftype t = funpack(x, &f, &e, &s);
	switch(t) {
	case zro: r = 0; break;
	case num: return(dpack(f, e, s));
	case inf: r = INF_DOUBLE; break;
	case nan: r = munge((0770000000000L | x) << 18);
	}
	if(s)
		r |= 1L << 62 | 1 << 30;
	return(r);
}

mix_double dpack(uint64_t f, int e, bool s) {
	uint64_t o;
	if(e > 07775) {
		e = 07777;
		o = 0;
	} else {
		if(e < 0) {
			if(e < -50)
				o = 1;
			else {
				uint64_t oo;
				o = f >> -e;
				oo = o << -e;
				if(oo != f)
					o |= 1;
			}
			e = 0;
		} else
			o = f;
	}
	o += o & 4? 2 : 1;
	o >>= 2;
	o += (uint64_t)e << 48;
	if(o >= 07777L << 48)
		oflow = true;
	if(s)
		o |= 1L << 63;
	return(munge(o));
}

mix_word fpack(uint64_t f, int e, bool s) {
	mix_word o;
	if(e > 04035) {
		e = 04037;
		o = 0;
	} else {
		o = f >> 24;
		if(f & (1 << 24) - 1)
			o |= 1;
		if(e < 03740) {
			if(e < 03740 - 26)
				o = 1;
			else {
				mix_word o0, oo;
				o0 = o;
				o >>= 03740 - e;
				oo = o << (03740 - e);
				if(o0 != oo)
					o |= 1;
			}
			e = 03740;
		}
	}
	o += o & 4? 2 : 1;
	o >>= 2;
	o += (e - 03740) << 24;
	if(o >= 07700000000)
		oflow = true;
	if(s)
		mix_word_reverse_sign(o);
	return(o);
}

ftype dunpack(mix_double x, uint64_t *f, int *e, bool *s) {
	uint64_t y = unmunge(x);
	int ee;
	*s = y >> 63;
	y &= ~(uint64_t)0 >> 2;
	*f = y << 2 & (1L << 48) - 1;
	ee = y >> 48;
	if(ee) {
		*e = ee - 1;
		*f |= 1L << 50;
		return(ee < 07777? num : *f == 1L << 50? inf : nan);
	}
	if(!*f) {
		*e = zero_exp;
		return(zro);
	}
	do {
		ee--;
		*f <<= 1;
	} while(!(*f >> 50));
	*e = ee;
	return(num);
}

ftype funpack(mix_word x, uint64_t *f, int *e, bool *s) {
	int ee;
	*s = mix_word_is_negative(x);
	*f = (uint64_t)(x & 077777777) << 24;
	ee = x >> 24 & 077;
	if(ee) {
		*e = ee + 03740 - 1;
		*f |= 1L << 50;
		return(ee < 077? num :
				mix_word_magnitude(x) == 07700000000? inf : nan);
	}
	if(!mix_word_magnitude(x)) {
		*e = zero_exp;
		return(zro);
	}
	do {
		ee--;
		*f <<= 1;
	} while(!(*f & 1L << 50));
	*e = ee + 03740;
	return(num);
}

mix_double munge(uint64_t sm) {
	mix_word s = mix_word_sign(sm >> 33);
	mix_word higher = sm >> 30 & 07777777777;
	mix_word lower = sm & 07777777777;
	higher ^= s;
	lower ^= s;
	return(combine_to_double(higher, lower));
}

uint64_t unmunge(mix_double md) {
	mix_word h, l;
	bool s;
	split_double(md, &h, &l);
	s = mix_word_is_negative(h);
	h &= MIX_WORD_MAX;
	l &= MIX_WORD_MAX;
	return((uint64_t)s << 63 | (uint64_t)h << 30 | l);
}
