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

/* Includes necesarios para la SUPERFICIE de la API (tipos y macros de asercion). */
#include <math.h>     /* fabs   — EXPECT_NEAR */
#include <setjmp.h>   /* jmp_buf, longjmp */
#include <stddef.h>   /* size_t — EXPECT_EQ_MEM */
#include <stdio.h>    /* snprintf */
#include <string.h>   /* strcmp, strstr, strncmp, strlen, memcmp */

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

/* =============================================================================
 * API PUBLICA — declaraciones
 * ============================================================================= */

#ifdef __cplusplus
extern "C" {
#endif

/** Firma de funcion de test: sin argumentos, sin retorno. */
typedef void (*tt_fn)(void);

/* --- Estado compartido referenciado por las macros de asercion ---------------
 * Estos simbolos los usan las macros que se expanden en TUS archivos, por eso
 * son visibles (extern). No los manipules directamente salvo tt_param. */
extern jmp_buf      _tt_jmp;
extern int          _tt_jmp_code;
extern char         _tt_errmsg[TT_MSG_MAX];
extern char         _tt_soft[TT_MAX_SOFT][TT_MSG_MAX];
extern int          _tt_soft_n;

/** Dato del caso actual en tests data-driven; lo fija tt_run_param(). */
extern const void  *tt_param;

/* --- Configuracion --- */
void tt_verbose(int v);            /**< 0=silencioso, 1=normal, 2=detallado. */
void tt_color(int mode);           /**< -1=auto, 0=sin color, 1=con color. */
void tt_output_junit(const char *path);  /**< Activa informe JUnit XML. */
void tt_parse_args(int argc, char **argv); /**< Procesa --filter/--verbose/--junit/--color/--help. */

/* --- Suites --- */
void tt_suite(const char *name);
void tt_suite_hooks(tt_fn setup_fn, tt_fn teardown_fn);       /**< Antes/despues de CADA test. */
void tt_suite_hooks_once(tt_fn before_all, tt_fn after_all);  /**< Una vez por suite. */

/* --- Tests --- */
void tt_run(const char *name, tt_fn fn);
void tt_run_param(const char *name, tt_fn fn, const void *param);
void tt_skip_test(const char *name, const char *reason);
void tt_xfail(const char *name, const char *reason, tt_fn fn);

/* --- Resumen --- */
int  tt_summary(void);  /**< 0 si todo paso, 1 si hubo fallos. Propagar desde main. */

#ifdef __cplusplus
} /* extern "C" */
#endif

/* =============================================================================
 * MACROS DE ASERCION — hard: detienen el test al primer fallo
 * ============================================================================= */

/** @brief Macro interna: hard-fail con mensaje formateado. */
#define _TT_FAIL(...) \
    do { snprintf(_tt_errmsg, TT_MSG_MAX, __VA_ARGS__); \
         _tt_jmp_code = 1; longjmp(_tt_jmp, 1); } while(0)

/** @brief Macro interna: soft-fail — acumula el mensaje sin parar. */
#define _TT_SOFT_FAIL(...) \
    do { if (_tt_soft_n < TT_MAX_SOFT) \
             snprintf(_tt_soft[_tt_soft_n++], TT_MSG_MAX, __VA_ARGS__); } while(0)

/**
 * @brief Salta el test actual con un motivo. Llamar desde dentro del test.
 * @param reason Cadena de texto explicando el salto.
 */
#define tt_skip(reason) \
    do { strncpy(_tt_errmsg, (reason), TT_MSG_MAX-1); \
         _tt_jmp_code = 2; longjmp(_tt_jmp, 2); } while(0)

/* Condiciones */
#define EXPECT_TRUE(c)      do{if(!(c))       _TT_FAIL("Expected TRUE: %s", #c);}while(0)
#define EXPECT_FALSE(c)     do{if((c))        _TT_FAIL("Expected FALSE: %s", #c);}while(0)

/* Condicion con mensaje personalizado (printf-style) */
#define EXPECT_MSG(c, ...)  do{if(!(c))       _TT_FAIL(__VA_ARGS__);}while(0)

/* Punteros */
#define EXPECT_NULL(p)      do{if((p)!=NULL)  _TT_FAIL("Expected NULL: %s", #p);}while(0)
#define EXPECT_NOT_NULL(p)  do{if((p)==NULL)  _TT_FAIL("Expected non-NULL: %s", #p);}while(0)
#define EXPECT_EQ_PTR(a,b) \
    do{const void*_a=(const void*)(a),*_b=(const void*)(b); \
       if(_a!=_b) _TT_FAIL("\n  - Expected ptr: %p\n  + Received ptr: %p",_b,_a);}while(0)

/* Enteros con signo */
#define EXPECT_EQ_INT(a,b) \
    do{long long _a=(long long)(a),_b=(long long)(b); \
       if(_a!=_b) _TT_FAIL("\n  - Expected: %lld\n  + Received: %lld",_b,_a);}while(0)
#define EXPECT_NEQ_INT(a,b) \
    do{long long _a=(long long)(a),_b=(long long)(b); \
       if(_a==_b) _TT_FAIL("%s == %s == %lld (expected different)",#a,#b,_a);}while(0)

/* Enteros sin signo */
#define EXPECT_EQ_UINT(a,b) \
    do{unsigned long long _a=(unsigned long long)(a),_b=(unsigned long long)(b); \
       if(_a!=_b) _TT_FAIL("\n  - Expected: %llu\n  + Received: %llu",_b,_a);}while(0)
#define EXPECT_NEQ_UINT(a,b) \
    do{unsigned long long _a=(unsigned long long)(a),_b=(unsigned long long)(b); \
       if(_a==_b) _TT_FAIL("%s == %s == %llu (expected different)",#a,#b,_a);}while(0)

/* Flotantes */
#define EXPECT_NEAR(a,b,eps) \
    do{double _d=fabs((double)(a)-(double)(b)); \
       if(_d>(double)(eps)) \
         _TT_FAIL("|%g - %g| = %g > eps=%g",(double)(a),(double)(b),_d,(double)(eps));}while(0)

/* Comparaciones de orden */
#define EXPECT_GT(a,b) \
    do{if(!((a)>(b)))  _TT_FAIL("%s (%lld) is not > %s (%lld)", #a,(long long)(a),#b,(long long)(b));}while(0)
#define EXPECT_GE(a,b) \
    do{if(!((a)>=(b))) _TT_FAIL("%s (%lld) is not >= %s (%lld)",#a,(long long)(a),#b,(long long)(b));}while(0)
#define EXPECT_LT(a,b) \
    do{if(!((a)<(b)))  _TT_FAIL("%s (%lld) is not < %s (%lld)", #a,(long long)(a),#b,(long long)(b));}while(0)
#define EXPECT_LE(a,b) \
    do{if(!((a)<=(b))) _TT_FAIL("%s (%lld) is not <= %s (%lld)",#a,(long long)(a),#b,(long long)(b));}while(0)

/* Cadenas */
#define EXPECT_EQ_STR(a,b) \
    do{if(strcmp((a),(b))!=0) \
         _TT_FAIL("\n  - Expected: \"%s\"\n  + Received: \"%s\"",(b),(a));}while(0)
#define EXPECT_NEQ_STR(a,b) \
    do{if(strcmp((a),(b))==0) \
         _TT_FAIL("Strings should differ but both = \"%s\"",(a));}while(0)
#define EXPECT_CONTAINS(s,sub) \
    do{if(!strstr((s),(sub))) \
         _TT_FAIL("\"%s\" does not contain \"%s\"",(s),(sub));}while(0)
#define EXPECT_STARTS_WITH(s,pre) \
    do{if(strncmp((s),(pre),strlen((pre)))!=0) \
         _TT_FAIL("\"%s\" does not start with \"%s\"",(s),(pre));}while(0)

/* Bloques de memoria */
#define EXPECT_EQ_MEM(a,b,n) \
    do{ if(memcmp((a),(b),(n))!=0){ \
            size_t _i; const unsigned char*_pa=(const unsigned char*)(a); \
            const unsigned char*_pb=(const unsigned char*)(b); \
            for(_i=0;_i<(size_t)(n);_i++) if(_pa[_i]!=_pb[_i]) break; \
            _TT_FAIL("memory differs at byte %lu: 0x%02X != 0x%02X", \
                     (unsigned long)_i,_pa[_i],_pb[_i]); } }while(0)

/* Fallo incondicional */
#define EXPECT_FAIL(msg) _TT_FAIL("%s", msg)

/* =============================================================================
 * ALIAS ASSERT_* — equivalentes a las EXPECT_* (en ctests, EXPECT ya es "hard")
 * ============================================================================= */

#define ASSERT_TRUE         EXPECT_TRUE
#define ASSERT_FALSE        EXPECT_FALSE
#define ASSERT_MSG          EXPECT_MSG
#define ASSERT_NULL         EXPECT_NULL
#define ASSERT_NOT_NULL     EXPECT_NOT_NULL
#define ASSERT_EQ_PTR       EXPECT_EQ_PTR
#define ASSERT_EQ_INT       EXPECT_EQ_INT
#define ASSERT_NEQ_INT      EXPECT_NEQ_INT
#define ASSERT_EQ_UINT      EXPECT_EQ_UINT
#define ASSERT_NEQ_UINT     EXPECT_NEQ_UINT
#define ASSERT_NEAR         EXPECT_NEAR
#define ASSERT_GT           EXPECT_GT
#define ASSERT_GE           EXPECT_GE
#define ASSERT_LT           EXPECT_LT
#define ASSERT_LE           EXPECT_LE
#define ASSERT_EQ_STR       EXPECT_EQ_STR
#define ASSERT_NEQ_STR      EXPECT_NEQ_STR
#define ASSERT_CONTAINS     EXPECT_CONTAINS
#define ASSERT_STARTS_WITH  EXPECT_STARTS_WITH
#define ASSERT_EQ_MEM       EXPECT_EQ_MEM
#define ASSERT_FAIL         EXPECT_FAIL

/* =============================================================================
 * MACROS SOFT — acumulan fallos, el test falla al terminar la funcion
 * ============================================================================= */

#define SOFT_EXPECT_TRUE(c)     do{if(!(c))       _TT_SOFT_FAIL("Expected TRUE: %s", #c);}while(0)
#define SOFT_EXPECT_FALSE(c)    do{if((c))        _TT_SOFT_FAIL("Expected FALSE: %s", #c);}while(0)
#define SOFT_EXPECT_MSG(c, ...) do{if(!(c))       _TT_SOFT_FAIL(__VA_ARGS__);}while(0)
#define SOFT_EXPECT_NULL(p)     do{if((p)!=NULL)  _TT_SOFT_FAIL("Expected NULL: %s", #p);}while(0)
#define SOFT_EXPECT_NOT_NULL(p) do{if((p)==NULL)  _TT_SOFT_FAIL("Expected non-NULL: %s", #p);}while(0)
#define SOFT_EXPECT_EQ_INT(a,b) \
    do{long long _a=(long long)(a),_b=(long long)(b); \
       if(_a!=_b) _TT_SOFT_FAIL("\n  - Expected: %lld\n  + Received: %lld",_b,_a);}while(0)
#define SOFT_EXPECT_EQ_UINT(a,b) \
    do{unsigned long long _a=(unsigned long long)(a),_b=(unsigned long long)(b); \
       if(_a!=_b) _TT_SOFT_FAIL("\n  - Expected: %llu\n  + Received: %llu",_b,_a);}while(0)
#define SOFT_EXPECT_EQ_STR(a,b) \
    do{if(strcmp((a),(b))!=0) \
         _TT_SOFT_FAIL("\n  - Expected: \"%s\"\n  + Received: \"%s\"",(b),(a));}while(0)
#define SOFT_EXPECT_NEAR(a,b,e) \
    do{double _d=fabs((double)(a)-(double)(b)); \
       if(_d>(double)(e)) \
         _TT_SOFT_FAIL("|%g - %g| = %g > eps=%g",(double)(a),(double)(b),_d,(double)(e));}while(0)
#define SOFT_EXPECT_GT(a,b) \
    do{if(!((a)>(b))) _TT_SOFT_FAIL("%s (%lld) is not > %s (%lld)",#a,(long long)(a),#b,(long long)(b));}while(0)
#define SOFT_EXPECT_LT(a,b) \
    do{if(!((a)<(b))) _TT_SOFT_FAIL("%s (%lld) is not < %s (%lld)",#a,(long long)(a),#b,(long long)(b));}while(0)

/* =============================================================================
 * WRAPPERS C++ (solo disponibles con compilador C++)
 * ============================================================================= */

#ifdef __cplusplus
#include <exception>
/** @brief C++: verifica que expr lanza ExType. */
#define EXPECT_THROW(expr, ExType) \
    do { bool _c=false; \
         try{expr;}catch(const ExType&){_c=true;}catch(...){} \
         if(!_c) _TT_FAIL("Expected " #ExType " from: " #expr); } while(0)

/** @brief C++: verifica que expr NO lanza ninguna excepcion. */
#define EXPECT_NO_THROW(expr) \
    do { try{expr;} \
         catch(const std::exception& _e) \
             {_TT_FAIL("Unexpected exception: %s", _e.what());} \
         catch(...){_TT_FAIL("%s","Unexpected unknown exception");} } while(0)

#define ASSERT_THROW    EXPECT_THROW
#define ASSERT_NO_THROW EXPECT_NO_THROW
#endif /* __cplusplus */

/* #############################################################################
 * #                          IMPLEMENTACION                                   #
 * #  Se compila SOLO donde se define CTESTS_IMPLEMENTATION (o en ctests.c).   #
 * ########################################################################### */
#ifdef CTESTS_IMPLEMENTATION

#include <ctype.h>    /* tolower */
#include <stdarg.h>   /* va_list */
#include <stdlib.h>   /* getenv, atoi, exit */
#include <time.h>     /* clock */

/* --- Compatibilidad de plataforma: TTY y consola Windows --- */
#ifdef _WIN32
#  include <io.h>
#  ifdef __cplusplus
extern "C" {
#  endif
__declspec(dllimport) void *__stdcall GetStdHandle(unsigned long);
__declspec(dllimport) int   __stdcall GetConsoleMode(void *, unsigned long *);
__declspec(dllimport) int   __stdcall SetConsoleMode(void *, unsigned long);
__declspec(dllimport) int   __stdcall SetConsoleOutputCP(unsigned int);
#  ifdef __cplusplus
}
#  endif
#  define _TT_ISATTY(fd) _isatty(fd)
#  define _TT_FILENO(f)  _fileno(f)
static void _tt_platform_init(void) {
    static int done = 0;
    void *h; unsigned long mode = 0;
    if (done) return;
    done = 1;
    h = GetStdHandle((unsigned long)-11);   /* STD_OUTPUT_HANDLE */
    if (h && GetConsoleMode(h, &mode))
        SetConsoleMode(h, mode | 0x0004);    /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */
    SetConsoleOutputCP(65001);               /* CP_UTF8 */
}
#else
#  include <unistd.h>
#  define _TT_ISATTY(fd) isatty(fd)
#  define _TT_FILENO(f)  fileno(f)
static void _tt_platform_init(void) {}
#endif

/* --- Codigos ANSI y simbolos Unicode --- */
#define _TTC_GREEN   "\033[32m"
#define _TTC_RED     "\033[31m"
#define _TTC_YELLOW  "\033[33m"
#define _TTC_CYAN    "\033[36m"
#define _TTC_MAGENTA "\033[35m"
#define _TTC_GRAY    "\033[90m"
#define _TTC_BOLD    "\033[1m"
#define _TTC_RESET   "\033[0m"

#define _TTS_TICK   "\xE2\x9C\x93"
#define _TTS_CROSS  "\xE2\x9C\x97"
#define _TTS_SKIP   "\xE2\x86\xB7"
#define _TTS_XFAIL  "\xE2\x88\xBC"
#define _TTS_XPASS  "!"
#define _TTS_BFULL  "\xE2\x96\x88"
#define _TTS_BEMPTY "\xE2\x96\x91"

/* --- Estados de resultado --- */
#define _TT_R_PASS  0
#define _TT_R_FAIL  1
#define _TT_R_SKIP  2
#define _TT_R_XFAIL 3
#define _TT_R_XPASS 4

typedef struct {
    char name[TT_NAME_MAX];
    int  status;
    char error[TT_MSG_MAX];
    int  ms;
} _tt_result;

#ifdef __cplusplus
extern "C" {
#endif

/* --- Estado compartido (definicion de los simbolos extern del header) --- */
jmp_buf      _tt_jmp;
int          _tt_jmp_code;
char         _tt_errmsg[TT_MSG_MAX];
char         _tt_soft[TT_MAX_SOFT][TT_MSG_MAX];
int          _tt_soft_n = 0;
const void  *tt_param = NULL;

/* --- Estado interno --- */
static _tt_result _tt_buf[TT_MAX_TESTS];
static int        _tt_buf_n = 0;
static char       _tt_suite_name[TT_NAME_MAX];
static clock_t    _tt_suite_t0;

static tt_fn      _tt_setup_fn      = NULL;
static tt_fn      _tt_teardown_fn   = NULL;
static tt_fn      _tt_before_all_fn = NULL;
static tt_fn      _tt_after_all_fn  = NULL;
static int        _tt_before_all_ran = 0;

static int        _tt_g_pass = 0, _tt_g_fail = 0, _tt_g_skip = 0;
static int        _tt_g_xfail = 0, _tt_g_xpass = 0;
static int        _tt_f_pass = 0, _tt_f_fail = 0;
static clock_t    _tt_global_t0;
static int        _tt_initialized = 0;
static int        _tt_verbose     = 1;
static int        _tt_color_mode  = -1;

static char       _tt_filter[TT_FILTER_MAX];
static int        _tt_filter_set = 0;
static FILE      *_tt_junit = NULL;

static int        _tt_slowest_ms = -1, _tt_fastest_ms = -1;
static char       _tt_slowest_label[TT_NAME_MAX * 2 + 4];
static char       _tt_fastest_label[TT_NAME_MAX * 2 + 4];

static char       _tt_fail_suite[TT_MAX_FAILURES][TT_NAME_MAX];
static char       _tt_fail_name [TT_MAX_FAILURES][TT_NAME_MAX];
static int        _tt_fail_n = 0;

static int        _tt_prog_active = 0;

/* --- Salida: color condicional y deteccion de terminal --- */
static int _tt_is_tty(void) {
    static int cached = -1;
    if (cached < 0) cached = _TT_ISATTY(_TT_FILENO(stdout)) ? 1 : 0;
    return cached;
}

static int _tt_color_on(void) {
    if (_tt_color_mode == 0) return 0;
    if (_tt_color_mode == 1) return 1;
    if (getenv("NO_COLOR") != NULL) return 0;
    return _tt_is_tty();
}

static void _tt_out(const char *fmt, ...) {
    char buf[4096];
    va_list ap;
    int n;
    const char *p;
    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if (_tt_color_on()) { fputs(buf, stdout); return; }
    for (p = buf; *p; ) {
        if (p[0] == '\033' && p[1] == '[') {
            const char *q = p + 2;
            while (*q && !(*q >= '@' && *q <= '~')) q++;
            if (*q == 'm') {                 /* SGR (color) -> descartar */
                p = *q ? q + 1 : q;
            } else {                         /* otro CSI -> conservar */
                const char *e = *q ? q + 1 : q;
                while (p < e) putchar(*p++);
            }
        } else {
            putchar(*p++);
        }
    }
}

static void _tt_lazy_init(void) {
    if (_tt_initialized) return;
    _tt_platform_init();
    _tt_global_t0   = clock();
    _tt_initialized = 1;
}

/* --- Display --- */
static void _tt_draw_bar(int pass, int total, int width) {
    int filled = (total > 0) ? (pass * width / total) : width;
    int i;
    _tt_out("[");
    for (i = 0; i < width; i++)
        _tt_out("%s", (i < filled) ? _TTC_GREEN _TTS_BFULL _TTC_RESET
                                    : _TTC_GRAY  _TTS_BEMPTY _TTC_RESET);
    _tt_out("]");
}

static void _tt_progress_print(const char *current_test) {
    int i, sp = 0, sf = 0, sk = 0;
    if (_tt_verbose == 0) return;
    if (!_tt_is_tty()) return;
    for (i = 0; i < _tt_buf_n; i++) {
        if (_tt_buf[i].status == _TT_R_PASS || _tt_buf[i].status == _TT_R_XFAIL) sp++;
        else if (_tt_buf[i].status == _TT_R_FAIL || _tt_buf[i].status == _TT_R_XPASS) sf++;
        else if (_tt_buf[i].status == _TT_R_SKIP) sk++;
    }
    _tt_out("\r\033[K"
            "  " _TTC_BOLD _TTC_CYAN "%-18.18s" _TTC_RESET
            "  " _TTC_GREEN _TTS_TICK " %d" _TTC_RESET
            "  " _TTC_RED   _TTS_CROSS "%d" _TTC_RESET
            "  " _TTC_YELLOW _TTS_SKIP "%d" _TTC_RESET
            "  " _TTC_GRAY "→ %.35s" _TTC_RESET,
            _tt_suite_name, sp, sf, sk, current_test);
    fflush(stdout);
    _tt_prog_active = 1;
}

static void _tt_progress_clear(void) {
    if (_tt_prog_active) {
        _tt_out("\r\033[K");
        fflush(stdout);
        _tt_prog_active = 0;
    }
}

/* --- JUnit XML --- */
static const char *_tt_xml_esc(const char *s, char *out, size_t cap) {
    size_t o = 0;
    if (!s) s = "";
    for (; *s && o + 7 < cap; s++) {
        switch (*s) {
            case '&':  memcpy(out + o, "&amp;",  5); o += 5; break;
            case '<':  memcpy(out + o, "&lt;",   4); o += 4; break;
            case '>':  memcpy(out + o, "&gt;",   4); o += 4; break;
            case '"':  memcpy(out + o, "&quot;", 6); o += 6; break;
            case '\n': out[o++] = ' '; break;
            case '\r': break;
            default:   out[o++] = *s; break;
        }
    }
    out[o] = '\0';
    return out;
}

static void _tt_junit_write_suite(int sf, int sk, int xf, int xp, int total_ms) {
    char e1[TT_MSG_MAX * 2], e2[TT_MSG_MAX * 2];
    int i;
    if (!_tt_junit) return;
    fprintf(_tt_junit,
            "  <testsuite name=\"%s\" tests=\"%d\" failures=\"%d\" skipped=\"%d\" time=\"%.3f\">\n",
            _tt_xml_esc(_tt_suite_name, e1, sizeof(e1)),
            _tt_buf_n, sf + xp, sk + xf, (double)total_ms / 1000.0);
    for (i = 0; i < _tt_buf_n; i++) {
        _tt_result *r = &_tt_buf[i];
        fprintf(_tt_junit, "    <testcase name=\"%s\" time=\"%.3f\"",
                _tt_xml_esc(r->name, e1, sizeof(e1)), (double)r->ms / 1000.0);
        switch (r->status) {
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

/* --- Flush de suite --- */
static void _tt_flush_suite(void) {
    int i, sp = 0, sf = 0, sk = 0, xf = 0, xp = 0, total_ms = 0;
    int any_hard_fail;

    if (_tt_buf_n == 0) return;
    _tt_progress_clear();

    if (_tt_after_all_fn) _tt_after_all_fn();

    for (i = 0; i < _tt_buf_n; i++) {
        total_ms += _tt_buf[i].ms;
        switch (_tt_buf[i].status) {
            case _TT_R_PASS:  sp++; break;
            case _TT_R_FAIL:  sf++; break;
            case _TT_R_SKIP:  sk++; break;
            case _TT_R_XFAIL: xf++; break;
            case _TT_R_XPASS: xp++; break;
        }
    }
    any_hard_fail = (sf > 0 || xp > 0);

    _tt_out(_TTC_BOLD "%s%s" _TTC_RESET " %s"
            _TTC_GRAY " (%d tests",
            any_hard_fail ? _TTC_RED   : _TTC_GREEN,
            any_hard_fail ? _TTS_CROSS : _TTS_TICK,
            _tt_suite_name, _tt_buf_n);
    if (sf) _tt_out(" | " _TTC_RED    "%d failed"  _TTC_GRAY, sf);
    if (sk) _tt_out(" | " _TTC_YELLOW "%d skipped" _TTC_GRAY, sk);
    if (xf) _tt_out(" | " _TTC_MAGENTA "%d xfail"  _TTC_GRAY, xf);
    if (xp) _tt_out(" | " _TTC_MAGENTA "%d xpass"  _TTC_GRAY, xp);
    _tt_out(") %dms" _TTC_RESET "\n", total_ms);

    for (i = 0; i < _tt_buf_n; i++) {
        _tt_result *r = &_tt_buf[i];
        if (_tt_verbose == 0 && r->status == _TT_R_PASS) continue;
        switch (r->status) {
            case _TT_R_PASS:
                _tt_out("    " _TTC_GREEN _TTS_TICK _TTC_RESET " %s "
                        _TTC_GRAY "%dms" _TTC_RESET "\n", r->name, r->ms);
                break;
            case _TT_R_FAIL: {
                char tmp[TT_MSG_MAX], *p, *nl;
                _tt_out("    " _TTC_RED _TTS_CROSS " %s %dms" _TTC_RESET "\n",
                        r->name, r->ms);
                strncpy(tmp, r->error, TT_MSG_MAX - 1);
                tmp[TT_MSG_MAX - 1] = '\0';
                p = tmp;
                while (*p) {
                    nl = strchr(p, '\n');
                    if (nl) *nl = '\0';
                    if (*p) _tt_out("      " _TTC_GRAY "%s" _TTC_RESET "\n", p);
                    if (!nl) break;
                    p = nl + 1;
                }
                break;
            }
            case _TT_R_SKIP:
                _tt_out("    " _TTC_YELLOW _TTS_SKIP _TTC_RESET " %s"
                        _TTC_GRAY " (skipped%s%s)" _TTC_RESET "\n",
                        r->name, r->error[0] ? ": " : "", r->error[0] ? r->error : "");
                break;
            case _TT_R_XFAIL:
                _tt_out("    " _TTC_MAGENTA _TTS_XFAIL _TTC_RESET " %s"
                        _TTC_GRAY " (expected failure: %s)" _TTC_RESET "\n",
                        r->name, r->error);
                break;
            case _TT_R_XPASS:
                _tt_out("    " _TTC_YELLOW _TTS_XPASS _TTC_RESET " %s"
                        _TTC_GRAY " (unexpected pass!)" _TTC_RESET "\n", r->name);
                break;
        }
    }
    _tt_out("\n");

    _tt_junit_write_suite(sf, sk, xf, xp, total_ms);

    _tt_g_pass  += sp;
    _tt_g_fail  += sf;
    _tt_g_skip  += sk;
    _tt_g_xfail += xf;
    _tt_g_xpass += xp;
    any_hard_fail ? _tt_f_fail++ : _tt_f_pass++;
    _tt_buf_n = 0;
}

/* --- Configuracion --- */
void tt_verbose(int v) { _tt_verbose = v; }
void tt_color(int mode) { _tt_color_mode = mode; }

void tt_output_junit(const char *path) {
    _tt_junit = fopen(path, "w");
    if (_tt_junit)
        fprintf(_tt_junit, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<testsuites>\n");
}

/* --- Linea de comandos --- */
static void _tt_print_help(const char *prog) {
    _tt_out("Uso: %s [opciones]\n"
            "  -f, --filter <texto>  Ejecuta solo los tests cuyo \"suite > nombre\"\n"
            "                        contenga <texto> (sin distinguir mayusculas).\n"
            "  -v, --verbose <0|1|2> Nivel de detalle de la salida.\n"
            "      --junit <archivo> Genera un informe JUnit XML.\n"
            "      --color           Fuerza salida con color.\n"
            "      --no-color        Desactiva el color.\n"
            "  -h, --help            Muestra esta ayuda y termina.\n",
            prog ? prog : "tests");
}

void tt_parse_args(int argc, char **argv) {
    int i;
    for (i = 1; i < argc; i++) {
        const char *a = argv[i];
        if ((!strcmp(a, "--filter") || !strcmp(a, "-f")) && i + 1 < argc) {
            strncpy(_tt_filter, argv[++i], TT_FILTER_MAX - 1);
            _tt_filter[TT_FILTER_MAX - 1] = '\0';
            _tt_filter_set = 1;
        } else if ((!strcmp(a, "--verbose") || !strcmp(a, "-v")) && i + 1 < argc) {
            _tt_verbose = atoi(argv[++i]);
        } else if (!strcmp(a, "--junit") && i + 1 < argc) {
            tt_output_junit(argv[++i]);
        } else if (!strcmp(a, "--color")) {
            _tt_color_mode = 1;
        } else if (!strcmp(a, "--no-color")) {
            _tt_color_mode = 0;
        } else if (!strcmp(a, "--help") || !strcmp(a, "-h")) {
            _tt_print_help(argv[0]);
            exit(0);
        }
    }
}

/* --- Suites --- */
void tt_suite(const char *name) {
    _tt_lazy_init();
    _tt_flush_suite();
    strncpy(_tt_suite_name, name, TT_NAME_MAX - 1);
    _tt_suite_name[TT_NAME_MAX - 1] = '\0';
    _tt_suite_t0       = clock();
    _tt_setup_fn       = NULL;
    _tt_teardown_fn    = NULL;
    _tt_before_all_fn  = NULL;
    _tt_after_all_fn   = NULL;
    _tt_before_all_ran = 0;
}

void tt_suite_hooks(tt_fn setup_fn, tt_fn teardown_fn) {
    _tt_setup_fn    = setup_fn;
    _tt_teardown_fn = teardown_fn;
}

void tt_suite_hooks_once(tt_fn before_all, tt_fn after_all) {
    _tt_before_all_fn  = before_all;
    _tt_after_all_fn   = after_all;
    _tt_before_all_ran = 0;
}

/* --- Filtrado --- */
static int _tt_ci_contains(const char *hay, const char *needle) {
    size_t nl = strlen(needle), i;
    if (nl == 0) return 1;
    for (; *hay; hay++) {
        for (i = 0; i < nl && hay[i] &&
                    tolower((unsigned char)hay[i]) == tolower((unsigned char)needle[i]); i++) {}
        if (i == nl) return 1;
    }
    return 0;
}

static int _tt_filtered_out(const char *name) {
    char label[TT_NAME_MAX * 2 + 4];
    if (!_tt_filter_set) return 0;
    snprintf(label, sizeof(label), "%s > %s", _tt_suite_name, name);
    return !_tt_ci_contains(label, _tt_filter);
}

static void _tt_maybe_before_all(void) {
    if (_tt_before_all_fn && !_tt_before_all_ran) {
        _tt_before_all_fn();
        _tt_before_all_ran = 1;
    }
}

/* --- Tests --- */
void tt_run(const char *name, tt_fn fn) {
    clock_t t0;
    _tt_result *r;
    int test_ok;

    _tt_lazy_init();
    if (_tt_filtered_out(name)) return;
    if (_tt_buf_n >= TT_MAX_TESTS) return;

    _tt_maybe_before_all();
    _tt_progress_print(name);

    r = &_tt_buf[_tt_buf_n++];
    strncpy(r->name, name, TT_NAME_MAX - 1);
    r->name[TT_NAME_MAX - 1] = '\0';
    r->error[0]  = '\0';
    _tt_soft_n   = 0;
    _tt_jmp_code = 0;

    if (_tt_setup_fn) _tt_setup_fn();
    t0 = clock();

    if (setjmp(_tt_jmp) == 0) {
        fn();
        test_ok = (_tt_soft_n == 0);
        if (!test_ok) {
            int i;
            _tt_errmsg[0] = '\0';
            for (i = 0; i < _tt_soft_n; i++) {
                strncat(_tt_errmsg, _tt_soft[i], TT_MSG_MAX - strlen(_tt_errmsg) - 3);
                strncat(_tt_errmsg, "\n",         TT_MSG_MAX - strlen(_tt_errmsg) - 1);
            }
            _tt_jmp_code = 1;
        }
    }

    r->ms = (int)((double)(clock() - t0) * 1000.0 / CLOCKS_PER_SEC);
    if (_tt_teardown_fn) _tt_teardown_fn();

    if (_tt_jmp_code == 2) {
        r->status = _TT_R_SKIP;
        strncpy(r->error, _tt_errmsg, TT_MSG_MAX - 1);
    } else if (_tt_jmp_code == 1) {
        r->status = _TT_R_FAIL;
        strncpy(r->error, _tt_errmsg, TT_MSG_MAX - 1);
        if (_tt_fail_n < TT_MAX_FAILURES) {
            strncpy(_tt_fail_suite[_tt_fail_n], _tt_suite_name, TT_NAME_MAX - 1);
            strncpy(_tt_fail_name [_tt_fail_n], name,           TT_NAME_MAX - 1);
            _tt_fail_n++;
        }
    } else {
        r->status = _TT_R_PASS;
    }

    {
        char label[TT_NAME_MAX * 2 + 4];
        snprintf(label, sizeof(label) - 1, "%s > %s", _tt_suite_name, name);
        if (_tt_slowest_ms < 0 || r->ms > _tt_slowest_ms) {
            _tt_slowest_ms = r->ms;
            strncpy(_tt_slowest_label, label, sizeof(_tt_slowest_label) - 1);
        }
        if (r->status != _TT_R_SKIP && (_tt_fastest_ms < 0 || r->ms < _tt_fastest_ms)) {
            _tt_fastest_ms = r->ms;
            strncpy(_tt_fastest_label, label, sizeof(_tt_fastest_label) - 1);
        }
    }
}

void tt_run_param(const char *name, tt_fn fn, const void *param) {
    tt_param = param;
    tt_run(name, fn);
    tt_param = NULL;
}

void tt_skip_test(const char *name, const char *reason) {
    _tt_result *r;
    _tt_lazy_init();
    if (_tt_filtered_out(name)) return;
    if (_tt_buf_n >= TT_MAX_TESTS) return;
    r = &_tt_buf[_tt_buf_n++];
    strncpy(r->name, name, TT_NAME_MAX - 1);
    r->name[TT_NAME_MAX - 1] = '\0';
    r->status = _TT_R_SKIP;
    r->ms     = 0;
    strncpy(r->error, reason ? reason : "", TT_MSG_MAX - 1);
}

void tt_xfail(const char *name, const char *reason, tt_fn fn) {
    clock_t t0;
    _tt_result *r;
    int test_failed;

    _tt_lazy_init();
    if (_tt_filtered_out(name)) return;
    if (_tt_buf_n >= TT_MAX_TESTS) return;

    _tt_maybe_before_all();
    _tt_progress_print(name);

    r = &_tt_buf[_tt_buf_n++];
    strncpy(r->name, name, TT_NAME_MAX - 1);
    r->name[TT_NAME_MAX - 1] = '\0';
    strncpy(r->error, reason ? reason : "", TT_MSG_MAX - 1);
    _tt_soft_n   = 0;
    _tt_jmp_code = 0;

    if (_tt_setup_fn) _tt_setup_fn();
    t0 = clock();
    if (setjmp(_tt_jmp) == 0) {
        fn();
        test_failed = (_tt_soft_n > 0);
    } else {
        test_failed = 1;
    }
    r->ms = (int)((double)(clock() - t0) * 1000.0 / CLOCKS_PER_SEC);
    if (_tt_teardown_fn) _tt_teardown_fn();
    r->status = test_failed ? _TT_R_XFAIL : _TT_R_XPASS;

    if (r->status == _TT_R_XPASS && _tt_fail_n < TT_MAX_FAILURES) {
        strncpy(_tt_fail_suite[_tt_fail_n], _tt_suite_name, TT_NAME_MAX - 1);
        strncpy(_tt_fail_name [_tt_fail_n], name,           TT_NAME_MAX - 1);
        _tt_fail_n++;
    }
}

/* --- Resumen --- */
int tt_summary(void) {
    double total_ms;
    int total_tests, total_files, pass_bar, total_bar;
    int i;

    _tt_lazy_init();
    _tt_flush_suite();

    total_ms    = (double)(clock() - _tt_global_t0) * 1000.0 / CLOCKS_PER_SEC;
    total_tests = _tt_g_pass + _tt_g_fail + _tt_g_skip + _tt_g_xfail + _tt_g_xpass;
    total_files = _tt_f_pass + _tt_f_fail;
    pass_bar  = _tt_g_pass + _tt_g_xfail;
    total_bar = total_tests - _tt_g_skip;

    if (_tt_junit) {
        fprintf(_tt_junit, "</testsuites>\n");
        fclose(_tt_junit);
        _tt_junit = NULL;
    }

    if (_tt_fail_n > 0) {
        int border = 40;
        _tt_out(_TTC_RED _TTC_BOLD "─── Failed Tests %d ", _tt_fail_n);
        for (i = 0; i < border; i++) _tt_out("─");
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
    if (_tt_f_fail)  _tt_out(_TTC_RED    "%d failed"  _TTC_RESET " | ", _tt_f_fail);
    _tt_out(_TTC_GREEN "%d passed" _TTC_RESET
            _TTC_GRAY  " (%d)"    _TTC_RESET "\n", _tt_f_pass, total_files);

    _tt_out(_TTC_BOLD "     Tests  " _TTC_RESET);
    if (_tt_g_fail)  _tt_out(_TTC_RED     "%d failed"  _TTC_RESET " | ", _tt_g_fail);
    if (_tt_g_xpass) _tt_out(_TTC_YELLOW  "%d xpass"   _TTC_RESET " | ", _tt_g_xpass);
    if (_tt_g_xfail) _tt_out(_TTC_MAGENTA "%d xfail"   _TTC_RESET " | ", _tt_g_xfail);
    if (_tt_g_skip)  _tt_out(_TTC_YELLOW  "%d skipped" _TTC_RESET " | ", _tt_g_skip);
    _tt_out(_TTC_GREEN "%d passed" _TTC_RESET
            _TTC_GRAY  " (%d)"    _TTC_RESET "\n", _tt_g_pass, total_tests);

    _tt_out(_TTC_BOLD "  Duration  " _TTC_RESET
            _TTC_GRAY  "%.1fms"   _TTC_RESET "\n", total_ms);

    if (_tt_verbose >= 1 && _tt_slowest_ms >= 0)
        _tt_out(_TTC_BOLD "   Slowest  " _TTC_RESET
                _TTC_GRAY  "%dms — %s" _TTC_RESET "\n",
                _tt_slowest_ms, _tt_slowest_label);
    if (_tt_verbose >= 1 && _tt_fastest_ms >= 0)
        _tt_out(_TTC_BOLD "   Fastest  " _TTC_RESET
                _TTC_GRAY  "%dms — %s" _TTC_RESET "\n",
                _tt_fastest_ms, _tt_fastest_label);

    return (_tt_g_fail > 0 || _tt_g_xpass > 0) ? 1 : 0;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CTESTS_IMPLEMENTATION */

#endif /* ctests_H */
