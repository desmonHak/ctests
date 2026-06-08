/*
 * VestaVM - Máquina Virtual Distribuida
 * Copyright © 2026 David López.T (DesmonHak) (Castilla y León, ES) — Licencia VMProject
 */

/**
 * @file meta_ok.c
 * @brief Meta-tests: comprueban el comportamiento de ctests que DEBE pasar.
 *
 * Verifica el despacho por tipo de EXPECT_EQ (si fuera incorrecto, p.ej. comparar
 * cadenas como punteros, estos tests fallarian), tt_param, y aserciones nuevas.
 * Exit 0 esperado.
 */
#include "ctests.h"
#include <string.h>

static void t_eq_dispatch(void)
{
    EXPECT_EQ(2 + 2, 4);   /* int  */
    EXPECT_EQ(7u, 7u);     /* uint */
    EXPECT_EQ(0.1 + 0.2, 0.3); /* float: tolerancia relativa */
    /* Cadenas: punteros distintos, mismo contenido -> debe despachar a strcmp. */
    char a[] = "hola";
    const char *b = "hola";
    EXPECT_EQ(a, b);
    /* Punteros */
    int x = 0;
    int *p = &x;
    EXPECT_EQ(p, &x);
    EXPECT_NE(3, 4);
    EXPECT_NE("abc", "abd");
}

struct Caso
{
    int a, b, w;
};
static const struct Caso g_casos[] = {{1, 1, 2}, {2, 3, 5}, {-4, 4, 0}};

static void t_param(void)
{
    const struct Caso *c = (const struct Caso *)tt_param;
    EXPECT_EQ(c->a + c->b, c->w);
}

static void t_misc(void)
{
    int u[3] = {1, 2, 3}, v[3] = {1, 2, 3};
    EXPECT_IN_RANGE(5, 0, 10);
    EXPECT_ARRAY_EQ(u, v, 3);
    EXPECT_NEAR_REL(1e9 + 1.0, 1e9 + 1.0, 1e-6);
}

int main(void)
{
    size_t i;
    tt_suite("meta/ok");
    tt_run("EXPECT_EQ despacha por tipo", t_eq_dispatch);
    for (i = 0; i < sizeof(g_casos) / sizeof(g_casos[0]); i++)
    {
        char n[24];
        snprintf(n, sizeof(n), "param %u", (unsigned)i);
        tt_run_param(n, t_param, &g_casos[i]);
    }
    tt_run("rango y arrays", t_misc);
    return tt_summary();
}
