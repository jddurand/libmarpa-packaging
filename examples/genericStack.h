#ifndef GENERIC_STACK_H
#define GENERIC_STACK_H

typedef struct genericStack genericStack_t;

typedef void (*genericStackFailureCallback_t)(const char *file, int line, int errnum, const char *function);
typedef int  (*genericStackFreeCallback_t)(void *elementPtr);
typedef int  (*genericStackCopyCallback_t)(void *elementDstPtr, void *elementSrcPtr);

genericStack_t *genericStackCreate(size_t                        elementSize,
				   genericStackFailureCallback_t genericStackFailureCallbackPtr,
				   genericStackFreeCallback_t    genericStackFreeCallbackPtr,
				   genericStackCopyCallback_t    genericStackCopyCallbackPtr);

size_t genericStackPush(genericStack_t *genericStackPtr, void *elementPtr);
void  *genericStackPop(genericStack_t *genericStackPtr);
void  *genericStackGet(genericStack_t *genericStackPtr, unsigned int index);
void  *genericStackSet(genericStack_t *genericStackPtr, unsigned int index, void *elementPtr);
void   genericStackFree(genericStack_t **genericStackPtrPtr);
size_t genericStackSize(genericStack_t *genericStackPtr);

#endif /* GENERIC_STACK_H */
