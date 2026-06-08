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
 * @brief Libreria de tests unitarios e integracion — C99/C++17, header-only.
 *
 * @details Sin dependencias externas. Compatible con C puro (C99) y C++17.
 *   Mecanismo: setjmp/longjmp para detener el test al primer EXPECT_* fallido.
 *   Los resultados se bufferean por suite para imprimir la cabecera con conteos exactos.
 *
 * Caracteristicas:
 *   - Barra de progreso live (se actualiza con \r durante la ejecucion)
 *   - Barra de resumen visual con bloques Unicode
 *   - Skip: tt_skip("razon") desde dentro de un test
 *   - xfail: test esperado a fallar — no cuenta como error si falla, si cuenta si pasa
 *   - Setup/teardown por suite: tt_suite_hooks(setup, teardown)
 *   - Soft assertions: acumulan todos los fallos sin parar, falla el test al finalizar
 *   - Modo verbosidad: tt_verbose(0/1/2)
 *   - Estadisticas de tiempo: slowest/fastest en el resumen
 *
 * Compilacion:
 * @code
 *   gcc  -std=c99  mis_tests.c   -o mis_tests -lm && ./mis_tests
 *   g++  -std=c++17 mis_tests.cpp -o mis_tests -lm && ./mis_tests
 * @endcode
 */
#ifndef ctests_H
#define ctests_H

#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* =============================================================================
 * CONFIGURACION — ajustar segun el proyecto
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

/* =============================================================================
 * CODIGOS ANSI Y SIMBOLOS UNICODE
 * ============================================================================= */

#define _TTC_GREEN   "\033[32m"
#define _TTC_RED     "\033[31m"
#define _TTC_YELLOW  "\033[33m"
#define _TTC_CYAN    "\033[36m"
#define _TTC_MAGENTA "\033[35m"
#define _TTC_GRAY    "\033[90m"
#define _TTC_BOLD    "\033[1m"
#define _TTC_RESET   "\033[0m"

/* UTF-8: U+2713 CHECK, U+2717 CROSS, U+21B7 SKIP, U+223C TILDE */
#define _TTS_TICK   "\xE2\x9C\x93"
#define _TTS_CROSS  "\xE2\x9C\x97"
#define _TTS_SKIP   "\xE2\x86\xB7"
#define _TTS_XFAIL  "\xE2\x88\xBC"
#define _TTS_XPASS  "!"

/* U+2588 FULL BLOCK, U+2591 LIGHT SHADE — barra de progreso */
#define _TTS_BFULL  "\xE2\x96\x88"
#define _TTS_BEMPTY "\xE2\x96\x91"

/* =============================================================================
 * TIPOS INTERNOS
 * ============================================================================= */

#define _TT_PASS  0
#define _TT_FAIL  1
#define _TT_SKIP  2
#define _TT_XFAIL 3   /**< Fallo esperado: test fallo como se anticipaba. */
#define _TT_XPASS 4   /**< Pase inesperado: test se esperaba que fallara pero paso. */

/** Resultado de un test individual. */
typedef struct {
    char name[TT_NAME_MAX];
    int  status;            /**< Uno de _TT_PASS/_TT_FAIL/_TT_SKIP/_TT_XFAIL/_TT_XPASS */
    char error[TT_MSG_MAX]; /**< Mensaje de asercion o razon de skip/xfail. */
    int  ms;                /**< Duracion en milisegundos. */
} _tt_result;

/** Firma de funcion de test: sin argumentos, sin retorno. */
typedef void (*tt_fn)(void);

/* =============================================================================
 * ESTADO GLOBAL
 * ============================================================================= */

static jmp_buf      _tt_jmp;
static int          _tt_jmp_code;           /**< 1=fallo hard, 2=skip */
static char         _tt_errmsg[TT_MSG_MAX];

/* Buffer de resultados de la suite en curso */
static _tt_result   _tt_buf[TT_MAX_TESTS];
static int          _tt_buf_n = 0;
static char         _tt_suite_name[TT_NAME_MAX];
static clock_t      _tt_suite_t0;

/* Hooks de setup/teardown por suite */
static tt_fn        _tt_setup_fn    = NULL;
static tt_fn        _tt_teardown_fn = NULL;

/* Soft assertions acumuladas */
static char         _tt_soft[TT_MAX_SOFT][TT_MSG_MAX];
static int          _tt_soft_n = 0;

/* Estadisticas globales */
static int          _tt_g_pass  = 0;
static int          _tt_g_fail  = 0;
static int          _tt_g_skip  = 0;
static int          _tt_g_xfail = 0;
static int          _tt_g_xpass = 0;
static int          _tt_f_pass  = 0;  /**< Suites sin fallos. */
static int          _tt_f_fail  = 0;  /**< Suites con al menos un fallo. */
static clock_t      _tt_global_t0;
static int          _tt_initialized = 0;
static int          _tt_verbose     = 1;  /**< 0=silencioso, 1=normal, 2=detallado */

/* Seguimiento de test mas lento y mas rapido */
static int          _tt_slowest_ms = -1;
static int          _tt_fastest_ms = -1;
static char         _tt_slowest_label[TT_NAME_MAX * 2 + 4];
static char         _tt_fastest_label[TT_NAME_MAX * 2 + 4];

/* Tabla de fallos para el resumen */
static char         _tt_fail_suite  [TT_MAX_FAILURES][TT_NAME_MAX];
static char         _tt_fail_name   [TT_MAX_FAILURES][TT_NAME_MAX];
static int          _tt_fail_n = 0;

/* Control de linea de progreso */
static int          _tt_prog_active = 0;

/* =============================================================================
 * UTILIDADES DE DISPLAY
 * ============================================================================= */

/**
 * @brief Imprime la barra de progreso proporcional.
 * @param pass  Numero de tests que han pasado.
 * @param total Total de tests (excluyendo skipped).
 * @param width Anchura en caracteres de la barra.
 */
static void _tt_draw_bar(int pass, int total, int width) {
    int filled = (total > 0) ? (pass * width / total) : width;
    int i;
    printf("[");
    for (i = 0; i < width; i++)
        printf("%s", (i < filled) ? _TTC_GREEN _TTS_BFULL _TTC_RESET
                                  : _TTC_GRAY  _TTS_BEMPTY _TTC_RESET);
    printf("]");
}

/**
 * @brief Imprime la linea de progreso live sobreescribiendo la anterior.
 * @param current_test Nombre del test que se va a ejecutar ahora.
 */
static void _tt_progress_print(const char *current_test) {
    int i, sp = 0, sf = 0, sk = 0;
    if (_tt_verbose == 0) return;
    for (i = 0; i < _tt_buf_n; i++) {
        if (_tt_buf[i].status == _TT_PASS || _tt_buf[i].status == _TT_XFAIL) sp++;
        else if (_tt_buf[i].status == _TT_FAIL || _tt_buf[i].status == _TT_XPASS) sf++;
        else if (_tt_buf[i].status == _TT_SKIP) sk++;
    }
    printf("\r\033[K"
           "  " _TTC_BOLD _TTC_CYAN "%-18.18s" _TTC_RESET
           "  " _TTC_GREEN _TTS_TICK " %d" _TTC_RESET
           "  " _TTC_RED   _TTS_CROSS "%d" _TTC_RESET
           "  " _TTC_YELLOW _TTS_SKIP "%d" _TTC_RESET
           "  " _TTC_GRAY "→ %.35s" _TTC_RESET,
           _tt_suite_name, sp, sf, sk, current_test);
    fflush(stdout);
    _tt_prog_active = 1;
}

/** @brief Borra la linea de progreso si estaba activa. */
static void _tt_progress_clear(void) {
    if (_tt_prog_active) {
        printf("\r\033[K");
        fflush(stdout);
        _tt_prog_active = 0;
    }
}

/* =============================================================================
 * FLUSH DE SUITE
 * ============================================================================= */

/** @brief Vuelca el buffer de la suite actual, imprimiendo cabecera + resultados. */
static void _tt_flush_suite(void) {
    int i, sp = 0, sf = 0, sk = 0, xf = 0, xp = 0, total_ms = 0;
    int any_hard_fail;

    if (_tt_buf_n == 0) return;
    _tt_progress_clear();

    for (i = 0; i < _tt_buf_n; i++) {
        total_ms += _tt_buf[i].ms;
        switch (_tt_buf[i].status) {
            case _TT_PASS:  sp++; break;
            case _TT_FAIL:  sf++; break;
            case _TT_SKIP:  sk++; break;
            case _TT_XFAIL: xf++; break;
            case _TT_XPASS: xp++; break;
        }
    }
    any_hard_fail = (sf > 0 || xp > 0);

    /* --- Cabecera de suite --- */
    printf(_TTC_BOLD "%s%s" _TTC_RESET " %s"
           _TTC_GRAY " (%d tests",
           any_hard_fail ? _TTC_RED   : _TTC_GREEN,
           any_hard_fail ? _TTS_CROSS : _TTS_TICK,
           _tt_suite_name, _tt_buf_n);
    if (sf) printf(" | " _TTC_RED    "%d failed"  _TTC_GRAY, sf);
    if (sk) printf(" | " _TTC_YELLOW "%d skipped" _TTC_GRAY, sk);
    if (xf) printf(" | " _TTC_MAGENTA "%d xfail"  _TTC_GRAY, xf);
    if (xp) printf(" | " _TTC_MAGENTA "%d xpass"  _TTC_GRAY, xp);
    printf(") %dms" _TTC_RESET "\n", total_ms);

    /* --- Resultados individuales --- */
    for (i = 0; i < _tt_buf_n; i++) {
        _tt_result *r = &_tt_buf[i];
        /* En modo silencioso solo mostrar los fallos */
        if (_tt_verbose == 0 && r->status == _TT_PASS) continue;

        switch (r->status) {
            case _TT_PASS:
                printf("    " _TTC_GREEN _TTS_TICK _TTC_RESET " %s "
                       _TTC_GRAY "%dms" _TTC_RESET "\n", r->name, r->ms);
                break;

            case _TT_FAIL: {
                char tmp[TT_MSG_MAX], *p, *nl;
                printf("    " _TTC_RED _TTS_CROSS " %s %dms" _TTC_RESET "\n",
                       r->name, r->ms);
                strncpy(tmp, r->error, TT_MSG_MAX - 1);
                tmp[TT_MSG_MAX - 1] = '\0';
                p = tmp;
                while (*p) {
                    nl = strchr(p, '\n');
                    if (nl) *nl = '\0';
                    if (*p) printf("      " _TTC_GRAY "%s" _TTC_RESET "\n", p);
                    if (!nl) break;
                    p = nl + 1;
                }
                break;
            }
            case _TT_SKIP:
                printf("    " _TTC_YELLOW _TTS_SKIP _TTC_RESET " %s"
                       _TTC_GRAY " (skipped%s%s)" _TTC_RESET "\n",
                       r->name,
                       r->error[0] ? ": " : "",
                       r->error[0] ? r->error : "");
                break;

            case _TT_XFAIL:
                printf("    " _TTC_MAGENTA _TTS_XFAIL _TTC_RESET " %s"
                       _TTC_GRAY " (expected failure: %s)" _TTC_RESET "\n",
                       r->name, r->error);
                break;

            case _TT_XPASS:
                printf("    " _TTC_YELLOW _TTS_XPASS _TTC_RESET " %s"
                       _TTC_GRAY " (unexpected pass!)" _TTC_RESET "\n", r->name);
                break;
        }
    }
    printf("\n");

    /* Actualizar estadisticas globales */
    _tt_g_pass  += sp;
    _tt_g_fail  += sf;
    _tt_g_skip  += sk;
    _tt_g_xfail += xf;
    _tt_g_xpass += xp;
    any_hard_fail ? _tt_f_fail++ : _tt_f_pass++;
    _tt_buf_n = 0;
}

/* =============================================================================
 * API PUBLICA — CONFIGURACION
 * ============================================================================= */

/**
 * @brief Establece el nivel de verbosidad de la salida.
 * @param v 0=silencioso (solo fallos), 1=normal (defecto), 2=detallado (slowest/fastest)
 */
static void tt_verbose(int v) { _tt_verbose = v; }

/* =============================================================================
 * API PUBLICA — SUITES
 * ============================================================================= */

/**
 * @brief Inicia una nueva suite de tests.
 * @details Hace flush automatico de la suite anterior si existe.
 *   Resetea setup/teardown: llamar a tt_suite_hooks() despues si los necesitas.
 * @param name Nombre descriptivo del grupo.
 */
static void tt_suite(const char *name) {
    if (!_tt_initialized) {
        _tt_global_t0   = clock();
        _tt_initialized = 1;
    }
    _tt_flush_suite();
    strncpy(_tt_suite_name, name, TT_NAME_MAX - 1);
    _tt_suite_name[TT_NAME_MAX - 1] = '\0';
    _tt_suite_t0     = clock();
    _tt_setup_fn     = NULL;
    _tt_teardown_fn  = NULL;
}

/**
 * @brief Registra setup y teardown para la suite actual.
 * @param setup_fn    Llamada ANTES de cada test. NULL para desactivar.
 * @param teardown_fn Llamada DESPUES de cada test (incluso si falla). NULL para desactivar.
 */
static void tt_suite_hooks(tt_fn setup_fn, tt_fn teardown_fn) {
    _tt_setup_fn    = setup_fn;
    _tt_teardown_fn = teardown_fn;
}

/* =============================================================================
 * API PUBLICA — TESTS
 * ============================================================================= */

/**
 * @brief Ejecuta un test y registra el resultado.
 * @param name Nombre del caso de prueba.
 * @param fn   Funcion de test: void fn(void).
 */
static void tt_run(const char *name, tt_fn fn) {
    clock_t t0;
    _tt_result *r;
    int test_ok;

    if (!_tt_initialized) { _tt_global_t0 = clock(); _tt_initialized = 1; }
    if (_tt_buf_n >= TT_MAX_TESTS) return;

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
        test_ok = (_tt_soft_n == 0); /* fallan soft-assertions pendientes? */
        if (!test_ok) {
            /* Combinar mensajes soft en _tt_errmsg */
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
        r->status = _TT_SKIP;
        strncpy(r->error, _tt_errmsg, TT_MSG_MAX - 1);
    } else if (_tt_jmp_code == 1) {
        r->status = _TT_FAIL;
        strncpy(r->error, _tt_errmsg, TT_MSG_MAX - 1);
        if (_tt_fail_n < TT_MAX_FAILURES) {
            strncpy(_tt_fail_suite[_tt_fail_n], _tt_suite_name, TT_NAME_MAX - 1);
            strncpy(_tt_fail_name [_tt_fail_n], name,           TT_NAME_MAX - 1);
            _tt_fail_n++;
        }
    } else {
        r->status = _TT_PASS;
    }

    /* Seguimiento de slowest/fastest */
    {
        char label[TT_NAME_MAX * 2 + 4];
        snprintf(label, sizeof(label) - 1, "%s > %s", _tt_suite_name, name);
        if (_tt_slowest_ms < 0 || r->ms > _tt_slowest_ms) {
            _tt_slowest_ms = r->ms;
            strncpy(_tt_slowest_label, label, sizeof(_tt_slowest_label) - 1);
        }
        if (r->status != _TT_SKIP && (_tt_fastest_ms < 0 || r->ms < _tt_fastest_ms)) {
            _tt_fastest_ms = r->ms;
            strncpy(_tt_fastest_label, label, sizeof(_tt_fastest_label) - 1);
        }
    }
}

/**
 * @brief Registra un test como saltado sin ejecutarlo.
 * @param name   Nombre del test.
 * @param reason Razon del salto (puede ser NULL).
 */
static void tt_skip_test(const char *name, const char *reason) {
    _tt_result *r;
    if (_tt_buf_n >= TT_MAX_TESTS) return;
    r = &_tt_buf[_tt_buf_n++];
    strncpy(r->name, name, TT_NAME_MAX - 1);
    r->name[TT_NAME_MAX - 1] = '\0';
    r->status = _TT_SKIP;
    r->ms     = 0;
    strncpy(r->error, reason ? reason : "", TT_MSG_MAX - 1);
}

/**
 * @brief Ejecuta un test marcado como "expected failure" (xfail).
 * @details Si el test FALLA → _TT_XFAIL (no cuenta como error).
 *          Si el test PASA  → _TT_XPASS (inesperado, cuenta como error).
 * @param name   Nombre del test.
 * @param reason Razon por la que se espera que falle (p.ej. "bug #42").
 * @param fn     Funcion de test.
 */
static void tt_xfail(const char *name, const char *reason, tt_fn fn) {
    clock_t t0;
    _tt_result *r;
    int test_failed;

    if (_tt_buf_n >= TT_MAX_TESTS) return;
    _tt_progress_print(name);

    r = &_tt_buf[_tt_buf_n++];
    strncpy(r->name, name, TT_NAME_MAX - 1);
    r->name[TT_NAME_MAX - 1] = '\0';
    strncpy(r->error, reason ? reason : "", TT_MSG_MAX - 1);
    _tt_soft_n   = 0;
    _tt_jmp_code = 0;

    t0 = clock();
    if (setjmp(_tt_jmp) == 0) {
        fn();
        test_failed = (_tt_soft_n > 0); /* soft failures cuentan */
    } else {
        test_failed = 1;
    }
    r->ms     = (int)((double)(clock() - t0) * 1000.0 / CLOCKS_PER_SEC);
    r->status = test_failed ? _TT_XFAIL : _TT_XPASS;

    if (r->status == _TT_XPASS) {
        if (_tt_fail_n < TT_MAX_FAILURES) {
            strncpy(_tt_fail_suite[_tt_fail_n], _tt_suite_name, TT_NAME_MAX - 1);
            strncpy(_tt_fail_name [_tt_fail_n], name,           TT_NAME_MAX - 1);
            _tt_fail_n++;
        }
    }
}

/* =============================================================================
 * API PUBLICA — CONTROL DENTRO DE UN TEST
 * ============================================================================= */

/**
 * @brief Salta el test actual con un motivo. Llamar desde dentro de la funcion de test.
 * @param reason Cadena de texto explicando el salto.
 */
#define tt_skip(reason) \
    do { strncpy(_tt_errmsg, (reason), TT_MSG_MAX-1); \
         _tt_jmp_code = 2; longjmp(_tt_jmp, 2); } while(0)

/* =============================================================================
 * API PUBLICA — RESUMEN FINAL
 * ============================================================================= */

/**
 * @brief Imprime el resumen completo con barra de progreso y estadisticas.
 * @return 0 si todos los tests pasaron (xfail no cuenta como fallo), 1 si hay fallos.
 */
static int tt_summary(void) {
    double total_ms;
    int total_tests, total_files, pass_bar, total_bar;
    int i;

    _tt_flush_suite();

    total_ms    = (double)(clock() - _tt_global_t0) * 1000.0 / CLOCKS_PER_SEC;
    total_tests = _tt_g_pass + _tt_g_fail + _tt_g_skip + _tt_g_xfail + _tt_g_xpass;
    total_files = _tt_f_pass + _tt_f_fail;
    /* La barra excluye skipped */
    pass_bar  = _tt_g_pass + _tt_g_xfail;
    total_bar = total_tests - _tt_g_skip;

    /* --- Tabla de fallos --- */
    if (_tt_fail_n > 0) {
        int border = 40;
        printf(_TTC_RED _TTC_BOLD "─── Failed Tests %d ", _tt_fail_n);
        for (i = 0; i < border; i++) printf("─");
        printf(_TTC_RESET "\n");
        for (i = 0; i < _tt_fail_n; i++)
            printf("  " _TTC_RED _TTC_BOLD "FAIL" _TTC_RESET "  %s > %s\n",
                   _tt_fail_suite[i], _tt_fail_name[i]);
        printf("\n");
    }

    /* --- Barra de progreso resumen --- */
    printf("  ");
    _tt_draw_bar(pass_bar, total_bar, 32);
    printf("  " _TTC_BOLD "%d/%d" _TTC_RESET
           "  " _TTC_GRAY "(%d%%)" _TTC_RESET "\n\n",
           pass_bar, total_bar,
           (total_bar > 0) ? (pass_bar * 100 / total_bar) : 100);

    /* --- Linea: Test Files --- */
    printf(_TTC_BOLD " Test Files  " _TTC_RESET);
    if (_tt_f_fail)  printf(_TTC_RED    "%d failed"  _TTC_RESET " | ", _tt_f_fail);
    printf(_TTC_GREEN "%d passed" _TTC_RESET
           _TTC_GRAY  " (%d)"    _TTC_RESET "\n", _tt_f_pass, total_files);

    /* --- Linea: Tests --- */
    printf(_TTC_BOLD "     Tests  " _TTC_RESET);
    if (_tt_g_fail)  printf(_TTC_RED     "%d failed"  _TTC_RESET " | ", _tt_g_fail);
    if (_tt_g_xpass) printf(_TTC_YELLOW  "%d xpass"   _TTC_RESET " | ", _tt_g_xpass);
    if (_tt_g_xfail) printf(_TTC_MAGENTA "%d xfail"   _TTC_RESET " | ", _tt_g_xfail);
    if (_tt_g_skip)  printf(_TTC_YELLOW  "%d skipped" _TTC_RESET " | ", _tt_g_skip);
    printf(_TTC_GREEN "%d passed" _TTC_RESET
           _TTC_GRAY  " (%d)"    _TTC_RESET "\n", _tt_g_pass, total_tests);

    /* --- Linea: Duration --- */
    printf(_TTC_BOLD "  Duration  " _TTC_RESET
           _TTC_GRAY  "%.1fms"   _TTC_RESET "\n", total_ms);

    /* --- Slowest / Fastest (verbose >= 1) --- */
    if (_tt_verbose >= 1 && _tt_slowest_ms >= 0)
        printf(_TTC_BOLD "   Slowest  " _TTC_RESET
               _TTC_GRAY  "%dms — %s" _TTC_RESET "\n",
               _tt_slowest_ms, _tt_slowest_label);

    if (_tt_verbose >= 1 && _tt_fastest_ms >= 0)
        printf(_TTC_BOLD "   Fastest  " _TTC_RESET
               _TTC_GRAY  "%dms — %s" _TTC_RESET "\n",
               _tt_fastest_ms, _tt_fastest_label);

    return (_tt_g_fail > 0 || _tt_g_xpass > 0) ? 1 : 0;
}

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

/* Condiciones */
#define EXPECT_TRUE(c)      do{if(!(c))       _TT_FAIL("Expected TRUE: %s", #c);}while(0)
#define EXPECT_FALSE(c)     do{if((c))        _TT_FAIL("Expected FALSE: %s", #c);}while(0)

/* Punteros */
#define EXPECT_NULL(p)      do{if((p)!=NULL)  _TT_FAIL("Expected NULL: %s", #p);}while(0)
#define EXPECT_NOT_NULL(p)  do{if((p)==NULL)  _TT_FAIL("Expected non-NULL: %s", #p);}while(0)

/* Enteros */
#define EXPECT_EQ_INT(a,b) \
    do{long long _a=(long long)(a),_b=(long long)(b); \
       if(_a!=_b) _TT_FAIL("\n  - Expected: %lld\n  + Received: %lld",_b,_a);}while(0)
#define EXPECT_NEQ_INT(a,b) \
    do{long long _a=(long long)(a),_b=(long long)(b); \
       if(_a==_b) _TT_FAIL("%s == %s == %lld (expected different)",#a,#b,_a);}while(0)

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

/* Fallo incondicional */
#define EXPECT_FAIL(msg) _TT_FAIL("%s", msg)

/* =============================================================================
 * MACROS SOFT — acumulan fallos, el test falla al terminar la funcion
 * ============================================================================= */

#define SOFT_EXPECT_TRUE(c)     do{if(!(c))       _TT_SOFT_FAIL("Expected TRUE: %s", #c);}while(0)
#define SOFT_EXPECT_FALSE(c)    do{if((c))        _TT_SOFT_FAIL("Expected FALSE: %s", #c);}while(0)
#define SOFT_EXPECT_NULL(p)     do{if((p)!=NULL)  _TT_SOFT_FAIL("Expected NULL: %s", #p);}while(0)
#define SOFT_EXPECT_NOT_NULL(p) do{if((p)==NULL)  _TT_SOFT_FAIL("Expected non-NULL: %s", #p);}while(0)
#define SOFT_EXPECT_EQ_INT(a,b) \
    do{long long _a=(long long)(a),_b=(long long)(b); \
       if(_a!=_b) _TT_SOFT_FAIL("\n  - Expected: %lld\n  + Received: %lld",_b,_a);}while(0)
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
#endif /* __cplusplus */

#endif /* ctests_H */
