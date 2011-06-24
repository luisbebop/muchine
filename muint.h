/*****************************************************************************
 *  This headerfile is used by the bytecode interpreter.                     *
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


#ifndef _MUINT_H
#define _MUINT_H	1

struct vm {
	unsigned char *mem;		// Memory
	unsigned short memsz;	// Memory size
	unsigned short iptr;		// Instruction pointer

	unsigned short *ops;		// Operand pointer
	unsigned short psw;		// Processor status word

	unsigned short stackptr;	// Stack pointer
	unsigned short stack[STACKSZ];

	unsigned long instructions;	// Instruction counter (for statistics)
} ivm;

struct settings {
	unsigned char disasm;	// Print disassembly while executing
	unsigned char memdump;
	unsigned char stackdump;
	unsigned char verbose;
	unsigned char cnt_inst;
} seti;


int vminit(unsigned short size);
void vmfree(void);

int loadfile(char *fil);

void errdump(char *str, unsigned char opcode);
void memdump(void);
void stackdump(void);
void printmnemonic(void);
int run(void);

int do_push(unsigned short val);
int do_pop();

// Functions which realize the opcodes
int op_noop();
int op_inc();
int op_dec();
int op_clr();
int op_jmpa();
int op_load();
int op_jmpz();
int op_add();
int op_sub();
int op_mul();
int op_div();
int op_and();
int op_or();
int op_not();
int op_xor();
int op_cmp();
int op_push();
int op_pop();
int op_mov();
int op_call();
int op_ret();
int op_mod();
int op_jz1();
int op_jz2();
int op_jnz1();
int op_jnz2();
int op_jequ();
int op_jlt();
int op_jgt();
int op_jlte();
int op_jgte();
int op_jne();

// Functions which realize the icalls
int op_icall();
int icall_exit();
int icall_putch();
int icall_getchar();

// Function pointer for OP-Codes
int (*op_funcs[OPCODE_MAX+1])() = {op_noop, op_inc, op_dec, op_clr, op_load,
		op_jmpa, op_jmpz, op_add, op_sub, op_mul, op_div, op_and, op_or,
		op_not, op_xor, op_cmp, op_push, op_pop, op_mov, op_icall, op_call,
		op_ret, op_mod, op_jz1, op_jz2, op_jnz1, op_jnz2, op_jequ, op_jlt,
		op_jgt, op_jlte, op_jgte, op_jne};

// Function pointer for ICALLS
int (*icall_funcs[ICALL_MAX+1])() = {icall_exit, icall_putch, icall_getchar};

#endif

// vim:ts=3:sw=3
