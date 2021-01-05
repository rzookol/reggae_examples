#ifndef API_MUI_H
#define API_MUI_H

#ifdef __amigaos4__
 #define __USE_INLINE__
#endif

#ifndef MAKE_ID
	#define MAKE_ID(a,b,c,d) ((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))
#endif


#ifndef NewMinList
    #define NewMinList(minlist)  NewList((struct List *)(minlist))
#endif


#ifdef _WIN32

  #define DISPATCHER_DEF(Name) ULONG Name##Dispatcher(struct IClass *cl, Object *obj, Msg msg);

  #define DISPATCHER(Name) ULONG Name##Dispatcher(struct IClass *cl, Object *obj, Msg msg)\
       {
  #define DISPATCHER_REF(Name) Name##Dispatcher
  #define DISPATCHER_END     }

 #define M_HOOK_h(n)\
   extern struct Hook n##_hook;

 #define M_HOOK(n, argA2, argA1) \
	LONG n##_GATE(void); \
	LONG n##_GATE2(struct Hook *h, argA2, argA1); \
	LONG n##_GATE(void) { \
		struct SystemInstance *_s = getSystemInstance(); \
		return (n##_GATE2((void *)_s->s_Hook,\
								(void *)_s->s_Arg1,\
								(void *)_s->s_Arg2));}\
	struct Hook n##_hook = {(void *)(void (*)(void))n##_GATE , NULL, NULL}; \
	LONG n##_GATE2(struct Hook *h, argA2, argA1)

#define M_HOOK_END //


#else
#ifdef __AROS__
#include <proto/alib.h>
#include <proto/muimaster.h>
#include <libraries/mui.h>

#include <utility/hooks.h>
#include <aros/asmcall.h>
 
#define M_HOOK_h(name)\
	extern struct Hook name##_hook;

#define M_HOOK(n, y, z) \
        AROS_UFP3(IPTR, n##_func, AROS_UFPA(struct Hook *, h, A0), AROS_UFPA(y, , A2), \
	AROS_UFPA(z, , A1)); \
	struct Hook n##_hook = { {NULL, NULL}, (HOOKFUNC)n##_func, NULL, NULL}; \
        AROS_UFH3(IPTR, n##_func, AROS_UFHA(struct Hook *, h, A0), AROS_UFHA(y, , A2), \
	    AROS_UFHA(z, , A1)) { AROS_USERFUNC_INIT

#define M_HOOK_END AROS_USERFUNC_EXIT }

  
#define DISPATCHER_DEF(ClassNAME) AROS_UFP3(IPTR, ClassNAME##Dispatcher, AROS_UFPA(struct IClass *, cl, A0), AROS_UFPA(Object *, obj, A2), AROS_UFPA(Msg, msg, A1));
#define DISPATCHER_REF(ClassNAME) ClassNAME##Dispatcher
 
#define DISPATCHER(ClassNAME) \
	AROS_UFH3(IPTR, ClassNAME##Dispatcher, \
	    AROS_UFHA(struct IClass *, cl, A0), \
	    AROS_UFHA(Object *, obj, A2), \
	    AROS_UFHA(Msg, msg, A1)) \
		{ AROS_USERFUNC_INIT \
		    IPTR retval = 0;
 
#define DISPATCHER_END AROS_USERFUNC_EXIT } 

#else

#define M_HOOK_END //

#ifdef __MORPHOS__
 #define ASM
 #define SAVEDS

 #define REG(x)

 #define DISPATCHER_DEF(Name) struct EmulLibEntry GATE##Name##_Dispatcher;
 
 #define DISPATCHER(Name) \
	ULONG Name##_Dispatcher(void); \
	struct EmulLibEntry GATE##Name##_Dispatcher = { TRAP_LIB, 0, (void (*)(void)) Name##_Dispatcher }; \
	ULONG Name##_Dispatcher(void) \
		{	struct IClass *cl=(struct IClass*)REG_A0; \
			Msg msg=(Msg)REG_A1; \
			Object *obj=(Object*)REG_A2;
 #define DISPATCHER_REF(Name) &GATE##Name##_Dispatcher
 #define DISPATCHER_END }


 #define M_HOOK_h(n)\
   extern struct Hook n##_hook;

 #define M_HOOK(n, argA2, argA1) \
	LONG n##_GATE(void); \
	LONG n##_GATE2(struct Hook *h, argA2, argA1); \
	struct EmulLibEntry n = { TRAP_LIB, 0, (void (*)(void))n##_GATE }; \
	LONG n##_GATE(void) { \
		return (n##_GATE2((void *)REG_A0,\
								(void *)REG_A2,\
								(void *)REG_A1));}\
	struct Hook n##_hook = { {NULL, NULL}, (void *)&n , NULL, NULL}; \
	LONG n##_GATE2(struct Hook *h, argA2, argA1)

#else

 #if defined(__MAXON__)
	#define STDARGS
	#define REGARGS
	#define INLINE inline
	#define SAVEDS
	#define ASM
	#define REG(arg)	__##arg
	#define pREG(arg, reg) register REG(#reg) arg
 #endif
 #if defined(__GNUC__) && !defined(__amigaos4__)
	#define STDARGS
	#define REGARGS
	#define INLINE
	#define SAVEDS
	#define ASM    __asm
	#define REG(arg)	__asm(#arg)
	#define pREG(arg, reg) arg __asm(#reg)
 #endif
 #if defined(__amigaos4__)
	#define STDARGS
	#define REGARGS
	// #define INLINE inline
	#define SAVEDS
	#define ASM
 #endif

 #ifndef __amigaos4__

  #define DISPATCHER_DEF(Name) ULONG SAVEDS Name##Dispatcher(pREG(struct IClass *cl, a0), pREG(Object *obj, a2), pREG(Msg msg, a1));

  #define DISPATCHER(Name) \
	 ULONG SAVEDS Name##Dispatcher(pREG(struct IClass *cl, a0), pREG(Object *obj, a2), pREG(Msg msg, a1))\
		{
  #define DISPATCHER_REF(Name) Name##Dispatcher
  #define DISPATCHER_END	}

  #define M_HOOK_h(n)\
         extern struct Hook n##_hook;

  #define M_HOOK(n, argA2, argA1) \
	LONG SAVEDS n##_func( pREG(struct Hook *h, a0), pREG(argA2, a2), pREG(argA1, a1) ); \
	struct Hook n##_hook = { {NULL, NULL}, (HOOKFUNC)n##_func, NULL, NULL}; \
	LONG SAVEDS n##_func( pREG(struct Hook *h, a0), pREG(argA2, a2), pREG(argA1, a1))
 
 #else

  #define DISPATCHER_DEF(Name) uint32 Name##Dispatcher(struct IClass *cl, Object *obj, Msg msg);

  #define DISPATCHER(Name) uint32 Name##Dispatcher(struct IClass *cl, Object *obj, Msg msg)\
       {
  #define DISPATCHER_REF(Name) Name##Dispatcher
  #define DISPATCHER_END     }

  #define M_HOOK_h(n)\
         extern struct Hook n##_hook;

  #define M_HOOK(n, y, z) \
    uint32 n##_func(struct Hook *h, y, z); \
    struct Hook n##_hook = { {NULL, NULL}, (HOOKFUNC)n##_func, NULL, NULL}; \
    uint32 n##_func(struct Hook *h, y, z)

 #endif

#endif
#endif
#endif

#endif

