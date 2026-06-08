/*
 * VestaVM - Máquina Virtual Distribuida
 *
 * Copyright © 2026 David López.T (DesmonHak) (Castilla y León, ES)
 * Licencia VMProject
 */

/**
 * @file advanced.c
 * @brief Demuestra las capacidades avanzadas de ctests.
 *
 * Cubre: tests data-driven (tt_run_param), hooks "once" por suite
 * (tt_suite_hooks_once), nuevas aserciones (EXPECT_EQ_UINT, EXPECT_EQ_MEM,
 * EXPECT_EQ_PTR, EXPECT_MSG), alias ASSERT_*, y el procesado de argumentos
 * de linea de comandos (tt_parse_args: --filter, --junit, --no-color, ...).
 *
 * @code
 *   gcc -std=c99 -I.. example/advanced.c ctests.c -o advanced -lm && ./advanced
 *   ./advanced --filter Aritmetica       # solo esa suite
 *   ./advanced --junit out.xml           # ademas genera JUnit XML
 * @endcode
 */
#include "ctests.h"
#include <string.h>

/* --- Tests data-driven: una funcion, varios casos via tt_param --- */

struct CasoSuma { int a, b, esperado; };

static const struct CasoSuma g_casos[] = {
    { 1,  1,  2 },
    { 2,  3,  5 },
    { -4, 4,  0 },
    { 10, 90, 100 },
};

static void test_suma_param(void) {
    const struct CasoSuma *c = (const struct CasoSuma *)tt_param;
    EXPECT_MSG(c->a + c->b == c->esperado,
               "%d + %d deberia ser %d, no %d", c->a, c->b, c->esperado, c->a + c->b);
}

/* --- Hooks "once": se ejecutan una sola vez por suite --- */

static int g_recurso_abierto = 0;

static void abrir_recurso(void)  { g_recurso_abierto = 1; }
static void cerrar_recurso(void) { g_recurso_abierto = 0; }

static void test_recurso_disponible(void) {
    ASSERT_TRUE(g_recurso_abierto);   /* alias de EXPECT_TRUE */
}

/* --- Nuevas aserciones: unsigned, memoria, punteros --- */

static void test_unsigned(void) {
    unsigned long long grande = 18446744073709551615ULL;  /* 2^64 - 1 */
    EXPECT_EQ_UINT(grande, 18446744073709551615ULL);
}

static void test_memoria(void) {
    unsigned char a[4] = { 0xDE, 0xAD, 0xBE, 0xEF };
    unsigned char b[4] = { 0xDE, 0xAD, 0xBE, 0xEF };
    EXPECT_EQ_MEM(a, b, sizeof(a));
}

static void test_punteros(void) {
    int x = 0;
    int *p = &x;
    EXPECT_EQ_PTR(p, &x);
}

int main(int argc, char **argv) {
    size_t i;
    tt_parse_args(argc, argv);   /* --filter, --junit, --verbose, --no-color, --help */

    tt_suite("Aritmetica (data-driven)");
        for (i = 0; i < sizeof(g_casos) / sizeof(g_casos[0]); i++) {
            char nombre[48];
            snprintf(nombre, sizeof(nombre), "caso %u: %d + %d",
                     (unsigned)i, g_casos[i].a, g_casos[i].b);
            tt_run_param(nombre, test_suma_param, &g_casos[i]);
        }

    tt_suite("Recurso compartido (hooks once)");
        tt_suite_hooks_once(abrir_recurso, cerrar_recurso);
        tt_run("recurso disponible #1", test_recurso_disponible);
        tt_run("recurso disponible #2", test_recurso_disponible);

    tt_suite("Aserciones nuevas");
        tt_run("enteros sin signo grandes", test_unsigned);
        tt_run("bloques de memoria",        test_memoria);
        tt_run("igualdad de punteros",      test_punteros);

    return tt_summary();
}
