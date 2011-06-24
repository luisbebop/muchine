/*****************************************************************************
 *  The simple Assembler for the MUchine interpreter.                        *
 *    It assembles 'assembler like' text files into bytecode                 *
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
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "muchine.h"
#include "muasm.h"


int main(int argc, char *argv[])
{
	FILE *ifp, *ofp;
	struct line iline;	// Stores an input line
	int i;
	int ret;
	
	char *delim;			// Address of next delimiter
	int sublen;				// Length of substring
	
	struct label *labellst;
	int lcnt;				// Line counter
	int address = 0;		// Address of current opcode, label, etc, ...
	int addr_max;			// Saves the maximum used address (for codesize output)

	int opcode;				// Temporary buffer for opcode
	int opval;				// Operand value
	
	char equbuf[MAXCHAR];	// Buffer for EQU name
	int equval;				// EQU value

	// Is set to true after the first real instruction (non asm directive)
	unsigned char first_inst;
	unsigned char constant;		// Are we processing a label or a constant?
	
	
	if(argc != 3) {
		usage();
		return -1;
	}

	if((ifp = fopen(argv[1], "rb")) == NULL) {
		puts("Unable to open input file");
		return -1;
	}

	if((ofp = fopen(argv[2], "wb")) == NULL) {
		puts("Unable to open output file");
		fclose(ifp);
		return -1;
	}

	if((labellst = new_label()) == NULL) {
		puts("Unable to allocate memory");
		fclose(ifp);
		fclose(ofp);
		return -1;
	}

	// First pass: find all label names/constants and get their addresses
	puts("Pass 1:");
	first_inst = FALSE;
	lcnt = 0;
	address = 0;
	while(readline(ifp, &iline) != NULL) {
		lcnt++;
		constant = FALSE;
		
		// Try to analyze labels and instructions
		if(is_label(iline)) {
			// Find newlines, ... and truncate the input
			for(i = 0; i < iline.len; i++) {
				if(isnewline(iline.data[i])) {
						iline.data[i] = '\0';
						break;	// We can stop after the first
				}
			}

			if(strlen(iline.data) > 0) {	// Test if it was an empty line
				// Try to find an EQU directive
				delim = next_delim(iline.pos);	// Get beginning of delimiter (if any)

				// This could be done without the additional buffer by modifying
				// the input data in iline.data (add an '\0' at the end of the
				// EQU-name so that the add_label2 function recognizes it as a
				// word) but it much more readable this way.
				strncpy(equbuf, iline.pos, delim-iline.pos);
				equbuf[delim-iline.pos] = '\0';
					
				iline.pos = skip_delim(delim);	//	Try to get beginning of the word EQU

				if(iline.pos - delim > 0) {	// Did we really find another field?
					if(iline.pos == '\0') {	// EQU expected but EOL found
						printf("Expected EQU in line %d (but got end of line)\n", lcnt);
						goto err_abort;
					}

					if(strncmp(iline.pos, "EQU", 3) != 0) {	// Got something but not EQU
						printf("Expected EQU in line %d\n", lcnt);
						goto err_abort;
					}
					
					if(first_inst == TRUE) {
						printf("Error in line %d. (EQUs are not allowed after the first instruction.)\n",
								lcnt);
						goto err_abort;
					}

					delim = next_delim(iline.pos);
					iline.pos = skip_delim(delim);
					delim = next_delim(iline.pos);

					if(delim - iline.pos == 0) {
						printf("Missing substitution value in EQU directive, line %d\n", lcnt);
						goto err_abort;
					}
					
					equval = get_const(iline.pos, delim, labellst, lcnt);
					if(equval < 0) {
						printf("Error while parsind EQU substitution value in line %d\n",	lcnt);
						goto err_abort;
					} else {
						// The clou is that the EQUs are saevd into the label
						// buffer. Thus, they are automatically recognized later
						// (while the output code is generated). ;-)
						constant = TRUE;	// Remember to save the EQU value and not
												// the current address.
					}
				}
				
				// If we get here, it is a label, not an EQU.
				if(check_label(iline.data) != 0 && !constant) {
					printf("Illegal label/constant in line %d\n", lcnt);
					goto err_abort;
				}
				
				if(constant) {
					ret = add_label2(labellst, equbuf, equval, lcnt);
				} else {
					ret = add_label2(labellst, iline.data, address, lcnt);
				}
				
				if(ret == NO_MEM) {
					puts("Unable to allocate memory");
					goto err_abort;
				}
				if(ret == DUPLICATE_LABEL) {
					printf("Duplicate label/constant in line %d\n", lcnt);
					goto err_abort;
				}
			}
		} else {
			// Skip leading spaces, tabs, etc.
			while(isspace(*iline.pos) && *iline.pos != '\0')
				iline.pos++;

			// Now, check for empty line and skip it.
			if(strlen(iline.pos) == 0) {
				continue;
			}

			delim = next_delim(iline.pos);
			sublen = delim - iline.pos;

			// Find the opcode
			for(opcode = 0; opcode <= OPCODE_MAX; opcode++) {
				if(strncmp(INSTNAMES[opcode], iline.pos, sublen) == 0)
					break;
			}
			
			if(opcode > OPCODE_MAX) {
				printf("Invalid Opcode name in Line %d\n", lcnt);
				goto err_abort;
			}
			
			// Calculate new address
			address += 1 + OPERANDS[opcode]*2;	// 1 Byte OPCode + n*2Bytes param.
			first_inst = TRUE;
		}
	}
	addr_max = address;

	list_labels(labellst);

	fseek(ifp, 0L, SEEK_SET);		// Rewind fp

	// Second pass: Ignore label definitions and assemble the Opcodes
	puts("Pass 2:");
	lcnt = 0;
	address = 0;
	while(readline(ifp, &iline) != NULL) {
		lcnt++;
		if(!is_label(iline)) {
			// Skip leading spaces, tabs, etc.
			while(isspace(*(iline.pos)) && *(iline.pos) != '\0')
				iline.pos++;

			// Now, check for empty line and skip it.
			if(strlen(iline.pos) == 0) {
				continue;
			}

			delim = next_delim(iline.pos);
			sublen = delim - iline.pos;

			// Find the opcode
			for(opcode = 0; opcode <= OPCODE_MAX; opcode++) {
				if(strncmp(INSTNAMES[opcode], iline.pos, sublen) == 0)
					break;
			}

			fputc(opcode, ofp);	// Write opcode
			
			// Try to find all operands
			for(i = 0; i < OPERANDS[opcode]; i++) {
				iline.pos = delim;
				iline.pos = skip_delim(iline.pos);

				// No operand found
				if(*iline.pos == '\0') {
					printf("Missing operand in line %d. Expected %d, found %d.\n",
							lcnt, OPERANDS[opcode], i);
					goto err_abort;
				}

				delim = next_delim(iline.pos);
				
				opval = get_const(iline.pos, delim, labellst, lcnt);
				if(opval < 0) {
					printf("Error in line %d, operand #%d\n",	lcnt, i);
					goto err_abort;
				} else {
					// Write out operand value
					fputc(0x00ff & opval, ofp);
					fputc(opval >> 8, ofp);
				}
			}
			
			// Calculate new address
			address += 1 + OPERANDS[opcode]*2;	// 1 Byte OPCode + n*2Bytes param.
			
		}
	}

	printf("Assembling done. Codesize is %d bytes\n", addr_max);
	
	// Nobody likes labels but it _IS_ the cleanest solution for error handling
err_abort:
	fclose(ifp);
	fclose(ofp);

	free_labellist(labellst);
	
	return 0;
}

void usage(void)
{
	printf("MUASM Version %s\nCopyright (C) 2005 by Wiesner Thomas\n\n",
			MU_VER);
	puts("Usage: muasm infile outfile");
	puts("THIS SOFTWARE COMES WITH ABSOLUTELY NO WARRANTY! "
			"USE AT YOUR OWN RISK!");
}

char *readline(FILE *fp, struct line *l)
{
	int i;
	char *ret;

	ret = fgets(l->data, MAXCHAR, fp);

	// Line ends at a comment mark
	for(i = 0; i < MAXCHAR && (l->data)[i] != '\0'; i++) {
		if((l->data)[i] == '#' && i > 0) {
			if((l->data)[i-1] != '\'') {		// Within a character constant?
				(l->data)[i] = '\0';
			}
		}
		if((l->data)[i] == '#' && i == 0) {
			(l->data)[i] = '\0';
		}
	}

	if(ret == NULL)
		return NULL;

	l->len = strlen(l->data);
	l->pos = l->data;

	return ret;
}


// We define, that all instructions start with a tab at the beginning
// of a line.
int is_label(struct line l)
{
	if(l.data[0] == '\t')
		return 0;
	return 1;
}

struct label *new_label(void)
{
	struct label *l;

	l = (struct label *) malloc(sizeof(struct label));

	if(l == NULL)
		return NULL;

	l->next = NULL;
	
	return l;
}

void free_labellist(struct label *l)
{
	struct label *buf, *cur;

	if(l == NULL)
		return;
	
	cur = l;
	do {
		buf = cur->next;
		free(cur);
		cur = buf;
	} while(buf != NULL);
}

int get_labeladdr(struct label *l, char *name)
{
	struct label *cur;

	cur = l;
	do {
		if(cur != NULL) {
			if(strcmp(name, cur->name) == 0) {
				return cur->address;
			}
		}
		cur = cur->next;
	} while(cur != NULL);

	return -1;	// Label not found
}

int set_labeladdr(struct label *l, char *name, int address)
{
	struct label *cur;

	cur = l;
	do {
		cur = cur->next;
		if(cur != NULL) {
			if(strcmp(name, cur->name) == 0) {
				cur->address = address;
				return 0;
			}
		}
	} while(cur != NULL);

	return -1;	// Label not found
}

int add_label(struct label *l, char *name, int line)
{
	struct label *cur;

	cur = l;
	while(cur->next != NULL) {
		if(strcmp(cur->name, name) == 0)
			return DUPLICATE_LABEL;
		cur = cur->next;
	}

	strcpy(cur->name, name);
	cur->line = line;
	
	if((cur->next = new_label()) == NULL)
		return -1;	// No mem left

	return 0;
}

int add_label2(struct label *l, char *name, int address, int line)
{
	struct label *cur;

	cur = l;
	while(cur->next != NULL) {
		if(strcmp(cur->name, name) == 0)
			return DUPLICATE_LABEL;
		cur = cur->next;
	}

	strcpy(cur->name, name);
	cur->line = line;
	cur->address = address;
	
	if((cur->next = new_label()) == NULL)
		return -1;	// No mem left

	return 0;
}

void list_labels(struct label *l)
{
	struct label *cur;

	puts("List of found labels and constants:");
	
	cur = l;
	while(cur->next != NULL) {
		printf("\t%s: line %d, address/value 0x%x\n", cur->name, cur->line,
				cur->address);
		cur = cur->next;
	}
}

// Returns a pointer to the next delimiter
char *next_delim(char *str)
{
	while(*str != '\t' && *str != '\n' && *str != '\r' &&
			*str != '\0')
		str++;
	return str;
}

// Returns a pointer to the first non-delimiter in the string or to '\0'
char *skip_delim(char *str)
{
	while((*str == '\t' || *str == '\n' || *str == '\r') &&
			*str != '\0')
		str++;
	return str;
}

// Find out the type of a constant and get it´s value
int get_const(char *str, char *delim, struct label *l, int line)
{
	int val;
	
	if(*str == '\'') {	// Character constant
		if((val = get_cconst(str, delim)) < 0) {
			printf("Illegal character constant in Line %d.\n", line);
			return ILLEGAL_CONSTANT;
		}
	} else if(*str == '0' && *(str+1) == 'x') {	// Hex constant
		if((val = get_hconst(str, delim)) < 0) {
			printf("Illegal hex constant in Line %d.\n", line);
			return ILLEGAL_CONSTANT;
		}
	} else if(isalpha(*str) || *str == '_') {	// Label
		if((val = get_lconst(str, delim, l)) < 0) {
			printf("Illegal label constant in line %d.\n", line);
			return ILLEGAL_CONSTANT;
		}
	} else if(isdigit(*str)) {
		if((val = get_dconst(str, delim)) < 0) {
			printf("Illegal decimal constant in line %d.\n", line);
			return ILLEGAL_CONSTANT;
		}
	} else {													// Unknown
		printf("Unknown constant in line %d\n", line);
		return ILLEGAL_CONSTANT;
	}

	return val;
}

// Check, parse and return a character constant (e.g. 'a')
// No Escaping is supported.
int get_cconst(char *str, char *delim)
{
	if(*str != '\'')
		return ILLEGAL_CONSTANT;
	if(*(str+2) != '\'')
		return ILLEGAL_CONSTANT;
	if(delim-str != 3)				// Check length
		return ILLEGAL_CONSTANT;
	return (*(str+1));
	
	return 0;
}

// Check, parse and return a hex constant (e.g. 0xfd76)
int get_hconst(char *str, char *delim)
{
	int i, j;
	unsigned short op;
	
	if(*str != '0' && *(str+1) != 'x')
		return ILLEGAL_CONSTANT;
	if(delim-str != 6)				// Check length
		return ILLEGAL_CONSTANT;
	
	str +=2;
	
	op = 0;
	for(i = 0; i < 4; i++) {
		for(j = 0; j < 16 && hexdigit[j] != tolower(*str); j++)
			;
		if(j >= 16)
			return ILLEGAL_CONSTANT;
		op |= j;
		if(i < 3)
			op = op << 4;
		str++;
	}
	
	return op;
}

// Check, parse and return a label constant.
int get_lconst(char *str, char *delim, struct label *l)
{
	int addr;
	char name[MAXCHAR];
	
	if(delim-str < 1)
		return ILLEGAL_CONSTANT;
	
	strncpy(name, str, delim-str);
	name[delim-str] = '\0';
	addr = get_labeladdr(l, name);

	if(addr < 0) {
		puts("Label not found");
		return ILLEGAL_CONSTANT;
	}

	return addr;
}

// Check, parse and return a decimal constant.
int get_dconst(char *str, char *delim)
{
	int val;
	char subs[MAXCHAR];
	
	if(delim-str < 1)
		return ILLEGAL_CONSTANT;
	
	strncpy(subs, str, delim-str);
	subs[delim-str] = '\0';
	val = atoi(subs);

	if(val > 65535)
		puts("Warning: Decimal constant > 65535");

	return val;
}

// Check if a label is valid
int check_label(char *str)
{
	// Labels have to start with a correct character or an underscore
	if(!isalpha(*str) && *str != '_')
		return -1;

	while(*str != '\0') {	// No spaces within a label are allowed
		if(isspace(*str))
			return -1;
		str++;
	}
	return 0;
}

int isnewline(int c)
{
	if(c == '\n' || c == '\r')
		return 1;
	return 0;
}

// vim:ts=3:sw=3
