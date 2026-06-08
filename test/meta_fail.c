/*
 * VestaVM - Máquina Virtual Distribuida
 * Copyright © 2026 David López.T (DesmonHak) (Castilla y León, ES) — Licencia VMProject
 */

/**
 * @file meta_fail.c
 * @brief Meta-test: una asercion que DEBE fallar. tt_summary() debe devolver 1.
 *        En CTest se marca WILL_FAIL TRUE. Usa macro tipada (vale en C99).
 */
#include "ctests.h"

static void t(void) { EXPECT_EQ_INT(1, 2); }

int main(void)
{
    tt_suite("meta/fail");
    tt_run("una asercion fallida -> exit 1", t);
    return tt_summary();
}
