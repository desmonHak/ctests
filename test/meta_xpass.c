/*
 * VestaVM - Máquina Virtual Distribuida
 * Copyright © 2026 David López.T (DesmonHak) (Castilla y León, ES) — Licencia VMProject
 */

/**
 * @file meta_xpass.c
 * @brief Meta-test: un xfail que PASA (xpass) debe contar como fallo -> exit 1.
 *        En CTest se marca WILL_FAIL TRUE.
 */
#include "ctests.h"

static void t(void) { EXPECT_TRUE(1 == 1); } /* pasa, pero se esperaba que fallara */

int main(void)
{
    tt_suite("meta/xpass");
    tt_xfail("xfail que pasa -> xpass -> exit 1", "deberia fallar pero pasa", t);
    return tt_summary();
}
