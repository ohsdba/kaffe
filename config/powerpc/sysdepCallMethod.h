/*
 * powerpc/sysdepCallMethod.h
 * Dynamically build function calls using PowerPC SVR4 ABI, AIX ABI or
 * Dawin ABI.
 *
 * Copyright (c) 2001
 *	Edouard G. Parmelan.  All rights reserved.
 *
 * Copyright (c) 2001
 *	Transvirtual Technologies, Inc.  All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file.
 */

/* This define will cause callMethodV and callMethodA to avoid
   introducing unused slots after jlongs and jdoubles.  */
#ifndef NO_HOLES
# define NO_HOLES 1
#endif


/* ARG_TYPE is the type of a register used for passing arguments.  */
#define ARG_TYPE	long

/* ARG_TYPES is a parameter list declaration for a function type
   that takes all possible arguments in registers.  */
#define ARG_TYPES	ARG_TYPE, ARG_TYPE, ARG_TYPE, ARG_TYPE, ARG_TYPE, ARG_TYPE, ARG_TYPE, ARG_TYPE

/* ARG_LIST is the argument list for such a function.  */
#define ARG_LIST	a0, a1, a2, a3, a4, a5, a6, a7

/* GPR_ARGS is the number of GPR (integer) registers for passing
   arguments.  */
#define GPR_ARGS	8

/* FPR_ARGS is the number of FPR (float) registers for passing
   arguments.  */
#if defined(__APPLE__) || defined(_AIX)
# define FPR_ARGS	13
#else
# define FPR_ARGS	8
#endif

/* ARG_DISPLACEMENT is the offset between the beginning of a
   variable-sized array, allocated in the stack, and the position of
   the first argument that can't be passed in a register.  */
#if defined(__APPLE__)
  /* -6 but __buildin_alloca() handle it */
# define ARG_DISPLACEMENT	0
#elif defined(_AIX)
# define ARG_DISPLACEMENT	-6
#else
# define ARG_DISPLACEMENT	-2
#endif

/* ARG_GPR a case label and a statement that arranges for one integer
   argument to be passed. */
#define ARG_GPR(N) \
    case N+1: a##N = gpr[N];

/* ARG_FPR a case label and a statement that arranges for one float
   argument to be passed. */
#define ARG_FPR(N) \
    case N+1: d##N = fpr[N];


/* Make a call to a native or Java (JIT) method.  This assembly code should
   build a standard C call using the passed call information.  By its
   nature this is highly processor specific.  This function is mandatory
   for both JIT and Interpreter (since stubs have now been deprecated).  */
static inline void sysdepCallMethod(callMethodInfo* call)
{
    void *func = call->function;
    jvalue *callargs = call->args;
    char *calltype = call->calltype;
    jvalue *args = callargs;
    jvalue *last = &callargs[call->nrargs];

    int nr_gpr = 0;
    int nr_fpr = 0;
#if !defined(__APPLE__)
    int nr_stack = 0;
#endif

    unsigned long *stack;
    unsigned long *gpr;
    double *fpr;

    /* Compute gpr[], fpr[] and stack[] arrays' size */
    while (args != last) {
	switch (calltype[(args++) - callargs]) {
	case 'D':
#if defined(__APPLE__)
	    if (nr_fpr < FPR_ARGS)
		nr_fpr++;
	    nr_gpr += 2;
#else
	    if (nr_fpr < FPR_ARGS) {
		nr_fpr++;
	    }
	    else {
		if (nr_stack & 1)
		    nr_stack += 3;
		else
		    nr_stack += 2;
	    }
#endif
	    break;

	case 'F':
#if defined(__APPLE__)
	    if (nr_fpr < FPR_ARGS)
		nr_fpr++;
	    nr_gpr++;
#else
	    if (nr_fpr < FPR_ARGS) {
		nr_fpr++;
	    }
	    else {
		nr_stack++;
	    }
#endif
	    break;

	case 'J':
#if defined(__APPLE__)
	    nr_gpr += 2;
#else
	    if (nr_gpr & 1)
		nr_gpr++;
	    if ((nr_gpr + 1) < GPR_ARGS) {
		nr_gpr += 2;
	    }
	    else {
		if (nr_stack & 1)
		    nr_stack += 3;
		else
		    nr_stack += 2;
	    }
#endif
	    break;

	default:
#if defined(__APPLE__)
	    nr_gpr++;
#else
	    if (nr_gpr < GPR_ARGS) {
		nr_gpr++;
	    }
	    else {
		nr_stack++;
	    }
#endif
	}
    }

    /* Allocate all arrays with one big alloca() */
#if defined(__APPLE__)
    {
	int nr;

	/* Must allocate space of _all_ GPR_ARGS even if they are not
           used.  */
	if (nr_gpr < GPR_ARGS)
	    nr_gpr = GPR_ARGS;
	nr = nr_gpr + 2 * nr_fpr;

	/* stack must be 16 bytes aligned */
	nr = nr ^ (nr & 3);

	stack = __builtin_alloca (4 * nr);

	/* fpr[] is in callee local variables area.  */
	fpr = (double*)(stack + nr_gpr);

	/* gpr[] and stack[] are in called parameters area.  */
	stack += ARG_DISPLACEMENT;
	gpr = stack - GPR_ARGS;
	stack = gpr;
    }
#else
    {
	int nr = nr_gpr + 2 * nr_fpr + nr_stack;

	/* stack, if used, must be 16 bytes aligned */
	if (nr_stack)
	    nr = nr ^ (nr & 3);

	/* stack[] is in called parameters area.  */
	stack = __builtin_alloca (4 * nr);

	/* gpr[] and fpr[] are in callee local variable area.  */
	gpr = stack + nr_stack;
	fpr = (double*)(gpr + nr_gpr);

	/* if __buildin_alloc() does not handle link-area, skip it.  */
	stack += ARG_DISPLACEMENT;

	nr_gpr = 0;
	nr_stack = 0;
    }
#endif

    /* build gpr[], fpr[] and stack[] arrays */
    nr_fpr = 0;
    args = callargs;

    while (args != last) {
	switch (calltype[args - callargs]) {
	case 'D':
#if defined(__APPLE__)
	    if (nr_fpr < FPR_ARGS)
		fpr[nr_fpr++] = args->d;
	    else
		*(double*)stack = args->d;
	    stack += 2;
#else
	    if (nr_fpr < FPR_ARGS) {
		fpr[nr_fpr++] = args->d;
	    }
	    else {
		if (((long)stack) & 4)
		    stack++;
		*(double*)stack = args->d;
		stack += 2;
	    }
#endif
	    break;

	case 'F':
#if defined(__APPLE__)
	    if (nr_fpr < FPR_ARGS)
		fpr[nr_fpr++] = (double)args->f;
	    else
		*(float*)stack = args->f;
	    stack++;
#else
	    if (nr_fpr < FPR_ARGS) {
		fpr[nr_fpr++] = (double)args->f;
	    }
	    else {
		*(float*)stack = args->f;
		stack++;
	    }
#endif
	    break;

	case 'J':
#if defined(__APPLE__)
	    *(long long*)stack = args->j;
	    stack += 2;
#else
	    if (nr_gpr & 1)
		nr_gpr++;
	    if ((nr_gpr + 1) < GPR_ARGS) {
		*((long long *) (&gpr[nr_gpr])) = args->j;
		nr_gpr += 2;
	    }
	    else {
		if (((long)stack) & 4)
		    stack++;
		*(long long*)stack = args->j;
		stack += 2;
	    }
#endif
	    break;

	default:
#if defined(__APPLE__)
	    *(long*)stack = args->i;
	    stack++;
#else
	    if (nr_gpr < GPR_ARGS) {
		gpr[nr_gpr++] = args->i;
	    }
	    else {
		*(long*)stack = args->i;
		stack++;
	    }
#endif
	}
	args++;
    }

#if defined(__APPLE__)
    nr_gpr = stack - gpr;
    if (nr_gpr > GPR_ARGS)
	nr_gpr = GPR_ARGS;
#endif

    {
	register ARG_TYPE a0 asm("r3");
	register ARG_TYPE a1 asm("r4");
	register ARG_TYPE a2 asm("r5");
	register ARG_TYPE a3 asm("r6");
	register ARG_TYPE a4 asm("r7");
	register ARG_TYPE a5 asm("r8");
	register ARG_TYPE a6 asm("r9");
	register ARG_TYPE a7 asm("r10");

	register double d0 asm("fr1");
	register double d1 asm("fr2");
	register double d2 asm("fr3");
	register double d3 asm("fr4");
	register double d4 asm("fr5");
	register double d5 asm("fr6");
	register double d6 asm("fr7");
	register double d7 asm("fr8");

#if FPR_ARGS == 13
	register double d8 asm("fr9");
	register double d9 asm("fr10");
	register double d10 asm("fr11");
	register double d11 asm("fr12");
	register double d12 asm("fr13");
#endif

	/* load FPR registers from fpr[] */
	switch (nr_fpr) {
#if FPR_ARGS == 13
	ARG_FPR(12);
	ARG_FPR(11);
	ARG_FPR(10);
	ARG_FPR(9);
	ARG_FPR(8);
#endif

	ARG_FPR(7);
	ARG_FPR(6);
	ARG_FPR(5);
	ARG_FPR(4);
	ARG_FPR(3);
	ARG_FPR(2);
	ARG_FPR(1);
	ARG_FPR(0);
	/* case 0: */
	}

	/* load GPR registers from gpr[] */
	switch (nr_gpr) {
	ARG_GPR(7);
	ARG_GPR(6);
	ARG_GPR(5);
	ARG_GPR(4);
	ARG_GPR(3);
	ARG_GPR(2);
	ARG_GPR(1);
	ARG_GPR(0);
	/* case 0: */
	}

	/* Ensure that the assignments to f* registers won't be optimized away. */
	asm ("" ::
	     "f" (d0), "f" (d1), "f" (d2), "f" (d3),
	     "f" (d4), "f" (d5), "f" (d6), "f" (d7));
#if FPR_ARGS == 13
	asm ("" ::
	     "f" (d8), "f" (d9), "f" (d10), "f" (d11), "f" (d12));
#endif

	switch(call->retsize) {
	case 0:
	    /* Must be void.  */
	    ((void (*)(ARG_TYPES))(func))(ARG_LIST);
	    break;

	case 1:
	    if (call->rettype == 'F')
		call->ret->f = ((jfloat (*)(ARG_TYPES))(func))(ARG_LIST);
	    else /* Must be 32-bit or smaller int.  */
		call->ret->i = ((jint (*)(ARG_TYPES))(func))(ARG_LIST);
	    break;

	default:
	    /* It could've been `case 2;', but then we'd get an additional cmp
	     * that we don't really need.  */
	    if (call->rettype == 'D')
		call->ret->d = ((jdouble (*)(ARG_TYPES))(func))(ARG_LIST);
	    else /* Must be jlong.  */
		call->ret->j = ((jlong (*)(ARG_TYPES))(func))(ARG_LIST);
	    break;
	}
    }
}

#undef ARG_TYPE
#undef ARG_TYPES
#undef ARG_LIST
#undef GPR_ARGS
#undef FPR_ARGS
#undef ARG_DISPLACEMENT
#undef ARG_GPR
#undef ARG_FPR
