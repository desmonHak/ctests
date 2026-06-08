/*
 * VestaVM - Máquina Virtual Distribuida
 * Copyright © 2026 David López.T (DesmonHak) (Castilla y León, ES) — Licencia VMProject
 */

/**
 * @file multifile_aritmetica.c
 * @brief Una suite de tests definida en su propio archivo (demo multi-archivo).
 */
#include "ctests.h"
#include "multifile.h"

static void test_suma(void)  { EXPECT_EQ_INT(2 + 2, 4); }
static void test_resta(void) { EXPECT_EQ_INT(9 - 4, 5); }

void registrar_suite_aritmetica(void) {
    tt_suite("Aritmetica");
        tt_run("suma",  test_suma);
        tt_run("resta", test_resta);
}
