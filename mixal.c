/*
 * mixal.c
 *
 *  Created on: Sep 7, 2015
 *      Author: eyal
 */

#include "mix_encode.h"
#include "mix_types.h"
#include "lexparse.h"
#include <search.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mix_flotio.h"

typedef struct {
	char line[81];
	mix_word inst;
	mix_word instb;
	mix_short memloc;
	instruction i;
} line;

typedef struct lh {
	line chunk[128];
	struct lh *next;
} line_hunk;

int readin(line_hunk *in); //Returns last linehunk; ignores comment lines.

bool readline(char *line);

static int varcmp(const void *a, const void *b) {
	return(strncmp(*(char **)a, *(char **)b, 10));
}

mix_short getlocforward(line_hunk *lh, int which, int off);

void print_preamble(void);
void print_hunk(line_hunk *lh);
void print_end(mix_short posn);

mix_short brefs[10], frefs[10];
void *label_tree = NULL;

int main(int argc, char **argv) {
	line_hunk input;
	line_hunk end;
	int lines = readin(&input);
	line *l, *e;
	int i, j, k;
	line_hunk *curi = &input, *cure = &end;
	mix_short curlit, endloc;
	char foo[5];
	mix_init_types();
	foo[4] = 0;
	mix_short location = 100;
	// Handle line numbers. In ORIG and EQU lines, no forward refs allowed
	for(i = 0; i < lines; i++) {
		var *v = calloc(1, sizeof(var));
		char *bar;
		if(i & 127)
			l++;
		else {
			l = curi->chunk;
			curi = curi->next;
		}
		for(j = 0; j < 4; j++)
			if((foo[j] = l->line[11 + j]) == ' ')
				foo[j] = 0;
		bar = bsearch(foo, &instr_name, nnames, 4, varcmp);
		l->i = instr_table[(bar - (char *)instr_name)/4];
		for(j = 0; j < 10; j++)
			if((v->label[j] = l->line[j]) == ' ')
				v->label[j] = 0;
		l->memloc = location;
		switch(l->i) {
		case END:
			lines = i;
			curlit = location;
			endloc = mix_word_to_short(compute_word(l->line + 16, 64,
					frefs, brefs, label_tree, location));
			free(v);
			break;
		case ORIG:
			v->res = mix_short_to_word(location);
			location = mix_word_to_short(compute_word(l->line + 16, 64,
					frefs, brefs, label_tree, location));
			goto storesym;
		case EQU:
			v->res = compute_word(l->line + 16, 64, frefs, brefs, label_tree, location);
			goto storesym;
		case LONG: case MIX_DBL:
			v->res = location;
			location += 2;
			goto storesym;
		default:
			v->res = mix_short_to_word(location++);
		storesym:
			if(*v->label >= '0' && *v->label <= '9' && v->label[1] == 'H') {
				brefs[*v->label - '0'] = mix_word_to_short(v->res);
				free(v);
			} else if(*v->label)
				tsearch(v, &label_tree, varcmp);
			else
				free(v);
		}
	}
	curi = &input;
	for(i = 0; i < 10; i++)
		frefs[i] = getlocforward(curi, i, 0);
	for(i = 0, j = 0, e = cure->chunk, e->i = END; i < lines; i++) {
		mix_fspec f;
		mix_byte ind = 0;
		mix_short m = 0;
		mix_byte op = 0;
		char *looking;
		bool sign = false;
		uint64_t longhold = 0;
		if(i & 127)
			l++;
		else
			l = curi->chunk;
		if(*l->line >= '0' && *l->line <= '9' && l->line[1] == 'H')
			frefs[*l->line - '0'] = getlocforward(curi, *l->line - '0',
					i & 127);
		if(~(i & 127))
			curi = curi->next;
		switch(l->i) {
		case EQU: case ORIG: case END: break;
		case CON:
			l->inst = compute_word(l->line + 16, 64, frefs, brefs, label_tree, l->memloc);
			break;
		case LONG:
			if(*(looking = l->line + 16) == '+' || *looking == '-')
				sign = *looking++ == '-';
			while(*looking >= '0' && *looking <= '9')
				longhold = longhold * 10 + *looking++ - '0';
			split_double((long)sign << 63 | longhold, &l->inst, &l->instb);
			break;
		case FLOT:
			l->inst = shorten(read_float(l->line + 16));
			break;
		case MIX_DBL:
			split_double(read_float(l->line + 16), &l->inst, &l->instb);
			break;
		case ALF:
			for(k = 0; k < 5; k++)
				l->inst = l->inst << 6 | ascii_to_mixcii(l->line[16 + k]);
			break;
		default:
			f = default_f[l->i];
			op = opcode[l->i];
			if(l->line[16] != ' ' && l->line[16]) {
				int n, k;
				if(l->line[16] == '=') {
					for(k = 0; l->line[17 + k] != '='; k++);
					m = e->memloc = curlit++;
					e->i = CON;
					e->inst = compute_word(l->line + 17, k, frefs, brefs, label_tree,
							l->memloc);
					if(j++ & 127)
						e++;
					else {
						e = cure->chunk;
						cure = cure->next = calloc(1, sizeof(line_hunk));
					}
					e->i = END;
					n = 18 + k;
				} else {
					for(k = 0; l->line[16 + k] != ','
							&& l->line[16 + k] != '('
							&& l->line[16 + k] != ' ' && l->line[16 + k]; k++);
					m = mix_word_to_short(compute_expr(l->line + 16, k, frefs,
							brefs, label_tree, l->memloc));
					n = 16 + k;
				}
				if(l->line[n] == ',') {
					n++;
					for(k = 0; l->line[n + k] != '('
							&& l->line[n + k] != ' ' && l->line[n + k]; k++);
					ind = mix_word_get_byte(compute_expr(l->line + n, k, frefs,
							brefs, label_tree, l->memloc), 5);
					n += k;
				}
				if(l->line[n] == '(') {
					n++;
					for(k = 0; l->line[n + k] != ')'
							&& l->line[n + k] != ' ' && l->line[n + k]; k++);
					f = mix_word_get_byte(compute_expr(l->line + n, k, frefs,
							brefs, label_tree, l->memloc), 5);
				}
			}
			l->inst = m << 18 | ind << 12 | f << 6 | op;
		}
		if(*l->line >= '0' && *l->line <= '9' && l->line[1] == 'H')
			brefs[*l->line - '0'] = l->memloc;
	}
	print_preamble();
	print_hunk(&input);
	print_hunk(&end);
	print_end(endloc);
}

mix_short getlocforward(line_hunk *lh, int which, int off) {
	int i;
	line *l = lh->chunk + off;
	for(i = off; l->i != END; i++) {
		if(*l->line == which + '0' && l->line[1] == 'H')
			return(l->memloc);
		if(i + 1 & 127)
			l++;
		else {
			lh = lh->next;
			l = lh->chunk;
		}
	}
	return(0);
}

int readin(line_hunk *in) {
	int i = 0;
	line *l = in->chunk;
	while(!feof(stdin))
		if(readline(l->line)) {
			if(++i & 127)
				l++;
			else
				l = (in = in->next = calloc(1, sizeof(line_hunk)))->chunk;
		}
	return(i);
}

bool readline(char *line) {
	int i, j;
	memset(line, 0, 81);
	fgets(line, 81, stdin);
	i = strlen(line);
	if(line[i - 1] == '\n')
		line[--i] = 0;
	else
		while((j = getchar()) != EOF && j != '\n');
	if(*line == '*' || !*line)
		return(false);
	for(j = 0; j < i; j++)
		line[j] = mixcii_to_ascii(ascii_to_mixcii(line[i]));
	return(true);
}

void flush(mix_word *lump, int nwords, mix_short locstart);

void print_hunk(line_hunk *lh) {
	mix_word lump[8];
	int i = 0;
	int pos = 0;
	mix_short locx = lh->chunk->memloc;
	mix_short locstart;
	while(true) {
		if(lh->chunk[i].memloc != locx) {
			flush(lump, pos, locstart);
			locstart = lh->chunk[i].memloc;
			locx = locstart + 1;
			pos = 0;
		}
		if(lh->chunk[i].i == END)
			break;
		lump[pos++] = lh->chunk[i].inst;
		if(lh->chunk[i].i == MIX_DBL || lh->chunk[i].i == LONG) {
			lump[pos++] = lh->chunk[i].instb;
			locx++;
		}
		if(pos >= 7) {
			flush(lump, 7, locstart);
			pos -= 7;
			locstart += 7;
			if(pos)
				lump[0] = lump[7];
		}
		i++;
		if((i %= 128) == 0)
			lh = lh->next;
	}
}

void flush(mix_word *lump, int nwords, mix_short locstart) {
	int i;
	if(nwords) {
		printf("     %c%04d", nwords + '0', locstart);
		for(i = 0; i < nwords; i++) {
			if(mix_word_is_positive(lump[i]))
				printf("%010d", lump[i]);
			else {
				lump[i] = mix_word_magnitude(lump[i]);
				printf("%09d%c", lump[i] / 10,
						mixcii_to_ascii(ascii_to_mixcii(lump[i] % 10) - 20));
			}
		}
		putchar('\n');
	}
}

void print_end(mix_short posn) {
	printf("     0%04d\n", posn);
}

void print_preamble(void) {
	char *instr =
			" O O6"   // IN   16(16)
			" Y O6"   // IN   28(16)
			"    I"   // LD1  0(0:0)
			"   B="   // ENTA 0
			" D O4"   // JBUS 4(16)
			" Z IQ"   // LD2N 29(1:1)
			" Z I3"   // STZ  29(1:1)
			" Z EN"   // LDX  29
			"    E"   // NUM
			"   EU"   // STA  0
			" 0BB="   // ENTA 30,2
			" H IU"   // STA  8(1:1)
			"   EJ"   // LD3  0
			"  CA."   // JAZ  0,3
			" ACB="   // ENTA 1,3
			"   EU\n" // STA  0 (end of first card)
			" 1A-H"   // LDA  31,1(5:5)
			" V A="   // DECA 25
			"  CEU"   // STA  0,3
			" 0AEH"   // LDA  30,1
			" 1AEN"   // LDX  31,1
			"    E"   // NUM
			"  CLU"   // STA  0,3(1:5)
			"  ABG"   // MOV  0,1(2)
			" H IH"   // LDA  8(1:1)
			" A A="   // DECA 1
			" J B."   // JAP  11
			" A  9";  // JMP  1
	printf("%s\n", instr);
}
