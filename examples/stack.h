#ifndef STACK_H
#define STACK_H

/*
 * Adapted from from http://rosettacode.org/wiki/Stack
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*

  Example:

  typedef struct s_stack {
    char *string;
    int value;
  } s_stack_t;
  DECL_STACK_TYPE(s_stack_t, my_stack);

  void stack_failure_callback(const char *file, int line, int errnum, const char *function);
  void stack_free_callback(s_stack_t *item);
  void stack_copy_callback(s_stack_t *new, s_stack_t *orig);

  int function() {
      s_my_stack_t    *stackp;
      s_stack_t        new = { "string", 0 };
      s_stack_t        *old;

      // Create stack
      stackp = s_my_stack_create(0, &stack_failure_callback, &stack_free_callback, &stack_copy_callback);

      // Push to stack
      s_marpa_stack_push(stackp, &new);

      // Pop from stack
      old = s_marpa_stack_pop(stackp);

      // Put item at index 15 in stack
      s_marpa_stack_set(stackp, &new, 15);

      // Get item at index 11 in stack
      old = s_marpa_stack_set(stackp, 11);

      // Delete stack
      s_my_stack_delete(stackp);
  }

  // failure callback in case of problem
  void stack_failure_callback(const char *file, int line, int errnum, const char *function) {
    fprintf(stderr, "%s(%d) : %s in function %s\n", file, line, strerror(errnum), function);
    exit(EXIT_FAILURE);
  }

  // free callback if you overwrite an existing item with "set", or delete the whole
  // stack and some items are not null
  void stack_free_callback(s_stack_t *item) {
    if (item->string != NULL) {
      free(item->string);
      item->string = NULL;
    }
  }

  // copy callback if you "push" or "set": the stack will allocate room for the new element
  // and ask you for eventual customization
  void stack_copy_callback(s_stack_t *new, s_stack_t *orig) {
    new->string = NULL;
    if (orig->string != NULL) {
      new->string = strdup(orig->string);
      if (new->string == NULL) {
        stack_failure_callback(__FILE__, __LINE__, errno, "stack_alloc_callback()");
      }
    }
    new->value = orig->value;
  }

*/
 
/* to read expanded code, run through cpp | indent -st */
#define DECL_STACK_TYPE(type, name)					\
  									\
  typedef void (*s_##name##_failure_callback_t)(const char *file,	\
						int line,		\
						int errnum,		\
						const char *function);	\
  									\
  typedef void (*s_##name##_free_callback_t)(type *item);		\
  									\
  typedef void (*s_##name##_copy_callback_t)(type *new, type *orig);	\
  									\
  typedef struct s_##name##_ {						\
    type **buf;								\
    size_t alloc_size;							\
    size_t len;								\
    size_t max_index;							\
    s_##name##_copy_callback_t copy_callback;				\
    s_##name##_free_callback_t free_callback;				\
    s_##name##_failure_callback_t failure_callback;			\
  } s_##name##_t;							\
  									\
  s_##name##_t *s_##name##_create(size_t init_size, s_##name##_failure_callback_t failure_callback, s_##name##_free_callback_t free_callback, s_##name##_copy_callback_t copy_callback) { \
    const static char *function = "s_" #name "_create()";		\
    s_##name##_t *s;							\
    									\
    if ((init_size) <= 0) {						\
      init_size = 4;							\
    }									\
    s = malloc(sizeof(s_##name##_t));					\
    if (s == NULL) {							\
      if (failure_callback != NULL) {					\
	(*failure_callback)(__FILE__, __LINE__, errno, function);	\
      }									\
      return NULL;							\
    }									\
    s->buf = malloc(sizeof(type *) * init_size);			\
    if (s->buf == NULL) {						\
      if (failure_callback != NULL) {					\
	(*failure_callback)(__FILE__, __LINE__, errno, function);	\
      }									\
      free(s);								\
      return NULL;							\
    }									\
    {									\
      unsigned int i;							\
      for (i = 0; i < init_size; i++) {					\
	s->buf[i] = NULL;						\
      }									\
    }									\
    s->len = 0;								\
    s->max_index = -1;							\
    s->alloc_size = init_size;						\
    s->copy_callback = copy_callback;					\
    s->free_callback = free_callback;					\
    s->failure_callback = failure_callback;				\
    									\
    return s;								\
  }									\
  									\
  type *s_##name##_push(s_##name##_t *s, type *item) {			\
    const static char *function = "s_" #name "_push()";			\
    if ((s) == NULL) {							\
      return NULL;							\
    }									\
    /* fprintf(stderr, "PUSH: len=%d alloc_size=%d\n", (s)->len, (s)->alloc_size); */  \
    if ((s)->len >= (s)->alloc_size) {					\
      size_t alloc_size = (s)->alloc_size * 2;				\
      type **tmp = realloc((s)->buf, alloc_size * sizeof(type *));	\
      if (tmp == NULL) {						\
	if ((s)->failure_callback != NULL) {				\
	  (*((s)->failure_callback))(__FILE__, __LINE__, errno, function); \
	}								\
	return NULL;							\
      }									\
      (s)->buf = tmp;							\
      {									\
	unsigned int i;							\
	for (i = (s)->alloc_size; i < alloc_size; i++) {		\
	  /* fprintf(stderr, "PUSH: s->buf[%d] = NULL\n", i); */	\
	  s->buf[i] = NULL;						\
	}								\
      }									\
      (s)->alloc_size = alloc_size;					\
    }									\
    if (item != NULL) {							\
      /* fprintf(stderr, "PUSH: malloc\n"); */				\
      (s)->buf[(s)->len] = malloc(sizeof(type));			\
      if ((s)->buf[(s)->len] == NULL) {					\
	if ((s)->failure_callback != NULL) {				\
	  (*((s)->failure_callback))(__FILE__, __LINE__, errno, function); \
	}								\
	return NULL;							\
      }									\
      /* fprintf(stderr, "PUSH: memcpy\n"); */				\
      memcpy((s)->buf[(s)->len], item, sizeof(type));			\
      if ((s)->copy_callback != NULL) {					\
	(*((s)->copy_callback))((s)->buf[(s)->len], item);		\
      }									\
    }									\
    (s)->max_index = (s)->len++;					\
    return (s)->buf[s->max_index];					\
  }									\
  									\
  type *s_##name##_pop(s_##name##_t *s) {				\
    const static char *function = "s_" #name "_pop()";			\
    type *value;							\
    if ((s) == NULL) {							\
      return NULL;							\
    }									\
    if ((s)->len <= 0) {						\
      if ((s)->failure_callback != NULL) {				\
	(*((s)->failure_callback))(__FILE__, __LINE__, ERANGE, function); \
      }									\
      return NULL;							\
    }									\
    value = (s)->buf[s->max_index];					\
    (s)->buf[s->max_index] = NULL;					\
    (s)->len = (s)->max_index--;					\
    if (((s)->len * 2) <= (s)->alloc_size && (s)->alloc_size >= 8) {	\
      type **tmp;							\
      size_t alloc_size = (s)->alloc_size / 2;				\
      tmp = realloc((s)->buf, alloc_size * sizeof(type *));		\
      /* If failure, memory is still here. We tried to shrink */	\
      /* and not to expand, so no need to call the failure callback */	\
      if (tmp != NULL) {						\
	(s)->alloc_size = alloc_size;					\
	(s)->buf = tmp;							\
      }									\
    }									\
    return value;							\
  }									\
  									\
  type *s_##name##_get(s_##name##_t *s, unsigned int index) {		\
    const static char *function = "s_" #name "_get()";			\
    if ((s) == NULL) {							\
      return NULL;							\
    }									\
    if (index > (s)->max_index) {					\
      if ((s)->failure_callback != NULL) {				\
	(*((s)->failure_callback))(__FILE__, __LINE__, ERANGE, function); \
      }									\
      return NULL;							\
    }									\
    return (s)->buf[index];						\
  }									\
  									\
  type *s_##name##_set(s_##name##_t *s, type *item, unsigned int index) { \
  const static char *function = "s_" #name "_set()";			\
    if ((s) == NULL) {							\
      return NULL;							\
    }									\
    while (index >= (s)->len) {						\
      s_##name##_push(s, NULL);						\
    }									\
    if ((s)->buf[index] != NULL) {					\
      if ((s)->free_callback != NULL) {					\
	(*((s)->free_callback))((s)->buf[index]);			\
      }									\
      free((s)->buf[index]);						\
      (s)->buf[index] = NULL;						\
    }									\
    if (item != NULL) {							\
      /* fprintf(stderr, "SET: malloc\n"); */				\
      (s)->buf[index] = malloc(sizeof(type));				\
      if ((s)->buf[index] == NULL) {					\
	if ((s)->failure_callback != NULL) {				\
	  (*((s)->failure_callback))(__FILE__, __LINE__, errno, function); \
	}								\
	return NULL;							\
      }									\
      /* fprintf(stderr, "SET: memcpy\n"); */				\
      memcpy((s)->buf[index], item, sizeof(type));			\
      if ((s)->copy_callback != NULL) {					\
	(*((s)->copy_callback))((s)->buf[index], item);			\
      }									\
    }									\
    return (s)->buf[index];						\
  }									\
									\
  void s_##name##_delete(s_##name##_t *s) {				\
    unsigned int i;							\
    if ((s) == NULL) {							\
      return;								\
    }									\
    if ((s)->len >= 0) {						\
      for (i = 0; i < (s)->len; i++) {					\
	if ((s)->buf[i] != NULL) {					\
	  if ((s)->free_callback != NULL) {				\
	    (*((s)->free_callback))((s)->buf[i]);			\
	  }								\
	  free((s)->buf[i]);						\
	  (s)->buf[i] = NULL;						\
	}								\
      }									\
    } 									\
    free((s)->buf);							\
    free((s));								\
  }									\
									\
  short s_##name##_is_empty(s_##name##_t *s) {				\
    if ((s) == NULL) {							\
      return 0;								\
    }									\
    return ((s)->len <= 0 ? 1 : 0);					\
  }									\
									\
  size_t s_##name##_size(s_##name##_t *s) {				\
    if ((s) == NULL) {							\
      return 0;								\
    }									\
    return ((s)->len);							\
  }									\
  size_t s_##name##_max_index(s_##name##_t *s) {			\
    if ((s) == NULL) {							\
      return -1;							\
    }									\
    return ((s)->max_index);						\
  }

#endif /* STACK_H */
