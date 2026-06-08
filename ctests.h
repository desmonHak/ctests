/*
 * VestaVM - Máquina Virtual Distribuida
 *
 * Copyright © 2026 David López.T (DesmonHak) (Castilla y León, ES)
 * Licencia VMProject
 *
 * USO LIBRE NO COMERCIAL con atribución obligatoria.
 * PROHIBIDO lucro sin permiso escrito.
 *
 * Descargo: Autor no responsable por modificaciones.
 */

/**
 * @file ctests.h
 * @brief Libreria de tests unitarios e integracion — C99/C++17.
 *
 * @details Sin dependencias externas. Compatible con C puro (C99) y C++17.
 *   Mecanismo: setjmp/longjmp para detener el test al primer EXPECT_* fallido.
 *   Los resultados se bufferean por suite para imprimir la cabecera con conteos exactos.
 *
 * ## Como integrarla (dos modos, elige UNO)
 *
 * **Modo A — un solo archivo (estilo "single header"):** en EXACTAMENTE un .c/.cpp
 * define la macro de implementacion antes de incluir el header:
 * @code
 *   #define CTESTS_IMPLEMENTATION
 *   #include "ctests.h"
 * @endcode
 * En el resto de archivos de test incluye `ctests.h` con normalidad (sin la macro).
 *
 * **Modo B — compilando ctests.c (recomendado con CMake):** anade `ctests.c` a tu
 * build (o enlaza el target CMake `ctests::ctests`) e incluye `ctests.h` en todos
 * tus archivos de test. CMake lo hace por ti.
 *
 * En ambos modos puedes repartir tus tests entre varios archivos: el estado es
 * compartido (símbolos `extern`), no se duplica.
 *
 * Compilacion manual (modo A):
 * @code
 *   gcc  -std=c99   mis_tests.c   -o mis_tests -lm && ./mis_tests
 *   g++  -std=c++17 mis_tests.cpp -o mis_tests -lm && ./mis_tests
 * @endcode
 *
 * @note Las macros de configuracion (TT_MAX_TESTS, TT_MSG_MAX, ...) deben ser
 *       consistentes en todos los archivos que incluyan este header.
 */
#ifndef ctests_H
#define ctests_H

/* Version de la libreria (semver). TT_VERSION_NUM permite comparaciones:
 * #if TT_VERSION_NUM >= 10000  (1.0.0). */
#define TT_VERSION_MAJOR 1
#define TT_VERSION_MINOR 0
#define TT_VERSION_PATCH 0
#define TT_VERSION "1.0.0"
#define TT_VERSION_NUM (TT_VERSION_MAJOR * 10000 + TT_VERSION_MINOR * 100 + TT_VERSION_PATCH)

/* Includes necesarios para la SUPERFICIE de la API (tipos y macros de asercion). */
#include <math.h>   /* fabs   — EXPECT_NEAR */
#include <setjmp.h> /* jmp_buf, longjmp */
#include <stddef.h> /* size_t — EXPECT_EQ_MEM */
#include <stdio.h>  /* snprintf */
#include <string.h> /* strcmp, strstr, strncmp, strlen, memcmp */

/* setjmp portable: en POSIX usamos sigsetjmp/siglongjmp (savesigs=1) para que,
 * tras recuperarnos de una senal (crash), la mascara quede restaurada y la misma
 * senal pueda volver a capturarse en tests posteriores. En Windows/otros, el
 * setjmp normal. Define CTESTS_NO_SIGJMP para forzar setjmp simple. */
#if (defined(__unix__) || defined(__APPLE__)) && !defined(CTESTS_NO_SIGJMP)
typedef sigjmp_buf _tt_jmpbuf;
#define _TT_SETJMP(b) sigsetjmp((b), 1)
#define _TT_LONGJMP(b, v) siglongjmp((b), (v))
#else
typedef jmp_buf _tt_jmpbuf;
#define _TT_SETJMP(b) setjmp(b)
#define _TT_LONGJMP(b, v) longjmp((b), (v))
#endif

/* =============================================================================
 * CONFIGURACION — ajustar segun el proyecto (consistente entre archivos)
 * ============================================================================= */

/** Maximo de tests por suite antes de hacer flush. */
#ifndef TT_MAX_TESTS
#define TT_MAX_TESTS 512
#endif

/** Longitud maxima del mensaje de error de una asercion. */
#ifndef TT_MSG_MAX
#define TT_MSG_MAX 512
#endif

/** Longitud maxima de un nombre de test o suite. */
#ifndef TT_NAME_MAX
#define TT_NAME_MAX 128
#endif

/** Maximo de soft-assertions acumuladas por test. */
#ifndef TT_MAX_SOFT
#define TT_MAX_SOFT 16
#endif

/** Maximo de fallos registrados en la tabla resumen final. */
#ifndef TT_MAX_FAILURES
#define TT_MAX_FAILURES 256
#endif

/** Longitud maxima del patron de filtrado (--filter). */
#ifndef TT_FILTER_MAX
#define TT_FILTER_MAX 256
#endif

/** Maximo de tests auto-registrados con la macro TEST(). */
#ifndef TT_MAX_REGISTERED
#define TT_MAX_REGISTERED 1024
#endif

/** Maximo de suites con hooks once registrados (TT_REGISTER_SUITE_HOOKS). */
#ifndef TT_MAX_SUITE_HOOKS
#define TT_MAX_SUITE_HOOKS 64
#endif

/* =============================================================================
 * API PUBLICA — declaraciones
 * ============================================================================= */

#ifdef __cplusplus
extern "C"
{
#endif

    /** Firma de funcion de test: sin argumentos, sin retorno. */
    typedef void (*tt_fn)(void);

    /* --- Estado compartido referenciado por las macros de asercion ---------------
     * Estos simbolos los usan las macros que se expanden en TUS archivos, por eso
     * son visibles (extern). No los manipules directamente salvo tt_param. */
    extern _tt_jmpbuf _tt_jmp;
    extern int _tt_jmp_code;
    extern char _tt_errmsg[TT_MSG_MAX];
    extern char _tt_soft[TT_MAX_SOFT][TT_MSG_MAX];
    extern int _tt_soft_n;

    /** Dato del caso actual en tests data-driven; lo fija tt_run_param(). */
    extern const void *tt_param;

    /* --- Backends de las aserciones unificadas (EXPECT_EQ/EXPECT_NE) ----------
     * Comparan dos valores ya tipados; en fallo reportan archivo:linea y o bien
     * paran (soft=0) o acumulan (soft=1). Los selecciona _Generic (C) o una
     * plantilla (C++). No usar directamente. */
    void _tt_eq_ll(const char *f, int ln, const char *ea, const char *eb, long long a, long long b, int soft);
    void _tt_eq_ull(const char *f, int ln, const char *ea, const char *eb, unsigned long long a, unsigned long long b, int soft);
    void _tt_eq_str(const char *f, int ln, const char *ea, const char *eb, const char *a, const char *b, int soft);
    void _tt_eq_dbl(const char *f, int ln, const char *ea, const char *eb, double a, double b, int soft);
    void _tt_eq_ptr(const char *f, int ln, const char *ea, const char *eb, const void *a, const void *b, int soft);
    void _tt_ne_ll(const char *f, int ln, const char *ea, const char *eb, long long a, long long b, int soft);
    void _tt_ne_ull(const char *f, int ln, const char *ea, const char *eb, unsigned long long a, unsigned long long b, int soft);
    void _tt_ne_str(const char *f, int ln, const char *ea, const char *eb, const char *a, const char *b, int soft);
    void _tt_ne_dbl(const char *f, int ln, const char *ea, const char *eb, double a, double b, int soft);
    void _tt_ne_ptr(const char *f, int ln, const char *ea, const char *eb, const void *a, const void *b, int soft);

    /* --- Configuracion --- */
    void tt_verbose(int v);                    /**< 0=silencioso, 1=normal, 2=detallado. */
    void tt_color(int mode);                   /**< -1=auto, 0=sin color, 1=con color. */
    void tt_output_junit(const char *path);    /**< Activa informe JUnit XML. */
    void tt_output_tap(const char *path);      /**< Activa informe TAP (Test Anything Protocol). */
    void tt_catch_crashes(int on);             /**< 1 (defecto): un crash en un test se reporta y la suite sigue. */
    void tt_timeout(int seconds);              /**< Limite por test en segundos (0=sin limite). Solo POSIX. */
    void tt_parse_args(int argc, char **argv); /**< --filter/--verbose/--junit/--tap/--color/--no-catch/--timeout/--help. */

    /* --- Suites --- */
    void tt_suite(const char *name);
    void tt_suite_hooks(tt_fn setup_fn, tt_fn teardown_fn);      /**< Antes/despues de CADA test. */
    void tt_suite_hooks_once(tt_fn before_all, tt_fn after_all); /**< Una vez por suite. */

    /* --- Captura de stdout/stderr (para testear lo que imprime el codigo) --- */
    void tt_capture_begin(void);                  /**< Redirige stdout+stderr a un buffer temporal. */
    size_t tt_capture_end(char *buf, size_t cap); /**< Restaura y copia lo capturado a buf (NUL-terminado). */

    /* --- Tests --- */
    void tt_run(const char *name, tt_fn fn);
    void tt_run_param(const char *name, tt_fn fn, const void *param);
    void tt_skip_test(const char *name, const char *reason);
    void tt_xfail(const char *name, const char *reason, tt_fn fn);

    /* --- Auto-registro (macro TEST) --- */
    void tt_register(const char *suite, const char *name, tt_fn fn);                     /**< Lo llama TEST(); rara vez a mano. */
    void tt_register_suite_hooks(const char *suite, tt_fn before_all, tt_fn after_all);  /**< Hooks once por suite (lo usa ctgen). */
    int tt_run_all(void);                                                                /**< Ejecuta los tests auto-registrados y devuelve tt_summary(). */

    /* --- Resumen --- */
    int tt_summary(void); /**< 0 si todo paso, 1 si hubo fallos. Propagar desde main. */

#ifdef __cplusplus
} /* extern "C" */
#endif

/* =============================================================================
 * MACROS DE ASERCION — hard: detienen el test al primer fallo
 * ============================================================================= */

/** @brief Macro interna: hard-fail con mensaje formateado, prefijado con archivo:linea. */
#define _TT_FAIL(...)                                                                \
    do                                                                               \
    {                                                                                \
        int _tt_o = snprintf(_tt_errmsg, TT_MSG_MAX, "%s:%d: ", __FILE__, __LINE__); \
        if (_tt_o < 0 || _tt_o > TT_MSG_MAX)                                         \
            _tt_o = 0;                                                               \
        snprintf(_tt_errmsg + _tt_o, (size_t)(TT_MSG_MAX - _tt_o), __VA_ARGS__);     \
        _tt_jmp_code = 1;                                                            \
        _TT_LONGJMP(_tt_jmp, 1);                                                     \
    } while (0)

/** @brief Macro interna: soft-fail — acumula el mensaje (con archivo:linea) sin parar. */
#define _TT_SOFT_FAIL(...)                                                       \
    do                                                                           \
    {                                                                            \
        if (_tt_soft_n < TT_MAX_SOFT)                                            \
        {                                                                        \
            int _tt_o = snprintf(_tt_soft[_tt_soft_n], TT_MSG_MAX, "%s:%d: ",    \
                                 __FILE__, __LINE__);                            \
            if (_tt_o < 0 || _tt_o > TT_MSG_MAX)                                 \
                _tt_o = 0;                                                       \
            snprintf(_tt_soft[_tt_soft_n] + _tt_o, (size_t)(TT_MSG_MAX - _tt_o), \
                     __VA_ARGS__);                                               \
            _tt_soft_n++;                                                        \
        }                                                                        \
    } while (0)

/**
 * @brief Salta el test actual con un motivo. Llamar desde dentro del test.
 * @param reason Cadena de texto explicando el salto.
 */
#define tt_skip(reason)                                \
    do                                                 \
    {                                                  \
        strncpy(_tt_errmsg, (reason), TT_MSG_MAX - 1); \
        _tt_jmp_code = 2;                              \
        _TT_LONGJMP(_tt_jmp, 2);                       \
    } while (0)

/* Condiciones */
#define EXPECT_TRUE(c)                         \
    do                                         \
    {                                          \
        if (!(c))                              \
            _TT_FAIL("Expected TRUE: %s", #c); \
    } while (0)
#define EXPECT_FALSE(c)                         \
    do                                          \
    {                                           \
        if ((c))                                \
            _TT_FAIL("Expected FALSE: %s", #c); \
    } while (0)

/* Condicion con mensaje personalizado (printf-style) */
#define EXPECT_MSG(c, ...)         \
    do                             \
    {                              \
        if (!(c))                  \
            _TT_FAIL(__VA_ARGS__); \
    } while (0)

/* Punteros */
#define EXPECT_NULL(p)                         \
    do                                         \
    {                                          \
        if ((p) != NULL)                       \
            _TT_FAIL("Expected NULL: %s", #p); \
    } while (0)
#define EXPECT_NOT_NULL(p)                         \
    do                                             \
    {                                              \
        if ((p) == NULL)                           \
            _TT_FAIL("Expected non-NULL: %s", #p); \
    } while (0)
#define EXPECT_EQ_PTR(a, b)                                                   \
    do                                                                        \
    {                                                                         \
        const void *_a = (const void *)(a), *_b = (const void *)(b);          \
        if (_a != _b)                                                         \
            _TT_FAIL("\n  - Expected ptr: %p\n  + Received ptr: %p", _b, _a); \
    } while (0)

/* Enteros con signo */
#define EXPECT_EQ_INT(a, b)                                               \
    do                                                                    \
    {                                                                     \
        long long _a = (long long)(a), _b = (long long)(b);               \
        if (_a != _b)                                                     \
            _TT_FAIL("\n  - Expected: %lld\n  + Received: %lld", _b, _a); \
    } while (0)
#define EXPECT_NEQ_INT(a, b)                                               \
    do                                                                     \
    {                                                                      \
        long long _a = (long long)(a), _b = (long long)(b);                \
        if (_a == _b)                                                      \
            _TT_FAIL("%s == %s == %lld (expected different)", #a, #b, _a); \
    } while (0)

/* Enteros sin signo */
#define EXPECT_EQ_UINT(a, b)                                                           \
    do                                                                                 \
    {                                                                                  \
        unsigned long long _a = (unsigned long long)(a), _b = (unsigned long long)(b); \
        if (_a != _b)                                                                  \
            _TT_FAIL("\n  - Expected: %llu\n  + Received: %llu", _b, _a);              \
    } while (0)
#define EXPECT_NEQ_UINT(a, b)                                                          \
    do                                                                                 \
    {                                                                                  \
        unsigned long long _a = (unsigned long long)(a), _b = (unsigned long long)(b); \
        if (_a == _b)                                                                  \
            _TT_FAIL("%s == %s == %llu (expected different)", #a, #b, _a);             \
    } while (0)

/* Flotantes */
#define EXPECT_NEAR(a, b, eps)                                                                \
    do                                                                                        \
    {                                                                                         \
        double _d = fabs((double)(a) - (double)(b));                                          \
        if (_d > (double)(eps))                                                               \
            _TT_FAIL("|%g - %g| = %g > eps=%g", (double)(a), (double)(b), _d, (double)(eps)); \
    } while (0)

/* Flotantes con tolerancia RELATIVA: |a-b| <= rel * max(|a|,|b|). Mejor que la
 * absoluta cuando las magnitudes son grandes. EXPECT_EQ_FLOAT usa rel=1e-9. */
#define EXPECT_NEAR_REL(a, b, rel)                                     \
    do                                                                 \
    {                                                                  \
        double _a = (double)(a), _b = (double)(b);                     \
        double _m = fabs(_a) > fabs(_b) ? fabs(_a) : fabs(_b);         \
        double _t = (double)(rel) * _m, _d = fabs(_a - _b);            \
        if (_d > _t)                                                   \
            _TT_FAIL("|%g - %g| = %g > rel*max = %g", _a, _b, _d, _t); \
    } while (0)
#define EXPECT_EQ_FLOAT(a, b) EXPECT_NEAR_REL((a), (b), 1e-9)

/* Comparaciones de orden */
#define EXPECT_GT(a, b)                                                                       \
    do                                                                                        \
    {                                                                                         \
        if (!((a) > (b)))                                                                     \
            _TT_FAIL("%s (%lld) is not > %s (%lld)", #a, (long long)(a), #b, (long long)(b)); \
    } while (0)
#define EXPECT_GE(a, b)                                                                        \
    do                                                                                         \
    {                                                                                          \
        if (!((a) >= (b)))                                                                     \
            _TT_FAIL("%s (%lld) is not >= %s (%lld)", #a, (long long)(a), #b, (long long)(b)); \
    } while (0)
#define EXPECT_LT(a, b)                                                                       \
    do                                                                                        \
    {                                                                                         \
        if (!((a) < (b)))                                                                     \
            _TT_FAIL("%s (%lld) is not < %s (%lld)", #a, (long long)(a), #b, (long long)(b)); \
    } while (0)
#define EXPECT_LE(a, b)                                                                        \
    do                                                                                         \
    {                                                                                          \
        if (!((a) <= (b)))                                                                     \
            _TT_FAIL("%s (%lld) is not <= %s (%lld)", #a, (long long)(a), #b, (long long)(b)); \
    } while (0)

/* Cadenas */
#define EXPECT_EQ_STR(a, b)                                                     \
    do                                                                          \
    {                                                                           \
        if (strcmp((a), (b)) != 0)                                              \
            _TT_FAIL("\n  - Expected: \"%s\"\n  + Received: \"%s\"", (b), (a)); \
    } while (0)
#define EXPECT_NEQ_STR(a, b)                                          \
    do                                                                \
    {                                                                 \
        if (strcmp((a), (b)) == 0)                                    \
            _TT_FAIL("Strings should differ but both = \"%s\"", (a)); \
    } while (0)
#define EXPECT_CONTAINS(s, sub)                                     \
    do                                                              \
    {                                                               \
        if (!strstr((s), (sub)))                                    \
            _TT_FAIL("\"%s\" does not contain \"%s\"", (s), (sub)); \
    } while (0)
#define EXPECT_STARTS_WITH(s, pre)                                     \
    do                                                                 \
    {                                                                  \
        if (strncmp((s), (pre), strlen((pre))) != 0)                   \
            _TT_FAIL("\"%s\" does not start with \"%s\"", (s), (pre)); \
    } while (0)

/* Bloques de memoria */
#define EXPECT_EQ_MEM(a, b, n)                                       \
    do                                                               \
    {                                                                \
        if (memcmp((a), (b), (n)) != 0)                              \
        {                                                            \
            size_t _i;                                               \
            const unsigned char *_pa = (const unsigned char *)(a);   \
            const unsigned char *_pb = (const unsigned char *)(b);   \
            for (_i = 0; _i < (size_t)(n); _i++)                     \
                if (_pa[_i] != _pb[_i])                              \
                    break;                                           \
            _TT_FAIL("memory differs at byte %lu: 0x%02X != 0x%02X", \
                     (unsigned long)_i, _pa[_i], _pb[_i]);           \
        }                                                            \
    } while (0)

/* Igualdad de arrays elemento a elemento (operador ==); reporta el indice del
 * primero que difiere. A diferencia de EXPECT_EQ_MEM, no compara byte a byte. */
#define EXPECT_ARRAY_EQ(a, b, n)                                       \
    do                                                                 \
    {                                                                  \
        size_t _i;                                                     \
        int _ok = 1;                                                   \
        for (_i = 0; _i < (size_t)(n); _i++)                           \
            if (!((a)[_i] == (b)[_i]))                                 \
            {                                                          \
                _ok = 0;                                               \
                break;                                                 \
            }                                                          \
        if (!_ok)                                                      \
            _TT_FAIL("arrays differ at index %lu", (unsigned long)_i); \
    } while (0)

/* Rango cerrado: lo <= x <= hi (se evalua como double). */
#define EXPECT_IN_RANGE(x, lo, hi)                                       \
    do                                                                   \
    {                                                                    \
        double _x = (double)(x), _lo = (double)(lo), _hi = (double)(hi); \
        if (_x < _lo || _x > _hi)                                        \
            _TT_FAIL("%s = %g not in [%g, %g]", #x, _x, _lo, _hi);       \
    } while (0)

/* Fallo incondicional */
#define EXPECT_FAIL(msg) _TT_FAIL("%s", msg)

/* =============================================================================
 * API UNIFICADA (RECOMENDADA) — EXPECT_EQ / EXPECT_NE para CUALQUIER tipo
 *
 * Un solo EXPECT_EQ(a, b) sirve para enteros (con/sin signo), cadenas (strcmp),
 * flotantes (tolerancia relativa) y punteros: el tipo se deduce de 'a'. Mismo
 * diff bonito que las macros tipadas. Convencion: EXPECT_EQ(recibido, esperado).
 *
 * Requiere C11 (_Generic) o C++ (plantillas). En C99 usa las macros tipadas
 * (EXPECT_EQ_INT, EXPECT_EQ_STR, ...) que siguen disponibles.
 * ============================================================================= */

#if defined(__cplusplus)
#include <type_traits>
/* Despacho por el tipo de 'a' (C++17 if constexpr) hacia el backend en C. */
template <class A, class B>
static inline void _tt_eq_dispatch(const char *f, int ln, const char *ea, const char *eb, A a, B b, int soft)
{
    if constexpr (std::is_same<A, char *>::value || std::is_same<A, const char *>::value)
        _tt_eq_str(f, ln, ea, eb, (const char *)a, (const char *)b, soft);
    else if constexpr (std::is_floating_point<A>::value)
        _tt_eq_dbl(f, ln, ea, eb, (double)a, (double)b, soft);
    else if constexpr (std::is_pointer<A>::value)
        _tt_eq_ptr(f, ln, ea, eb, (const void *)a, (const void *)b, soft);
    else if constexpr (std::is_unsigned<A>::value)
        _tt_eq_ull(f, ln, ea, eb, (unsigned long long)a, (unsigned long long)b, soft);
    else
        _tt_eq_ll(f, ln, ea, eb, (long long)a, (long long)b, soft);
}
template <class A, class B>
static inline void _tt_ne_dispatch(const char *f, int ln, const char *ea, const char *eb, A a, B b, int soft)
{
    if constexpr (std::is_same<A, char *>::value || std::is_same<A, const char *>::value)
        _tt_ne_str(f, ln, ea, eb, (const char *)a, (const char *)b, soft);
    else if constexpr (std::is_floating_point<A>::value)
        _tt_ne_dbl(f, ln, ea, eb, (double)a, (double)b, soft);
    else if constexpr (std::is_pointer<A>::value)
        _tt_ne_ptr(f, ln, ea, eb, (const void *)a, (const void *)b, soft);
    else if constexpr (std::is_unsigned<A>::value)
        _tt_ne_ull(f, ln, ea, eb, (unsigned long long)a, (unsigned long long)b, soft);
    else
        _tt_ne_ll(f, ln, ea, eb, (long long)a, (long long)b, soft);
}
#define EXPECT_EQ(a, b) _tt_eq_dispatch(__FILE__, __LINE__, #a, #b, (a), (b), 0)
#define EXPECT_NE(a, b) _tt_ne_dispatch(__FILE__, __LINE__, #a, #b, (a), (b), 0)
#define SOFT_EXPECT_EQ(a, b) _tt_eq_dispatch(__FILE__, __LINE__, #a, #b, (a), (b), 1)
#define SOFT_EXPECT_NE(a, b) _tt_ne_dispatch(__FILE__, __LINE__, #a, #b, (a), (b), 1)
#define CTESTS_HAS_GENERIC 1

#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
/* C11: _Generic elige el backend segun el tipo de 'a'. */
#define _TT_EQ_FN(a) _Generic((a),                                                       \
    char *: _tt_eq_str, const char *: _tt_eq_str,                                        \
    float: _tt_eq_dbl, double: _tt_eq_dbl,                                               \
    _Bool: _tt_eq_ll, char: _tt_eq_ll, signed char: _tt_eq_ll, short: _tt_eq_ll,         \
    int: _tt_eq_ll, long: _tt_eq_ll, long long: _tt_eq_ll,                               \
    unsigned char: _tt_eq_ull, unsigned short: _tt_eq_ull, unsigned: _tt_eq_ull,         \
    unsigned long: _tt_eq_ull, unsigned long long: _tt_eq_ull,                           \
    default: _tt_eq_ptr)
#define _TT_NE_FN(a) _Generic((a),                                                       \
    char *: _tt_ne_str, const char *: _tt_ne_str,                                        \
    float: _tt_ne_dbl, double: _tt_ne_dbl,                                               \
    _Bool: _tt_ne_ll, char: _tt_ne_ll, signed char: _tt_ne_ll, short: _tt_ne_ll,         \
    int: _tt_ne_ll, long: _tt_ne_ll, long long: _tt_ne_ll,                               \
    unsigned char: _tt_ne_ull, unsigned short: _tt_ne_ull, unsigned: _tt_ne_ull,         \
    unsigned long: _tt_ne_ull, unsigned long long: _tt_ne_ull,                           \
    default: _tt_ne_ptr)
#define EXPECT_EQ(a, b) _TT_EQ_FN(a)(__FILE__, __LINE__, #a, #b, (a), (b), 0)
#define EXPECT_NE(a, b) _TT_NE_FN(a)(__FILE__, __LINE__, #a, #b, (a), (b), 0)
#define SOFT_EXPECT_EQ(a, b) _TT_EQ_FN(a)(__FILE__, __LINE__, #a, #b, (a), (b), 1)
#define SOFT_EXPECT_NE(a, b) _TT_NE_FN(a)(__FILE__, __LINE__, #a, #b, (a), (b), 1)
#define CTESTS_HAS_GENERIC 1
#endif

/* =============================================================================
 * ALIAS ASSERT_* — equivalentes a las EXPECT_* (en ctests, EXPECT ya es "hard")
 * ============================================================================= */

#define ASSERT_TRUE EXPECT_TRUE
#define ASSERT_FALSE EXPECT_FALSE
#define ASSERT_MSG EXPECT_MSG
#define ASSERT_NULL EXPECT_NULL
#define ASSERT_NOT_NULL EXPECT_NOT_NULL
#define ASSERT_EQ_PTR EXPECT_EQ_PTR
#define ASSERT_EQ_INT EXPECT_EQ_INT
#define ASSERT_NEQ_INT EXPECT_NEQ_INT
#define ASSERT_EQ_UINT EXPECT_EQ_UINT
#define ASSERT_NEQ_UINT EXPECT_NEQ_UINT
#define ASSERT_NEAR EXPECT_NEAR
#define ASSERT_NEAR_REL EXPECT_NEAR_REL
#define ASSERT_EQ_FLOAT EXPECT_EQ_FLOAT
#define ASSERT_IN_RANGE EXPECT_IN_RANGE
#define ASSERT_ARRAY_EQ EXPECT_ARRAY_EQ
#define ASSERT_GT EXPECT_GT
#define ASSERT_GE EXPECT_GE
#define ASSERT_LT EXPECT_LT
#define ASSERT_LE EXPECT_LE
#define ASSERT_EQ_STR EXPECT_EQ_STR
#define ASSERT_NEQ_STR EXPECT_NEQ_STR
#define ASSERT_CONTAINS EXPECT_CONTAINS
#define ASSERT_STARTS_WITH EXPECT_STARTS_WITH
#define ASSERT_EQ_MEM EXPECT_EQ_MEM
#define ASSERT_FAIL EXPECT_FAIL
#ifdef CTESTS_HAS_GENERIC
#define ASSERT_EQ EXPECT_EQ
#define ASSERT_NE EXPECT_NE
#endif

/* =============================================================================
 * MACROS SOFT — acumulan fallos, el test falla al terminar la funcion
 * ============================================================================= */

#define SOFT_EXPECT_TRUE(c)                         \
    do                                              \
    {                                               \
        if (!(c))                                   \
            _TT_SOFT_FAIL("Expected TRUE: %s", #c); \
    } while (0)
#define SOFT_EXPECT_FALSE(c)                         \
    do                                               \
    {                                                \
        if ((c))                                     \
            _TT_SOFT_FAIL("Expected FALSE: %s", #c); \
    } while (0)
#define SOFT_EXPECT_MSG(c, ...)         \
    do                                  \
    {                                   \
        if (!(c))                       \
            _TT_SOFT_FAIL(__VA_ARGS__); \
    } while (0)
#define SOFT_EXPECT_NULL(p)                         \
    do                                              \
    {                                               \
        if ((p) != NULL)                            \
            _TT_SOFT_FAIL("Expected NULL: %s", #p); \
    } while (0)
#define SOFT_EXPECT_NOT_NULL(p)                         \
    do                                                  \
    {                                                   \
        if ((p) == NULL)                                \
            _TT_SOFT_FAIL("Expected non-NULL: %s", #p); \
    } while (0)
#define SOFT_EXPECT_EQ_INT(a, b)                                               \
    do                                                                         \
    {                                                                          \
        long long _a = (long long)(a), _b = (long long)(b);                    \
        if (_a != _b)                                                          \
            _TT_SOFT_FAIL("\n  - Expected: %lld\n  + Received: %lld", _b, _a); \
    } while (0)
#define SOFT_EXPECT_EQ_UINT(a, b)                                                      \
    do                                                                                 \
    {                                                                                  \
        unsigned long long _a = (unsigned long long)(a), _b = (unsigned long long)(b); \
        if (_a != _b)                                                                  \
            _TT_SOFT_FAIL("\n  - Expected: %llu\n  + Received: %llu", _b, _a);         \
    } while (0)
#define SOFT_EXPECT_EQ_STR(a, b)                                                     \
    do                                                                               \
    {                                                                                \
        if (strcmp((a), (b)) != 0)                                                   \
            _TT_SOFT_FAIL("\n  - Expected: \"%s\"\n  + Received: \"%s\"", (b), (a)); \
    } while (0)
#define SOFT_EXPECT_NEAR(a, b, e)                                                                \
    do                                                                                           \
    {                                                                                            \
        double _d = fabs((double)(a) - (double)(b));                                             \
        if (_d > (double)(e))                                                                    \
            _TT_SOFT_FAIL("|%g - %g| = %g > eps=%g", (double)(a), (double)(b), _d, (double)(e)); \
    } while (0)
#define SOFT_EXPECT_IN_RANGE(x, lo, hi)                                  \
    do                                                                   \
    {                                                                    \
        double _x = (double)(x), _lo = (double)(lo), _hi = (double)(hi); \
        if (_x < _lo || _x > _hi)                                        \
            _TT_SOFT_FAIL("%s = %g not in [%g, %g]", #x, _x, _lo, _hi);  \
    } while (0)
#define SOFT_EXPECT_GT(a, b)                                                                       \
    do                                                                                             \
    {                                                                                              \
        if (!((a) > (b)))                                                                          \
            _TT_SOFT_FAIL("%s (%lld) is not > %s (%lld)", #a, (long long)(a), #b, (long long)(b)); \
    } while (0)
#define SOFT_EXPECT_LT(a, b)                                                                       \
    do                                                                                             \
    {                                                                                              \
        if (!((a) < (b)))                                                                          \
            _TT_SOFT_FAIL("%s (%lld) is not < %s (%lld)", #a, (long long)(a), #b, (long long)(b)); \
    } while (0)

/* =============================================================================
 * AUTO-REGISTRO DE TESTS — macro TEST(suite, nombre) { ... }
 *
 * Evita declarar funciones y llamarlas en main: define el test y lo registra
 * automaticamente al arrancar el programa. Luego en main: return tt_run_all();
 *
 *   TEST(Aritmetica, suma) { EXPECT_EQ_INT(2 + 2, 4); }
 *
 * 'suite' y 'nombre' deben ser identificadores validos (sin espacios ni comillas).
 * Para nombres arbitrarios sigue usando tt_suite()/tt_run().
 *
 * Mecanismo: constructor de arranque. Probado en GCC/Clang/MinGW; en MSVC usa
 * la seccion .CRT$XCU. En compiladores sin ninguno de los dos, usa la API
 * imperativa (tt_run).
 * ============================================================================= */

#if defined(_MSC_VER)
#if defined(_WIN64)
#define _TT_REG_SYM(s) #s
#else
#define _TT_REG_SYM(s) "_" #s
#endif
#pragma section(".CRT$XCU", read)
#define _TT_REG_CTOR(fn)                                         \
    static void fn(void);                                        \
    __declspec(allocate(".CRT$XCU")) void (*fn##__p)(void) = fn; \
    __pragma(comment(linker, "/include:" _TT_REG_SYM(fn##__p))) static void fn(void)
#elif defined(__GNUC__) || defined(__clang__)
#define _TT_REG_CTOR(fn) __attribute__((constructor)) static void fn(void)
#else
/* Sin soporte de constructor: el registro no se ejecutara; usa tt_run(). */
#define _TT_REG_CTOR(fn) static void fn(void)
#endif

#define TEST(suite, name)                                    \
    static void _tt_fn_##suite##_##name(void);               \
    _TT_REG_CTOR(_tt_reg_##suite##_##name)                   \
    {                                                        \
        tt_register(#suite, #name, _tt_fn_##suite##_##name); \
    }                                                        \
    static void _tt_fn_##suite##_##name(void)

/* Registra una funcion de test YA definida, con nombres de suite/test
 * ARBITRARIOS (cadenas, no identificadores). La emite la herramienta ctgen:
 * 'fn' es un identificador unico generado y 'suite'/'name' son strings. */
#define TT_REGISTER(suite, name, fn)        \
    _TT_REG_CTOR(_tt_ctor_for_##fn)         \
    {                                       \
        tt_register((suite), (name), (fn)); \
    }

/* Registra hooks once (before_all/after_all) para una suite por su nombre.
 * 'setup_fn' es un identificador unico generado por ctgen. */
#define TT_REGISTER_SUITE_HOOKS(suite, setup_fn, teardown_fn)        \
    _TT_REG_CTOR(_tt_ctor_sh_##setup_fn)                             \
    {                                                                \
        tt_register_suite_hooks((suite), (setup_fn), (teardown_fn)); \
    }

/* =============================================================================
 * WRAPPERS C++ (solo disponibles con compilador C++)
 * ============================================================================= */

#ifdef __cplusplus
#include <exception>
/** @brief C++: verifica que expr lanza ExType. */
#define EXPECT_THROW(expr, ExType)                         \
    do                                                     \
    {                                                      \
        bool _c = false;                                   \
        try                                                \
        {                                                  \
            expr;                                          \
        }                                                  \
        catch (const ExType &)                             \
        {                                                  \
            _c = true;                                     \
        }                                                  \
        catch (...)                                        \
        {                                                  \
        }                                                  \
        if (!_c)                                           \
            _TT_FAIL("Expected " #ExType " from: " #expr); \
    } while (0)

/** @brief C++: verifica que expr NO lanza ninguna excepcion. */
#define EXPECT_NO_THROW(expr)                                \
    do                                                       \
    {                                                        \
        try                                                  \
        {                                                    \
            expr;                                            \
        }                                                    \
        catch (const std::exception &_e)                     \
        {                                                    \
            _TT_FAIL("Unexpected exception: %s", _e.what()); \
        }                                                    \
        catch (...)                                          \
        {                                                    \
            _TT_FAIL("%s", "Unexpected unknown exception");  \
        }                                                    \
    } while (0)

#define ASSERT_THROW EXPECT_THROW
#define ASSERT_NO_THROW EXPECT_NO_THROW
#endif /* __cplusplus */

/* #############################################################################
 * #                          IMPLEMENTACION                                   #
 * #  Se compila SOLO donde se define CTESTS_IMPLEMENTATION (o en ctests.c).   #
 * ########################################################################### */
#ifdef CTESTS_IMPLEMENTATION

#include <ctype.h>  /* tolower */
#include <signal.h> /* signal, raise — manejo de crashes */
#include <stdarg.h> /* va_list */
#include <stdlib.h> /* getenv, atoi, exit */
#include <time.h>   /* clock */

/* --- Compatibilidad de plataforma: TTY y consola Windows --- */
#ifdef _WIN32
#include <io.h>
#ifdef __cplusplus
extern "C"
{
#endif
    __declspec(dllimport) void *__stdcall GetStdHandle(unsigned long);
    __declspec(dllimport) int __stdcall GetConsoleMode(void *, unsigned long *);
    __declspec(dllimport) int __stdcall SetConsoleMode(void *, unsigned long);
    __declspec(dllimport) int __stdcall SetConsoleOutputCP(unsigned int);
#ifdef __cplusplus
}
#endif
#define _TT_ISATTY(fd) _isatty(fd)
#define _TT_FILENO(f) _fileno(f)
#define _TT_DUP(fd) _dup(fd)
#define _TT_DUP2(a, b) _dup2(a, b)
#define _TT_CLOSE(fd) _close(fd)
static void _tt_platform_init(void)
{
    static int done = 0;
    void *h;
    unsigned long mode = 0;
    if (done)
        return;
    done = 1;
    h = GetStdHandle((unsigned long)-11); /* STD_OUTPUT_HANDLE */
    if (h && GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | 0x0004); /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */
    SetConsoleOutputCP(65001);            /* CP_UTF8 */
}
#else
#include <unistd.h>
#define _TT_ISATTY(fd) isatty(fd)
#define _TT_FILENO(f) fileno(f)
#define _TT_DUP(fd) dup(fd)
#define _TT_DUP2(a, b) dup2(a, b)
#define _TT_CLOSE(fd) close(fd)
static void _tt_platform_init(void) {}
#if !defined(CTESTS_NO_ALARM)
#define _TT_HAS_ALARM 1 /* alarm()/SIGALRM disponibles para timeout por test */
#endif
#endif

/* --- Codigos ANSI y simbolos Unicode --- */
#define _TTC_GREEN "\033[32m"
#define _TTC_RED "\033[31m"
#define _TTC_YELLOW "\033[33m"
#define _TTC_CYAN "\033[36m"
#define _TTC_MAGENTA "\033[35m"
#define _TTC_GRAY "\033[90m"
#define _TTC_BOLD "\033[1m"
#define _TTC_RESET "\033[0m"

#define _TTS_TICK "\xE2\x9C\x93"
#define _TTS_CROSS "\xE2\x9C\x97"
#define _TTS_SKIP "\xE2\x86\xB7"
#define _TTS_XFAIL "\xE2\x88\xBC"
#define _TTS_XPASS "!"
#define _TTS_BFULL "\xE2\x96\x88"
#define _TTS_BEMPTY "\xE2\x96\x91"

/* --- Estados de resultado --- */
#define _TT_R_PASS 0
#define _TT_R_FAIL 1
#define _TT_R_SKIP 2
#define _TT_R_XFAIL 3
#define _TT_R_XPASS 4

typedef struct
{
    char name[TT_NAME_MAX];
    int status;
    char error[TT_MSG_MAX];
    int ms;
} _tt_result;

#ifdef __cplusplus
extern "C"
{
#endif

    /* --- Estado compartido (definicion de los simbolos extern del header) --- */
    jmp_buf _tt_jmp;
    int _tt_jmp_code;
    char _tt_errmsg[TT_MSG_MAX];
    char _tt_soft[TT_MAX_SOFT][TT_MSG_MAX];
    int _tt_soft_n = 0;
    const void *tt_param = NULL;

    /* --- Estado interno --- */
    static _tt_result _tt_buf[TT_MAX_TESTS];
    static int _tt_buf_n = 0;
    static char _tt_suite_name[TT_NAME_MAX];
    static clock_t _tt_suite_t0;

    static tt_fn _tt_setup_fn = NULL;
    static tt_fn _tt_teardown_fn = NULL;
    static tt_fn _tt_before_all_fn = NULL;
    static tt_fn _tt_after_all_fn = NULL;
    static int _tt_before_all_ran = 0;

    static int _tt_g_pass = 0, _tt_g_fail = 0, _tt_g_skip = 0;
    static int _tt_g_xfail = 0, _tt_g_xpass = 0;
    static int _tt_f_pass = 0, _tt_f_fail = 0;
    static clock_t _tt_global_t0;
    static int _tt_initialized = 0;
    static int _tt_verbose = 1;
    static int _tt_color_mode = -1;

    static char _tt_filter[TT_FILTER_MAX];
    static int _tt_filter_set = 0;
    static FILE *_tt_junit = NULL;
    static FILE *_tt_tap = NULL;
    static int _tt_tap_n = 0;

    /* Manejo de crashes (senales) */
    static int _tt_catch = 1;     /* capturar SIGSEGV/etc. y reportar el test como fallo */
    static int _tt_in_test = 0;   /* 1 mientras se ejecuta la funcion de un test */
    static int _tt_crash_sig = 0; /* numero de senal capturada */
    static int _tt_timeout_s = 0; /* limite por test en segundos (0=off); solo POSIX */

    static int _tt_slowest_ms = -1, _tt_fastest_ms = -1;
    static char _tt_slowest_label[TT_NAME_MAX * 2 + 4];
    static char _tt_fastest_label[TT_NAME_MAX * 2 + 4];

    static char _tt_fail_suite[TT_MAX_FAILURES][TT_NAME_MAX];
    static char _tt_fail_name[TT_MAX_FAILURES][TT_NAME_MAX];
    static int _tt_fail_n = 0;

    static int _tt_prog_active = 0;

    /* --- Salida: color condicional y deteccion de terminal --- */
    static int _tt_is_tty(void)
    {
        static int cached = -1;
        if (cached < 0)
            cached = _TT_ISATTY(_TT_FILENO(stdout)) ? 1 : 0;
        return cached;
    }

    static int _tt_color_on(void)
    {
        if (_tt_color_mode == 0)
            return 0;
        if (_tt_color_mode == 1)
            return 1;
        if (getenv("NO_COLOR") != NULL)
            return 0;
        return _tt_is_tty();
    }

    static void _tt_out(const char *fmt, ...)
    {
        char buf[4096];
        va_list ap;
        int n;
        const char *p;
        va_start(ap, fmt);
        n = vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
        if (n < 0)
            return;
        if (_tt_color_on())
        {
            fputs(buf, stdout);
            return;
        }
        for (p = buf; *p;)
        {
            if (p[0] == '\033' && p[1] == '[')
            {
                const char *q = p + 2;
                while (*q && !(*q >= '@' && *q <= '~'))
                    q++;
                if (*q == 'm')
                { /* SGR (color) -> descartar */
                    p = *q ? q + 1 : q;
                }
                else
                { /* otro CSI -> conservar */
                    const char *e = *q ? q + 1 : q;
                    while (p < e)
                        putchar(*p++);
                }
            }
            else
            {
                putchar(*p++);
            }
        }
    }

    static void _tt_lazy_init(void)
    {
        if (_tt_initialized)
            return;
        _tt_platform_init();
        _tt_global_t0 = clock();
        _tt_initialized = 1;
    }

    /* --- Backends de EXPECT_EQ / EXPECT_NE (los selecciona _Generic / plantilla) --- */
    static void _tt_report(const char *f, int ln, int soft, const char *fmt, ...)
    {
        char body[TT_MSG_MAX];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(body, sizeof(body), fmt, ap);
        va_end(ap);
        if (soft)
        {
            if (_tt_soft_n < TT_MAX_SOFT)
                snprintf(_tt_soft[_tt_soft_n++], TT_MSG_MAX, "%s:%d: %s", f, ln, body);
        }
        else
        {
            snprintf(_tt_errmsg, TT_MSG_MAX, "%s:%d: %s", f, ln, body);
            _tt_jmp_code = 1;
            _TT_LONGJMP(_tt_jmp, 1);
        }
    }

    void _tt_eq_ll(const char *f, int ln, const char *ea, const char *eb, long long a, long long b, int soft)
    {
        (void)ea;
        (void)eb;
        if (a != b)
            _tt_report(f, ln, soft, "\n  - Expected: %lld\n  + Received: %lld", b, a);
    }
    void _tt_eq_ull(const char *f, int ln, const char *ea, const char *eb, unsigned long long a, unsigned long long b, int soft)
    {
        (void)ea;
        (void)eb;
        if (a != b)
            _tt_report(f, ln, soft, "\n  - Expected: %llu\n  + Received: %llu", b, a);
    }
    void _tt_eq_str(const char *f, int ln, const char *ea, const char *eb, const char *a, const char *b, int soft)
    {
        (void)ea;
        (void)eb;
        if (!a || !b || strcmp(a, b) != 0)
            _tt_report(f, ln, soft, "\n  - Expected: \"%s\"\n  + Received: \"%s\"", b ? b : "(null)", a ? a : "(null)");
    }
    void _tt_eq_dbl(const char *f, int ln, const char *ea, const char *eb, double a, double b, int soft)
    {
        double m = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
        double tol = 1e-9 * m;
        (void)ea;
        (void)eb;
        if (fabs(a - b) > tol)
            _tt_report(f, ln, soft, "\n  - Expected: %g\n  + Received: %g  (|d|=%g > tol=%g)", b, a, fabs(a - b), tol);
    }
    void _tt_eq_ptr(const char *f, int ln, const char *ea, const char *eb, const void *a, const void *b, int soft)
    {
        (void)ea;
        (void)eb;
        if (a != b)
            _tt_report(f, ln, soft, "\n  - Expected ptr: %p\n  + Received ptr: %p", (void *)b, (void *)a);
    }
    void _tt_ne_ll(const char *f, int ln, const char *ea, const char *eb, long long a, long long b, int soft)
    {
        if (a == b)
            _tt_report(f, ln, soft, "%s == %s == %lld (expected different)", ea, eb, a);
    }
    void _tt_ne_ull(const char *f, int ln, const char *ea, const char *eb, unsigned long long a, unsigned long long b, int soft)
    {
        if (a == b)
            _tt_report(f, ln, soft, "%s == %s == %llu (expected different)", ea, eb, a);
    }
    void _tt_ne_str(const char *f, int ln, const char *ea, const char *eb, const char *a, const char *b, int soft)
    {
        if (a && b && strcmp(a, b) == 0)
            _tt_report(f, ln, soft, "%s == %s == \"%s\" (expected different)", ea, eb, a);
    }
    void _tt_ne_dbl(const char *f, int ln, const char *ea, const char *eb, double a, double b, int soft)
    {
        double m = fabs(a) > fabs(b) ? fabs(a) : fabs(b);
        if (fabs(a - b) <= 1e-9 * m)
            _tt_report(f, ln, soft, "%s and %s are ~equal (%g) (expected different)", ea, eb, a);
    }
    void _tt_ne_ptr(const char *f, int ln, const char *ea, const char *eb, const void *a, const void *b, int soft)
    {
        if (a == b)
            _tt_report(f, ln, soft, "%s == %s == %p (expected different)", ea, eb, (void *)a);
    }

    /* --- Manejo de crashes (senales) --- */
    static const char *_tt_signame(int s)
    {
        switch (s)
        {
        case SIGSEGV:
            return "SIGSEGV";
        case SIGFPE:
            return "SIGFPE";
        case SIGILL:
            return "SIGILL";
        case SIGABRT:
            return "SIGABRT";
#ifdef SIGBUS
        case SIGBUS:
            return "SIGBUS";
#endif
        default:
            return "signal";
        }
    }

    static void _tt_sig_handler(int sig)
    {
        if (!_tt_in_test)
        { /* crash fuera de un test: comportamiento por defecto */
            signal(sig, SIG_DFL);
            raise(sig);
            return;
        }
        _tt_crash_sig = sig;
        _tt_jmp_code = 3;
        _TT_LONGJMP(_tt_jmp, 1);
    }

    /* Re-instala (o restaura) los handlers segun _tt_catch. Se llama por test
     * para re-armar la disposicion (signal() puede resetearla tras una entrega). */
    static void _tt_install_signals(void)
    {
        void (*h)(int) = _tt_catch ? _tt_sig_handler : SIG_DFL;
        signal(SIGSEGV, h);
        signal(SIGFPE, h);
        signal(SIGILL, h);
        signal(SIGABRT, h);
#ifdef SIGBUS
        signal(SIGBUS, h);
#endif
    }

#ifdef _TT_HAS_ALARM
    static void _tt_alarm_handler(int sig)
    {
        (void)sig;
        if (!_tt_in_test)
            return;
        _tt_jmp_code = 4; /* timeout */
        _TT_LONGJMP(_tt_jmp, 1);
    }
    static void _tt_arm_timeout(void)
    {
        if (_tt_timeout_s > 0)
        {
            signal(SIGALRM, _tt_alarm_handler);
            alarm((unsigned)_tt_timeout_s);
        }
    }
    static void _tt_disarm_timeout(void)
    {
        if (_tt_timeout_s > 0)
            alarm(0);
    }
#else
static void _tt_arm_timeout(void) {}
static void _tt_disarm_timeout(void) {}
#endif

    /* --- Display --- */
    static void _tt_draw_bar(int pass, int total, int width)
    {
        int filled = (total > 0) ? (pass * width / total) : width;
        int i;
        _tt_out("[");
        for (i = 0; i < width; i++)
            _tt_out("%s", (i < filled) ? _TTC_GREEN _TTS_BFULL _TTC_RESET
                                       : _TTC_GRAY _TTS_BEMPTY _TTC_RESET);
        _tt_out("]");
    }

    static void _tt_progress_print(const char *current_test)
    {
        int i, sp = 0, sf = 0, sk = 0;
        if (_tt_verbose == 0)
            return;
        if (!_tt_is_tty())
            return;
        for (i = 0; i < _tt_buf_n; i++)
        {
            if (_tt_buf[i].status == _TT_R_PASS || _tt_buf[i].status == _TT_R_XFAIL)
                sp++;
            else if (_tt_buf[i].status == _TT_R_FAIL || _tt_buf[i].status == _TT_R_XPASS)
                sf++;
            else if (_tt_buf[i].status == _TT_R_SKIP)
                sk++;
        }
        _tt_out("\r\033[K"
                "  " _TTC_BOLD _TTC_CYAN "%-18.18s" _TTC_RESET
                "  " _TTC_GREEN _TTS_TICK " %d" _TTC_RESET
                "  " _TTC_RED _TTS_CROSS "%d" _TTC_RESET
                "  " _TTC_YELLOW _TTS_SKIP "%d" _TTC_RESET
                "  " _TTC_GRAY "→ %.35s" _TTC_RESET,
                _tt_suite_name, sp, sf, sk, current_test);
        fflush(stdout);
        _tt_prog_active = 1;
    }

    static void _tt_progress_clear(void)
    {
        if (_tt_prog_active)
        {
            _tt_out("\r\033[K");
            fflush(stdout);
            _tt_prog_active = 0;
        }
    }

    /* --- JUnit XML --- */
    static const char *_tt_xml_esc(const char *s, char *out, size_t cap)
    {
        size_t o = 0;
        if (!s)
            s = "";
        for (; *s && o + 7 < cap; s++)
        {
            switch (*s)
            {
            case '&':
                memcpy(out + o, "&amp;", 5);
                o += 5;
                break;
            case '<':
                memcpy(out + o, "&lt;", 4);
                o += 4;
                break;
            case '>':
                memcpy(out + o, "&gt;", 4);
                o += 4;
                break;
            case '"':
                memcpy(out + o, "&quot;", 6);
                o += 6;
                break;
            case '\n':
                out[o++] = ' ';
                break;
            case '\r':
                break;
            default:
                out[o++] = *s;
                break;
            }
        }
        out[o] = '\0';
        return out;
    }

    static void _tt_junit_write_suite(int sf, int sk, int xf, int xp, int total_ms)
    {
        char e1[TT_MSG_MAX * 2], e2[TT_MSG_MAX * 2];
        int i;
        if (!_tt_junit)
            return;
        fprintf(_tt_junit,
                "  <testsuite name=\"%s\" tests=\"%d\" failures=\"%d\" skipped=\"%d\" time=\"%.3f\">\n",
                _tt_xml_esc(_tt_suite_name, e1, sizeof(e1)),
                _tt_buf_n, sf + xp, sk + xf, (double)total_ms / 1000.0);
        for (i = 0; i < _tt_buf_n; i++)
        {
            _tt_result *r = &_tt_buf[i];
            fprintf(_tt_junit, "    <testcase name=\"%s\" time=\"%.3f\"",
                    _tt_xml_esc(r->name, e1, sizeof(e1)), (double)r->ms / 1000.0);
            switch (r->status)
            {
            case _TT_R_FAIL:
            case _TT_R_XPASS:
                fprintf(_tt_junit, ">\n      <failure message=\"%s\"></failure>\n    </testcase>\n",
                        _tt_xml_esc(r->status == _TT_R_XPASS ? "unexpected pass" : r->error,
                                    e2, sizeof(e2)));
                break;
            case _TT_R_SKIP:
            case _TT_R_XFAIL:
                fprintf(_tt_junit, ">\n      <skipped message=\"%s\"/>\n    </testcase>\n",
                        _tt_xml_esc(r->error, e2, sizeof(e2)));
                break;
            default:
                fprintf(_tt_junit, "/>\n");
                break;
            }
        }
        fprintf(_tt_junit, "  </testsuite>\n");
    }

    /* --- TAP (Test Anything Protocol) --- */
    static void _tt_oneline(const char *src, char *dst, size_t cap)
    {
        size_t o = 0;
        if (!src)
            src = "";
        for (; *src && o + 1 < cap; src++)
            dst[o++] = (*src == '\n' || *src == '\r') ? ' ' : *src;
        dst[o] = '\0';
    }

    static void _tt_tap_write_suite(void)
    {
        char buf[TT_MSG_MAX];
        int i;
        if (!_tt_tap)
            return;
        for (i = 0; i < _tt_buf_n; i++)
        {
            _tt_result *r = &_tt_buf[i];
            _tt_tap_n++;
            switch (r->status)
            {
            case _TT_R_SKIP:
                _tt_oneline(r->error, buf, sizeof(buf));
                fprintf(_tt_tap, "ok %d - %s > %s # SKIP %s\n", _tt_tap_n, _tt_suite_name, r->name, buf);
                break;
            case _TT_R_XFAIL:
                _tt_oneline(r->error, buf, sizeof(buf));
                fprintf(_tt_tap, "ok %d - %s > %s # TODO %s\n", _tt_tap_n, _tt_suite_name, r->name, buf);
                break;
            case _TT_R_XPASS:
                fprintf(_tt_tap, "not ok %d - %s > %s # unexpected pass\n", _tt_tap_n, _tt_suite_name, r->name);
                break;
            case _TT_R_FAIL:
                _tt_oneline(r->error, buf, sizeof(buf));
                fprintf(_tt_tap, "not ok %d - %s > %s\n", _tt_tap_n, _tt_suite_name, r->name);
                fprintf(_tt_tap, "  ---\n  message: %s\n  ...\n", buf[0] ? buf : "failed");
                break;
            default:
                fprintf(_tt_tap, "ok %d - %s > %s\n", _tt_tap_n, _tt_suite_name, r->name);
                break;
            }
        }
    }

    /* --- Flush de suite --- */
    static void _tt_flush_suite(void)
    {
        int i, sp = 0, sf = 0, sk = 0, xf = 0, xp = 0, total_ms = 0;
        int any_hard_fail;

        if (_tt_buf_n == 0)
            return;
        _tt_progress_clear();

        if (_tt_after_all_fn)
            _tt_after_all_fn();

        for (i = 0; i < _tt_buf_n; i++)
        {
            total_ms += _tt_buf[i].ms;
            switch (_tt_buf[i].status)
            {
            case _TT_R_PASS:
                sp++;
                break;
            case _TT_R_FAIL:
                sf++;
                break;
            case _TT_R_SKIP:
                sk++;
                break;
            case _TT_R_XFAIL:
                xf++;
                break;
            case _TT_R_XPASS:
                xp++;
                break;
            }
        }
        any_hard_fail = (sf > 0 || xp > 0);

        _tt_out(_TTC_BOLD "%s%s" _TTC_RESET " %s" _TTC_GRAY " (%d tests",
                any_hard_fail ? _TTC_RED : _TTC_GREEN,
                any_hard_fail ? _TTS_CROSS : _TTS_TICK,
                _tt_suite_name, _tt_buf_n);
        if (sf)
            _tt_out(" | " _TTC_RED "%d failed" _TTC_GRAY, sf);
        if (sk)
            _tt_out(" | " _TTC_YELLOW "%d skipped" _TTC_GRAY, sk);
        if (xf)
            _tt_out(" | " _TTC_MAGENTA "%d xfail" _TTC_GRAY, xf);
        if (xp)
            _tt_out(" | " _TTC_MAGENTA "%d xpass" _TTC_GRAY, xp);
        _tt_out(") %dms" _TTC_RESET "\n", total_ms);

        for (i = 0; i < _tt_buf_n; i++)
        {
            _tt_result *r = &_tt_buf[i];
            if (_tt_verbose == 0 && r->status == _TT_R_PASS)
                continue;
            switch (r->status)
            {
            case _TT_R_PASS:
                _tt_out("    " _TTC_GREEN _TTS_TICK _TTC_RESET " %s " _TTC_GRAY "%dms" _TTC_RESET "\n", r->name, r->ms);
                break;
            case _TT_R_FAIL:
            {
                char tmp[TT_MSG_MAX], *p, *nl;
                _tt_out("    " _TTC_RED _TTS_CROSS " %s %dms" _TTC_RESET "\n",
                        r->name, r->ms);
                strncpy(tmp, r->error, TT_MSG_MAX - 1);
                tmp[TT_MSG_MAX - 1] = '\0';
                p = tmp;
                while (*p)
                {
                    nl = strchr(p, '\n');
                    if (nl)
                        *nl = '\0';
                    if (*p)
                        _tt_out("      " _TTC_GRAY "%s" _TTC_RESET "\n", p);
                    if (!nl)
                        break;
                    p = nl + 1;
                }
                break;
            }
            case _TT_R_SKIP:
                _tt_out("    " _TTC_YELLOW _TTS_SKIP _TTC_RESET " %s" _TTC_GRAY " (skipped%s%s)" _TTC_RESET "\n",
                        r->name, r->error[0] ? ": " : "", r->error[0] ? r->error : "");
                break;
            case _TT_R_XFAIL:
                _tt_out("    " _TTC_MAGENTA _TTS_XFAIL _TTC_RESET " %s" _TTC_GRAY " (expected failure: %s)" _TTC_RESET "\n",
                        r->name, r->error);
                break;
            case _TT_R_XPASS:
                _tt_out("    " _TTC_YELLOW _TTS_XPASS _TTC_RESET " %s" _TTC_GRAY " (unexpected pass!)" _TTC_RESET "\n", r->name);
                break;
            }
        }
        _tt_out("\n");

        _tt_junit_write_suite(sf, sk, xf, xp, total_ms);
        _tt_tap_write_suite();

        _tt_g_pass += sp;
        _tt_g_fail += sf;
        _tt_g_skip += sk;
        _tt_g_xfail += xf;
        _tt_g_xpass += xp;
        any_hard_fail ? _tt_f_fail++ : _tt_f_pass++;
        _tt_buf_n = 0;
    }

    /* --- Configuracion --- */
    void tt_verbose(int v) { _tt_verbose = v; }
    void tt_color(int mode) { _tt_color_mode = mode; }

    void tt_output_junit(const char *path)
    {
        _tt_junit = fopen(path, "w");
        if (_tt_junit)
            fprintf(_tt_junit, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<testsuites>\n");
    }

    void tt_output_tap(const char *path)
    {
        _tt_tap = fopen(path, "w");
        if (_tt_tap)
            fprintf(_tt_tap, "TAP version 13\n");
    }

    void tt_catch_crashes(int on) { _tt_catch = on; }
    void tt_timeout(int seconds) { _tt_timeout_s = seconds; }

    /* --- Captura de stdout/stderr --- */
    static int _tt_cap_so = -1, _tt_cap_se = -1;
    static FILE *_tt_cap_fp = NULL;
    static char _tt_cap_path[1024];

    /* tmpfile() falla a menudo en Windows (intenta crear en la raiz del disco),
     * asi que en Windows usamos un archivo propio en el directorio temporal. */
    static FILE *_tt_open_tmp(void)
    {
#ifdef _WIN32
        static unsigned ctr = 0;
        const char *d = getenv("TEMP");
        if (!d)
            d = getenv("TMP");
        if (!d)
            d = ".";
        snprintf(_tt_cap_path, sizeof(_tt_cap_path), "%s\\ctests_cap_%u.tmp", d, ++ctr);
        return fopen(_tt_cap_path, "wb+");
#else
    _tt_cap_path[0] = '\0';
    return tmpfile();
#endif
    }

    void tt_capture_begin(void)
    {
        fflush(stdout);
        fflush(stderr);
        _tt_cap_fp = _tt_open_tmp();
        if (!_tt_cap_fp)
            return; /* sin captura disponible: no rompemos nada */
        _tt_cap_so = _TT_DUP(_TT_FILENO(stdout));
        _tt_cap_se = _TT_DUP(_TT_FILENO(stderr));
        _TT_DUP2(_TT_FILENO(_tt_cap_fp), _TT_FILENO(stdout));
        _TT_DUP2(_TT_FILENO(_tt_cap_fp), _TT_FILENO(stderr));
    }

    size_t tt_capture_end(char *buf, size_t cap)
    {
        size_t n = 0;
        fflush(stdout);
        fflush(stderr);
        if (_tt_cap_so >= 0)
        {
            _TT_DUP2(_tt_cap_so, _TT_FILENO(stdout));
            _TT_CLOSE(_tt_cap_so);
            _tt_cap_so = -1;
        }
        if (_tt_cap_se >= 0)
        {
            _TT_DUP2(_tt_cap_se, _TT_FILENO(stderr));
            _TT_CLOSE(_tt_cap_se);
            _tt_cap_se = -1;
        }
        if (_tt_cap_fp)
        {
            rewind(_tt_cap_fp);
            if (buf && cap > 0)
                n = fread(buf, 1, cap - 1, _tt_cap_fp);
            fclose(_tt_cap_fp);
            _tt_cap_fp = NULL;
            if (_tt_cap_path[0])
                remove(_tt_cap_path);
        }
        if (buf && cap > 0)
            buf[n] = '\0';
        return n;
    }

    /* --- Linea de comandos --- */
    static void _tt_print_help(const char *prog)
    {
        _tt_out("Uso: %s [opciones]\n"
                "  -f, --filter <texto>  Ejecuta solo los tests cuyo \"suite > nombre\"\n"
                "                        contenga <texto> (sin distinguir mayusculas).\n"
                "  -v, --verbose <0|1|2> Nivel de detalle de la salida.\n"
                "      --junit <archivo> Genera un informe JUnit XML.\n"
                "      --tap <archivo>   Genera un informe TAP.\n"
                "      --color           Fuerza salida con color.\n"
                "      --no-color        Desactiva el color.\n"
                "      --no-catch        No captura crashes (SIGSEGV/etc.).\n"
                "      --timeout <seg>   Limite por test en segundos (solo POSIX).\n"
                "  -h, --help            Muestra esta ayuda y termina.\n",
                prog ? prog : "tests");
    }

    void tt_parse_args(int argc, char **argv)
    {
        int i;
        for (i = 1; i < argc; i++)
        {
            const char *a = argv[i];
            if ((!strcmp(a, "--filter") || !strcmp(a, "-f")) && i + 1 < argc)
            {
                strncpy(_tt_filter, argv[++i], TT_FILTER_MAX - 1);
                _tt_filter[TT_FILTER_MAX - 1] = '\0';
                _tt_filter_set = 1;
            }
            else if ((!strcmp(a, "--verbose") || !strcmp(a, "-v")) && i + 1 < argc)
            {
                _tt_verbose = atoi(argv[++i]);
            }
            else if (!strcmp(a, "--junit") && i + 1 < argc)
            {
                tt_output_junit(argv[++i]);
            }
            else if (!strcmp(a, "--tap") && i + 1 < argc)
            {
                tt_output_tap(argv[++i]);
            }
            else if (!strcmp(a, "--no-catch"))
            {
                _tt_catch = 0;
            }
            else if (!strcmp(a, "--catch"))
            {
                _tt_catch = 1;
            }
            else if (!strcmp(a, "--timeout") && i + 1 < argc)
            {
                _tt_timeout_s = atoi(argv[++i]);
            }
            else if (!strcmp(a, "--color"))
            {
                _tt_color_mode = 1;
            }
            else if (!strcmp(a, "--no-color"))
            {
                _tt_color_mode = 0;
            }
            else if (!strcmp(a, "--help") || !strcmp(a, "-h"))
            {
                _tt_print_help(argv[0]);
                exit(0);
            }
        }
    }

    /* --- Suites --- */
    void tt_suite(const char *name)
    {
        _tt_lazy_init();
        _tt_flush_suite();
        strncpy(_tt_suite_name, name, TT_NAME_MAX - 1);
        _tt_suite_name[TT_NAME_MAX - 1] = '\0';
        _tt_suite_t0 = clock();
        _tt_setup_fn = NULL;
        _tt_teardown_fn = NULL;
        _tt_before_all_fn = NULL;
        _tt_after_all_fn = NULL;
        _tt_before_all_ran = 0;
    }

    void tt_suite_hooks(tt_fn setup_fn, tt_fn teardown_fn)
    {
        _tt_setup_fn = setup_fn;
        _tt_teardown_fn = teardown_fn;
    }

    void tt_suite_hooks_once(tt_fn before_all, tt_fn after_all)
    {
        _tt_before_all_fn = before_all;
        _tt_after_all_fn = after_all;
        _tt_before_all_ran = 0;
    }

    /* --- Filtrado --- */
    static int _tt_ci_contains(const char *hay, const char *needle)
    {
        size_t nl = strlen(needle), i;
        if (nl == 0)
            return 1;
        for (; *hay; hay++)
        {
            for (i = 0; i < nl && hay[i] &&
                        tolower((unsigned char)hay[i]) == tolower((unsigned char)needle[i]);
                 i++)
            {
            }
            if (i == nl)
                return 1;
        }
        return 0;
    }

    static int _tt_filtered_out(const char *name)
    {
        char label[TT_NAME_MAX * 2 + 4];
        if (!_tt_filter_set)
            return 0;
        snprintf(label, sizeof(label), "%s > %s", _tt_suite_name, name);
        return !_tt_ci_contains(label, _tt_filter);
    }

    static void _tt_maybe_before_all(void)
    {
        if (_tt_before_all_fn && !_tt_before_all_ran)
        {
            _tt_before_all_fn();
            _tt_before_all_ran = 1;
        }
    }

    /* --- Tests --- */
    void tt_run(const char *name, tt_fn fn)
    {
        clock_t t0;
        _tt_result *r;
        int test_ok;

        _tt_lazy_init();
        if (_tt_filtered_out(name))
            return;
        if (_tt_buf_n >= TT_MAX_TESTS)
            return;

        _tt_maybe_before_all();
        _tt_progress_print(name);

        r = &_tt_buf[_tt_buf_n++];
        strncpy(r->name, name, TT_NAME_MAX - 1);
        r->name[TT_NAME_MAX - 1] = '\0';
        r->error[0] = '\0';
        _tt_soft_n = 0;
        _tt_jmp_code = 0;

        if (_tt_setup_fn)
            _tt_setup_fn();
        t0 = clock();

        _tt_install_signals();
        _tt_in_test = 1;
        _tt_arm_timeout();
        if (_TT_SETJMP(_tt_jmp) == 0)
        {
            fn();
            test_ok = (_tt_soft_n == 0);
            if (!test_ok)
            {
                int i;
                _tt_errmsg[0] = '\0';
                for (i = 0; i < _tt_soft_n; i++)
                {
                    strncat(_tt_errmsg, _tt_soft[i], TT_MSG_MAX - strlen(_tt_errmsg) - 3);
                    strncat(_tt_errmsg, "\n", TT_MSG_MAX - strlen(_tt_errmsg) - 1);
                }
                _tt_jmp_code = 1;
            }
        }
        _tt_disarm_timeout();
        _tt_in_test = 0;

        r->ms = (int)((double)(clock() - t0) * 1000.0 / CLOCKS_PER_SEC);
        /* Tras un crash o timeout no ejecutamos teardown: el estado es sospechoso. */
        if (_tt_teardown_fn && _tt_jmp_code != 3 && _tt_jmp_code != 4)
            _tt_teardown_fn();

        if (_tt_jmp_code == 2)
        {
            r->status = _TT_R_SKIP;
            strncpy(r->error, _tt_errmsg, TT_MSG_MAX - 1);
        }
        else if (_tt_jmp_code == 1 || _tt_jmp_code == 3 || _tt_jmp_code == 4)
        {
            r->status = _TT_R_FAIL;
            if (_tt_jmp_code == 3)
                snprintf(r->error, TT_MSG_MAX, "crashed: %s", _tt_signame(_tt_crash_sig));
            else if (_tt_jmp_code == 4)
                snprintf(r->error, TT_MSG_MAX, "timeout after %d s", _tt_timeout_s);
            else
                strncpy(r->error, _tt_errmsg, TT_MSG_MAX - 1);
            if (_tt_fail_n < TT_MAX_FAILURES)
            {
                strncpy(_tt_fail_suite[_tt_fail_n], _tt_suite_name, TT_NAME_MAX - 1);
                strncpy(_tt_fail_name[_tt_fail_n], name, TT_NAME_MAX - 1);
                _tt_fail_n++;
            }
        }
        else
        {
            r->status = _TT_R_PASS;
        }

        {
            char label[TT_NAME_MAX * 2 + 4];
            snprintf(label, sizeof(label) - 1, "%s > %s", _tt_suite_name, name);
            if (_tt_slowest_ms < 0 || r->ms > _tt_slowest_ms)
            {
                _tt_slowest_ms = r->ms;
                strncpy(_tt_slowest_label, label, sizeof(_tt_slowest_label) - 1);
            }
            if (r->status != _TT_R_SKIP && (_tt_fastest_ms < 0 || r->ms < _tt_fastest_ms))
            {
                _tt_fastest_ms = r->ms;
                strncpy(_tt_fastest_label, label, sizeof(_tt_fastest_label) - 1);
            }
        }
    }

    void tt_run_param(const char *name, tt_fn fn, const void *param)
    {
        tt_param = param;
        tt_run(name, fn);
        tt_param = NULL;
    }

    void tt_skip_test(const char *name, const char *reason)
    {
        _tt_result *r;
        _tt_lazy_init();
        if (_tt_filtered_out(name))
            return;
        if (_tt_buf_n >= TT_MAX_TESTS)
            return;
        r = &_tt_buf[_tt_buf_n++];
        strncpy(r->name, name, TT_NAME_MAX - 1);
        r->name[TT_NAME_MAX - 1] = '\0';
        r->status = _TT_R_SKIP;
        r->ms = 0;
        strncpy(r->error, reason ? reason : "", TT_MSG_MAX - 1);
    }

    void tt_xfail(const char *name, const char *reason, tt_fn fn)
    {
        clock_t t0;
        _tt_result *r;
        int test_failed;

        _tt_lazy_init();
        if (_tt_filtered_out(name))
            return;
        if (_tt_buf_n >= TT_MAX_TESTS)
            return;

        _tt_maybe_before_all();
        _tt_progress_print(name);

        r = &_tt_buf[_tt_buf_n++];
        strncpy(r->name, name, TT_NAME_MAX - 1);
        r->name[TT_NAME_MAX - 1] = '\0';
        strncpy(r->error, reason ? reason : "", TT_MSG_MAX - 1);
        _tt_soft_n = 0;
        _tt_jmp_code = 0;

        if (_tt_setup_fn)
            _tt_setup_fn();
        t0 = clock();
        _tt_install_signals();
        _tt_in_test = 1;
        _tt_arm_timeout();
        if (_TT_SETJMP(_tt_jmp) == 0)
        {
            fn();
            test_failed = (_tt_soft_n > 0);
        }
        else
        {
            test_failed = 1; /* fallo, crash o timeout: cuentan como el fallo esperado */
        }
        _tt_disarm_timeout();
        _tt_in_test = 0;
        r->ms = (int)((double)(clock() - t0) * 1000.0 / CLOCKS_PER_SEC);
        if (_tt_teardown_fn && _tt_jmp_code != 3 && _tt_jmp_code != 4)
            _tt_teardown_fn();
        r->status = test_failed ? _TT_R_XFAIL : _TT_R_XPASS;

        if (r->status == _TT_R_XPASS && _tt_fail_n < TT_MAX_FAILURES)
        {
            strncpy(_tt_fail_suite[_tt_fail_n], _tt_suite_name, TT_NAME_MAX - 1);
            strncpy(_tt_fail_name[_tt_fail_n], name, TT_NAME_MAX - 1);
            _tt_fail_n++;
        }
    }

    /* --- Auto-registro (macro TEST) --- */
    typedef struct
    {
        const char *suite;
        const char *name;
        tt_fn fn;
    } _tt_reg_entry;

    static _tt_reg_entry _tt_regs[TT_MAX_REGISTERED];
    static int _tt_regs_n = 0;

    void tt_register(const char *suite, const char *name, tt_fn fn)
    {
        if (_tt_regs_n >= TT_MAX_REGISTERED)
            return;
        _tt_regs[_tt_regs_n].suite = suite;
        _tt_regs[_tt_regs_n].name = name;
        _tt_regs[_tt_regs_n].fn = fn;
        _tt_regs_n++;
    }

    /* Hooks once por suite (registrados por nombre). */
    typedef struct
    {
        const char *suite;
        tt_fn before, after;
    } _tt_shook_entry;
    static _tt_shook_entry _tt_shooks[TT_MAX_SUITE_HOOKS];
    static int _tt_shooks_n = 0;

    void tt_register_suite_hooks(const char *suite, tt_fn before_all, tt_fn after_all)
    {
        int i;
        for (i = 0; i < _tt_shooks_n; i++)
            if (strcmp(_tt_shooks[i].suite, suite) == 0)
            {
                _tt_shooks[i].before = before_all;
                _tt_shooks[i].after = after_all;
                return;
            }
        if (_tt_shooks_n >= TT_MAX_SUITE_HOOKS)
            return;
        _tt_shooks[_tt_shooks_n].suite = suite;
        _tt_shooks[_tt_shooks_n].before = before_all;
        _tt_shooks[_tt_shooks_n].after = after_all;
        _tt_shooks_n++;
    }

    static void _tt_apply_suite_hooks(const char *suite)
    {
        int i;
        for (i = 0; i < _tt_shooks_n; i++)
            if (strcmp(_tt_shooks[i].suite, suite) == 0)
            {
                tt_suite_hooks_once(_tt_shooks[i].before, _tt_shooks[i].after);
                return;
            }
    }

    int tt_run_all(void)
    {
        const char *cur = NULL;
        int i;
        for (i = 0; i < _tt_regs_n; i++)
        {
            if (!cur || strcmp(cur, _tt_regs[i].suite) != 0)
            {
                tt_suite(_tt_regs[i].suite);
                _tt_apply_suite_hooks(_tt_regs[i].suite);
                cur = _tt_regs[i].suite;
            }
            tt_run(_tt_regs[i].name, _tt_regs[i].fn);
        }
        return tt_summary();
    }

    /* --- Resumen --- */
    int tt_summary(void)
    {
        double total_ms;
        int total_tests, total_files, pass_bar, total_bar;
        int i;

        _tt_lazy_init();
        _tt_flush_suite();

        total_ms = (double)(clock() - _tt_global_t0) * 1000.0 / CLOCKS_PER_SEC;
        total_tests = _tt_g_pass + _tt_g_fail + _tt_g_skip + _tt_g_xfail + _tt_g_xpass;
        total_files = _tt_f_pass + _tt_f_fail;
        pass_bar = _tt_g_pass + _tt_g_xfail;
        total_bar = total_tests - _tt_g_skip;

        if (_tt_junit)
        {
            fprintf(_tt_junit, "</testsuites>\n");
            fclose(_tt_junit);
            _tt_junit = NULL;
        }
        if (_tt_tap)
        {
            fprintf(_tt_tap, "1..%d\n", _tt_tap_n); /* plan al final (permitido por TAP) */
            fclose(_tt_tap);
            _tt_tap = NULL;
        }

        if (_tt_fail_n > 0)
        {
            int border = 40;
            _tt_out(_TTC_RED _TTC_BOLD "─── Failed Tests %d ", _tt_fail_n);
            for (i = 0; i < border; i++)
                _tt_out("─");
            _tt_out(_TTC_RESET "\n");
            for (i = 0; i < _tt_fail_n; i++)
                _tt_out("  " _TTC_RED _TTC_BOLD "FAIL" _TTC_RESET "  %s > %s\n",
                        _tt_fail_suite[i], _tt_fail_name[i]);
            _tt_out("\n");
        }

        _tt_out("  ");
        _tt_draw_bar(pass_bar, total_bar, 32);
        _tt_out("  " _TTC_BOLD "%d/%d" _TTC_RESET
                "  " _TTC_GRAY "(%d%%)" _TTC_RESET "\n\n",
                pass_bar, total_bar,
                (total_bar > 0) ? (pass_bar * 100 / total_bar) : 100);

        _tt_out(_TTC_BOLD " Test Files  " _TTC_RESET);
        if (_tt_f_fail)
            _tt_out(_TTC_RED "%d failed" _TTC_RESET " | ", _tt_f_fail);
        _tt_out(_TTC_GREEN "%d passed" _TTC_RESET _TTC_GRAY " (%d)" _TTC_RESET "\n", _tt_f_pass, total_files);

        _tt_out(_TTC_BOLD "     Tests  " _TTC_RESET);
        if (_tt_g_fail)
            _tt_out(_TTC_RED "%d failed" _TTC_RESET " | ", _tt_g_fail);
        if (_tt_g_xpass)
            _tt_out(_TTC_YELLOW "%d xpass" _TTC_RESET " | ", _tt_g_xpass);
        if (_tt_g_xfail)
            _tt_out(_TTC_MAGENTA "%d xfail" _TTC_RESET " | ", _tt_g_xfail);
        if (_tt_g_skip)
            _tt_out(_TTC_YELLOW "%d skipped" _TTC_RESET " | ", _tt_g_skip);
        _tt_out(_TTC_GREEN "%d passed" _TTC_RESET _TTC_GRAY " (%d)" _TTC_RESET "\n", _tt_g_pass, total_tests);

        _tt_out(_TTC_BOLD "  Duration  " _TTC_RESET _TTC_GRAY "%.1fms" _TTC_RESET "\n", total_ms);

        if (_tt_verbose >= 1 && _tt_slowest_ms >= 0)
            _tt_out(_TTC_BOLD "   Slowest  " _TTC_RESET _TTC_GRAY "%dms — %s" _TTC_RESET "\n",
                    _tt_slowest_ms, _tt_slowest_label);
        if (_tt_verbose >= 1 && _tt_fastest_ms >= 0)
            _tt_out(_TTC_BOLD "   Fastest  " _TTC_RESET _TTC_GRAY "%dms — %s" _TTC_RESET "\n",
                    _tt_fastest_ms, _tt_fastest_label);

        return (_tt_g_fail > 0 || _tt_g_xpass > 0) ? 1 : 0;
    }

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CTESTS_IMPLEMENTATION */

#endif /* ctests_H */
