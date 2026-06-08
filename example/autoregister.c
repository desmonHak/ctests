/*
 * VestaVM - Máquina Virtual Distribuida
 * Copyright © 2026 David López.T (DesmonHak) (Castilla y León, ES) — Licencia VMProject
 */

/**
 * @file autoregister.c
 * @brief Auto-registro de tests con la macro TEST() y captura de salida.
 *
 * Con TEST(suite, nombre) no hace falta declarar funciones ni llamarlas en main:
 * cada test se registra solo al arrancar el programa. main() solo llama a
 * tt_run_all(). Funciona en GCC/Clang/MinGW (constructor) y MSVC (.CRT$XCU).
 *
 * @code
 *   gcc -std=c99 -I.. example/autoregister.c ctests.c -o autoregister -lm && ./autoregister
 * @endcode
 */
#include "ctests.h"
#include <stdio.h>

TEST(Aritmetica, suma) { EXPECT_EQ_INT(2 + 2, 4); }
TEST(Aritmetica, rango) { EXPECT_IN_RANGE(42, 0, 100); }
TEST(Aritmetica, flotante_relativo) { EXPECT_EQ_FLOAT(1e9 + 1.0, 1e9 + 1.0); }

TEST(Arrays, igualdad)
{
    int a[3] = {1, 2, 3}, b[3] = {1, 2, 3};
    EXPECT_ARRAY_EQ(a, b, 3);
}

/* Captura de stdout/stderr: testea lo que imprime una funcion. */
static void imprime_saludo(const char *quien) { printf("Hola, %s!\n", quien); }

TEST(Salida, captura)
{
    char buf[128];
    tt_capture_begin();
    imprime_saludo("mundo");
    tt_capture_end(buf, sizeof buf);
    EXPECT_CONTAINS(buf, "Hola, mundo!");
}

int main(int argc, char **argv)
{
    tt_parse_args(argc, argv); /* --filter, --tap, --junit, --no-catch, ... */
    return tt_run_all();       /* ejecuta todos los TEST() registrados */
}
