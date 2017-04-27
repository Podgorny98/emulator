#include <stdio.h>
#include <string.h>
#include <stdlib.h>
enum COMANDS {
	ADD = 06,
	MOVE = 01,
	HALT = 00
};

typedef unsigned char byte;
typedef unsigned short int word;
typedef short int adr;
#define pc reg[7]
enum {
	NO_PARAM,
	HAS_DD,
	HAS_SS = (1 << 1)
};
enum {
	ARG_REG,
	ARG_MEM
}; 
char* compare(word w);
byte mem[64 * 1024];
word reg[8];
//======================================================================
struct Command {
	word mask;
	word opcode;
	char* name;
	void (*do_action)();
	word param;
};
struct SS_DD {
	word val;
	adr a;
	int arg_type;
} ss, dd;
//======================================================================
void do_mov();
void do_add();
void do_halt();
void do_unknown();
//======================================================================
struct Command command_list[] = {
	{0xFFFF, 0, "HALT", do_halt, NO_PARAM},
	{0170000, 0010000, "MOV", do_mov, HAS_SS | HAS_DD},
	{0170000, 0060000, "ADD", do_add, HAS_SS | HAS_DD},
	{0, 0, "UNKNOWN", do_unknown, NO_PARAM}
};
//======================================================================
byte b_read(adr a);
void b_write(adr a, byte val);
word w_read(adr a);
void w_write(adr a, word val);
struct SS_DD get_mr(word w);
void load_file(FILE* f);
void mem_dump(adr start, word n);
void run_programm();
//======================================================================


//======================================================================
int main(int argc, char **argv)
{
	//mem_dump(0200, 16);
	FILE *f;
	char * filename = "add.o";
	if(argc != 1) {
		filename = argv[1];
	}
	f = fopen(filename, "r");
	if(f == NULL) {
		perror(filename);
		exit(1);
	}
	load_file(f);
	run_programm();
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
			if(n == 7) {
				res.a = pc;
				res.val = w_read(res.a);
				printf("#%d ", mem[pc]);
			}
			else {
				res.a = reg[n] + 2;
				res.val = w_read(res.a);
				printf("(R%d)+ ", n);
			}
			break;
		default :
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
}
void do_add() {
	if(dd.arg_type == ARG_REG)
		reg[dd.a] = dd.val + ss.val;
	else
		w_write(dd.a, dd.val + ss.val);
}
void do_halt() {
	printf("\nTHE END!\n");
	for(int i = 0; i < 8; i++)
		printf("R%d = %d\n", i, reg[i]);
	exit(0);
}
void do_unknown() {}

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
	pc = 01000;
	while(1) {
		word w = w_read(pc);
		printf("%06o : %06o ", pc, w);
		pc += 2;
		for(int i = 0; ; i++) {
			struct Command cmd = command_list[i];
			if((w & cmd.mask) == cmd.opcode) {
				printf("%s ", cmd.name);
				if((cmd.param >> 1) & 1)
					ss = get_mr(w >> 6);
				if(cmd.param & 1)
					dd = get_mr(w);
				cmd.do_action();
				break;
			}
		}
	printf("\n");
	}
}

