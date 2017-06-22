enum COMANDS {
	ADD = 06,
	MOVE = 01,
	HALT = 00
};

enum {
	NO_PARAM,
	HAS_DD,
	HAS_SS = (1 << 1),
	HAS_R = (1 << 2),
	HAS_NN = (1 << 3),
	HAS_XX = (1 << 4),
	IS_BYTE_COM = (1 << 5),
};

enum {
	ARG_REG,
	ARG_MEM
};

enum {
	ostat = 0177564,
	odata = 0177566
};

enum {RTS_OP_CODE = 0000200};

#define pc reg[7]
#define sp reg[6]

typedef unsigned char byte;
typedef unsigned short int word;
typedef unsigned short int adr;

char* compare(word w);
byte mem[64 * 1024];
word reg[8];
byte r;
byte nn;
word xx;
int IsByteCommand;
byte N;
byte Z;
byte C;

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

byte b_read(adr a);
void b_write(adr a, byte val);
word w_read(adr a);
void w_write(adr a, word val);
struct SS_DD get_mr(word w);
void load_file(FILE* f);
void mem_dump(adr start, word n);
void run_programm();
void reg_dump();

void do_mov();
void do_movb();
void do_add();
void do_halt();
void do_sob();
void do_clr();
void do_beq();
void do_br();
void do_tstb();
void do_bpl();
void do_jsr();
void do_rts();
void do_bic();
void do_asr();
void do_incb();
void do_inc();
void do_ash();
void do_tst();
void do_unknown();


