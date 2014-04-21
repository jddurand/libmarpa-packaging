#ifndef GENERIC_STACK_H
#define GENERIC_STACK_H

typedef struct genericStack genericStack_t;

#define GENERICSTACK_OPTION_GROW_ON_GET 0x01
#define GENERICSTACK_OPTION_GROW_ON_SET 0x02

#define GENERICSTACK_OPTION_DEFAULT (GENERICSTACK_OPTION_GROW_ON_GET | GENERICSTACK_OPTION_GROW_ON_SET)

/* Set this define at compile time to get trace callbacks */
/* #undef GENERICSTACK_DEBUG */

typedef void (*genericStackFailureCallback_t)(const char *file, int line, int errnum, const char *function);
typedef int  (*genericStackFreeCallback_t)(void *elementPtr);
typedef int  (*genericStackCopyCallback_t)(void *elementDstPtr, void *elementSrcPtr);
typedef void (*genericStackTraceCallback_t)(const char *file, int line, const char *function, const char *format, ...);

genericStack_t *genericStackCreate(size_t                        elementSize,
				   unsigned int                  options,
				   genericStackFailureCallback_t genericStackFailureCallbackPtr,
				   genericStackFreeCallback_t    genericStackFreeCallbackPtr,
				   genericStackCopyCallback_t    genericStackCopyCallbackPtr,
				   genericStackTraceCallback_t   genericStackTraceCallbackPtr);

size_t genericStackPush(genericStack_t *genericStackPtr, void *elementPtr);
void  *genericStackPop(genericStack_t *genericStackPtr);
void  *genericStackGet(genericStack_t *genericStackPtr, unsigned int index);
size_t genericStackSet(genericStack_t *genericStackPtr, unsigned int index, void *elementPtr);
void   genericStackFree(genericStack_t **genericStackPtrPtr);
size_t genericStackSize(genericStack_t *genericStackPtr);

#endif /* GENERIC_STACK_H */
