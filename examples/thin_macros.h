#ifndef THIN_MACROS_H
#define THIN_MACROS_H

#define ARRAY_LENGTH(array) (sizeof((array))/sizeof((array)[0]))

#define CREATE_SYMBOL(symbolId, grammar) {				\
    marpa_g_error_clear(grammar);					\
    symbolId = marpa_g_symbol_new(grammar);				\
    _check(marpa_g_error(grammar, NULL), "marpa_g_symbol_new()", symbolId < 0); \
  }

#define CREATE_RULE(ruleId, grammar, ...) {				\
    marpa_g_error_clear(grammar);					\
    ruleId = marpa_g_rule_new(grammar, __VA_ARGS__);			\
    _check(marpa_g_error(grammar, NULL), "marpa_g_rule_new()", ruleId < 0); \
  }

#define SET_START_SYMBOL(symbolId, grammar) {				\
    marpa_g_error_clear(grammar);					\
    int result = marpa_g_start_symbol_set(grammar, symbolId);		\
    _check(marpa_g_error(grammar, NULL), "marpa_g_start_symbol_set()", result < 0); \
  }

#define PRECOMPUTE(grammar) {						\
    int result;								\
    marpa_g_error_clear(grammar);					\
    result = marpa_g_precompute(grammar);				\
    _check(marpa_g_error(grammar, NULL), "marpa_g_precompute()", result < 0); \
  }

#define ALTERNATIVE(r, token_id, value, length) {			\
    int result = marpa_r_alternative (r, token_id, value, length);	\
    if (result != MARPA_ERR_NONE) {					\
      fprintf(stderr, "marpa_r_alternative() failure, error code %d", result); \
    }									\
  }

#define EARLEME_COMPLETE(r) {						\
    Marpa_Earleme result = marpa_r_earleme_complete(r);			\
    if (result < 0) {							\
      fprintf(stderr, "marpa_r_earleme_complete() failure, error code %d", result); \
    }									\
  }

#define PUSH_TO_STACK(index, str, val) {			\
    _push_to_stack(&stackp, &stacksize, (index), (str), (val)); \
  }

#endif /* THIN_MACROS_H */

