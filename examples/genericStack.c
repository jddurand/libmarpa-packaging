/*
 * Variation on http://rosettacode.org/wiki/Stack
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "genericStack.h"

#define STACK_INIT_SIZE 4

struct genericStack {
  void                        **buf;
  size_t                        allocSize;
  size_t                        stackSize;
  size_t                        elementSize;
  genericStackFailureCallback_t failureCallback;
  genericStackFreeCallback_t    freeCallback;
  genericStackCopyCallback_t    copyCallback;
  genericStackTraceCallback_t   traceCallback;
  short                         optionGrowOnSet;
  short                         optionGrowOnGet;
};

genericStack_t *genericStackCreate(size_t                        elementSize,
				   unsigned int                  options,
				   genericStackFailureCallback_t genericStackFailureCallbackPtr,
				   genericStackFreeCallback_t    genericStackFreeCallbackPtr,
				   genericStackCopyCallback_t    genericStackCopyCallbackPtr,
				   genericStackTraceCallback_t   genericStackTraceCallbackPtr)
{
  const static char *function = "genericStackCreate";
  genericStack_t    *genericStackPtr;
  unsigned int       i;

  if (elementSize <= 0) {
    if (genericStackFailureCallbackPtr != NULL) {
      (*genericStackFailureCallbackPtr)(__FILE__, __LINE__, EINVAL, function);
    }
#ifdef GENERICSTACK_DEBUG
    if (genericStackTraceCallbackPtr != NULL) {
      (*genericStackTraceCallbackPtr)(__FILE__, __LINE__, function, "elementSize=%ld <= 0!\n", (long) elementSize);
      (*genericStackTraceCallbackPtr)(__FILE__, __LINE__, function, "return NULL\n");
    }
#endif
    return NULL;
  }

  genericStackPtr = malloc(sizeof(genericStack_t));
#ifdef GENERICSTACK_DEBUG
  if (genericStackTraceCallbackPtr != NULL) {
    (*genericStackTraceCallbackPtr)(__FILE__, __LINE__, function, "genericStackPtr = malloc(%ld) gives 0x%lx\n", (long) sizeof(genericStack_t), (unsigned long) genericStackPtr);
  }
#endif
  if (genericStackPtr == NULL) {
    if (genericStackFailureCallbackPtr != NULL) {
      (*genericStackFailureCallbackPtr)(__FILE__, __LINE__, errno, function);
    }
#ifdef GENERICSTACK_DEBUG
    if (genericStackTraceCallbackPtr != NULL) {
      (*genericStackTraceCallbackPtr)(__FILE__, __LINE__, function, "malloc() failure! return NULL\n");
    }
#endif
    return NULL;
  }
  genericStackPtr->buf = malloc(elementSize * STACK_INIT_SIZE);
#ifdef GENERICSTACK_DEBUG
  if (genericStackTraceCallbackPtr != NULL) {
    (*genericStackTraceCallbackPtr)(__FILE__, __LINE__, function, "genericStackPtr->buf = malloc(elementSize=%ld * STACK_INIT_SIZE=%ld) gives 0x%lx\n", (long) elementSize, (long) STACK_INIT_SIZE, (unsigned long) genericStackPtr->buf);
  }
#endif
  if (genericStackPtr->buf == NULL) {
    if (genericStackFailureCallbackPtr != NULL) {
      (*genericStackFailureCallbackPtr)(__FILE__, __LINE__, errno, function);
    }
#ifdef GENERICSTACK_DEBUG
    if (genericStackTraceCallbackPtr != NULL) {
      (*genericStackTraceCallbackPtr)(__FILE__, __LINE__, function, "malloc() failure! free(genericStackPtr=0x%lx)\n", (unsigned long) genericStackPtr);
    }
#endif
    free(genericStackPtr);
#ifdef GENERICSTACK_DEBUG
    if (genericStackTraceCallbackPtr != NULL) {
      (*genericStackTraceCallbackPtr)(__FILE__, __LINE__, function, "malloc() failure! return NULL\n");
    }
#endif
    return NULL;
  }
  for (i = 0; i < STACK_INIT_SIZE; i++) {
#ifdef GENERICSTACK_DEBUG
    if (genericStackTraceCallbackPtr != NULL) {
      (*genericStackTraceCallbackPtr)(__FILE__, __LINE__, function, "genericStackPtr->buf[%ld] setted to NULL\n", (long) i);
    }
#endif
    genericStackPtr->buf[i] = NULL;
  }

  genericStackPtr->allocSize       = STACK_INIT_SIZE;
  genericStackPtr->stackSize       = 0;
  genericStackPtr->elementSize     = elementSize;
  genericStackPtr->copyCallback    = genericStackCopyCallbackPtr;
  genericStackPtr->freeCallback    = genericStackFreeCallbackPtr;
  genericStackPtr->failureCallback = genericStackFailureCallbackPtr;
  genericStackPtr->traceCallback   = genericStackTraceCallbackPtr;
  genericStackPtr->optionGrowOnGet = ((options & GENERICSTACK_OPTION_GROW_ON_GET) == GENERICSTACK_OPTION_GROW_ON_GET) ? 1 : 0;
  genericStackPtr->optionGrowOnSet = ((options & GENERICSTACK_OPTION_GROW_ON_SET) == GENERICSTACK_OPTION_GROW_ON_SET) ? 1 : 0;

#ifdef GENERICSTACK_DEBUG
  if (genericStackPtr->traceCallback != NULL) {
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "genericStackPtr->allocSize setted to %ld\n", (long) genericStackPtr->allocSize);
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "genericStackPtr->stackSize setted to %ld\n", (long) genericStackPtr->stackSize);
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "genericStackPtr->elementSize setted to %ld\n", (long) genericStackPtr->elementSize);
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "genericStackPtr->copyCallback setted to 0x%lx\n", (unsigned long) genericStackPtr->copyCallback);
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "genericStackPtr->freeCallback setted to 0x%lx\n", (unsigned long) genericStackPtr->freeCallback);
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "genericStackPtr->failureCallback setted to 0x%lx\n", (unsigned long) genericStackPtr->failureCallback);
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "genericStackPtr->traceCallback setted to 0x%lx\n", (unsigned long) genericStackPtr->traceCallback);
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "genericStackPtr->optionGrowOnGet setted to %d\n", (int) genericStackPtr->optionGrowOnGet);
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "genericStackPtr->optionGrowOnSet setted to %d\n", (int) genericStackPtr->optionGrowOnSet);
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "return genericStackPtr=0x%lx\n", (unsigned long) genericStackPtr);
  }
#endif

  return genericStackPtr;
}

size_t genericStackPush(genericStack_t *genericStackPtr, void *elementPtr)
{
  const static char *function = "genericStackPush()";
  unsigned int i;

  if (genericStackPtr == NULL) {
    return 0;
  }

#ifdef GENERICSTACK_DEBUG
  if (genericStackPtr->traceCallback != NULL) {
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "Pushing elementPtr=0x%lx\n", (unsigned long) elementPtr);
  }
#endif

  if (genericStackPtr->stackSize >= genericStackPtr->allocSize) {
    size_t allocSize = genericStackPtr->allocSize * 2;
    void **buf;

    buf = realloc(genericStackPtr->buf, allocSize * sizeof(void *));
#ifdef GENERICSTACK_DEBUG
    if (genericStackPtr->traceCallback != NULL) {
      (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "buf = realloc(genericStackPtr->buf=0x%lx, allocSize=%ld * sizeof(void *)=%ld) gives 0x%lx\n", (unsigned long) genericStackPtr->buf, (long) allocSize, (long) sizeof(void), (unsigned long) buf);
    }
#endif
    if (buf == NULL) {
      if (genericStackPtr->failureCallback != NULL) {
	(*(genericStackPtr->failureCallback))(__FILE__, __LINE__, errno, function);
      }
#ifdef GENERICSTACK_DEBUG
      if (genericStackPtr->traceCallback != NULL) {
	(*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "realloc() failure! return 0\n");
      }
#endif
      return 0;
    }
#ifdef GENERICSTACK_DEBUG
    if (genericStackPtr->traceCallback != NULL) {
      (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "genericStackPtr->buf setted to 0x%lx\n", (unsigned long) buf);
    }
#endif
    genericStackPtr->buf = buf;
    for (i = genericStackPtr->allocSize; i < allocSize; i++) {
#ifdef GENERICSTACK_DEBUG
      if (genericStackPtr->traceCallback != NULL) {
	(*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "genericStackPtr->buf[%ld] setted to NULL\n", (long) i);
      }
#endif
      genericStackPtr->buf[i] = NULL;
    }
#ifdef GENERICSTACK_DEBUG
    if (genericStackPtr->traceCallback != NULL) {
      (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "genericStackPtr->allocSize setted to %ld\n", (long) allocSize);
    }
#endif
    genericStackPtr->allocSize = allocSize;
  }
  if (elementPtr != NULL) {
    void *newElementPtr = malloc(genericStackPtr->elementSize);
#ifdef GENERICSTACK_DEBUG
    if (genericStackPtr->traceCallback != NULL) {
      (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "newElementPtr = malloc(genericStackPtr->elementSize=%ld) gives 0x%lx\n", (long) genericStackPtr->elementSize, (unsigned long) newElementPtr);
    }
#endif
    if (newElementPtr == NULL) {
      if (genericStackPtr->failureCallback != NULL) {
	(*(genericStackPtr->failureCallback))(__FILE__, __LINE__, errno, function);
      }
#ifdef GENERICSTACK_DEBUG
      if (genericStackPtr->traceCallback != NULL) {
	(*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "malloc() failure! return 0\n");
      }
#endif
      return 0;
    }
#ifdef GENERICSTACK_DEBUG
    if (genericStackPtr->traceCallback != NULL) {
      (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "memcpy(newElementPtr=0x%lx, elementPtr=0x%lx, genericStackPtr->elementSize=%ld)\n", (unsigned long) newElementPtr, (unsigned long) elementPtr, (long) genericStackPtr->elementSize);
    }
#endif
    memcpy(newElementPtr, elementPtr, genericStackPtr->elementSize);
    if (genericStackPtr->copyCallback != NULL) {
      int errnum = (*(genericStackPtr->copyCallback))(newElementPtr, elementPtr);
      if (errnum != 0) {
	if (genericStackPtr->failureCallback != NULL) {
	  (*(genericStackPtr->failureCallback))(__FILE__, __LINE__, errnum, function);
	}
      }
    }
    genericStackPtr->buf[genericStackPtr->stackSize] = newElementPtr;
  } else {
    genericStackPtr->buf[genericStackPtr->stackSize] = NULL;
  }
#ifdef GENERICSTACK_DEBUG
  if (genericStackPtr->traceCallback != NULL) {
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "genericStackPtr->buf[genericStackPtr->stackSize=%ld] setted to 0x%lx\n", (long) genericStackPtr->stackSize, (unsigned long) genericStackPtr->buf[genericStackPtr->stackSize]);
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "genericStackPtr->stackSize changed from %ld to %ld\n", (long) genericStackPtr->stackSize, (long) (genericStackPtr->stackSize + 1));
  }
#endif
  genericStackPtr->stackSize++;

  return 1;
}

void  *genericStackPop(genericStack_t *genericStackPtr)
{
  const static char *function = "genericStackPop()";
  void *value;

  if (genericStackPtr == NULL) {
    return NULL;
  }

#ifdef GENERICSTACK_DEBUG
  if (genericStackPtr->traceCallback != NULL) {
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "Popping last element\n");
  }
#endif

  if (genericStackPtr->stackSize <= 0) {
#ifdef GENERICSTACK_DEBUG
    if (genericStackPtr->traceCallback != NULL) {
      (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "genericStackPtr->stackSize=%ld!\n", (unsigned long) genericStackPtr->stackSize);
    }
#endif
    if (genericStackPtr->failureCallback != NULL) {
      (*(genericStackPtr->failureCallback))(__FILE__, __LINE__, ERANGE, function);
    }
#ifdef GENERICSTACK_DEBUG
    if (genericStackPtr->traceCallback != NULL) {
      (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "return NULL\n");
    }
#endif
    return NULL;
  }
#ifdef GENERICSTACK_DEBUG
    if (genericStackPtr->traceCallback != NULL) {
      (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "genericStackPtr->buf[genericStackPtr->stackSize=%ld - 1] contains 0x%lx\n", (unsigned long) genericStackPtr->stackSize, (unsigned long) genericStackPtr->buf[genericStackPtr->stackSize - 1]);
    }
#endif
  value = genericStackPtr->buf[genericStackPtr->stackSize-- - 1];
  if ((genericStackPtr->stackSize * 2) <= genericStackPtr->allocSize && genericStackPtr->allocSize >= 8) {
    size_t allocSize = genericStackPtr->allocSize / 2;
    void **buf = realloc(genericStackPtr->buf, allocSize * sizeof(void *));
#ifdef GENERICSTACK_DEBUG
    if (genericStackPtr->traceCallback != NULL) {
      (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "realloc(genericStackPtr->buf=0x%lx, allocSize=%ld * sizeof(void *)=%ld) gives 0x%lx\n", (unsigned long) genericStackPtr->buf, (unsigned long) allocSize, (unsigned long) sizeof(void *));
    }
#endif
    /* If failure, memory is still here. We tried to shrink */
    /* and not to expand, so no need to call the failure callback */
    if (buf != NULL) {
      genericStackPtr->allocSize = allocSize;
      genericStackPtr->buf = buf;
#ifdef GENERICSTACK_DEBUG
      if (genericStackPtr->traceCallback != NULL) {
	(*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "genericStackPtr->allocSize setted to %ld\n", (long) genericStackPtr->allocSize);
	(*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "genericStackPtr->buf setted to %ld\n", (long) genericStackPtr->buf);
      }
#endif
    }
  }
  return value;
}

void  *genericStackGet(genericStack_t *genericStackPtr, unsigned int index)
{
  const static char *function = "genericStackGet()";

  if (genericStackPtr == NULL) {
    return NULL;
  }
#ifdef GENERICSTACK_DEBUG
  if (genericStackPtr->traceCallback != NULL) {
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "Getting element at index %ld\n", (unsigned long) index);
  }
#endif
  if (index >= genericStackPtr->allocSize && genericStackPtr->optionGrowOnGet == 1) {
#ifdef GENERICSTACK_DEBUG
    if (genericStackPtr->traceCallback != NULL) {
      (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "Extending allocation size\n");
    }
#endif
    while (index >= genericStackPtr->allocSize) {
      genericStackPush(genericStackPtr, NULL);
    }
  }
  if (index >= genericStackPtr->stackSize && genericStackPtr->optionGrowOnGet != 1) {
    if (genericStackPtr->failureCallback != NULL) {
      (*(genericStackPtr->failureCallback))(__FILE__, __LINE__, EINVAL, function);
    }
    return NULL;
  }
#ifdef GENERICSTACK_DEBUG
  if (genericStackPtr->traceCallback != NULL) {
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "return 0x%lx\n", (unsigned long) genericStackPtr->buf[index]);
  }
#endif
  return genericStackPtr->buf[index];
}

size_t genericStackSet(genericStack_t *genericStackPtr, unsigned int index, void *elementPtr)
{
  const static char *function = "genericStackSet()";
  size_t minStackSize = index+1;

  if (genericStackPtr == NULL) {
    return 0;
  }
#ifdef GENERICSTACK_DEBUG
  if (genericStackPtr->traceCallback != NULL) {
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "Setting element at index %ld using 0x%lx\n", (unsigned long) index, (unsigned long) elementPtr);
  }
#endif
  if (index >= genericStackPtr->allocSize && genericStackPtr->optionGrowOnSet == 1) {
#ifdef GENERICSTACK_DEBUG
    if (genericStackPtr->traceCallback != NULL) {
      (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "Extending allocation size\n");
    }
#endif
    while (index >= genericStackPtr->allocSize) {
      genericStackPush(genericStackPtr, NULL);
    }
  }
  if (index >= genericStackPtr->stackSize && genericStackPtr->optionGrowOnSet != 1) {
    if (genericStackPtr->failureCallback != NULL) {
      (*(genericStackPtr->failureCallback))(__FILE__, __LINE__, EINVAL, function);
    }
    return 0;
  }
  if (genericStackPtr->buf[index] != NULL) {
#ifdef GENERICSTACK_DEBUG
    if (genericStackPtr->traceCallback != NULL) {
      (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "Freeing element 0x%lx already present at index %ld\n", (unsigned long) genericStackPtr->buf[index], (unsigned long) index);
    }
#endif
    if (genericStackPtr->freeCallback != NULL) {
      int errnum = (*(genericStackPtr->freeCallback))(genericStackPtr->buf[index]);
      if (errnum != 0) {
	if (genericStackPtr->failureCallback != NULL) {
	  (*(genericStackPtr->failureCallback))(__FILE__, __LINE__, errnum, function);
	}
      }
    }
    free(genericStackPtr->buf[index]);
    genericStackPtr->buf[index] = NULL;
  }
  if (elementPtr != NULL) {
    void *newElementPtr = malloc(genericStackPtr->elementSize);
#ifdef GENERICSTACK_DEBUG
    if (genericStackPtr->traceCallback != NULL) {
      (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "malloc(genericStackPtr->elementSize=%ld) gives 0x%lx\n", (unsigned long) genericStackPtr->elementSize, (unsigned long) newElementPtr);
    }
#endif
    if (newElementPtr == NULL) {
      if (genericStackPtr->failureCallback != NULL) {
	(*(genericStackPtr->failureCallback))(__FILE__, __LINE__, errno, function);
      }
#ifdef GENERICSTACK_DEBUG
    if (genericStackPtr->traceCallback != NULL) {
      (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "malloc() failure! return 0\n");
    }
#endif
      return 0;
    }
#ifdef GENERICSTACK_DEBUG
    if (genericStackPtr->traceCallback != NULL) {
      (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "memcpy(newElementPtr=0x%lx, elementPtr=0x%lx, genericStackPtr->elementSize=%ld)\n", (unsigned long) newElementPtr, (unsigned long) elementPtr, (unsigned long) genericStackPtr->elementSize);
    }
#endif
    memcpy(newElementPtr, elementPtr, genericStackPtr->elementSize);
    if (genericStackPtr->copyCallback != NULL) {
      int errnum = (*(genericStackPtr->copyCallback))(newElementPtr, elementPtr);
      if (errnum != 0) {
	if (genericStackPtr->failureCallback != NULL) {
	  (*(genericStackPtr->failureCallback))(__FILE__, __LINE__, errnum, function);
	}
      }
    }
    genericStackPtr->buf[index] = newElementPtr;
  }
#ifdef GENERICSTACK_DEBUG
  if (genericStackPtr->traceCallback != NULL) {
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "genericStackPtr->buf[index=%d] setted to 0x%lx\n", (unsigned long) index, (unsigned long) genericStackPtr->buf[index]);
  }
#endif
  if (genericStackPtr->stackSize < minStackSize) {
#ifdef GENERICSTACK_DEBUG
    if (genericStackPtr->traceCallback != NULL) {
      (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "genericStackPtr->stackSize changed from %ld to %ld\n", (unsigned long) genericStackPtr->stackSize, (unsigned long) minStackSize);
    }
#endif
    genericStackPtr->stackSize = minStackSize;
  }
  return 1;
}

void genericStackFree(genericStack_t **genericStackPtrPtr)
{
  const static char *function = "genericStackFree()";
  genericStack_t *genericStackPtr;
  unsigned int i;

  if (genericStackPtrPtr == NULL) {
    return;
  }
  genericStackPtr = *genericStackPtrPtr;
  if (genericStackPtr == NULL) {
    return;
  }

#ifdef GENERICSTACK_DEBUG
  if (genericStackPtr->traceCallback != NULL) {
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "Freeing the stack\n");
  }
#endif

  /* genericStackPtr->allocSize is always > 0 per def */
  for (i = 0; i < genericStackPtr->allocSize; i++) {
    if (genericStackPtr->buf[i] != NULL) {

#ifdef GENERICSTACK_DEBUG
      if (genericStackPtr->traceCallback != NULL) {
	(*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "Freeing the element 0x%lx at index %ld\n", (unsigned long) genericStackPtr->buf[i], (unsigned long) i);
      }
#endif

      if (genericStackPtr->freeCallback != NULL) {
	int errnum = (*(genericStackPtr->freeCallback))(genericStackPtr->buf[i]);
	if (errnum != 0) {
	  if (genericStackPtr->failureCallback != NULL) {
	    (*(genericStackPtr->failureCallback))(__FILE__, __LINE__, errnum, function);
	  }
	}
      }
      free(genericStackPtr->buf[i]);
      genericStackPtr->buf[i] = NULL;
    }
  }

#ifdef GENERICSTACK_DEBUG
  if (genericStackPtr->traceCallback != NULL) {
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "Freeing genericStackPtr->buf = 0x%lx\n", (unsigned long) genericStackPtr->buf);
  }
#endif

  free(genericStackPtr->buf);

#ifdef GENERICSTACK_DEBUG
  if (genericStackPtr->traceCallback != NULL) {
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "Freeing genericStackPtr = 0x%lx\n", (unsigned long) genericStackPtr);
  }
#endif

  free(genericStackPtr);

#ifdef GENERICSTACK_DEBUG
  if (genericStackPtr->traceCallback != NULL) {
    (*genericStackPtr->traceCallback)(__FILE__, __LINE__, function, "Setting *genericStackPtrPtr=*0x%x to NULL\n", (unsigned long) genericStackPtrPtr);
  }
#endif

  *genericStackPtrPtr = NULL;
  return;
}

size_t genericStackSize(genericStack_t *genericStackPtr)
{
  if (genericStackPtr == NULL) {
    return 0;
  }
  return (genericStackPtr->stackSize);
}
