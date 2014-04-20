#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <marpa.h>
#include "thin_macros.h"

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

/* A stack here is composed of two elements, the string and a corresponding int */
typedef struct s_stack {
  char *string;
  int value;
} s_stack_;

static char *_make_str    (const char *fmt, ...);
static void _check        (Marpa_Error_Code errorCode, const char *call, int forcedCondition);
static void _push_to_stack(s_stack_ **stackpp, int *stacksizep, int index, char *string, int value);

struct marpa_error_description_s {
  Marpa_Error_Code error_code;
  const char*name;
  const char*suggested;
};
extern const struct marpa_error_description_s marpa_error_description[];

int main() {
  Marpa_Config        marpa_configuration;
  Marpa_Grammar       g;
  Marpa_Symbol_ID     S, E, op, number;
  Marpa_Rule_ID       start_rule_id, op_rule_id, number_rule_id;
  Marpa_Recognizer    r;
  char               *token_values[] = { "0", "1", "2", "3", "0", "-", "+", "*" };
  s_stack_            expected_results[5] = {
    {"(2-(0*(3+1))) == 2", 2},
    {"(((2-0)*3)+1) == 7", 7},
    {"((2-(0*3))+1) == 3", 3},
    {"((2-0)*(3+1)) == 8", 8},
    {"(2-((0*3)+1)) == 1", 1}
  };
  Marpa_Earley_Set_ID latest_earley_set_ID;
  Marpa_Bocage        bocage;
  Marpa_Order         order;
  Marpa_Tree          tree;
  s_stack_           *stackp = NULL;
  int                 stacksize = -1;
  int                 zero                 = 4;      /* Indice 4 in token_values */
  int                 minus_token_value    = 5;      /* Indice 5 in token_values */
  int                 plus_token_value     = 6;      /* Indice 6 in token_values */
  int                 multiply_token_value = 7;      /* Indice 7 in token_values */

  
  INIT_CONFIG(marpa_configuration);
  CREATE_GRAMMAR(g, marpa_configuration);

  /* Symbols creation */
  CREATE_SYMBOL(S, g);
  CREATE_SYMBOL(E, g);
  CREATE_SYMBOL(op, g);
  CREATE_SYMBOL(number, g);

  /* ::start ::= S */
  SET_START_SYMBOL(S, g);

  /* Rules creation */
  {Marpa_Symbol_ID rhs[] = { E };        CREATE_RULE(start_rule_id,  g, S, rhs, ARRAY_LENGTH(rhs));}
  {Marpa_Symbol_ID rhs[] = { E, op, E }; CREATE_RULE(op_rule_id,     g, E, rhs, ARRAY_LENGTH(rhs));};
  {Marpa_Symbol_ID rhs[] = { number };   CREATE_RULE(number_rule_id, g, E, rhs, ARRAY_LENGTH(rhs));}

  PRECOMPUTE(g);

  CREATE_RECOGNIZER(g, r);

  START_INPUT(g, r);

  /*
    The numbers from 1 to 3 are themselves --
    that is, they index their own token value.
    Important: zero cannot be itself!
  */

  ALTERNATIVE(g, r, number, 2, 1);
  EARLEME_COMPLETE(g, r);
  ALTERNATIVE(g, r, op, minus_token_value, 1);
  EARLEME_COMPLETE(g, r);
  ALTERNATIVE(g, r, number, zero, 1);
  EARLEME_COMPLETE(g, r);
  ALTERNATIVE(g, r, op, multiply_token_value, 1);
  EARLEME_COMPLETE(g, r);
  ALTERNATIVE(g, r, number, 3, 1);
  EARLEME_COMPLETE(g, r);
  ALTERNATIVE(g, r, op, plus_token_value, 1);
  EARLEME_COMPLETE(g, r);
  ALTERNATIVE(g, r, number, 1, 1);
  EARLEME_COMPLETE(g, r);

  latest_earley_set_ID = marpa_r_latest_earley_set(r); /* This function always succeed as per doc */

  CREATE_BOCAGE(g, r, latest_earley_set_ID, bocage);

  CREATE_ORDER(g, bocage, order);

  CREATE_TREE(g, order, tree);

  while (marpa_t_next(tree) >= 0) {
    int nextok = 1;
    Marpa_Value valuator;

    CREATE_VALUATOR(g, tree, valuator);

    marpa_v_symbol_is_valued_set(valuator, op_rule_id, 1);
    marpa_v_symbol_is_valued_set(valuator, start_rule_id, 1);
    marpa_v_symbol_is_valued_set(valuator, number_rule_id, 1);

    while (nextok) {
      Marpa_Step_Type type     = marpa_v_step(valuator);

      switch (type) {
      case MARPA_STEP_TOKEN:
	{
	  /* Marpa_Symbol_ID token_id = marpa_v_token(valuator); */ /* unused, but you know you can get it -; */
	  int token_value_ix       = marpa_v_token_value(valuator);
	  int arg_n                = marpa_v_result(valuator);

	  PUSH_TO_STACK(arg_n, strdup(token_values[token_value_ix]), atoi(token_values[token_value_ix]));
	  break;
	}
      case MARPA_STEP_RULE:
	{
	  Marpa_Rule_ID rule_id    = marpa_v_rule(valuator);
	  int arg_0                = marpa_v_arg_0(valuator);
	  int arg_n                = marpa_v_arg_n(valuator);

	  if (rule_id == start_rule_id) {
	    PUSH_TO_STACK(arg_0, _make_str("%s == %d", stackp[arg_n].string, stackp[arg_n].value), stackp[arg_n].value);
	  }
	  else if (rule_id == number_rule_id) {
	    PUSH_TO_STACK(arg_0, _make_str("%d", stackp[arg_0].value), stackp[arg_0].value);
	  }
	  else if (rule_id == op_rule_id) {
	    char *left_string  = stackp[arg_0].string;
	    int   left_value   = stackp[arg_0].value;
	    char *op           = stackp[arg_0 + 1].string;
	    char *right_string = stackp[arg_n].string;
	    int   right_value  = stackp[arg_n].value;
	    char  *text        = _make_str("(%s%s%s)", left_string, op, right_string);

	    switch (*op) {
	    case '+':
	      PUSH_TO_STACK(arg_0, text, left_value + right_value);
	      break;
	    case '-':
	      PUSH_TO_STACK(arg_0, text, left_value - right_value);
	      break;
	    case '*':
	      PUSH_TO_STACK(arg_0, text, left_value * right_value);
	      break;
	    default:
	      fprintf(stderr, "Unknown op %s\n", op);
	      exit(EXIT_FAILURE);
	    }
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
    if (stacksize < 0) {
      fprintf(stderr, "No result !?\n");
    } else {
      int i;
      s_stack_ *expected_resultp = NULL;

      for (i = 0; i <= ARRAY_LENGTH(expected_results); i++) {
	if (strcmp(expected_results[i].string, stackp[0].string) == 0) {
	  expected_resultp = &(expected_results[i]);
	}
      }

      if (expected_resultp == NULL) {
	fprintf(stderr, "Totally unexpected result %s, value %d\n", stackp[0].string, stackp[0].value);
      } else {
	if (expected_resultp->value == stackp[0].value) {
	  fprintf(stderr, "Expected result %s, value %d\n", expected_resultp->string, expected_resultp->value);
	} else {
	  fprintf(stderr, "Unexpected result %s, value %d instead of %d\n", stackp[0].string, stackp[0].value, expected_resultp->value);
	}
      }
      
      for (i = 0; i <= stacksize; i++) {
	free(stackp[i].string);
	stackp[i].string = NULL;
      }
      free(stackp);
      stackp = NULL;
      stacksize = -1;
    }

    /* Free valuator */
    marpa_v_unref(valuator);
  }

  /* Free marpa */
  marpa_t_unref(tree);
  marpa_o_unref(order);
  marpa_b_unref(bocage);
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

static void _push_to_stack(s_stack_ **stackpp, int *stacksizep, int index, char *string, int  value) {
  s_stack_ *stackp = *stackpp;
  int       stacksize = *stacksizep;

  if (stacksize < index) {
    if (stacksize < 0) {
      stackp = malloc(sizeof(s_stack_));
      
    } else {
      stackp = realloc(stackp, sizeof(s_stack_) * (index + 1));
    }
    stacksize = index;
    stackp[index].string = NULL;
  }
  if (stackp == NULL) {
    exit(EXIT_FAILURE);
  }
  if (stackp[index].string != NULL) {
    free(stackp[index].string);
  }
  stackp[index].string = string;
  stackp[index].value = value;

  *stacksizep = stacksize;
  *stackpp = stackp;
}
