/*****************************************************************************
 *  This headerfile is needed by all the MU-programs.                        *
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


#ifndef _MUCHINE_H
#define _MUCHINE_H	1

// Maximum valid opcode number
#define OPCODE_MAX	32

// Maximum valid interpreter call number
#define ICALL_MAX		2

// Entry point address
#define ENTRY			0

#define MEM_MIN		16
#define MEM_MAX		65536
#define STACKSZ		32	// Stacksize in words

// Delay between exectution of opcodes
#define EXEC_DELAY	0L
//#define EXEC_DELAY	100000L	// 10 IPS
//#define EXEC_DELAY	20000L	// 50 IPS

// OP-Code names
#define OP_NOOP		0
#define OP_INC			1		// Increment register
#define OP_DEC			2		// Decrement register
#define OP_CLR			3		// Clear register
#define OP_LOAD		4		// Load register with value
#define OP_JMPA		5		// Jump absolute
#define OP_JMPZ		6		// Jump on zero
#define OP_ADD			7
#define OP_SUB			8
#define OP_MUL			9
#define OP_DIV			10
#define OP_AND			11
#define OP_OR			12
#define OP_NOT			13
#define OP_XOR			14
#define OP_CMP			15
#define OP_PUSH		16
#define OP_POP			17
#define OP_MOV			18
#define OP_ICALL		19		// Interpreter call
#define OP_CALL		20
#define OP_RET			21
#define OP_MOD			22		// Modulo
#define OP_JZ1			23		// Jump on zero 1 (for jumps after OP_CMP)
#define OP_JZ2			24		// Jump on zero 2(for jumps after OP_CMP)
#define OP_JNZ1		25		// Jump on not zero 1 (for jumps after OP_CMP)
#define OP_JNZ2		26		// Jump on not zero 2 (for jumps after OP_CMP)
#define OP_JEQU		27		// Jump on equal (for jumps after OP_CMP)
#define OP_JLT			28		// Jump on lower than (for jumps after OP_CMP)
#define OP_JGT			29		// Jump on greater than (for jumps after OP_CMP)
#define OP_JLTE		30		// Jump on lower or equal (for jumps after OP_CMP)
#define OP_JGTE		31		// Jump on greater or equal (for jumps after OP_CMP)
#define OP_JNE			32		// Jump on not equal (for jumps after OP_CMP)

// ICALL names
#define ICALL_EXIT		0
#define ICALL_PUTCH		1
#define ICALL_GETCHAR	2

// Errors and internal flags
#define OPC_OK			0		// Opcode execution succeeded, IPTR not changed
#define IPTR_CHGD		1		// Operation changed IPTR, operation succeeded
#define OP_OOR			(-1)	// Operand out of range
#define STACK_OV		(-2)	// Stack overflow
#define STACK_UV		(-3)	// Stack underflow
#define INV_ICALL		(-4)	// Invalid Interpreter call
#define DIV_BY_ZERO	(-5)	// Division by zero error


// PSW Bits
#define PSW_ZERO1		0x0001	// Operand 1 is zero
#define PSW_ZERO2		0x0002	// Operand 2 is zero
#define PSW_EQU		0x0004	// Operands are equal
#define PSW_LT			0x0008	// Operand 1 is lower than operand 2
#define PSW_GT			0x0010	// Operand 1 is greater than operand 2
#define PSW_LTE		0x0020	// Operand 1 is lower of equal operand 2
#define PSW_GTE		0x0040	// Operand 1 is greater or equal operand 2


static char INSTNAMES[][8] = {"NOOP",
										"INC",
										"DEC",
										"CLR",
										"LOAD",
										"JMPA",
										"JMPZ",
										"ADD",
										"SUB",
										"MUL",
										"DIV",
										"AND",
										"OR",
										"NOT",
										"XOR",
										"CMP",
										"PUSH",
										"POP",
										"MOV",
										"ICALL",
										"CALL",
										"RET",
										"MOD",
										"JZ1",
										"JZ2",
										"JNZ1",
										"JNZ2",
										"JEQU",
										"JLT",
										"JGT",
										"JLTE",
										"JGTE",
										"JNE"
										};

static char ICALLNAMES[][8] = {"EXIT",
										 "PUTCH",
										 "GETCHAR"
										 };

// How many operands each opcode has
static unsigned char OPERANDS[] = { 0,		/* NOOP */
												1,		/* INC */
												1,		/* DEC */
												1,		/* CLR */
												2,		/* LOAD */
												1,		/* JMPA */
												2,		/* JMPZ */
												3,		/* ADD */
												3,		/* SUB */
												3,		/* MUL */
												3,		/* DIV */
												3,		/* AND */
												3,		/* OR */
												2,		/* NOT */
												3,		/* XOR */
												2,		/* CMP */
												1,		/* PUSH */
												1,		/* POP */
												2,		/* MOV */
												1,		/* ICALL */
												1,		/* CALL */	
												0,		/* RET */
												3,		/* MOD */
												1,		/* JZ1 */
												1,		/* JZ2 */
												1,		/* JNZ1 */
												1,		/* JNZ2 */
												1,		/* JEQU */
												1,		/* JLT */
												1,		/* JGT */
												1,		/* JLTE */
												1,		/* JGTE */
												1		/* JNE */
												};
#endif

// vim:ts=3:sw=3
