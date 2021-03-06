#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "emulator.h"

struct Command command_list[] = {
	{0xFFFF, 0, "HALT", do_halt, NO_PARAM},
	{0170000, 0010000, "MOV", do_mov, HAS_SS | HAS_DD},
	{0170000, 0060000, "ADD", do_add, HAS_SS | HAS_DD},
	{0177000, 0077000, "SOB", do_sob, HAS_R | HAS_NN}, 
	{0177700, 0005000, "CLR", do_clr, HAS_DD},
	{0170000, 0110000, "MOVB", do_movb, HAS_SS | HAS_DD | IS_BYTE_COM},
	{0177700, 0001400, "BEQ", do_beq, HAS_XX},
	{0xFF00, 0000400, "BR", do_br, HAS_XX},
	{0177700, 0105700, "TSTB", do_tstb, HAS_DD | IS_BYTE_COM},
	{0177000, 0100000, "BPL", do_bpl, HAS_XX},
	{0177000, 0004000, "JSR", do_jsr, HAS_DD | HAS_R},
	{0177770, 0000200, "RTS", do_rts, HAS_R},
	{0170000, 0040000, "BIC", do_bic, HAS_SS | HAS_DD},
	{0177700, 0006200, "ASR", do_asr, HAS_DD},
	{0177700, 0105200, "INCB", do_incb, HAS_DD},
	{0177700, 0005200, "INC", do_inc, HAS_DD},
	{0177000, 0072000, "ASH", do_ash, HAS_DD | HAS_R},
	{0177700, 0005700, "TST", do_tst, HAS_DD},
	{0, 0, "UNKNOWN", do_unknown, NO_PARAM}
};

int main(int argc, char **argv)
{
	FILE *f;
	char * filename = "vos.o";
	if(argc != 1) {
		filename = argv[1];
	}
	f = fopen(filename, "r");
	if(f == NULL) {
		perror(filename);
		exit(1);
	}
	load_file(f);
	//mem_dump(01000, 20);
	run_programm();
	fclose(f);
	return 0;
}
//======================================================================


//======================================================================
byte b_read(adr a) {
	return mem[a];
}
void b_write(adr a, byte val) {
	mem[a] = val;
}
//======================================================================
word w_read(adr a) {
	byte x = b_read(a + 1);
	byte y = b_read(a);
	return ((word)x << 8) + y;
}
void w_write(adr a, word val) {
	b_write(a + 1, (byte)(val >> 8));
	b_write(a, (byte)val);
}
//======================================================================
void reg_dump() {
	printf("\n");
	for(int i = 0; i < 8; i++)
		printf("R%d = %o  ", i, reg[i]);
}
//======================================================================
struct SS_DD get_mr(word w) {
	struct SS_DD res = {};
	int n = w & 7;
	int mode = (w >> 3) & 7;
	switch(mode) {
		case 0:						//R1
			res.arg_type = ARG_REG;
			res.a = n;
			res.val = reg[n];
			printf("R%d ", n);
			break;
		case 1:						//(R1)
			res.arg_type = ARG_MEM;
			res.a = reg[n];
			res.val = w_read(res.a);
			printf("(R%d) ", n);
			break;
		case 2:						//(R1)+
			res.arg_type = ARG_MEM;
			res.a = reg[n];
			res.val = w_read(res.a);
			if(IsByteCommand && n != 6 && n != 7)
				reg[n]++;  // TODO +1
			else
				reg[n] += 2;
			if (n == 7)
				printf("#%o ", res.val);
			else
				printf("(R%d)+ ", n);
			
			break;
		case 3:						//@(R1)+
			res.arg_type = ARG_MEM;
			res.a = w_read(reg[n]);
			res.val = w_read(res.a);
			if(IsByteCommand && n != 6 && n != 7)
				reg[n]++;
			else
				reg[n] += 2;  
			if (n == 7)
				printf("@#%o ", res.a);
			else
				printf("@(R%d)+ ", n);
			
			break;
		case 4:						//-(R)
			res.arg_type = ARG_MEM;
			if(IsByteCommand && n != 6 && n != 7)
				reg[n]--;
			else
				reg[n] -= 2;
			res.a = reg[n];
			res.val = w_read(res.a);
			printf("-(R%d) ", n);
			break;
		case 6:						//nn(R)
			res.arg_type = ARG_MEM;
			if(n != 7) {
				res.a = reg[n] + w_read(pc);
				res.val = w_read(res.a);
				printf("%o(R%d) ", w_read(pc), n);
				pc += 2;
			}
			else {
				res.a = pc + 2 + w_read(pc);
				pc += 2;
				res.val = w_read(res.a);
				printf("%o ", res.a);
			}
			break;
		default :
			printf("Not implemented yet mode %d\n", mode);
			exit(1);
			break;
	}
	return res;
}
//======================================================================
void do_mov() {
	if(dd.arg_type == ARG_REG)
		reg[dd.a] =  ss.val;
	else
		w_write(dd.a, ss.val);
	N = 0;
	Z = 0;
	if(ss.val == 0)
		Z = 1;
	if(ss.val < 0)
		N = 1;
}
void do_movb() {
	byte b = 0;
	if(ss.arg_type == ARG_REG)
		b = reg[ss.a] & 0xFF;
	else
		b = b_read(ss.a);

	if(dd.arg_type == ARG_REG) {
		reg[dd.a] = b;
		if(b >> 7)
			reg[dd.a] = reg[dd.a] | 0xFF00;
		else
			reg[dd.a] = reg[dd.a] & 0x00FF;
	}
	else
		b_write(dd.a, b);

	Z = 0;
	N = 0;
	if(b < 0)
		N = 1;
	if(b == 0)
		Z = 1;

	if(dd.a == 0177566)
		fprintf(stderr, "%c", ss.val);
}
void do_add() {
	word res = dd.val + ss.val;
	if(dd.arg_type == ARG_REG) {
		reg[dd.a] = res;
		printf("   R%d = %06o", dd.a, reg[dd.a]);
	}
	else
		w_write(dd.a, res);

	Z = 0;
	N = 0;
	if(res < 0)
		N = 1;
	if(res == 0)
		Z = 1;
}
void do_halt() {
	printf("\nHALTED\n");
	for(int i = 0; i < 7; i++)
		printf("R%d = %o\n", i, reg[i]);
	printf("pc = %d\n", pc);
	exit(0);
}
void do_sob() {
	printf("%06o", pc - 2 * nn);
	if((--reg[r]))
		pc -= 2 * nn;
}
void do_clr() {
	if(dd.arg_type == ARG_REG)
		reg[dd.a] = 0;
	else
		w_write(dd.a, 0);
	N = 0;
	Z = 1;
}
void do_unknown() {
	exit(1);
}
void do_beq() {
	printf("%06o ", pc + 2 * xx);
	if(Z == 1)
		do_br();
}
void do_br() {
	pc = pc + 2 * xx;
	printf("%06o ", pc);
}
void do_tstb() {
	Z = 0;
	N = 0;
	byte b = b_read(dd.a);
	if(b & (1 << 7))
		N = 1;
	if(b == 0)
		Z = 1;
}
void do_bpl() {
	printf("%06o ", pc + 2 * xx);
	if(N == 0)
		do_br();
}
void do_jsr() {
	sp -= 2;
	w_write(sp, reg[r]);
	reg[r] = pc;
	pc = dd.a;
}
void do_rts() {
	pc = reg[r];
	reg[r] = w_read(sp);
	sp += 2;
}
void do_bic() {
	word res = dd.val & (~ss.val);
	if(dd.arg_type == ARG_REG)
		reg[dd.a] = res;
	else
		w_write(dd.a, res);
	
	//fprintf(stderr, "\nss = %06o\ndd = %06o\nres = %06o\n-----------------\n", ss.val, dd.val, res);

	Z = 0;
	N = 0;
	if(res < 0)
		N = 1;
	if(res == 0)
		Z = 1;
}
void do_asr() {
	word res = (dd.val >> 1);
	//if(dd.val >> 15)
	//	res = res & 0100000;

	if(dd.arg_type == ARG_REG)
		reg[dd.a] = res;
	else
		w_write(dd.a, res);

	Z = 0;
	N = 0;
	if(res < 0)
		N = 1;
	if(res == 0)
		Z = 1;
}
void do_incb() {
	byte res = b_read(dd.a);
	res++;
	b_write(dd.a, res);

	Z = 0;
	N = 0;
	if(res < 0)
		N = 1;
	if(res == 0)
		Z = 1;
}
void do_inc() {
	word res = w_read(dd.a);
	res++;
	w_write(dd.a, res);

	Z = 0;
	N = 0;
	if(res < 0)
		N = 1;
	if(res == 0)
		Z = 1;
}
void do_ash() {
	int n = dd.val & 077;
	if(n >= 0) {
		reg[r] = reg[r] << n;
		reg[r] = (reg[r] >> 1) << 1;
	}
	else {
		word first_bit = reg[r] >> 15;
		reg[r] = reg[r] >> (n * (-1));
		if(first_bit)
			reg[r] = reg[r] & 0100000;
	}

	word res = reg[r];
	Z = 0;
	N = 0;
	if(res < 0)
		N = 1;
	if(res == 0)
		Z = 1;
}
void do_tst() {
	N = 0;
	Z = 0;
	if(dd.val < 0)
		N = 1;
	if(dd.val == 0)
		Z = 1;
}
//======================================================================
void load_file(FILE* f) {
	unsigned x = 0;
	unsigned a = 0;
	unsigned int n = 0;
	unsigned int i = 0;
	while(fscanf(f, "%x%x", &a, &n) == 2) {
		for(i = 0; i < n; i++) {
			fscanf(f, "%x", &x);
			b_write(a, (byte)x);
			a++;
		}
	}
}
//======================================================================
void mem_dump(adr start, word n) {
	word i = 0;
	word w = 0;
	for(i = 0; i < n; i += 2) {
		w = w_read(start);
		printf("%06o : %06o\n", start, w);
		start += 2;
	}
}
//======================================================================
void run_programm() {
	b_write(ostat, 0xFF);
	pc = 01000;
	while(1) {
		word w = w_read(pc);
		printf("%06o : %06o ", pc, w);
		pc += 2;
		for(int i = 0; ; i++) {
			struct Command cmd = command_list[i];
			if((w & cmd.mask) == cmd.opcode) {
				printf("%s ", cmd.name);
				if(cmd.param & IS_BYTE_COM)
					IsByteCommand = 1;
				else
					IsByteCommand = 0;
				if(cmd.param & HAS_R) {
					if(cmd.opcode == RTS_OP_CODE)
						r = w & 7;
					else
						r = (w >> 6) & 7;
					printf("R%d ", r);
				}
				if(cmd.param & HAS_SS)
					ss = get_mr(w >> 6);
				if(cmd.param & HAS_DD)
					dd = get_mr(w);
				if(cmd.param & HAS_NN)
					nn = w & 077;
				if(cmd.param & HAS_XX) {
					xx = w & 0xFF;
					if(xx & (1 << 7))
						xx = xx - 0400;
				}
				cmd.do_action();
				reg_dump();
				//reg_dump();
				break;
			}
		}
	printf("\n");
	}
}

