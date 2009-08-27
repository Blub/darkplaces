#undef RUNAWAYCHECK
#undef GLOBALSIZE
#undef PTR_GBL
#undef PTR_FLD
#undef PTR_STR
#undef PTR_MEM
#undef PTR_ISGBL
#undef PTR_ISFLD
#undef PTR_ISSTR
#undef PTR_ISMEM
#undef PTR_VALUE
#undef PTR_ISVALID
#undef PTR_ptr
#undef PTR_ptr2
#undef PTR_ptr3

#if PRVMRUNAWAYCHECK
#  define RUNAWAYCHECK()						\
	do {								\
		if (++jumpcount == 10000000)				\
		{							\
			prog->xstatement = st - prog->statements;	\
			PRVM_Profile(1<<30, 1000000);			\
			PRVM_ERROR("%s runaway loop counter hit limit of %d jumps\ntip: read above for list of most-executed functions", PRVM_NAME, jumpcount);	\
		}							\
	} while(0)
#else
#    define RUNAWAYCHECK()
#endif
#define GLOBALSIZE ((signed)sizeof(*prog->globals.generic)*prog->progs->numglobals)

// TODO: (64bit)
// 0x3fffffffffffffff
// ~(3 << 62)
#define PTR_VALUE(x) ((unsigned int)(x) & 0x3FFFFFFF)

// Pointer:
// [type: 2 bits] [value: 30 bits]
// types:
//   0  -  Entity field area
//   1  -  Global area
//   2  -  Malloc()ed area

// TODO: (64 bit)
// s/30/62/ in the following lines:
#define PTR_FLD(x)   (PTR_VALUE(x))
#define PTR_ISFLD(x) ( ((x)>>30) == 0 )

#define PTR_GBL(x)   (PTR_VALUE(x) | (1<<30))
#define PTR_ISGBL(x) ( ((x)>>30) == 1 )

#define PTR_MEM(x)   (PTR_VALUE(x) | (2<<30))
#define PTR_ISMEM(x) ( ((x)>>30) == 2 )

#define PTR_size (signed)sizeof(int)

#define PTR_ISVALID(x)							\
	(								\
	ptrvalA = ((int *)(x) - (int *)prog->edictsfields),		\
	ptrvalB = ((int *)(x) - (int *)prog->globals.generic),		\
	ptrvalC = ((int *)(x) - (int *)/* TODO: fill in memory area*/ 0), \
	(ptrvalA >= 0 && ptrvalA + PTR_size <= prog->edictareasize) ? 1 : \
	(ptrvalB >= 0 && ptrvalB + PTR_size <= GLOBALSIZE) ? 1 :	\
	(ptrvalC >= 0 && ptrvalC + PTR_size <= /* TODO: fill in memory area size */0) ? 1 : \
	0 )

#if PRVMBOUNDSCHECK
#define PTR_ptr3(from, off, access)					\
	if (PTR_ISGBL(from))						\
	{								\
		ptrval_t p = PTR_VALUE(from) + (off);			\
		if (p < 0 || p + (access)*PTR_size > GLOBALSIZE) \
		{							\
			prog->xfunction->profile += (st - startst);	\
			prog->xstatement = st - prog->statements;	\
			PRVM_ERROR("%s attempted to write to an out of bounds global (%i)", PRVM_NAME, (int)p);	\
			goto cleanup;					\
		}							\
		ptr = (prvm_eval_t*)((int *)prog->globals.generic + p);	\
	}								\
	else if (PTR_ISMEM(from))					\
	{								\
		ptrval_t p = PTR_VALUE(from) + (off);			\
		if (p < 0 || p + (access)*PTR_size > 0 /* TODO: FILL IN */) \
		{							\
			prog->xfunction->profile += (st - startst);	\
			prog->xstatement = st - prog->statements;	\
			PRVM_ERROR("%s attempted to write to out of bounds memory (%i)", PRVM_NAME, (int)p); \
			goto cleanup;					\
		}							\
		ptr = (prvm_eval_t*)((int *)0 /* TODO: FILL IN*/ + p /* TODO: REMOVE: */ * 0); \
	}								\
	else								\
	{								\
		ptrval_t p = PTR_VALUE(from) + (off);			\
		if (p < 0 || p + (access)*PTR_size > prog->edictareasize) \
		{							\
			prog->xfunction->profile += (st - startst);	\
			prog->xstatement = st - prog->statements;	\
			PRVM_ERROR("%s attempted to write to an out of bounds edict field (%i)", PRVM_NAME, (int)p); \
			goto cleanup;					\
		}							\
		ptr = (prvm_eval_t*)((int *)prog->edictsfields + p);	\
	}
#else

#define PTR_ptr3(from, off, access)						\
	if (PTR_ISGBL(from))						\
	{								\
		ptrval_t p = PTR_VALUE(from) + (off);			\
		ptr = (prvm_eval_t *)((int*)prog->globals.generic + p);	\
	}								\
	else if (PTR_ISMEM(from))					\
	{								\
		ptrval_t p = PTR_VALUE(from) + (off);			\
		ptr = (prvm_eval_t *)((int*)0 /* TODO: FILL IN*/ + p /* TODO: REMOVE: */ * 0); \
	}								\
	else								\
	{								\
		ptrval_t p = PTR_VALUE(from) + (off);			\
		ptr = (prvm_eval_t *)((int*)prog->edictsfields + p);	\
	}
#endif
#define PTR_ptr2(from, off) PTR_ptr3(from, off, 1)
#define PTR_ptr(from) PTR_ptr3(from, 0, 1)

typedef long ptrval_t;

// This code isn't #ifdef/#define protectable, don't try.
prvm_eval_t *swtch = NULL;
int swtchtype = 0;
ptrval_t ptrvalA, ptrvalB, ptrvalC;
ptrvalA = 0; // get rid of the unused-warning for now
ptrvalB = 0;
ptrvalC = 0;
		while (1)
		{
			st++;

#if PRVMTRACE
			PRVM_PrintStatement(st);
#endif
#if PRVMSTATEMENTPROFILING
			prog->statement_profile[st - prog->statements]++;
#endif

			switch (st->op)
			{
			case OP_ADD_F:
				OPC->_float = OPA->_float + OPB->_float;
				break;
			case OP_ADD_V:
				OPC->vector[0] = OPA->vector[0] + OPB->vector[0];
				OPC->vector[1] = OPA->vector[1] + OPB->vector[1];
				OPC->vector[2] = OPA->vector[2] + OPB->vector[2];
				break;
			case OP_SUB_F:
				OPC->_float = OPA->_float - OPB->_float;
				break;
			case OP_SUB_V:
				OPC->vector[0] = OPA->vector[0] - OPB->vector[0];
				OPC->vector[1] = OPA->vector[1] - OPB->vector[1];
				OPC->vector[2] = OPA->vector[2] - OPB->vector[2];
				break;
			case OP_MUL_F:
				OPC->_float = OPA->_float * OPB->_float;
				break;
			case OP_MUL_V:
				OPC->_float = OPA->vector[0]*OPB->vector[0] + OPA->vector[1]*OPB->vector[1] + OPA->vector[2]*OPB->vector[2];
				break;
			case OP_MUL_FV:
				OPC->vector[0] = OPA->_float * OPB->vector[0];
				OPC->vector[1] = OPA->_float * OPB->vector[1];
				OPC->vector[2] = OPA->_float * OPB->vector[2];
				break;
			case OP_MUL_VF:
				OPC->vector[0] = OPB->_float * OPA->vector[0];
				OPC->vector[1] = OPB->_float * OPA->vector[1];
				OPC->vector[2] = OPB->_float * OPA->vector[2];
				break;
			case OP_DIV_F:
				if( OPB->_float != 0.0f )
				{
					OPC->_float = OPA->_float / OPB->_float;
				}
				else
				{
					if( developer.integer >= 1 )
					{
						prog->xfunction->profile += (st - startst);
						startst = st;
						prog->xstatement = st - prog->statements;
						VM_Warning( "Attempted division by zero in %s\n", PRVM_NAME );
					}
					OPC->_float = 0.0f;
				}
				break;
			case OP_BITAND:
				OPC->_float = (int)OPA->_float & (int)OPB->_float;
				break;
			case OP_BITOR:
				OPC->_float = (int)OPA->_float | (int)OPB->_float;
				break;
			case OP_GE:
				OPC->_float = OPA->_float >= OPB->_float;
				break;
			case OP_LE:
				OPC->_float = OPA->_float <= OPB->_float;
				break;
			case OP_GT:
				OPC->_float = OPA->_float > OPB->_float;
				break;
			case OP_LT:
				OPC->_float = OPA->_float < OPB->_float;
				break;
			case OP_AND:
				OPC->_float = FLOAT_IS_TRUE_FOR_INT(OPA->_int) && FLOAT_IS_TRUE_FOR_INT(OPB->_int); // TODO change this back to float, and add AND_I to be used by fteqcc for anything not a float
				break;
			case OP_OR:
				OPC->_float = FLOAT_IS_TRUE_FOR_INT(OPA->_int) || FLOAT_IS_TRUE_FOR_INT(OPB->_int); // TODO change this back to float, and add OR_I to be used by fteqcc for anything not a float
				break;
			case OP_NOT_F:
				OPC->_float = !FLOAT_IS_TRUE_FOR_INT(OPA->_int);
				break;
			case OP_NOT_V:
				OPC->_float = !OPA->vector[0] && !OPA->vector[1] && !OPA->vector[2];
				break;
			case OP_NOT_S:
				OPC->_float = !OPA->string || !*PRVM_GetString(OPA->string);
				break;
			case OP_NOT_FNC:
				OPC->_float = !OPA->function;
				break;
			case OP_NOT_ENT:
				OPC->_float = (OPA->edict == 0);
				break;
			case OP_EQ_F:
				OPC->_float = OPA->_float == OPB->_float;
				break;
			case OP_EQ_V:
				OPC->_float = (OPA->vector[0] == OPB->vector[0]) && (OPA->vector[1] == OPB->vector[1]) && (OPA->vector[2] == OPB->vector[2]);
				break;
			case OP_EQ_S:
				OPC->_float = !strcmp(PRVM_GetString(OPA->string),PRVM_GetString(OPB->string));
				break;
			case OP_EQ_E:
				OPC->_float = OPA->_int == OPB->_int;
				break;
			case OP_EQ_FNC:
				OPC->_float = OPA->function == OPB->function;
				break;
			case OP_NE_F:
				OPC->_float = OPA->_float != OPB->_float;
				break;
			case OP_NE_V:
				OPC->_float = (OPA->vector[0] != OPB->vector[0]) || (OPA->vector[1] != OPB->vector[1]) || (OPA->vector[2] != OPB->vector[2]);
				break;
			case OP_NE_S:
				OPC->_float = strcmp(PRVM_GetString(OPA->string),PRVM_GetString(OPB->string));
				break;
			case OP_NE_E:
				OPC->_float = OPA->_int != OPB->_int;
				break;
			case OP_NE_FNC:
				OPC->_float = OPA->function != OPB->function;
				break;

		//==================
			case OP_STORE_F:
			case OP_STORE_ENT:
			case OP_STORE_FLD:		// integers
			case OP_STORE_S:
			case OP_STORE_FNC:		// pointers
			case OP_STORE_P:
				OPB->_int = OPA->_int;
				break;
			case OP_STORE_V:
				OPB->ivector[0] = OPA->ivector[0];
				OPB->ivector[1] = OPA->ivector[1];
				OPB->ivector[2] = OPA->ivector[2];
				break;

			case OP_STOREP_F:
			case OP_STOREP_ENT:
			case OP_STOREP_FLD:		// integers
			case OP_STOREP_S:
			case OP_STOREP_FNC:		// pointers
			case OP_STOREP_P:
				PTR_ptr(OPB->_int);
				ptr->_int = OPA->_int;
				break;
			case OP_STOREP_V:
				PTR_ptr3(OPB->_int, 0, 3);
				ptr->ivector[0] = OPA->ivector[0];
				ptr->ivector[1] = OPA->ivector[1];
				ptr->ivector[2] = OPA->ivector[2];
				break;

			case OP_ADDRESS:
#if PRVMBOUNDSCHECK
				if (OPA->edict < 0 || OPA->edict >= prog->max_edicts)
				{
					prog->xfunction->profile += (st - startst);
					prog->xstatement = st - prog->statements;
					PRVM_ERROR ("%s Progs attempted to address an out of bounds edict number", PRVM_NAME);
					goto cleanup;
				}
				if ((unsigned int)(OPB->_int) >= (unsigned int)(prog->progs->entityfields))
				{
					prog->xfunction->profile += (st - startst);
					prog->xstatement = st - prog->statements;
					PRVM_ERROR("%s attempted to address an invalid field (%i) in an edict", PRVM_NAME, OPB->_int);
					goto cleanup;
				}
#endif
				if (OPA->edict == 0 && !prog->allowworldwrites)
				{
					prog->xfunction->profile += (st - startst);
					prog->xstatement = st - prog->statements;
					PRVM_ERROR("forbidden assignment to null/world entity in %s", PRVM_NAME);
					goto cleanup;
				}
				ed = PRVM_PROG_TO_EDICT(OPA->edict);
				OPC->_int = PTR_FLD(((int *)ed->fields.vp + OPB->_int) - (int *)prog->edictsfields);
				break;

			case OP_LOAD_F:
			case OP_LOAD_FLD:
			case OP_LOAD_ENT:
			case OP_LOAD_S:
			case OP_LOAD_FNC:
			case OP_LOAD_P:
#if PRVMBOUNDSCHECK
				if (OPA->edict < 0 || OPA->edict >= prog->max_edicts)
				{
					prog->xfunction->profile += (st - startst);
					prog->xstatement = st - prog->statements;
					PRVM_ERROR ("%s Progs attempted to read an out of bounds edict number", PRVM_NAME);
					goto cleanup;
				}
				if ((unsigned int)(OPB->_int) >= (unsigned int)(prog->progs->entityfields))
				{
					prog->xfunction->profile += (st - startst);
					prog->xstatement = st - prog->statements;
					PRVM_ERROR("%s attempted to read an invalid field in an edict (%i)", PRVM_NAME, OPB->_int);
					goto cleanup;
				}
#endif
				ed = PRVM_PROG_TO_EDICT(OPA->edict);
				OPC->_int = ((prvm_eval_t *)((int *)ed->fields.vp + OPB->_int))->_int;
				break;

			case OP_LOAD_V:
#if PRVMBOUNDSCHECK
				if (OPA->edict < 0 || OPA->edict >= prog->max_edicts)
				{
					prog->xfunction->profile += (st - startst);
					prog->xstatement = st - prog->statements;
					PRVM_ERROR ("%s Progs attempted to read an out of bounds edict number", PRVM_NAME);
					goto cleanup;
				}
				if (OPB->_int < 0 || OPB->_int + 2 >= prog->progs->entityfields)
				{
					prog->xfunction->profile += (st - startst);
					prog->xstatement = st - prog->statements;
					PRVM_ERROR("%s attempted to read an invalid field in an edict (%i)", PRVM_NAME, OPB->_int);
					goto cleanup;
				}
#endif
				ed = PRVM_PROG_TO_EDICT(OPA->edict);
				OPC->ivector[0] = ((prvm_eval_t *)((int *)ed->fields.vp + OPB->_int))->ivector[0];
				OPC->ivector[1] = ((prvm_eval_t *)((int *)ed->fields.vp + OPB->_int))->ivector[1];
				OPC->ivector[2] = ((prvm_eval_t *)((int *)ed->fields.vp + OPB->_int))->ivector[2];
				break;

		//==================

			case OP_IFNOT:
				if(!FLOAT_IS_TRUE_FOR_INT(OPA->_int))
				// TODO add an "int-if", and change this one to OPA->_float
				// although mostly unneeded, thanks to the only float being false being 0x0 and 0x80000000 (negative zero)
				// and entity, string, field values can never have that value
				{
					prog->xfunction->profile += (st - startst);
					st += st->b - 1;	// offset the s++
					startst = st;
					
					// no bounds check needed, it is done when loading progs
					RUNAWAYCHECK();
				}
				break;

			case OP_IF:
				if(FLOAT_IS_TRUE_FOR_INT(OPA->_int))
				// TODO add an "int-if", and change this one, as well as the FLOAT_IS_TRUE_FOR_INT usages, to OPA->_float
				// although mostly unneeded, thanks to the only float being false being 0x0 and 0x80000000 (negative zero)
				// and entity, string, field values can never have that value
				{
					prog->xfunction->profile += (st - startst);
					st += st->b - 1;	// offset the s++
					startst = st;
					RUNAWAYCHECK();
				}
				break;

			case OP_IFNOT_I:
				if (!OPA->_int)
				{
					prog->xfunction->profile += (st - startst);
					st += st->b - 1;	// offset the s++
					startst = st;
					RUNAWAYCHECK();
				}
				break;

			case OP_IF_I:
				if (OPA->_int)
				{
					prog->xfunction->profile += (st - startst);
					st += st->b - 1;	// offset the s++
					startst = st;
					// no bounds check needed, it is done when loading progs
					RUNAWAYCHECK();

				}
				break;

			case OP_GOTO:
				prog->xfunction->profile += (st - startst);
				st += st->a - 1;	// offset the s++
				startst = st;
				// no bounds check needed, it is done when loading progs
				RUNAWAYCHECK();
				break;

				// CALLxH used and tested, they do work!
			case OP_CALL8H:
			case OP_CALL7H:
			case OP_CALL6H:
			case OP_CALL5H:
			case OP_CALL4H:
			case OP_CALL3H:
			case OP_CALL2H:
				PRVM_G_VECTOR(OFS_PARM1)[0] = OPC->vector[0];
				PRVM_G_VECTOR(OFS_PARM1)[1] = OPC->vector[1];
				PRVM_G_VECTOR(OFS_PARM1)[2] = OPC->vector[2];
			case OP_CALL1H:
				PRVM_G_VECTOR(OFS_PARM0)[0] = OPB->vector[0];
				PRVM_G_VECTOR(OFS_PARM0)[1] = OPB->vector[1];
				PRVM_G_VECTOR(OFS_PARM0)[2] = OPB->vector[2];
				// fall through
			case OP_CALL0:
			case OP_CALL1:
			case OP_CALL2:
			case OP_CALL3:
			case OP_CALL4:
			case OP_CALL5:
			case OP_CALL6:
			case OP_CALL7:
			case OP_CALL8:
				prog->xfunction->profile += (st - startst);
				startst = st;
				prog->xstatement = st - prog->statements;
				// handle CALLxH too
				if (st->op >= OP_CALL1H && st->op <= OP_CALL8H)
					prog->argc = st->op - (OP_CALL1H-1);
				else
					prog->argc = st->op - OP_CALL0;
				if (!OPA->function)
					PRVM_ERROR("NULL function in %s", PRVM_NAME);

#if PRVMBOUNDSCHECK
				if(!OPA->function || OPA->function >= (unsigned int)prog->progs->numfunctions)
				{
					prog->xfunction->profile += (st - startst);
					prog->xstatement = st - prog->statements; // we better stay on the previously executed statement
					PRVM_ERROR("%s CALL outside the program", PRVM_NAME);
					goto cleanup;
				}
#endif

				newf = &prog->functions[OPA->function];
				newf->callcount++;

				if (newf->first_statement < 0)
				{
					// negative statements are built in functions
					int builtinnumber = -newf->first_statement;
					prog->xfunction->builtinsprofile++;
					if (builtinnumber < prog->numbuiltins && prog->builtins[builtinnumber])
						prog->builtins[builtinnumber]();
					else
						PRVM_ERROR("No such builtin #%i in %s; most likely cause: outdated engine build. Try updating!", builtinnumber, PRVM_NAME);
				}
				else
					st = prog->statements + PRVM_EnterFunction(newf);
				startst = st;
				break;

			case OP_DONE:
			case OP_RETURN:
				prog->xfunction->profile += (st - startst);
				prog->xstatement = st - prog->statements;

				prog->globals.generic[OFS_RETURN] = prog->globals.generic[(unsigned short) st->a];
				prog->globals.generic[OFS_RETURN+1] = prog->globals.generic[(unsigned short) st->a+1];
				prog->globals.generic[OFS_RETURN+2] = prog->globals.generic[(unsigned short) st->a+2];

				st = prog->statements + PRVM_LeaveFunction();
				startst = st;
				if (prog->depth <= exitdepth)
					goto cleanup; // all done
				if (prog->trace != cachedpr_trace)
					goto chooseexecprogram;
				break;

			case OP_STATE:
				if(prog->flag & PRVM_OP_STATE)
				{
					ed = PRVM_PROG_TO_EDICT(PRVM_GLOBALFIELDVALUE(prog->globaloffsets.self)->edict);
					PRVM_EDICTFIELDVALUE(ed,prog->fieldoffsets.nextthink)->_float = PRVM_GLOBALFIELDVALUE(prog->globaloffsets.time)->_float + 0.1;
					PRVM_EDICTFIELDVALUE(ed,prog->fieldoffsets.frame)->_float = OPA->_float;
					PRVM_EDICTFIELDVALUE(ed,prog->fieldoffsets.think)->function = OPB->function;
				}
				else
				{
					prog->xfunction->profile += (st - startst);
					prog->xstatement = st - prog->statements;
					PRVM_ERROR("OP_STATE not supported by %s", PRVM_NAME);
				}
				break;

// LordHavoc: to be enabled when Progs version 7 (or whatever it will be numbered) is finalized

			case OP_ADD_I:
				OPC->_int = OPA->_int + OPB->_int;
				break;
			case OP_ADD_IF:
				OPC->_int = OPA->_int + (int) OPB->_float;
				break;
			case OP_ADD_FI:
				OPC->_float = OPA->_float + (float) OPB->_int;
				break;
			case OP_SUB_I:
				OPC->_int = OPA->_int - OPB->_int;
				break;
			case OP_SUB_IF:
				OPC->_int = OPA->_int - (int) OPB->_float;
				break;
			case OP_SUB_FI:
				OPC->_float = OPA->_float - (float) OPB->_int;
				break;
			case OP_MUL_I:
				OPC->_int = OPA->_int * OPB->_int;
				break;
			case OP_MUL_IF:
				OPC->_int = OPA->_int * (int) OPB->_float;
				break;
			case OP_MUL_FI:
				OPC->_float = OPA->_float * (float) OPB->_int;
				break;
			case OP_MUL_VI:
				OPC->vector[0] = (float) OPB->_int * OPA->vector[0];
				OPC->vector[1] = (float) OPB->_int * OPA->vector[1];
				OPC->vector[2] = (float) OPB->_int * OPA->vector[2];
				break;
			case OP_DIV_VF:
				{
					float temp = 1.0f / OPB->_float;
					OPC->vector[0] = temp * OPA->vector[0];
					OPC->vector[1] = temp * OPA->vector[1];
					OPC->vector[2] = temp * OPA->vector[2];
				}
				break;
			case OP_DIV_I:
				OPC->_int = OPA->_int / OPB->_int;
				break;
			case OP_DIV_IF:
				OPC->_int = OPA->_int / (int) OPB->_float;
				break;
			case OP_DIV_FI:
				OPC->_float = OPA->_float / (float) OPB->_int;
				break;
			case OP_STORE_IF:
				OPB->_float = OPA->_int;
				break;
			case OP_STORE_FI:
				OPB->_int = OPA->_float;
				break;
			case OP_BITAND_I:
				OPC->_int = OPA->_int & OPB->_int;
				break;
			case OP_BITOR_I:
				OPC->_int = OPA->_int | OPB->_int;
				break;
			case OP_BITAND_IF:
				OPC->_int = OPA->_int & (int)OPB->_float;
				break;
			case OP_BITOR_IF:
				OPC->_int = OPA->_int | (int)OPB->_float;
				break;
			case OP_BITAND_FI:
				OPC->_float = (int)OPA->_float & OPB->_int;
				break;
			case OP_BITOR_FI:
				OPC->_float = (int)OPA->_float | OPB->_int;
				break;
			case OP_GE_I:
				OPC->_float = OPA->_int >= OPB->_int;
				break;
			case OP_LE_I:
				OPC->_float = OPA->_int <= OPB->_int;
				break;
			case OP_GT_I:
				OPC->_float = OPA->_int > OPB->_int;
				break;
			case OP_LT_I:
				OPC->_float = OPA->_int < OPB->_int;
				break;
			case OP_AND_I:
				OPC->_float = OPA->_int && OPB->_int;
				break;
			case OP_OR_I:
				OPC->_float = OPA->_int || OPB->_int;
				break;
			case OP_GE_IF:
				OPC->_float = (float)OPA->_int >= OPB->_float;
				break;
			case OP_LE_IF:
				OPC->_float = (float)OPA->_int <= OPB->_float;
				break;
			case OP_GT_IF:
				OPC->_float = (float)OPA->_int > OPB->_float;
				break;
			case OP_LT_IF:
				OPC->_float = (float)OPA->_int < OPB->_float;
				break;
			case OP_AND_IF:
				OPC->_float = (float)OPA->_int && OPB->_float;
				break;
			case OP_OR_IF:
				OPC->_float = (float)OPA->_int || OPB->_float;
				break;
			case OP_GE_FI:
				OPC->_float = OPA->_float >= (float)OPB->_int;
				break;
			case OP_LE_FI:
				OPC->_float = OPA->_float <= (float)OPB->_int;
				break;
			case OP_GT_FI:
				OPC->_float = OPA->_float > (float)OPB->_int;
				break;
			case OP_LT_FI:
				OPC->_float = OPA->_float < (float)OPB->_int;
				break;
			case OP_AND_FI:
				OPC->_float = OPA->_float && (float)OPB->_int;
				break;
			case OP_OR_FI:
				OPC->_float = OPA->_float || (float)OPB->_int;
				break;
			case OP_NOT_I:
				OPC->_float = !OPA->_int;
				break;
			case OP_EQ_I:
				OPC->_float = OPA->_int == OPB->_int;
				break;
			case OP_EQ_IF:
				OPC->_float = (float)OPA->_int == OPB->_float;
				break;
			case OP_EQ_FI:
				OPC->_float = OPA->_float == (float)OPB->_int;
				break;
			case OP_NE_I:
				OPC->_float = OPA->_int != OPB->_int;
				break;
			case OP_NE_IF:
				OPC->_float = (float)OPA->_int != OPB->_float;
				break;
			case OP_NE_FI:
				OPC->_float = OPA->_float != (float)OPB->_int;
				break;
			case OP_STORE_I:
				OPB->_int = OPA->_int;
				break;
			case OP_STOREP_I:
				PTR_ptr(OPB->_int);
				ptr->_int = OPA->_int;
				break;
			case OP_LOAD_I:
#if PRBOUNDSCHECK
				if (OPA->edict < 0 || OPA->edict >= prog->max_edicts)
				{
					prog->xfunction->profile += (st - startst);
					prog->xstatement = st - prog->statements;
					PRVM_ERROR ("%s Progs attempted to read an out of bounds edict number", PRVM_NAME);
					goto cleanup;
				}
				if (OPB->_int < 0 || OPB->_int >= prog->progs->entityfields)
				{
					prog->xfunction->profile += (st - startst);
					prog->xstatement = st - prog->statements;
					PRVM_ERROR ("%s Progs attempted to read an invalid field in an edict", PRVM_NAME);
					goto cleanup;
				}
#endif
				ed = PRVM_PROG_TO_EDICT(OPA->edict);
				OPC->_int = ((prvm_eval_t *)((int *)ed->fields.vp + OPB->_int))->_int;
				break;
			case OP_LOADA_I:
			case OP_LOADA_F:
			case OP_LOADA_FLD:
			case OP_LOADA_ENT:
			case OP_LOADA_S:
			case OP_LOADA_FNC:
				ptr = (prvm_eval_t *)(&OPA->_int + OPB->_int);
#if PRVMBOUNDSCHECK
				if (!PTR_ISVALID(ptr))
				{
					prog->xfunction->profile += (st - startst);
					prog->xstatement = st - prog->statements;
					PRVM_ERROR ("%s Progs attempted to read from an out of bounds address (%i)", PRVM_NAME, OPB->_int);
					goto cleanup;
				}
#endif
				OPC->_int = ptr->_int;
				break;

			case OP_LOADA_V:
				ptr = (prvm_eval_t *)(&OPA->_int + OPB->_int);
#if PRVMBOUNDSCHECK
				if (!PTR_ISVALID(ptr))
				{
					prog->xfunction->profile += (st - startst);
					prog->xstatement = st - prog->statements;
					PRVM_ERROR ("%s Progs attempted to read from an out of bounds address (%i)", PRVM_NAME, OPB->_int);
					goto cleanup;
				}
#endif
				OPC->vector[0] = ptr->vector[0];
				OPC->vector[1] = ptr->vector[1];
				OPC->vector[2] = ptr->vector[2];
				break;
			case OP_BOUNDCHECK:
				if ((unsigned int)OPA->_int < (unsigned int)st->c || (unsigned int)OPA->_int >= (unsigned int)st->b)
				{
					prog->xfunction->profile += (st - startst);
					prog->xstatement = st - prog->statements;
					PRVM_ERROR ("%s Progs boundcheck failed at line number %d, value is < 0 or >= %d", PRVM_NAME, st->b, st->c);
					goto cleanup;
				}
				break;

			case OP_RAND0: // random number between 0 and 1
				PRVM_G_FLOAT(OFS_RETURN) = lhrandom(0,1);
				break;
			case OP_RAND1: // random number between 0 and OPA
				PRVM_G_FLOAT(OFS_RETURN) = lhrandom(0,OPA->_float);
				break;
			case OP_RAND2: // random number between OPA and OPB
				if(OPA->_float < OPB->_float)
					PRVM_G_FLOAT(OFS_RETURN) = lhrandom(OPA->_float, OPB->_float);
				else
					PRVM_G_FLOAT(OFS_RETURN) = lhrandom(OPB->_float, OPA->_float);
				break;
			case OP_RANDV0: // random vector between 0 and 1
				PRVM_G_VECTOR(OFS_RETURN)[0] = lhrandom(0,1);
				PRVM_G_VECTOR(OFS_RETURN)[1] = lhrandom(0,1);
				PRVM_G_VECTOR(OFS_RETURN)[2] = lhrandom(0,1);
				break;
			case OP_RANDV1: // random vector between 0 and OPA
				PRVM_G_VECTOR(OFS_RETURN)[0] = lhrandom(0,OPA->vector[0]);
				PRVM_G_VECTOR(OFS_RETURN)[1] = lhrandom(0,OPA->vector[1]);
				PRVM_G_VECTOR(OFS_RETURN)[2] = lhrandom(0,OPA->vector[2]);
				break;
			case OP_RANDV2: // random vector between OPA and OPB
			{
				int i;
				for(i = 0; i < 3; ++i)
				{
					if(OPA->vector[i] < OPB->vector[i])
						PRVM_G_VECTOR(OFS_RETURN)[i] = lhrandom(OPA->vector[i], OPB->vector[i]);
					else
						PRVM_G_VECTOR(OFS_RETURN)[i] = lhrandom(OPB->vector[i], OPA->vector[i]);
				}
				break;
			}

			case OP_SWITCH_F:
			case OP_SWITCH_V:
			case OP_SWITCH_S:
			case OP_SWITCH_E:
			case OP_SWITCH_I:
			case OP_SWITCH_FNC:
			{
				swtch = OPA;
				swtchtype = st->op;
				RUNAWAYCHECK();
				prog->xfunction->profile += (st - startst);
				startst = st;
				st += st->b - 1;	// offset the s++
				break;
			}
			case OP_CASE:
			{
				switch(swtchtype)
				{
				case OP_SWITCH_F:
					if(swtch->_float == OPA->_float)
					{
						RUNAWAYCHECK();
						prog->xfunction->profile += (st - startst);
						startst = st;
						st += st->b - 1;
					}
					break;
				case OP_SWITCH_E:
				case OP_SWITCH_I:
				case OP_SWITCH_FNC:
					if(swtch->_int == OPA->_int)
					{
						RUNAWAYCHECK();
						prog->xfunction->profile += (st - startst);
						startst = st;
						st += st->b-1;
					}
					break;
				case OP_SWITCH_S:
					if(swtch->_int == OPA->_int)
					{
						RUNAWAYCHECK();
						prog->xfunction->profile += (st - startst);
						startst = st;
						st += st->b-1;
					}
					// switch(string) case "string" ->
					// one of them may be null, not both
					if((!swtch->_int && PRVM_GetString(OPA->string)) || (!OPA->_int && PRVM_GetString(swtch->string)))
						break;
					if(!strcmp(PRVM_GetString(swtch->string), PRVM_GetString(OPA->string)))
					{
						RUNAWAYCHECK();
						prog->xfunction->profile += (st - startst);
						startst = st;
						st += st->b-1;
					}
					break;
				case OP_SWITCH_V:
					if(swtch->vector[0] == OPA->vector[0] &&
					   swtch->vector[1] == OPA->vector[1] &&
					   swtch->vector[2] == OPA->vector[2])
					{
						RUNAWAYCHECK();
						prog->xfunction->profile += (st - startst);
						startst = st;
						st += st->b-1;
					}
					break;
				default:
					PRVM_ERROR("OP_CASE with bad/missing OP_SWITCH %i", swtchtype);
					break;
				}
				break;
			}

			case OP_CASERANGE:
				switch(swtchtype)
				{
				case OP_SWITCH_F:
					if(swtch->_float >= OPA->_float && swtch->_float <= OPB->_float)
					{
						RUNAWAYCHECK();
						st += st->c-1;
					}
					break;
				default:
					PRVM_ERROR("OP_CASERANGE with bad/missing OP_SWITCH %i", swtchtype);
				}
				break;

			// fteqcc somehow doesn't like creating these
			case OP_ADDSTORE_F:
				OPB->_float += OPA->_float;
				break;
			case OP_ADDSTORE_V:
				OPB->vector[0] += OPA->vector[0];
				OPB->vector[1] += OPA->vector[1];
				OPB->vector[2] += OPA->vector[2];
				break;
			case OP_ADDSTOREP_F:
				PTR_ptr(OPB->_int);
				OPC->_float = (ptr->_float += OPA->_float);
				break;
			case OP_ADDSTOREP_V:
				PTR_ptr3(OPB->_int, 0, 3);
				OPC->vector[0] = (ptr->vector[0] += OPA->vector[0]);
				OPC->vector[1] = (ptr->vector[1] += OPA->vector[1]);
				OPC->vector[2] = (ptr->vector[2] += OPA->vector[2]);
				break;
			case OP_SUBSTORE_F:
				OPB->_float -= OPA->_float;
				break;
			case OP_SUBSTORE_V:
				OPB->vector[0] -= OPA->vector[0];
				OPB->vector[1] -= OPA->vector[1];
				OPB->vector[2] -= OPA->vector[2];
				break;
			case OP_SUBSTOREP_F:
				PTR_ptr(OPB->_int);
				OPC->_float = (ptr->_float -= OPA->_float);
				break;
			case OP_SUBSTOREP_V:
				PTR_ptr3(OPB->_int, 0, 3);
				OPC->vector[0] = (ptr->vector[0] -= OPA->vector[0]);
				OPC->vector[1] = (ptr->vector[1] -= OPA->vector[1]);
				OPC->vector[2] = (ptr->vector[2] -= OPA->vector[2]);
				break;

			case OP_CONV_ITOF:
				OPC->_float = (float)OPA->_int;
				break;
			case OP_CONV_FTOI:
				OPC->_int = (int)(OPA->_float);
				break;
			case OP_CP_ITOF:
				PTR_ptr(OPA->_int);
				OPC->_float = (float)(ptr->_int);
				break;
			case OP_CP_FTOI:
				PTR_ptr(OPA->_int);
				OPC->_int = (int)(ptr->_float);
				break;

			case OP_MULSTORE_F:
				OPB->_float *= OPA->_float;
				break;
			case OP_MULSTORE_V:
				OPB->vector[0] *= OPA->vector[0];
				OPB->vector[1] *= OPA->vector[1];
				OPB->vector[2] *= OPA->vector[2];
				break;
			case OP_MULSTOREP_F:
				PTR_ptr(OPB->_int);
				OPC->_float = (ptr->_float *= OPA->_float);
				break;
			case OP_MULSTOREP_V:
				// e.v *= a_float!
				PTR_ptr3(OPB->_int, 0, 3);
				OPC->vector[0] = (ptr->vector[0] -= OPA->_float);
				OPC->vector[1] = (ptr->vector[1] -= OPA->_float);
				OPC->vector[2] = (ptr->vector[2] -= OPA->_float);
				break;
			case OP_DIVSTORE_F:
				OPB->_float /= OPA->_float;
				break;
			case OP_DIVSTOREP_F:
				PTR_ptr(OPB->_int);
				OPC->_float = (ptr->_float /= OPA->_float);
				break;
			case OP_IFNOTS:
				if(!OPA->string || !PRVM_GetString(OPA->string))
					st += st->b-1;
				break;
			case OP_IFS:
				if(OPA->string && PRVM_GetString(OPA->string))
					st += st->b-1;
				break;

			case OP_GLOBALADDRESS:
				OPC->_int = ((&(OPA->_int) + OPB->_int) - (int*)prog->globals.generic);
#if PRVMBOUNDSCHECK
				if (OPC->_int < 0 || OPC->_int + 4 > GLOBALSIZE)
				{
					prog->xfunction->profile += (st - startst);
					prog->xstatement = st - prog->statements;
					PRVM_ERROR("%s attempted to address an invalid global (%i)", PRVM_NAME, OPC->_int);
					goto cleanup;
				}
#endif
				OPC->_int = PTR_GBL(OPC->_int);
				break;

			case OP_POINTER_ADD:
				// NOTE: dp pointers are no pointers as in byte-offsets, but item-offsets
				// so we use *1
				// I hope this is never used as an optimization for "A = B + C * 4" :P
				OPC->_int = OPA->_int + OPB->_int*1;
				break;

			case OP_ADD_SF: //(char*)c = (char*)a + (float)b
				OPC->_int = OPA->_int + (int)OPB->_float;
				break;
			case OP_SUB_S:  //(float)c = (char*)a - (char*)b
				OPC->_int = OPA->_int - OPB->_int;
				break;

			case OP_LOADP_I:
			case OP_LOADP_F:
			case OP_LOADP_FLD:
			case OP_LOADP_ENT:
			case OP_LOADP_S:
			case OP_LOADP_FNC:
				PTR_ptr2(OPA->_int, OPB->_int);
				OPC->_int = ptr->_int;
				break;
			case OP_LOADP_V:
				PTR_ptr3(OPA->_int, OPB->_int, 3);
				OPC->vector[0] = ptr->vector[0];
				OPC->vector[1] = ptr->vector[1];
				OPC->vector[2] = ptr->vector[2];
				break;
			/*
			// not wanted, keeping it in this comment for now though, because...
			case OP_STOREP_C:
				PTR_ptr(OPB->_int);
				break;

			// ... because this could still be reconsidered, after all it only loads
			case OP_LOADP_C:
				PTR_ptr2(OPA->_int, (int)OPB->_float);
				OPC->_int = ptr->_int;
				break;
			*/

			case OP_XOR_I:
				OPC->_int = OPA->_int ^ OPB->_int;
				break;
			case OP_RSHIFT_I:
				OPC->_int = OPA->_int >> OPB->_int;
				break;
			case OP_LSHIFT_I:
				OPC->_int = OPA->_int << OPB->_int;
				break;

			// hexen2 opcodes.. afaik fteqcc shouldn't use those
/*
			case OP_CSTATE:
				CStateOp(prog, OPA->_float, OPB->_float, newf);
				break;

			case OP_CWSTATE:
				CWStateOp(prog, OPA->_float, OPB->_float, newf);
				break;
*/
			case OP_THINKTIME:
				ThinkTimeOp(prog, PRVM_PROG_TO_EDICT(OPA->edict), OPB->_float);
				break;

			default:
				prog->xfunction->profile += (st - startst);
				prog->xstatement = st - prog->statements;
				// prvm_opnames
				if(st->op < OP_NUMOPS)
					PRVM_ERROR("Instruction %i: %s\n", st->op, prvm_opnames[st->op]);
				PRVM_ERROR ("Bad opcode %i in %s", st->op, PRVM_NAME);
				goto cleanup;
			}
		}

