/*****************************************************************************
 *  The MUchine interpreter. Executes the bytecode.                          *
 *  Copyright (C) 2005 Wiesner Thomas                                        *
 *                                                                           *
 *  This program is free software; you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation; either version 2 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program; if not, write to the Free Software              *
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA *
 *****************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "muchine.h"
#include "muint.h"

// ------------------------------- MAIN ----------------------------------
int main(int argc, char *argv[])
{
	int i;

	bzero(&seti, sizeof(struct settings));
	
	if(argc < 3) {
		printf("MUCHINE Version %s\nCopyright (C) 2005 by Wiesner Thomas\n\n",
				MU_VER);
		printf("Usage: %s memsz file [disasm] [stackdump] [verbose] [cnt_inst]\n", argv[0]);
		printf("THIS SOFTWARE COMES WITH ABSOLUTELY NO WARRANTY! "
				"USE AT YOUR OWN RISK!\n");
		exit(-1);
	}

	for(i = 3; i < argc; i++) {
		if(strcmp(argv[i], "disasm") == 0)
			seti.disasm = 1;
		if(strcmp(argv[i], "memdump") == 0)
			seti.memdump = 1;
		if(strcmp(argv[i], "stackdump") == 0)
			seti.stackdump = 1;
		if(strcmp(argv[i], "verbose") == 0)
			seti.verbose = 1;
		if(strcmp(argv[i], "cnt_inst") == 0)
			seti.cnt_inst = 1;
	}

	if(seti.verbose) {
		printf("MUCHINE Version %s\nCopyright (C) 2005 by Wiesner Thomas\n\n",
				MU_VER);
	}
	
	i = atoi(argv[1]);

	if(i < MEM_MIN || i > MEM_MAX) {
		printf("Invalid memory size: Range is %d-%d\n", MEM_MIN, MEM_MAX);
		exit(-1);
	}
	
	if(vminit(i) < 0) {
		puts("Error on initializing memory");
		exit(-1);
	}

	if(seti.verbose) {
		puts("VM Initialized.");
		printf("Virtual memory size is %d Bytes.\n", ivm.memsz);
		printf("Stack size is %d Words.\n", STACKSZ);
	}
	
	if(loadfile(argv[2]) < 0) {
		puts("Unable to load file");
		vmfree();
		exit(-1);
	}

	if(seti.verbose)
		printf("Bytecode loaded.\n");

#if EXEC_DELAY != 0
	if(seti.verbose) {
		printf("Execution speed is: %ld Instruction/s\n", 1000000/(EXEC_DELAY));
		puts("Press Enter");
		getchar();
	}
#endif
	
	run();
	
	vmfree();
	return 0;
}

// ------------------------ DIVERSE FUNCTIONS ----------------------------
void errdump(char *str, unsigned char opcode)
{
	printf("%s\nIPTR=0x%x; OPCODE=0x%x\n", str, ivm.iptr, opcode);
}

void stackdump(void)
{
	int i;
	int c = 0;

	for(i = 0; i < STACKSZ; i++) {
		printf("%4x ", ivm.stack[i]);
		c++;
		if(c >= 8) {
			c = 0;
			if(i < ivm.memsz - 1)
				putchar('\n');
		}
	}
	putchar('\n');
}

void memdump(void)
{
	int i, c = 0;
	int j;

	for(i = 0; i < ivm.memsz; i++) {
		printf("%2x ", ivm.mem[i]);
		c++;
		if(c >= 16) {
			c = 0;
			printf("   ");
			for(j = 0; j < 16; j++) {
				if(isprint(ivm.mem[i-15+j]))
					putchar(ivm.mem[i-15+j]);
				else
					putchar('.');
			}
			if(i < ivm.memsz - 1)
				putchar('\n');
		}
	}
	putchar('\n');
}

// Prints the mnemonic of the instruction onto which the IPTR points
void printmnemonic(void)
{
	unsigned char opcode = ivm.mem[ivm.iptr];
	int i;

	if(opcode > OPCODE_MAX) {
		puts("printmnemonic: Not printing mnemonic. Opcode out of range");
		return;
	}

	if(opcode == OP_ICALL && ivm.ops[0] > ICALL_MAX) {
		puts("printmnemonic: Not printing mnemonic. ICALL out of range");
		return;
	}
	
	printf("%s %s ", INSTNAMES[opcode], 
			opcode == OP_ICALL ? ICALLNAMES[ivm.ops[0]] : "");

	if(opcode != OP_ICALL) {
		for(i = 0; i < OPERANDS[opcode]; i++) {
			printf("0x%x%s", ivm.ops[i], i < OPERANDS[opcode]-1 ? ", " : "");
		}
	} else {
		for(i = 1; i < OPERANDS[opcode]; i++) {
			printf("0x%x%s", ivm.ops[i], i < OPERANDS[opcode]-1 ? ", " : "");
		}
	}
}

int run(void)
{
	unsigned char opcode;
	int newiptr;
	int opret = -1;

	
	while(1) {
		opcode = ivm.mem[ivm.iptr];
		if(opcode > OPCODE_MAX) {
			errdump("Invalid opcode", opcode);
			return -1;
		}

		// Prepare operands. Remember: Operands are of datatype short!
		if(OPERANDS[opcode]*2 + ivm.iptr >= ivm.memsz) {
			errdump("Operand wrap around is not allowed", opcode);
			return -1;
		}
		
		ivm.ops = (unsigned short *) &ivm.mem[ivm.iptr+1];

		if(seti.memdump) {
			memdump();
			putchar('\n');
		}
		
		if(seti.stackdump)
			stackdump();
		
		if(seti.disasm) {
			printf("IPTR=%x\tSTACKPTR=%x\tOPCODE=%x\tPSW=%x \t(", ivm.iptr, ivm.stackptr, opcode, ivm.psw);
			printmnemonic();
			puts(")");
		}
	
		opret = -1;
		opret = op_funcs[opcode]();
		
		if(opret < 0) {
			errdump("Error caused by opcode", opcode);
			switch(opret) {
				case OP_OOR:		puts("Invalid address");
					break;
				case STACK_OV:		puts("Stack overflow");
					break;
				case STACK_UV:		puts("Stack underflow");
					break;
				case INV_ICALL:	puts("Invalid ICALL");
					break;
				case DIV_BY_ZERO:	puts("Division by zero");
					break;
				default: printf("Undefined error #%d\n", opret);
					break;
			}
			return -1;
		}

		// Increase IP correctly (wraps around on overflow)
		if(opret != IPTR_CHGD) {
			newiptr = ivm.iptr + OPERANDS[opcode]*2 + 1;
			if(newiptr >= ivm.memsz) {
				ivm.iptr = newiptr - ivm.memsz;
			} else
				ivm.iptr = newiptr;
		}
		
		ivm.instructions++;
		
#if EXEC_DELAY != 0
		usleep(EXEC_DELAY);
#endif
		if(seti.stackdump || seti.memdump)
			putchar('\n');
	}
}

int vminit(unsigned short size)
{
	
	ivm.memsz = size;
	
	ivm.mem = (unsigned char *) malloc(ivm.memsz);

	if(ivm.mem == NULL)
		return -1;

	bzero(ivm.mem, ivm.memsz);
	bzero(ivm.stack, STACKSZ*sizeof(unsigned short));
	ivm.stackptr = 0;
	ivm.psw = 0;
	ivm.iptr = ENTRY;
	
	return 0;
}

void vmfree(void)
{
	free(ivm.mem);
}

int loadfile(char *fil)
{
	FILE *fp;
	struct stat info;

	if(stat(fil, &info) < 0) {
		puts("loadfile: Cannot stat");
		return -1;
	}

	if(info.st_size > ivm.memsz) {
		puts("loadfile: File doesn't fit into memory");
		return -2;
	}

	if(info.st_size == 0)
		return 0;
	
	if((fp = fopen(fil, "rb")) == NULL) {
		puts("loadfile: Unable to open file for reading");
		return -3;
	}
	
	if(fread(ivm.mem, info.st_size, 1, fp) != 1) {
		puts("loadfile: Error on reading file");
		return -4;
	}
	
	fclose(fp);

	return 0;
}

int do_push(unsigned short val)
{
	if(ivm.stackptr > STACKSZ - 1)	// Stack overflow
		return STACK_OV;
	
	ivm.stack[ivm.stackptr] = val;
	ivm.stackptr++;

	return 0;
}

int do_pop()
{
	if(ivm.stackptr < 1)	// Stack underflow
		return STACK_UV;
	
	ivm.stackptr--;
	return ivm.stack[ivm.stackptr];
}

// ------------------ IMPLEMENTATION OF INSTRUCTIONS ---------------------

// NOOP Operation.
int op_noop()
{
	return OPC_OK;
}

// INC Operation.
// Operand 0 = address
int op_inc()
{	
	if(ivm.ops[0] >= ivm.memsz - 1)	// Address out of bounds
		return OP_OOR;
	
	// Very ugly contruct (Cast the unsigned char to short)
	(*((unsigned short *) &ivm.mem[ivm.ops[0]]))++;
	
	return OPC_OK;
}

// DEC Operation.
// Operand 0 = address
int op_dec()
{
	if(ivm.ops[0] >= ivm.memsz - 1)	// Address out of bounds
		return OP_OOR;
	
	// Very ugly contruct (Cast the unsigned char to short)
	(*((unsigned short *) &ivm.mem[ivm.ops[0]]))--;
	
	return OPC_OK;
}

// CLR Operation.
// Operand 0 = address
int op_clr()
{
	if(ivm.ops[0] >= ivm.memsz - 1)	// Address out of bounds
		return OP_OOR;
	
	// Very ugly contruct (Cast the unsigned char to short)
	*((unsigned short *) &ivm.mem[ivm.ops[0]]) = 0;
	
	return OPC_OK;
}

// LOAD Operation.
// Operand 0 = destination address
// Operand 1 = (literal) value
int op_load()
{
	if(ivm.ops[0] >= ivm.memsz - 1)	// Address out of bounds
		return OP_OOR;
	
	// Very ugly contruct (Cast the unsigned char to short)
	*((unsigned short *) &ivm.mem[ivm.ops[0]]) = ivm.ops[1];
	
	return OPC_OK;
}

// JMPA Operation.
// Operand 0 = address
int op_jmpa()
{
	if(ivm.ops[0] >= ivm.memsz)	// Address out of bounds
		return OP_OOR;
	
	ivm.iptr = ivm.ops[0];	// Load new address
	
	return IPTR_CHGD;
}

// JMPZ Operation.
// Operand 0 = destination address
// Operand 1 = test register
int op_jmpz()
{
	if(ivm.ops[0] >= ivm.memsz)	// Address out of bounds
		return OP_OOR;

	if(ivm.ops[1] >= ivm.memsz -1)
		return OP_OOR;
	
	if(*((unsigned short *) &ivm.mem[ivm.ops[1]]) == 0) {
		ivm.iptr = ivm.ops[0];	// Load new address
		return IPTR_CHGD;
	}

	return OPC_OK;
}

// ADD Operation.
// Operand 0 = destination address
// Operand 1 = first summand (address)
// Operand 2 = second summand (address)
int op_add()
{
	unsigned short buf;
	
	if(ivm.ops[0] >= ivm.memsz - 1)	// Address out of bounds
		return OP_OOR;

	if(ivm.ops[1] >= ivm.memsz - 1)
		return OP_OOR;

	if(ivm.ops[2] >= ivm.memsz - 1)
		return OP_OOR;
	
	buf = *((unsigned short *) &ivm.mem[ivm.ops[1]]);
	buf += *((unsigned short *) &ivm.mem[ivm.ops[2]]);
	*((unsigned short *) &ivm.mem[ivm.ops[0]]) = buf;

	return OPC_OK;
}

// SUB Operation.
// Operand 0 = destination address
// Operand 1 = value (address)
// Operand 2 = value (subtractor)
int op_sub()
{
	unsigned short buf;
	
	if(ivm.ops[0] >= ivm.memsz - 1)	// Address out of bounds
		return OP_OOR;

	if(ivm.ops[1] >= ivm.memsz - 1)
		return OP_OOR;

	if(ivm.ops[2] >= ivm.memsz - 1)
		return OP_OOR;
	
	buf = *((unsigned short *) &ivm.mem[ivm.ops[1]]);
	buf -= *((unsigned short *) &ivm.mem[ivm.ops[2]]);
	*((unsigned short *) &ivm.mem[ivm.ops[0]]) = buf;

	return OPC_OK;
}

// MUL Operation.
// Operand 0 = destination address (Note: result is long!)
// Operand 1 = first value (address)
// Operand 2 = second value (address)
int op_mul()
{
	unsigned long buf;
	
	if(ivm.ops[0] >= ivm.memsz - 3)	// Address out of bounds
		return OP_OOR;

	if(ivm.ops[1] >= ivm.memsz - 1)
		return OP_OOR;

	if(ivm.ops[2] >= ivm.memsz - 1)
		return OP_OOR;
	
	buf = *((unsigned short *) &ivm.mem[ivm.ops[1]]);
	buf *= *((unsigned short *) &ivm.mem[ivm.ops[2]]);
	*((unsigned long *) &ivm.mem[ivm.ops[0]]) = buf;

	return OPC_OK;
}

// DIV Operation.
// Operand 0 = destination address (address, long value!)
// Operand 1 = divisor value (address, long value!)
// Operand 2 = dividend value (address, long value!)
int op_div()
{
	unsigned long buf;
	
	if(ivm.ops[0] >= ivm.memsz - 1)	// Address out of bounds
		return OP_OOR;

	if(ivm.ops[1] >= ivm.memsz - 3)
		return OP_OOR;

	if(ivm.ops[2] >= ivm.memsz - 3)
		return OP_OOR;
	
	if(*((unsigned long *) &ivm.mem[ivm.ops[2]]) == 0)
		return DIV_BY_ZERO;
	
	buf = *((unsigned long *) &ivm.mem[ivm.ops[1]]);
	buf /= *((unsigned long *) &ivm.mem[ivm.ops[2]]);
	*((unsigned long *) &ivm.mem[ivm.ops[0]]) = buf;

	return OPC_OK;
}

// AND Operation.
// Operand 0 = destination address
// Operand 1 = 1st value (address)
// Operand 2 = 2nd value (address)
int op_and()
{
	unsigned short buf;
	
	if(ivm.ops[0] >= ivm.memsz - 1)	// Address out of bounds
		return OP_OOR;

	if(ivm.ops[1] >= ivm.memsz - 1)
		return OP_OOR;

	if(ivm.ops[2] >= ivm.memsz - 1)
		return OP_OOR;
	
	buf = *((unsigned short *) &ivm.mem[ivm.ops[1]]);
	buf &= *((unsigned short *) &ivm.mem[ivm.ops[2]]);
	*((unsigned short *) &ivm.mem[ivm.ops[0]]) = buf;

	return OPC_OK;
}

// OR Operation.
// Operand 0 = destination address
// Operand 1 = 1st value (address)
// Operand 2 = 2nd value (address)
int op_or()
{
	unsigned short buf;
	
	if(ivm.ops[0] >= ivm.memsz - 1)	// Address out of bounds
		return OP_OOR;

	if(ivm.ops[1] >= ivm.memsz - 1)
		return OP_OOR;

	if(ivm.ops[2] >= ivm.memsz - 1)
		return OP_OOR;
	
	buf = *((unsigned short *) &ivm.mem[ivm.ops[1]]);
	buf |= *((unsigned short *) &ivm.mem[ivm.ops[2]]);
	*((unsigned short *) &ivm.mem[ivm.ops[0]]) = buf;

	return OPC_OK;
}

// NOT Operation.
// Operand 0 = destination address
// Operand 1 = value (address)
int op_not()
{
	unsigned short buf;
	
	if(ivm.ops[0] >= ivm.memsz - 1)	// Address out of bounds
		return OP_OOR;

	if(ivm.ops[1] >= ivm.memsz - 1)
		return OP_OOR;
	
	buf = *((unsigned short *) &ivm.mem[ivm.ops[1]]);
	*((unsigned short *) &ivm.mem[ivm.ops[0]]) = ~buf;

	return 0;
}

// XOR Operation.
// Operand 0 = destination address
// Operand 1 = 1st value (address)
// Operand 2 = 2nd value (address)
int op_xor()
{
	unsigned short buf;
	
	if(ivm.ops[0] >= ivm.memsz - 1)	// Address out of bounds
		return OP_OOR;

	if(ivm.ops[1] >= ivm.memsz - 1)
		return OP_OOR;

	if(ivm.ops[2] >= ivm.memsz - 1)
		return OP_OOR;
	
	buf = *((unsigned short *) &ivm.mem[ivm.ops[1]]);
	buf ^= *((unsigned short *) &ivm.mem[ivm.ops[2]]);
	*((unsigned short *) &ivm.mem[ivm.ops[0]]) = buf;

	return OPC_OK;
}

// CMP Operation.
// Operand 0 = 1st value (address)
// Operand 1 = 2nd value (address)
int op_cmp()
{
	unsigned long s1, s2;
	
	if(ivm.ops[0] >= ivm.memsz - 1)	// Address out of bounds
		return -1;

	if(ivm.ops[1] >= ivm.memsz - 1)
		return -1;
	
	s1 = *((unsigned short *) &ivm.mem[ivm.ops[0]]);
	s2 = *((unsigned short *) &ivm.mem[ivm.ops[1]]);
	
	ivm.psw = 0;

	if(s1 == 0) {
		ivm.psw |= PSW_ZERO1;
	}
	if(s2 == 0) {
		ivm.psw |= PSW_ZERO2;
	}
	if(s1 == s2) {
		ivm.psw |= PSW_EQU;
	}
	if(s1 < s2) {
		ivm.psw |= PSW_LT;
	}
	if(s1 > s2) {
		ivm.psw |= PSW_GT;
	}
	if(s1 >= s2) {
		ivm.psw |= PSW_GTE;
	}
	if(s1 <= s2) {
		ivm.psw |= PSW_LTE;
	}

	return OPC_OK;
}

// PUSH Operation.
// Operand 0 = word address
int op_push()
{
	if(ivm.ops[0] >= ivm.memsz - 1)	// Address out of bounds
		return OP_OOR;

	return do_push(*((unsigned short *) &ivm.mem[ivm.ops[0]]));
}

// POP Operation.
// Operand 0 = word address
int op_pop()
{
	int ret;
	
	if(ivm.ops[0] >= ivm.memsz - 1)	// Address out of bounds
		return OP_OOR;

	ret = do_pop();

	if(ret < 0)
		return ret;
	
	*((unsigned short *) &ivm.mem[ivm.ops[0]]) = (unsigned short) ret;

	return OPC_OK;
}

// MOV Operation.
// Operand 0 = destination address
// Operand 1 = source address
int op_mov()
{
	unsigned short buf;
	
	if(ivm.ops[0] >= ivm.memsz - 1)	// Address out of bounds
		return OP_OOR;

	if(ivm.ops[1] >= ivm.memsz - 1)
		return OP_OOR;
	
	buf = *((unsigned short *) &ivm.mem[ivm.ops[1]]);
	*((unsigned short *) &ivm.mem[ivm.ops[0]]) = buf;

	return OPC_OK;
}

// ICALL
// Operand 0 = ICALL Number
int op_icall()
{
	int icall_ret = INV_ICALL;
	
	if(ivm.ops[0] > ICALL_MAX)
		return INV_ICALL;

	icall_ret = icall_funcs[ivm.ops[0]]();

	if(icall_ret < 0)
		return icall_ret;
	
	return OPC_OK;
}

// CALL Operation
// Operand 0 = address
int op_call()
{
	int ret;
	
	if(ivm.ops[0] >= ivm.memsz)	// Address out of bounds
		return OP_OOR;
	
	// Inc IPTR to next instruction and save it onto stack
	ret = do_push(ivm.iptr + OPERANDS[OP_CALL]*2 + 1);
	
	if(ret < 0)
		return ret;
	
	ivm.iptr = ivm.ops[0];	// Load new address
	
	return IPTR_CHGD;
}

// RET Operation
// No operands
int op_ret()
{
	int ret;
	
	ret = do_pop();
	
	if(ret < 0)
		return ret;
	
	if(ret >= ivm.memsz)	// Address out of bounds
		return OP_OOR;
	
	ivm.iptr = ret;	// Load new address
	
	return IPTR_CHGD;
}

// MOD Operation.
// Operand 0 = destination address (address, long value!)
// Operand 1 = divisor value (address, long value!)
// Operand 2 = dividend value (address, long value!)
int op_mod()
{
	unsigned long buf;
	
	if(ivm.ops[0] >= ivm.memsz - 1)	// Address out of bounds
		return OP_OOR;

	if(ivm.ops[1] >= ivm.memsz - 3)
		return OP_OOR;

	if(ivm.ops[2] >= ivm.memsz - 3)
		return OP_OOR;
	
	buf = *((unsigned long *) &ivm.mem[ivm.ops[1]]);
	buf %= *((unsigned long *) &ivm.mem[ivm.ops[2]]);
	*((unsigned long *) &ivm.mem[ivm.ops[0]]) = buf;

	return OPC_OK;
}

// JZ1 Operation.
// Operand 0 = destination address
int op_jz1()
{
	if(ivm.ops[0] >= ivm.memsz)	// Address out of bounds
		return OP_OOR;

	if((ivm.psw & PSW_ZERO1) > 0) {
		ivm.iptr = ivm.ops[0];	// Load new address
		return IPTR_CHGD;
	}

	return OPC_OK;
}

// JZ2 Operation.
// Operand 0 = destination address
int op_jz2()
{
	if(ivm.ops[0] >= ivm.memsz)	// Address out of bounds
		return OP_OOR;

	if((ivm.psw & PSW_ZERO2) > 0) {
		ivm.iptr = ivm.ops[0];	// Load new address
		return IPTR_CHGD;
	}

	return OPC_OK;
}

// JNZ1 Operation.
// Operand 0 = destination address
int op_jnz1()
{
	if(ivm.ops[0] >= ivm.memsz)	// Address out of bounds
		return OP_OOR;

	if((ivm.psw & PSW_ZERO1) == 0) {
		ivm.iptr = ivm.ops[0];	// Load new address
		return IPTR_CHGD;
	}

	return OPC_OK;
}

// JNZ2 Operation.
// Operand 0 = destination address
int op_jnz2()
{
	if(ivm.ops[0] >= ivm.memsz)	// Address out of bounds
		return OP_OOR;

	if((ivm.psw & PSW_ZERO2) == 0) {
		ivm.iptr = ivm.ops[0];	// Load new address
		return IPTR_CHGD;
	}

	return OPC_OK;
}

// JEQU Operation.
// Operand 0 = destination address
int op_jequ()
{
	if(ivm.ops[0] >= ivm.memsz)	// Address out of bounds
		return OP_OOR;

	if((ivm.psw & PSW_EQU) > 0) {
		ivm.iptr = ivm.ops[0];	// Load new address
		return IPTR_CHGD;
	}

	return OPC_OK;
}

// JLT Operation.
// Operand 0 = destination address
int op_jlt()
{
	if(ivm.ops[0] >= ivm.memsz)	// Address out of bounds
		return OP_OOR;

	if((ivm.psw & PSW_LT) > 0) {
		ivm.iptr = ivm.ops[0];	// Load new address
		return IPTR_CHGD;
	}

	return OPC_OK;
}

// JGT Operation.
// Operand 0 = destination address
int op_jgt()
{
	if(ivm.ops[0] >= ivm.memsz)	// Address out of bounds
		return OP_OOR;

	if((ivm.psw & PSW_GT) > 0) {
		ivm.iptr = ivm.ops[0];	// Load new address
		return IPTR_CHGD;
	}

	return OPC_OK;
}

// JLTE Operation.
// Operand 0 = destination address
int op_jlte()
{
	if(ivm.ops[0] >= ivm.memsz)	// Address out of bounds
		return OP_OOR;

	if((ivm.psw & PSW_LTE) > 0) {
		ivm.iptr = ivm.ops[0];	// Load new address
		return IPTR_CHGD;
	}

	return OPC_OK;
}

// JGTE Operation.
// Operand 0 = destination address
int op_jgte()
{
	if(ivm.ops[0] >= ivm.memsz)	// Address out of bounds
		return OP_OOR;

	if((ivm.psw & PSW_GTE) > 0) {
		ivm.iptr = ivm.ops[0];	// Load new address
		return IPTR_CHGD;
	}

	return OPC_OK;
}

// JNE Operation.
// Operand 0 = destination address
int op_jne()
{
	if(ivm.ops[0] >= ivm.memsz)	// Address out of bounds
		return OP_OOR;

	if((ivm.psw & PSW_EQU) == 0) {
		ivm.iptr = ivm.ops[0];	// Load new address
		return IPTR_CHGD;
	}

	return OPC_OK;
}

// -------------------- IMPLEMENTATION OF ICALLS -------------------------
// EXIT Icall
int icall_exit()
{
	if(seti.cnt_inst)
		printf("Executed %lu instructions (%d Bit counter)\n",
				ivm.instructions, sizeof(ivm.instructions)*8);
	vmfree();
	exit(0);
}

// PUTCH Icall
// Pops one value from stack which is printed.
int icall_putch()
{
	int ret;
	
	ret = do_pop();
	
	if(ret < 0)
		return ret;
	
	putchar(ret & 0x00ff);
	fflush(stdout);

	return 0;
}

// GETCHAR Icall
// Like getchar(). Pushed the char onto the stack.
int icall_getchar()
{
	int ret;
	
	ret = do_push(getchar() & 0x00ff);
	
	if(ret < 0)
		return ret;
	
	return 0;
}

// vim:ts=3:sw=3
