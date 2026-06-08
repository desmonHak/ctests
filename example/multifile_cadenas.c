/*
 * VestaVM - Máquina Virtual Distribuida
 * Copyright © 2026 David López.T (DesmonHak) (Castilla y León, ES) — Licencia VMProject
 */

/**
 * @file multifile_cadenas.c
 * @brief Otra suite en otro archivo, compartiendo el estado de ctests.
 */
#include "ctests.h"
#include "multifile.h"

static void test_contiene(void) { EXPECT_CONTAINS("the quick brown fox", "brown"); }
static void test_prefijo(void)  { EXPECT_STARTS_WITH("https://x", "https://"); }

void registrar_suite_cadenas(void) {
    tt_suite("Cadenas");
        tt_run("contiene subcadena", test_contiene);
        tt_run("empieza con prefijo", test_prefijo);
}
