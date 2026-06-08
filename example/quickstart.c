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
 * @file quickstart.c
 * @brief El ejemplo minimo posible con ctests.h.
 *
 * @code
 *   gcc -std=c99 -I.. example/quickstart.c -o quickstart -lm && ./quickstart
 * @endcode
 */
#include "ctests.h"

static int sumar(int a, int b) { return a + b; }

static void test_suma(void) {
    EXPECT_EQ_INT(sumar(2, 3), 5);
}

static void test_suma_negativos(void) {
    EXPECT_EQ_INT(sumar(-2, -3), -5);
}

int main(void) {
    tt_suite("sumar");
        tt_run("2 + 3 == 5",    test_suma);
        tt_run("-2 + -3 == -5", test_suma_negativos);
    return tt_summary();   /* devuelve 0 si todo paso, 1 si hubo fallos */
}
