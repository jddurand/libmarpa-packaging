#ifndef THIN_MACROS_H
#define THIN_MACROS_H

#define ARRAY_LENGTH(array) (sizeof((array))/sizeof((array)[0]))

#define INIT_CONFIG(c) {			\
    marpa_c_init(&(c));				\
  }

#define CREATE_GRAMMAR(g, c) {						\
    g = marpa_g_new(&(c));						\
    _check(marpa_c_error(&(c), NULL), "marpa_g_new()", g == NULL);	\
  }

#define CREATE_RECOGNIZER(r, g) {					\
    marpa_g_error_clear(g);						\
    r = marpa_r_new(g);							\
    _check(marpa_g_error(g, NULL), "marpa_r_new()", r == NULL);		\
  }

#define CREATE_SYMBOL(id, g) {						\
    marpa_g_error_clear(g);						\
    id = marpa_g_symbol_new(g);						\
    _check(marpa_g_error((g), NULL), "marpa_g_symbol_new()", id < 0);	\
  }

#define CREATE_BOCAGE(b, g, r, latest_earley_set_ID) {			\
    marpa_g_error_clear(g);						\
    b = marpa_b_new((r), latest_earley_set_ID);				\
    _check(marpa_g_error((g), NULL), "marpa_b_new()", b == NULL);	\
  }

#define CREATE_ORDER(o, b, g) {						\
    marpa_g_error_clear(g);						\
    o = marpa_o_new(b);							\
    _check(marpa_g_error((g), NULL), "marpa_b_new()", o == NULL);	\
  }

#define CREATE_TREE(t, o, g) {						\
    marpa_g_error_clear(g);						\
    t = marpa_t_new(o);							\
    _check(marpa_g_error((g), NULL), "marpa_t_new()", t == NULL);	\
  }

#define CREATE_VALUATOR(v, t, g) {					\
    marpa_g_error_clear(g);						\
    v = marpa_v_new(t);							\
    _check(marpa_g_error((g), NULL), "marpa_v_new()", v == NULL);	\
  }

#define CREATE_RULE(ruleId, g, ...) {					\
    marpa_g_error_clear(g);						\
    ruleId = marpa_g_rule_new((g), __VA_ARGS__);			\
    _check(marpa_g_error((g), NULL), "marpa_g_rule_new()", ruleId < 0);	\
  }

#define START_INPUT(r, g) {						\
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

#define ALTERNATIVE(r, g, token_id, value, length) {			\
    marpa_g_error_clear(g);						\
    _check(marpa_g_error((g), NULL), "marpa_r_alternative()", marpa_r_alternative((r), (token_id), (value), (length)) != MARPA_ERR_NONE); \
  }

#define EARLEME_COMPLETE(r, g) {					\
    marpa_g_error_clear(g);						\
    _check(marpa_g_error((g), NULL), "marpa_r_earleme_complete()", marpa_r_earleme_complete(r) < 0); \
  }

#endif /* THIN_MACROS_H */

