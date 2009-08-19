/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// this file is shared by quake and qcc

#ifndef PR_COMP_H
#define PR_COMP_H

typedef unsigned int	func_t;
typedef int	string_t;

typedef enum etype_e {ev_void, ev_string, ev_float, ev_vector, ev_entity, ev_field, ev_function, ev_pointer} etype_t;


#define	OFS_NULL		0
#define	OFS_RETURN		1
#define	OFS_PARM0		4		// leave 3 ofs for each parm to hold vectors
#define	OFS_PARM1		7
#define	OFS_PARM2		10
#define	OFS_PARM3		13
#define	OFS_PARM4		16
#define	OFS_PARM5		19
#define	OFS_PARM6		22
#define	OFS_PARM7		25
#define	RESERVED_OFS	28


enum opcode_e
{
	OP_DONE,
	OP_MUL_F,
	OP_MUL_V,
	OP_MUL_FV,
	OP_MUL_VF,
	OP_DIV_F,
	OP_ADD_F,
	OP_ADD_V,
	OP_SUB_F,
	OP_SUB_V,

	OP_EQ_F,
	OP_EQ_V,
	OP_EQ_S,
	OP_EQ_E,
	OP_EQ_FNC,

	OP_NE_F,
	OP_NE_V,
	OP_NE_S,
	OP_NE_E,
	OP_NE_FNC,

	OP_LE,
	OP_GE,
	OP_LT,
	OP_GT,

	OP_LOAD_F,
	OP_LOAD_V,
	OP_LOAD_S,
	OP_LOAD_ENT,
	OP_LOAD_FLD,
	OP_LOAD_FNC,

	OP_ADDRESS,

	OP_STORE_F,
	OP_STORE_V,
	OP_STORE_S,
	OP_STORE_ENT,
	OP_STORE_FLD,
	OP_STORE_FNC,

	OP_STOREP_F,
	OP_STOREP_V,
	OP_STOREP_S,
	OP_STOREP_ENT,
	OP_STOREP_FLD,
	OP_STOREP_FNC,

	OP_RETURN,
	OP_NOT_F,
	OP_NOT_V,
	OP_NOT_S,
	OP_NOT_ENT,
	OP_NOT_FNC,
	OP_IF,
	OP_IFNOT,
	OP_CALL0,
	OP_CALL1,
	OP_CALL2,
	OP_CALL3,
	OP_CALL4,
	OP_CALL5,
	OP_CALL6,
	OP_CALL7,
	OP_CALL8,
	OP_STATE,
	OP_GOTO,
	OP_AND,
	OP_OR,

	OP_BITAND,
	OP_BITOR,




	//these following ones are Hexen 2 constants.

	OP_MULSTORE_F,
	OP_MULSTORE_V,
	OP_MULSTOREP_F,
	OP_MULSTOREP_V,

	OP_DIVSTORE_F,	//70
	OP_DIVSTOREP_F,

	OP_ADDSTORE_F,
	OP_ADDSTORE_V,
	OP_ADDSTOREP_F,
	OP_ADDSTOREP_V,

	OP_SUBSTORE_F,
	OP_SUBSTORE_V,
	OP_SUBSTOREP_F,
	OP_SUBSTOREP_V,

	OP_FETCH_GBL_F,	//80
	OP_FETCH_GBL_V,
	OP_FETCH_GBL_S,
	OP_FETCH_GBL_E,
	OP_FETCH_GBL_FNC,

	OP_CSTATE,
	OP_CWSTATE,

	OP_THINKTIME,

	OP_BITSET,
	OP_BITSETP,
	OP_BITCLR,		//90
	OP_BITCLRP,

	OP_RAND0,
	OP_RAND1,
	OP_RAND2,
	OP_RANDV0,
	OP_RANDV1,
	OP_RANDV2,

	OP_SWITCH_F,
	OP_SWITCH_V,
	OP_SWITCH_S,	//100
	OP_SWITCH_E,
	OP_SWITCH_FNC,

	OP_CASE,
	OP_CASERANGE,





	//the rest are added
	//mostly they are various different ways of adding two vars with conversions.

	OP_CALL1H,
	OP_CALL2H,
	OP_CALL3H,
	OP_CALL4H,
	OP_CALL5H,
	OP_CALL6H,		//110
	OP_CALL7H,
	OP_CALL8H,


	OP_STORE_I,
	OP_STORE_IF,
	OP_STORE_FI,

	OP_ADD_I,
	OP_ADD_FI,
	OP_ADD_IF,		//110

	OP_SUB_I,
	OP_SUB_FI,
	OP_SUB_IF,

	OP_CONV_ITOF,
	OP_CONV_FTOI,
	OP_CP_ITOF,
	OP_CP_FTOI,
	OP_LOAD_I,
	OP_STOREP_I,
	OP_STOREP_IF,	//120
	OP_STOREP_FI,

	OP_BITAND_I,
	OP_BITOR_I,

	OP_MUL_I,
	OP_DIV_I,
	OP_EQ_I,
	OP_NE_I,

	OP_IFNOTS,
	OP_IFS,

	OP_NOT_I,		//130

	OP_DIV_VF,

	OP_XOR_I,
	OP_RSHIFT_I,
	OP_LSHIFT_I,

	OP_GLOBALADDRESS,
	OP_POINTER_ADD,	//32 bit pointers

	OP_LOADA_F,
	OP_LOADA_V,
	OP_LOADA_S,
	OP_LOADA_ENT,	//140
	OP_LOADA_FLD,
	OP_LOADA_FNC,
	OP_LOADA_I,

	OP_STORE_P,
	OP_LOAD_P,

	OP_LOADP_F,
	OP_LOADP_V,
	OP_LOADP_S,
	OP_LOADP_ENT,
	OP_LOADP_FLD,	//150
	OP_LOADP_FNC,
	OP_LOADP_I,

	OP_LE_I,
	OP_GE_I,
	OP_LT_I,
	OP_GT_I,

	OP_LE_IF,
	OP_GE_IF,
	OP_LT_IF,
	OP_GT_IF,		//160

	OP_LE_FI,
	OP_GE_FI,
	OP_LT_FI,
	OP_GT_FI,

	OP_EQ_IF,
	OP_EQ_FI,

	//-------------------------------------
	//string manipulation.
	OP_ADD_SF,	//(char*)c = (char*)a + (float)b
	OP_SUB_S,	//(float)c = (char*)a - (char*)b
	OP_STOREP_C,//(float)c = *(char*)b = (float)a
	OP_LOADP_C,	//(float)c = *(char*)					//170
	//-------------------------------------


	OP_MUL_IF,
	OP_MUL_FI,
	OP_MUL_VI,
	OP_MUL_IV,
	OP_DIV_IF,
	OP_DIV_FI,
	OP_BITAND_IF,
	OP_BITOR_IF,
	OP_BITAND_FI,
	OP_BITOR_FI,		//180
	OP_AND_I,
	OP_OR_I,
	OP_AND_IF,
	OP_OR_IF,
	OP_AND_FI,
	OP_OR_FI,
	OP_NE_IF,
	OP_NE_FI,

//erm... FTEQCC doesn't make use of these... These are for DP.
	OP_GSTOREP_I,
	OP_GSTOREP_F,		//190
	OP_GSTOREP_ENT,
	OP_GSTOREP_FLD,		// integers
	OP_GSTOREP_S,
	OP_GSTOREP_FNC,		// pointers
	OP_GSTOREP_V,
	OP_GADDRESS,
	OP_GLOAD_I,
	OP_GLOAD_F,
	OP_GLOAD_FLD,
	OP_GLOAD_ENT,		//200
	OP_GLOAD_S,
	OP_GLOAD_FNC,
	OP_BOUNDCHECK,

//back to ones that we do use.
	OP_STOREP_P,
	OP_PUSH,	//push 4octets onto the local-stack (which is ALWAYS poped on function return). Returns a pointer.
	OP_POP,		//pop those ones that were pushed (don't over do it). Needs assembler.

	OP_SWITCH_I,
	OP_GLOAD_V,

//extensions for fixing stuff
	OP_IF_I,
	OP_IFNOT_I,
	
	OP_NUMOPS
};


typedef struct statement_s
{
	unsigned short	op;
	signed short	a,b,c;
}
dstatement_t;

typedef struct ddef_s
{
	unsigned short	type;		// if DEF_SAVEGLOBGAL bit is set
								// the variable needs to be saved in savegames
	unsigned short	ofs;
	int			s_name;
}
ddef_t;
#define	DEF_SAVEGLOBAL	(1<<15)

#define	MAX_PARMS	8

typedef struct dfunction_s
{
	int		first_statement;	// negative numbers are builtins
	int		parm_start;
	int		locals;				// total ints of parms + locals

	int		profile;		// runtime

	int		s_name;
	int		s_file;			// source file defined in

	int		numparms;
	unsigned char	parm_size[MAX_PARMS];
}
dfunction_t;

typedef struct mfunction_s
{
	int		first_statement;	// negative numbers are builtins
	int		parm_start;
	int		locals;				// total ints of parms + locals

	// these are doubles so that they can count up to 54bits or so rather than 32bit
	double	profile;		// runtime
	double	builtinsprofile; // cost of builtin functions called by this function
	double	callcount; // times the functions has been called since the last profile call
	double  totaltime; // total execution time of this function DIRECTLY FROM THE ENGINE

	int		s_name;
	int		s_file;			// source file defined in

	int		numparms;
	unsigned char	parm_size[MAX_PARMS];
}
mfunction_t;


#define	PROG_VERSION	6
#define	PROG_EXTENDED	7
typedef struct dprograms_s
{
	int		version;
	int		crc;			// check of header file

	int		ofs_statements;
	int		numstatements;	// statement 0 is an error

	int		ofs_globaldefs;
	int		numglobaldefs;

	int		ofs_fielddefs;
	int		numfielddefs;

	int		ofs_functions;
	int		numfunctions;	// function 0 is an empty

	int		ofs_strings;
	int		numstrings;		// first string is a null string

	int		ofs_globals;
	int		numglobals;

	int		entityfields;
}
dprograms_t;

#endif

