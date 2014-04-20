#ifndef THIN_MACROS_H
#define THIN_MACROS_H

#define ARRAY_LENGTH(array) (sizeof((array))/sizeof((array)[0]))

#define INIT_CONFIG(config) {			\
    marpa_c_init(&marpa_configuration);		\
  }

#define CREATE_GRAMMAR(g, config) {					\
    g = marpa_g_new(&(config));						\
    _check(marpa_c_error(&(config), NULL), "marpa_g_new()", g == NULL); \
  }

#define CREATE_RECOGNIZER(g, r) {					\
    marpa_g_error_clear(g);						\
    r = marpa_r_new(g);							\
    _check(marpa_g_error(g, NULL), "marpa_r_new()", r == NULL);		\
  }

#define CREATE_SYMBOL(symbolId, g) {					\
    marpa_g_error_clear(g);						\
    symbolId = marpa_g_symbol_new(g);					\
    _check(marpa_g_error((g), NULL), "marpa_g_symbol_new()", symbolId < 0); \
  }

#define CREATE_BOCAGE(g, r, latest_earley_set_ID, b) {			\
    marpa_g_error_clear(g);						\
    b = marpa_b_new((r), latest_earley_set_ID);				\
    _check(marpa_g_error((g), NULL), "marpa_b_new()", b == NULL); \
  }

#define CREATE_ORDER(g, b, o) {						\
    marpa_g_error_clear(g);						\
    o = marpa_o_new(b);							\
    _check(marpa_g_error((g), NULL), "marpa_b_new()", o == NULL);	\
  }

#define CREATE_TREE(g, o, t) {						\
    marpa_g_error_clear(g);						\
    t = marpa_t_new(o);							\
    _check(marpa_g_error((g), NULL), "marpa_t_new()", t == NULL);	\
  }

#define CREATE_VALUATOR(g, t, v) {					\
    marpa_g_error_clear(g);						\
    v = marpa_v_new(t);							\
    _check(marpa_g_error((g), NULL), "marpa_v_new()", v == NULL);	\
  }

#define CREATE_RULE(ruleId, g, ...) {					\
    marpa_g_error_clear(g);						\
    ruleId = marpa_g_rule_new((g), __VA_ARGS__);			\
    _check(marpa_g_error((g), NULL), "marpa_g_rule_new()", ruleId < 0);	\
  }

#define START_INPUT(g, r) {						\
    marpa_g_error_clear(g);						\
    _check(marpa_g_error((g), NULL), "marpa_r_start_input()", marpa_r_start_input(r) < 0); \
  }

#define SET_START_SYMBOL(symbolId, g) {					\
    marpa_g_error_clear(g);						\
    _check(marpa_g_error((g), NULL), "marpa_g_start_symbol_set()", marpa_g_start_symbol_set((g), symbolId) < 0); \
  }

#define PRECOMPUTE(g) {							\
    marpa_g_error_clear(g);						\
    _check(marpa_g_error((g), NULL), "marpa_g_precompute()", marpa_g_precompute(g) < 0); \
  }

#define ALTERNATIVE(g, r, token_id, value, length) {			\
    marpa_g_error_clear(g);						\
    _check(marpa_g_error((g), NULL), "marpa_r_alternative()", marpa_r_alternative((r), (token_id), (value), (length)) != MARPA_ERR_NONE); \
  }

#define EARLEME_COMPLETE(g, r) {					\
    marpa_g_error_clear(g);						\
    _check(marpa_g_error((g), NULL), "marpa_r_earleme_complete()", marpa_r_earleme_complete(r) < 0); \
  }

#define PUSH_TO_STACK(index, str, val) {			\
    _push_to_stack(&stackp, &stacksize, (index), (str), (val)); \
  }

#endif /* THIN_MACROS_H */

