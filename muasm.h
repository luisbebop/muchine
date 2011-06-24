/*****************************************************************************
 *  Header file needed for MUASM                                             *
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


#ifndef _MUASM_H
#define _MUASM_H	1

#define TRUE		1
#define FALSE		0

#define MAXCHAR	80		// Maximum number of chars per line

#define NO_MEM					(-1)
#define DUPLICATE_LABEL		(-2)
#define ILLEGAL_CONSTANT	(-3)

struct line {
	char data[MAXCHAR];
	char *pos;
	int len;
};

struct label {
	char name[MAXCHAR];
	int address;
	struct label *next;
	int line;
};

const char hexdigit[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		'a', 'b', 'c', 'd', 'e', 'f'};

void usage(void);
char *readline(FILE *fp, struct line *l);
int is_label(struct line l);
struct label *new_label(void);
void free_labellist(struct label *l);
int get_labeladdr(struct label *l, char *name);
int set_labeladdr(struct label *l, char *name, int address);
int add_label(struct label *l, char *name, int line);
int add_label2(struct label *l, char *name, int address, int line);
void list_labels(struct label *l);
char *next_delim(char *str);
char *skip_delim(char *str);
int get_const(char *str, char *delim, struct label *l, int line);
int get_cconst(char *str, char *delim);
int get_hconst(char *str, char *delim);
int get_lconst(char *str, char *delim, struct label *l);
int get_dconst(char *str, char *delim);
int check_label(char *str);
int isnewline(int c);
#endif

// vim:ts=3:sw=3
