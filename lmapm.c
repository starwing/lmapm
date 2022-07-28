#include <m_apm.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define LUA_LIB
#include <lua.h>
#include <lauxlib.h>


#define LM_NAME         "mapm"
#define LM_VERSION      LM_NAME " " MAPM_LIB_SHORT_VERSION
#define LM_COPYRIGHT    MAPM_LIB_VERSION
#define LM_TYPE         "mapm bignumber"


/* use lm_State instead of static variables (close defaultly) */
#ifndef LM_STATE
#  define LM_STATE 0
#endif

/* options */
#define LM_WORKS              2 /* count of static working M_APM object. */
#define LM_INPLACE            0 /* it must 0, if you use lm_mul :-( */
#define LM_DEFALUT_TS_PREC   -1 /* default precision of tostring */
#define LM_DEFALUT_PREC     128 /* default precision in float pointing calculate */


#if LUA_VERSION_NUM == 501
#define luaL_newlib(L, b)           luaL_register(L, lua_tostring(L, 1), b)
#define luaL_setmetatable(L, name) (luaL_getmetatable(L, name), lua_setmetatable(L, -2))
#define luaL_setfuncs(L, b, nup)    luaL_register(L, NULL, b)

void *luaL_testudata (lua_State *L, int ud, const char *tname) {
  void *p = lua_touserdata(L, ud);
  if (p != NULL) {  /* value is a userdata? */
    if (lua_getmetatable(L, ud)) {  /* does it have a metatable? */
      luaL_getmetatable(L, tname);  /* get correct metatable */
      if (!lua_rawequal(L, -1, -2))  /* not the same? */
        p = NULL;  /* value is a userdata with wrong metatable */
      lua_pop(L, 2);  /* remove both metatables */
      return p;
    }
  }
  return NULL;  /* value is not a userdata with a metatable */
}
#endif


#if LM_STATE

#define GET_STATE    lm_State *state = \
        (lm_State*)lua_touserdata(L, lua_upvalueindex(1));
#define S(field)     (state->field)
typedef struct lm_State lm_State;

struct lm_State {
    M_APM lm_works[LM_WORKS];
    int lm_tostring_precision;
    int lm_precision;
};

static void lm_state_init(lm_State *state) {
    memset(S(lm_works), 0, sizeof S(lm_works));
    S(lm_tostring_precision) = LM_DEFALUT_TS_PREC;
    S(lm_precision) = LM_DEFALUT_PREC;
}

#else /* !LM_STATE */

#define GET_STATE   /* nothing */
#define S(field)    field
static M_APM lm_works[LM_WORKS] = {NULL};
static int lm_tostring_precision = LM_DEFALUT_TS_PREC;
static int lm_precision = LM_DEFALUT_PREC;

#endif

static lua_State *LL = NULL;

void M_apm_log_error_msg(int fatal, char *message) {
#ifdef IGNORE_MAPM_WARNINGS
    if (fatal)
#endif
        luaL_error(LL, "(MAPM) %s", message);
}

static M_APM lm_get(lua_State *L, int narg) {
    GET_STATE
    M_APM x = NULL;
    LL = L;
    assert(narg <= LM_WORKS);
    switch (lua_type(L, narg)) {
    case LUA_TNUMBER:
        if ((x = S(lm_works)[narg-1]) == NULL)
            x = S(lm_works)[narg-1] = m_apm_init();
        m_apm_set_double(x, lua_tonumber(L, narg));
        break;
    case LUA_TSTRING:
        if ((x = S(lm_works)[narg-1]) == NULL)
            x = S(lm_works)[narg-1] = m_apm_init();
        m_apm_set_string(x, (char*)lua_tostring(L, narg));
        break;
    case LUA_TUSERDATA:
        x = *(M_APM*)luaL_checkudata(L, narg, LM_TYPE);
        break;
    default:
        lua_pushfstring(L, "number/string/mapm bignumber expected, got %s",
                luaL_typename(L, narg));
        lua_error(L);
    }
    return x;
}

static int lm_newvalue(lua_State *L, M_APM x) {
    if (x == NULL)
        lua_pushnil(L);
    else {
        *(M_APM*)lua_newuserdata(L, sizeof(M_APM)) = x;
        luaL_setmetatable(L, LM_TYPE);
    }
    return 1;
}

static int lm_setvalue(lua_State *L, int narg, M_APM x) {
    if (x == NULL)
        lua_pushnil(L);
    else if (lua_type(L, narg) != LUA_TUSERDATA)
        lm_newvalue(L, x);
    else {
        M_APM *px = (M_APM*)lua_touserdata(L, narg);
        if (*px != x) {
            m_apm_free(*px);
            *px = x;
        }
        lua_pushvalue(L, narg);
    }
    return 1;
}

static int lm_new_impl(lua_State *L, int narg) {
    M_APM x = m_apm_init();
    m_apm_copy(x, lm_get(L, narg));
    return lm_newvalue(L, x);
}
static int lm_new(lua_State *L) { return lm_new_impl(L, 1); }
static int lm_libnew(lua_State *L) { return lm_new_impl(L, 2); }

static int lm_gc(lua_State *L) {
    M_APM *px = (M_APM*)luaL_testudata(L, 1, LM_TYPE);
    if (px && *px != NULL) {
        m_apm_free(*px);
        *px = NULL;
        lua_pushnil(L);
        lua_setmetatable(L, 1);
    }
    return 0;
}

static int lm_freeall(lua_State *L) {
    GET_STATE
    int i;
    for (i = 0; i < LM_WORKS; ++i) {
        if (S(lm_works)[i] != NULL) {
            m_apm_free(S(lm_works)[i]);
            S(lm_works)[i] = NULL;
        }
    }
    m_apm_free_all_mem();
    return 0;
}

static int lm_reset(lua_State *L) {
    GET_STATE
    int i;
    for (i = 0; i < LM_WORKS; ++i) {
        if (S(lm_works)[i] != NULL) {
            m_apm_free(S(lm_works)[i]);
            S(lm_works)[i] = NULL;
        }
    }
    m_apm_trim_mem_usage();
    return 0;
}

static int lm_tostring(lua_State *L) {
    GET_STATE
    char *s;
    int n = (int)luaL_optinteger(L, 2, S(lm_tostring_precision));
    M_APM a = lm_get(L, 1);
    if (lua_toboolean(L, 3)) {
        int m = (n < 0) ? m_apm_significant_digits(a) : n;
        s = malloc(m + 16);
        if (s != NULL) m_apm_to_string(s, n, a);
    }
    else
        s = m_apm_to_fixpt_stringexp(
                n < 0 && m_apm_is_integer(a) ? 0 : n,
                a, '.', 0, 0);
    lua_pushstring(L, s);
    if (s != NULL) free(s);
    return 1;
}

static int lm_tonumber(lua_State *L) {
    lua_settop(L,1);
    lua_pushinteger(L,20); /* enough for IEEE doubles */
    lua_pushboolean(L,1);
    lm_tostring(L);
    lua_pushnumber(L,lua_tonumber(L,-1));
    return 0;
}

static int lm_set(lua_State *L) {
    M_APM x = m_apm_init();
    m_apm_copy(x, lm_get(L, 2));
    lm_setvalue(L, 1, x);
    lua_settop(L, 1);
    return 1;
}

static int lm_setprecision(lua_State *L) {
    GET_STATE
    int precision = S(lm_precision);
    int digits = (int)luaL_optinteger(L, 1, S(lm_precision));
    if (!lua_isinteger(L, 2))
        S(lm_tostring_precision) = lua_toboolean(L, 2) ?  -1 : digits;
    else
        S(lm_tostring_precision) = (int)lua_tointeger(L, 2);
    S(lm_precision) = digits < 0 ? 0 : digits;
    lua_pushinteger(L, precision);
    return 1;
}

static int lm_mod_impl(lua_State *L, int inplace) {
    M_APM a = lm_get(L, 1), b = lm_get(L, 2);
    M_APM x = m_apm_init(), y = inplace && LM_INPLACE ? a : m_apm_init();
    m_apm_integer_div_rem(x, y, a, b);
    m_apm_free(x);
    return inplace ? lm_setvalue(L, 1, y) : lm_newvalue(L, y);
}

static int lm_pow_impl(lua_State *L, int inplace) {
    GET_STATE
    M_APM a = lm_get(L, 1);
    M_APM x = inplace && LM_INPLACE ? a : m_apm_init();
    lua_Number b = lua_tonumber(L, 2);
    int n = (int)luaL_optinteger(L, 3, S(lm_precision));
    if (lua_type(L, 2) == LUA_TNUMBER && b == (int)b) {
        int nb = (int)b;
        if (lua_isnoneornil(L, 3) && m_apm_is_integer(a))
            m_apm_integer_pow_nr(x, a, nb);
        else
            m_apm_integer_pow(x, n < 0 ? 0 : n, a, nb);
    }
    else {
        M_APM b = lm_get(L, 2);
        m_apm_pow(x, n < 0 ? 0 : n, a, b);
    }
    return inplace ? lm_setvalue(L, 1, x) : lm_newvalue(L, x);
}

static int lm_divrem_impl(lua_State *L, int inplace) {
    M_APM a = lm_get(L, 1), b = lm_get(L, 2);
    M_APM x = inplace && LM_INPLACE ? a : m_apm_init();
    M_APM y = inplace && LM_INPLACE ? b : m_apm_init();
    m_apm_integer_div_rem(x, y, a, b);
    if (inplace) {
        lm_newvalue(L, x);
        lm_newvalue(L, y);
    }
    else {
        lm_setvalue(L, 1, x);
        lm_setvalue(L, 2, y);
    }
    return 2;
}

#define MAKE_FUNC_I(name) \
    static int lm_##name(lua_State *L) { return lm_##name##_impl(L, 0); } \
    static int lm_i##name(lua_State *L) { return lm_##name##_impl(L, 1); }
MAKE_FUNC_I(mod)
MAKE_FUNC_I(pow)
MAKE_FUNC_I(divrem)
#undef MAKE_FUNC_I

static int lm_eq(lua_State *L) {
    M_APM a = lm_get(L, 1), b = lm_get(L, 2);
    lua_pushboolean(L, m_apm_compare(a, b) == 0);
    return 1;
}

static int lm_lt(lua_State *L) {
    M_APM a = lm_get(L, 1), b = lm_get(L, 2);
    lua_pushboolean(L, m_apm_compare(a, b) < 0);
    return 1;
}

static int lm_cmp(lua_State *L) {
    M_APM a = lm_get(L, 1), b = lm_get(L, 2);
    lua_pushinteger(L, m_apm_compare(a, b));
    return 1;
}

static int lm_random(lua_State *L) {
    M_APM x = m_apm_init();
    m_apm_get_random(x);
    return lm_newvalue(L, x);
}

static int lm_randomseed(lua_State *L) {
    m_apm_set_random_seed((char*)luaL_checkstring(L, 1));
    return 0;
}

static int lm_sin_cos(lua_State *L) {
    GET_STATE
    M_APM a = lm_get(L, 1);
    M_APM x = m_apm_init(), y = m_apm_init();
    int n = (int)luaL_optinteger(L, 2, S(lm_precision));
    m_apm_sin_cos(x, y, n < 0 ? 0 : n, a);
    lm_newvalue(L, x);
    lm_newvalue(L, y);
    return 2;
}

/*  FUNC_(RSI)( prototype, lua_name, mapm_name ) */ \
#define EXPORT_FUNCTIONS \
    FUNC_I( MM,  abs,      absolute_value     ) \
    FUNC_I( MM,  neg,      negate             ) \
    FUNC_I( MM,  copy,     copy               ) \
    FUNC_I( MnM, round,    round              ) \
 /* FUNC_I( nMM, cmp,      compare            ) */ \
    FUNC_R( 'i', sign,     sign               ) \
    FUNC_R( 'i', exponent, exponent           ) \
    FUNC_R( 'i', digits,   significant_digits ) \
    FUNC_R( 'b', isint,    is_integer         ) \
    FUNC_R( 'b', iseven,   is_even            ) \
    FUNC_R( 'b', isodd,    is_odd             ) \
    \
    FUNC_I( MMM, gcd, gcd ) \
    FUNC_I( MMM, lcm, lcm ) \
    \
    FUNC_I( MMM,  add,       add             ) \
    FUNC_I( MMM,  sub,       subtract        ) \
    FUNC_I( MMM,  mul,       multiply        ) \
    FUNC_I( MnMM, div,       divide          ) \
    FUNC_I( MMM,  divi,      integer_divide  ) \
 /* FUNC_I( MMMM, divrem,    integer_div_rem ) */ \
    FUNC_I( MnM,  inv,       reciprocal      ) \
    FUNC_I( MM,   factorial, factorial       ) \
    FUNC_I( MM,   floor,     floor           ) \
    FUNC_I( MM,   ceil,      ceil            ) \
 /* FUNC_R( 'M',  random,    get_random      ) */ \
 /* FUNC_I( c,    randseed,  set_random_seed ) */ \
    \
    FUNC_I( MnM,  sqrt,    sqrt           ) \
    FUNC_I( MnM,  cbrt,    cbrt           ) \
    FUNC_I( MnM,  log,     log            ) \
    FUNC_I( MnM,  log10,   log10          ) \
    FUNC_I( MnM,  exp,     exp            ) \
 /* FUNC_I( MnMM, pow,     pow            ) */ \
 /* FUNC_I( MnMn, powi,    integer_pow    ) */ \
 /* FUNC_I( MMn,  powi_nr, integer_pow_nr ) */ \
    \
 /* FUNC_I( MMnM, sin_cos, sin_cos ) */ \
    FUNC_I( MnM,  sin,     sin     ) \
    FUNC_I( MnM,  cos,     cos     ) \
    FUNC_I( MnM,  tan,     tan     ) \
    FUNC_I( MnM,  asin,    arcsin  ) \
    FUNC_I( MnM,  acos,    arccos  ) \
    FUNC_I( MnM,  atan,    arctan  ) \
    FUNC_I( MnMM, atan2,   arctan2 ) \
    \
    FUNC_I( MnM, sinh,    sinh    ) \
    FUNC_I( MnM, cosh,    cosh    ) \
    FUNC_I( MnM, tanh,    tanh    ) \
    FUNC_I( MnM, asinh,   arcsinh ) \
    FUNC_I( MnM, acosh,   arccosh ) \
    FUNC_I( MnM, atanh,   arctanh ) \


typedef void (*mapm_fMM)   (M_APM x, M_APM a);
typedef void (*mapm_fMnM)  (M_APM x, int n, M_APM a);
typedef void (*mapm_fMMM)  (M_APM x, M_APM a, M_APM b);
typedef void (*mapm_fMnMM) (M_APM x, int n, M_APM a, M_APM b);

static M_APM lm_doMM(lua_State *L, int inplace, mapm_fMM f) {
    M_APM a = lm_get(L, 1);
    M_APM x = inplace ? a : m_apm_init();
    f(x, a);
    return x;
}

static M_APM lm_doMnM(lua_State *L, int inplace, mapm_fMnM f) {
    GET_STATE
    M_APM a = lm_get(L, 1);
    M_APM x = inplace ? a : m_apm_init();
    int n = (int)luaL_optinteger(L, 2, S(lm_precision));
    f(x, n < 0 ? 0 : n, a);
    return x;
}

static M_APM lm_doMMM(lua_State *L, int inplace, mapm_fMMM f) {
    M_APM a = lm_get(L, 1), b = lm_get(L, 2);
    M_APM x = inplace ? a : m_apm_init();
    f(x, a, b);
    return x;
}

static M_APM lm_doMnMM(lua_State *L, int inplace, mapm_fMnMM f) {
    GET_STATE
    M_APM a = lm_get(L, 1), b = lm_get(L, 2);
    M_APM x = inplace ? a : m_apm_init();
    int n = (int)luaL_optinteger(L, 3, S(lm_precision));
    f(x, n < 0 ? 0 : n, a, b);
    return x;
}

#define FUNC_R(n, name, func) \
    static int lm_##name (lua_State *L) { \
        M_APM a = lm_get(L, 1); \
             if (n == 'i') lua_pushinteger(L, m_apm_##func(a)); \
        else if (n == 'b') lua_pushboolean(L, m_apm_##func(a)); \
        else return 0; return 1; }
#define FUNC_S(s, name, func) \
    static int lm_##name (lua_State *L) { \
        return lm_newvalue(L, lm_do##s(L, 0, m_apm_##func)); }
#define FUNC_I(s, name, func) FUNC_S(s, name, func) \
    static int lm_i##name (lua_State *L) { \
        return lm_setvalue(L, 1, lm_do##s(L, LM_INPLACE, m_apm_##func)); }
EXPORT_FUNCTIONS
#undef FUNC_R
#undef FUNC_S
#undef FUNC_I

static void lm_init_constants(lua_State *L) {
    struct {
        const char *name;
        M_APM *pn;
    } lm_constants[] = {
        { "pi",     &MM_PI            },
        { "halfpi", &MM_HALF_PI       },
        { "twopi",  &MM_2_PI          },
        { "e",      &MM_E             },
        { "ln10",   &MM_LOG_E_BASE_10 },
        { "loge",   &MM_LOG_10_BASE_E },
        { "ln2",    &MM_LOG_2_BASE_E  },
        { "ln3",    &MM_LOG_3_BASE_E  },
        { NULL, NULL }
    }, *p = lm_constants;
    /*lua_newtable(L);*/
    for (; p->name != NULL; ++p) {
        M_APM x = m_apm_init();
        m_apm_copy(x, *p->pn);
        lm_newvalue(L, x);
        lua_setfield(L, -2, p->name);
    }
    /*lua_setfield(L, -2, "constants");*/
}

static void lm_install_freehook(lua_State *L) {
    lua_newuserdata(L, 1);
    lua_newtable(L);
#if LM_STATE
    lua_pushvalue(L, -5); /* state libtable mttable ud table */
    lua_pushcclosure(L, lm_freeall, 1);
#else
    lua_pushcfunction(L, lm_freeall);
#endif
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    lua_setfield(L, -2, "__freeall");
}

LUALIB_API int luaopen_mapm(lua_State *L) {
    luaL_Reg mapm_funcs[] = {
        { "delete",   lm_gc           },
        { "setprec",  lm_setprecision },
#define FUNC_R(s, name, func) { #name, lm_##name },
#define FUNC_S(s, name, func) { #name, lm_##name },
#define FUNC_I(s, name, func) { #name, lm_##name }, { "i"#name, lm_i##name },
        EXPORT_FUNCTIONS
#undef FUNC_R
#undef FUNC_S
#undef FUNC_I
#define ENTRY(name) { #name, lm_##name }
        ENTRY(new),
        ENTRY(freeall),
        ENTRY(reset),
        ENTRY(tostring),
        ENTRY(tonumber),
        ENTRY(set),
        ENTRY(mod),    ENTRY(imod),
        ENTRY(pow),    ENTRY(ipow),
        ENTRY(divrem), ENTRY(idivrem),
        ENTRY(cmp),
        ENTRY(eq),
        ENTRY(random),
        ENTRY(randomseed),
        ENTRY(sin_cos),
#undef ENTRY
        { NULL, NULL }
    };

    luaL_Reg mapm_meta[] = {
        { "__len",   lm_digits },
        { "__idiv",  lm_divi   },
        { "__close", lm_gc     },
#define ENTRY(name) { "__" #name, lm_##name }
        ENTRY(add),
        ENTRY(sub),
        ENTRY(mul),
        ENTRY(div),
        ENTRY(mod),
        ENTRY(pow),
        ENTRY(neg),
        ENTRY(eq),
        ENTRY(lt),
        ENTRY(gc),
        ENTRY(tostring),
#undef  ENTRY
        { NULL, NULL }
    };

#if LM_STATE
    lm_state_init(lua_newuserdata(L, sizeof(lm_State)));
    lua_createtable(L, 0, sizeof(mapm_funcs)/sizeof(mapm_funcs[0])-1);
    lua_pushvalue(L, -2);
    luaL_setfuncs(L, mapm_funcs, 1);
    if (luaL_newmetatable(L, LM_TYPE)) {
        lua_pushvalue(L, -3);
        luaL_setfuncs(L, mapm_meta, 1);
    }
#else
    luaL_newlib(L, mapm_funcs);
    if (luaL_newmetatable(L, LM_TYPE))
        luaL_setfuncs(L, mapm_meta, 0);
#endif
    lua_pushstring(L, LM_VERSION);
    lua_setfield(L, -3, "_VERSION");
    lua_pushstring(L, LM_COPYRIGHT);
    lua_setfield(L, -3, "_COPYRIGHT");
    lua_pushvalue(L, -2);
    lua_setfield(L, -2, "__index");
    lm_install_freehook(L);
    lua_pop(L, 1);
    lm_init_constants(L);
    lua_createtable(L, 0, 1);
#if LM_STATE
    lua_pushvalue(L, -3); /* state libtable table */
    lua_pushcclosure(L, lm_libnew, 1);
#else
    lua_pushcfunction(L, lm_libnew);
#endif
    lua_setfield(L, -2, "__call");
    lua_setmetatable(L, -2);
    return 1;
}

/* win32cc: flags+='-mdll -O3  -DLUA_BUILD_AS_DLL -Imapm' input+='mapm_one.c'
 * win32cc: output='mapm.dll' libs+='-llua54' */

