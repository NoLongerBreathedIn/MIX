/*
 * lexparse.c
 *
 *  Created on: Sep 10, 2015
 *      Author: eyal
 */

#include "lexparse.h"
#include <search.h>
#include <string.h>
#include <stdlib.h>

static int varcmp(const void *a, const void *b) {
	return(strncmp(*(char **)a, *(char **)b, 10));
}

typedef enum {
	SYMBOL, INT, AST, PLUS, MINUS, SLASH, SLASHSLASH, COLON, OPEN, CLOSE, COMMA
} tok_label;

typedef struct {
	tok_label t;
	union {
		mix_word number;
		char symbol[10];
	} u;
} token;

typedef struct ex {
	token operation;
	token value;
	struct ex *next;
} expr;

typedef struct wp {
	expr *a;
	expr *f;
	struct wp *next;
} wpart;

token tok_list[80];


void free_expr(expr *e) {
	while(e != NULL) {
		expr *f = e->next;
		free(e);
		e = f;
	}
}

void free_wpart(wpart *w) {
	while(w != NULL) {
		wpart *x = w->next;
		free_expr(w->a);
		free_expr(w->f);
		free(w);
		w = x;
	}
}

mix_word eval_expr(expr *e, mix_short *fref, mix_short *bref, void *symtab,
		mix_short loc);
mix_word eval_wpart(wpart *w, mix_short *fref, mix_short *bref, void *symtab,
		mix_short loc);
int tokenize(char *c, int len);
expr *parse_expr(int stok, int ntok);
wpart *parse_wpart(int ntok);

mix_word compute_word(char *a, int nch, mix_short *fref, mix_short *bref,
		void *symtab, mix_short loc) {
	wpart *w = parse_wpart(tokenize(a, nch));
	mix_word r = eval_wpart(w, fref, bref, symtab, loc);
	free_wpart(w);
	return(r);
}

mix_word compute_expr(char *a, int nch, mix_short *fref, mix_short *bref,
		void *symtab, mix_short loc) {
	expr *e = parse_expr(0, tokenize(a, nch));
	mix_word r = eval_expr(e, fref, bref, symtab, loc);
	free_expr(e);
	return(r);
}

mix_word eval_wpart(wpart *w, mix_short *fref, mix_short *bref, void *symtab,
		mix_short loc) {
	mix_word cur = 0;
	while(w != NULL) {
		mix_word ares;
		mix_byte fres;
		ares = eval_expr(w->a, fref, bref, symtab, loc);
		fres = 5;
		if(w->f != NULL)
			fres = mix_word_get_byte(eval_expr(w->f, fref, bref, symtab, loc), 5);
		cur = mix_word_store_field(fres, ares, cur);
		w = w->next;
	}
	return(cur);
}

mix_word eval_expr(expr *e, mix_short *fref, mix_short *bref, void *symtab,
		mix_short loc) {
	mix_word cur = 0;
	while(e != NULL) {
		mix_word v;
		var sym;
		switch(e->value.t) {
		case INT:
			v = e->value.u.number;
			break;
		case SYMBOL:
			if(*e->value.u.symbol >= '0' && *e->value.u.symbol <= '9'
					&& !e->value.u.symbol[2]) {
				if(e->value.u.symbol[1] == 'B') {
					v = mix_short_to_word(bref[*e->value.u.symbol - '0']);
					break;
				} else if(e->value.u.symbol[1] == 'F') {
					v = mix_short_to_word(fref[*e->value.u.symbol - '0']);
					break;
				}
			}
			stpncpy(sym.label, e->value.u.symbol, 10);
			sym = **(var **)tfind(&sym, &symtab, varcmp);
			v = sym.res;
			break;
		case AST:
			v = mix_short_to_word(loc);
			break;
		default:
			exit(1);
		}
		switch(e->operation.t) {
		case PLUS:
			cur = mix_word_add(cur, v);
			break;
		case MINUS:
			cur = mix_word_sub(cur, v);
			break;
		case AST:
			mix_word_mul(cur, v, NULL, &cur);
			break;
		case SLASH:
			mix_word_div(mix_word_sign(cur), cur, v, &cur, NULL);
			break;
		case SLASHSLASH:
			mix_word_div(cur, 0, v, &cur, NULL);
			break;
		case COLON:
			mix_word_mul(cur, 8, NULL, &cur);
			cur = mix_word_add(cur, v);
			break;
		default:
			exit(1);
		}
	}
	return(cur);
}

int tokenize(char *c, int len) {
	int tkp = 0;
	while(len) {
		switch(*c) {
		case '/':
			if(len > 1 && c[1] == '/') {
				len -= 2;
				tok_list[tkp++].t = SLASHSLASH;
				c += 2;
			} else {
				len--;
				c++;
				tok_list[tkp++].t = SLASH;
			}
			break;
		case '+':
			tok_list[tkp++].t = PLUS;
			break;
		case '-':
			tok_list[tkp++].t = MINUS;
			break;
		case '*':
			tok_list[tkp++].t = AST;
			break;
		case ':':
			tok_list[tkp++].t = COLON;
			break;
		case ',':
			tok_list[tkp++].t = COMMA;
			break;
		case '(':
			tok_list[tkp++].t = OPEN;
			break;
		case ')':
			tok_list[tkp++].t = CLOSE;
			break;
		default:
			if(*c == ' ' || !*c) {
				len = 0;
				break;
			}
			tok_list[tkp].t = SYMBOL;
			if((*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9')) {
				int i, t = 1;
				for(i = 0; i < 10; i++)
					tok_list[tkp].u.symbol[i] = 0;
				for(i = 0; i < 10; i++, c++, len--) {
					if((*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9')) {
						if(*c > '9')
							t = 0;
						tok_list[tkp].u.symbol[i] = *c;
					}
				}
				if(t) {
					tok_list[tkp].t = INT;
					for(i = t = 0; i < 10 && tok_list[tkp].u.symbol[i]; i++)
						t = t * 10 + tok_list[tkp].u.symbol[i] - '0';
					tok_list[tkp].u.number = t;
				}
			} else
				exit(1);
			tkp++;
		}
	}
	return(tkp);
}

wpart *parse_wpart(int ntok) {
	int i, curtok = 0;
	wpart *r, *w;
	if(ntok == 0)
		return(NULL);
	r = w = calloc(1, sizeof(wpart));
	while(curtok < ntok) {
		for(i = curtok; i < ntok && tok_list[i].t != COMMA
					&& tok_list[i].t != OPEN; i++);
		i -= curtok;
		w->a = parse_expr(curtok, i);
		curtok += i;
		w->f = NULL;
		if(curtok != ntok && tok_list[curtok++].t == OPEN) {
			for(i = curtok; i < ntok && tok_list[i].t != CLOSE; i++);
			i -= curtok;
			w->f = parse_expr(curtok, i);
			curtok += i + 2;
		}
		if(curtok < ntok)
			w = w->next = calloc(1, sizeof(wpart));
	}
	return(r);
}

expr *parse_expr(int stok, int ntok) {
	int i = 1;
	expr *r, *ex;
	if(ntok == 0)
		return(NULL);
	r = ex = calloc(1, sizeof(expr));
	ex->operation.t = PLUS;
	ex->value.t = INT;
	ex->value.u.number = 0;
	switch(tok_list[stok].t) {
	case PLUS: case MINUS:
		ex->operation.t = tok_list[stok].t;
		ex->value = tok_list[stok + 1];
		i = 2;
		break;
	default:
		ex->value = tok_list[stok];
	}
	for(; i < ntok - 1; i += 2) {
		ex = ex->next = calloc(1, sizeof(expr));
		ex->operation.t = tok_list[stok + i].t;
		ex->value = tok_list[stok + i];
	}
	return(r);
}
