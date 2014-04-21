#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <marpa.h>
#include "thin_macros.h"
#include "genericStack.h"

/*
  C version of first example in file t/thin_eq.t
  from Marpa::R2 CPAN distribution

  Grammar is:

  :start ::= S
  S ::= E
  E ::= E op E
  E ::= number

  Expected values:
  
  (2-(0*(3+1))) == 2 => 1
  (((2-0)*3)+1) == 7 => 1
  ((2-(0*3))+1) == 3 => 1
  ((2-0)*(3+1)) == 8 => 1
  (2-((0*3)+1)) == 1 => 1

  Compilation: cc -g -o ./thin ./thin.c -lmarpa
  Execution  : ./thin

*/

/* 
 * A stack here is composed of two elements, the string and a corresponding int
 */
typedef struct s_stack {
  char *string;
  int value;
} s_stack_t;


static char *_make_str    (const char *fmt, ...);
static void _check        (Marpa_Error_Code errorCode, const char *call, int forcedCondition);

struct marpa_error_description_s {
  Marpa_Error_Code error_code;
  const char*name;
  const char*suggested;
};
extern const struct marpa_error_description_s marpa_error_description[];
void stack_failure_callback(const char *file, int line, int errnum, const char *function);
int  stack_free_callback(void *elementPtr);
int  stack_copy_callback(void *elementDstPtr, void *elementSrcPtr);

int main() {
  /* Marpa variables */
  Marpa_Config        c;
  Marpa_Grammar       g;
  Marpa_Symbol_ID     S, E, op, number;
  Marpa_Rule_ID       start_rule_id, op_rule_id, number_rule_id;
  Marpa_Recognizer    r;
  Marpa_Earley_Set_ID latest_earley_set_ID;
  Marpa_Bocage        b;
  Marpa_Order         o;
  Marpa_Tree          t;
  /* User variables */
  char               *token_values[] = { "0", "1", "2", "3", "0", "-", "+", "*" };
  s_stack_t           expected_results[5] = {
    {"(2-(0*(3+1))) == 2", 2},
    {"(((2-0)*3)+1) == 7", 7},
    {"((2-(0*3))+1) == 3", 3},
    {"((2-0)*(3+1)) == 8", 8},
    {"(2-((0*3)+1)) == 1", 1}
  };
  s_stack_t           *resultp;
  int                 zero                 = 4;      /* Indice 4 in token_values */
  int                 minus_token_value    = 5;      /* Indice 5 in token_values */
  int                 plus_token_value     = 6;      /* Indice 6 in token_values */
  int                 multiply_token_value = 7;      /* Indice 7 in token_values */

  /* Initialize configuration */
  /* ------------------------ */
  INIT_CONFIG(c);

  /* Create grammar */
  /* -------------- */
  CREATE_GRAMMAR(g, c);

  /* Create symbols */
  /* -------------- */
  CREATE_SYMBOL(S, g);
  CREATE_SYMBOL(E, g);
  CREATE_SYMBOL(op, g);
  CREATE_SYMBOL(number, g);

  /* Set start symbol */
  /* ---------------- */
  SET_START_SYMBOL(S, g);

  /* Create rules */
  /* ------------ */
  {
    Marpa_Symbol_ID rhs[] = { E };
    CREATE_RULE(start_rule_id,  g, S, rhs, ARRAY_LENGTH(rhs));
  }
  {
    Marpa_Symbol_ID rhs[] = { E, op, E };
    CREATE_RULE(op_rule_id,     g, E, rhs, ARRAY_LENGTH(rhs));
  }
  {
    Marpa_Symbol_ID rhs[] = { number };
    CREATE_RULE(number_rule_id, g, E, rhs, ARRAY_LENGTH(rhs));
  }

  /* Precompute grammar */
  /* ------------------ */
  PRECOMPUTE(g);

  /* Create recognizer */
  /* ----------------- */
  CREATE_RECOGNIZER(r, g);

  /* Start input */
  /* ----------- */
  START_INPUT(r, g);

  /* Feed lexemes : alternative(s) and earleme completion */
  /* ---------------------------------------------------- */
  ALTERNATIVE(r, g, number, 2, 1);
  EARLEME_COMPLETE(r, g);
  ALTERNATIVE(r, g, op, minus_token_value, 1);
  EARLEME_COMPLETE(r, g);
  ALTERNATIVE(r, g, number, zero, 1);
  EARLEME_COMPLETE(r, g);
  ALTERNATIVE(r, g, op, multiply_token_value, 1);
  EARLEME_COMPLETE(r, g);
  ALTERNATIVE(r, g, number, 3, 1);
  EARLEME_COMPLETE(r, g);
  ALTERNATIVE(r, g, op, plus_token_value, 1);
  EARLEME_COMPLETE(r, g);
  ALTERNATIVE(r, g, number, 1, 1);
  EARLEME_COMPLETE(r, g);

  /* Get latest Earley set */
  /* --------------------- */
  latest_earley_set_ID = marpa_r_latest_earley_set(r); /* This function always succeed as per doc */

  /* Get full set of parses (this is a bocage) */
  /* ----------------------------------------- */
  CREATE_BOCAGE(b, g, r, latest_earley_set_ID);

  /* Parses in a bocage must be ordered before iterating on them */
  /* ----------------------------------------------------------- */
  CREATE_ORDER(o, b, g);

  /* Create iterator on parses */
  /* ------------------------- */
  CREATE_TREE(t, o, g);

  /* Loop until no more parse */
  /* ------------------------ */
  while (marpa_t_next(t) >= 0) {
    int nextok = 1;
    Marpa_Value v;
    genericStack_t *genericStackPtr;

    /* Create a valuator */
    /* ----------------- */
    CREATE_VALUATOR(v, t, g);

    /* Create a stack */
    /* --------------- */
    genericStackPtr = genericStackCreate(sizeof(s_stack_t),
					 &stack_failure_callback,
					 &stack_free_callback,
					 &stack_copy_callback);

    marpa_v_rule_is_valued_set(v, op_rule_id, 1);
    marpa_v_rule_is_valued_set(v, start_rule_id, 1);
    marpa_v_rule_is_valued_set(v, number_rule_id, 1);

    /*
     * In the future, maybe:
     marpa_v_valued_force(v);
    */


    while (nextok) {
      Marpa_Step_Type type     = marpa_v_step(v);
      s_stack_t       new;

      switch (type) {
      case MARPA_STEP_TOKEN:
	{
	  /* Marpa_Symbol_ID token_id = marpa_v_token(v); */ /* unused, but you know you can get it -; */
	  int token_value_ix       = marpa_v_token_value(v);
	  int arg_n                = marpa_v_result(v);

	  new.string = token_values[token_value_ix];
	  new.value = atoi(token_values[token_value_ix]);
	  genericStackSet(genericStackPtr, arg_n, &new);

	  break;
	}
      case MARPA_STEP_RULE:
	{
	  Marpa_Rule_ID rule_id    = marpa_v_rule(v);
	  int arg_0                = marpa_v_arg_0(v);
	  int arg_n                = marpa_v_arg_n(v);
	  s_stack_t *stack_0       = genericStackGet(genericStackPtr, arg_0);
	  s_stack_t *stack_n       = genericStackGet(genericStackPtr, arg_n);

	  if (rule_id == start_rule_id) {
	    new.string = _make_str("%s == %d", stack_n->string, stack_n->value);
	    new.value = stack_n->value;
	    genericStackSet(genericStackPtr, arg_0, &new);
	    free(new.string);
	  }
	  else if (rule_id == number_rule_id) {
	    new.string = _make_str("%d", stack_0->value);
	    new.value = stack_0->value;
	    genericStackSet(genericStackPtr, arg_0, &new);
	    free(new.string);
	  }
	  else if (rule_id == op_rule_id) {
	    s_stack_t *stack_0_plus_1  = genericStackGet(genericStackPtr, arg_0 + 1);
	    char *left_string  = stack_0->string;
	    int   left_value   = stack_0->value;
	    char *op           = stack_0_plus_1->string;
	    char *right_string = stack_n->string;
	    int   right_value  = stack_n->value;

	    new.string = _make_str("(%s%s%s)", left_string, op, right_string);

	    switch (*op) {
	    case '+':
	      new.value = left_value + right_value;
	      genericStackSet(genericStackPtr, arg_0, &new);
	      break;
	    case '-':
	      new.value = left_value - right_value;
	      genericStackSet(genericStackPtr, arg_0, &new);
	      break;
	    case '*':
	      new.value = left_value * right_value;
	      genericStackSet(genericStackPtr, arg_0, &new);
	      break;
	    default:
	      fprintf(stderr, "Unknown op %s\n", op);
	      exit(EXIT_FAILURE);
	    }

	    free(new.string);
	  } else {
	    fprintf(stderr, "Unknown rule %d\n", rule_id);
	    exit(EXIT_FAILURE);
	  }
	  break;
	}
      case MARPA_STEP_INACTIVE:
	nextok = 0;
	break;
      default:
	fprintf(stderr, "Unexpected type %d\n", type);
	exit(EXIT_FAILURE);
      }
    }
    /* Check result and free the stack */
    resultp = genericStackGet(genericStackPtr, 0);
    if (resultp == NULL) {
      fprintf(stderr, "No result !?\n");
    } else {
      unsigned int i;
      s_stack_t *expected_resultp = NULL;

      for (i = 0; i <= ARRAY_LENGTH(expected_results); i++) {
	if (strcmp(expected_results[i].string, resultp->string) == 0) {
	  expected_resultp = &(expected_results[i]);
	}
      }

      if (expected_resultp == NULL) {
	fprintf(stderr, "Totally unexpected result %s, value %d\n", resultp->string, resultp->value);
      } else {
	if (expected_resultp->value == resultp->value) {
	  fprintf(stderr, "Expected result %s, value %d\n", expected_resultp->string, expected_resultp->value);
	} else {
	  fprintf(stderr, "Unexpected result %s, value %d instead of %d\n", resultp->string, resultp->value, expected_resultp->value);
	}
      }
    }
    genericStackFree(&genericStackPtr);

    /* Free valuator */
    marpa_v_unref(v);
  }

  /* Free marpa */
  marpa_t_unref(t);
  marpa_o_unref(o);
  marpa_b_unref(b);
  marpa_r_unref(r);
  marpa_g_unref(g);

  exit(EXIT_SUCCESS);
}

/* Copied almost verbatim from snprintf man page on Linux */
static char *_make_str(const char *fmt, ...) {
  int n;
  int size = 100;     /* Guess we need no more than 100 bytes */
  char *p, *np;
  va_list ap;

  p = malloc(size);
  if (p == NULL) {
    fprintf(stderr, "malloc() failure\n");
    exit(EXIT_FAILURE);
  }

  while (1) {

    /* Try to print in the allocated space */

    va_start(ap, fmt);
    n = vsnprintf(p, size, fmt, ap);
    va_end(ap);

    /* Check error code */

    if (n < 0) {
      free(p);
      fprintf(stderr, "vsnprintf() failure\n");
      exit(EXIT_FAILURE);
    }

    /* If that worked, return the string */

    if (n < size) {
      return p;
    }

    /* Else try again with more space */

    size = n + 1;       /* Precisely what is needed */

    np = realloc(p, size);
    if (np == NULL) {
      free(p);
      fprintf(stderr, "realloc() failure\n");
      exit(EXIT_FAILURE);
    } else {
      p = np;
    }
  }
}

static void _check(Marpa_Error_Code errorCode, const char *call, int forcedCondition) {
  if (forcedCondition != 0 || errorCode != MARPA_ERR_NONE) {
    const char *function = (call != NULL) ? call : "<unknown>";
    const char *msg = (errorCode >= 0 && errorCode < MARPA_ERROR_COUNT) ? marpa_error_description[errorCode].name : "Generic error";
    fprintf(stderr,"%s: %s", function, msg);
    exit(EXIT_FAILURE);
  }
}

void stack_failure_callback(const char *file, int line, int errnum, const char *function) {
  fprintf(stderr, "%s(%d) : %s in function %s\n", file, line, strerror(errnum), function);
  exit(EXIT_FAILURE);
}

int stack_free_callback(void *elementPtr) {
  s_stack_t *item = (s_stack_t *) elementPtr;

  if (item->string != NULL) {
    free(item->string);
    item->string = NULL;
  }

  return 0;
}

int stack_copy_callback(void *elementDstPtr, void *elementSrcPtr) {
  s_stack_t *new = (s_stack_t *) elementDstPtr;
  s_stack_t *orig = (s_stack_t *) elementSrcPtr;

  new->string = NULL;
  if (orig->string != NULL) {
    new->string = strdup(orig->string);
    if (new->string == NULL) {
      return errno;
    }
  }
  new->value = orig->value;

  return 0;
}
