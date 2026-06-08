/*
 * VestaVM - Máquina Virtual Distribuida
 * Copyright © 2026 David López.T (DesmonHak) (Castilla y León, ES) — Licencia VMProject
 */

/**
 * @file annotated.c
 * @brief Fuente ANOTADA: los tests se generan con la herramienta ctgen.
 *
 * Este archivo no incluye ctests.h ni tiene main: es codigo bajo prueba con
 * anotaciones (tags con prefijo arroba) en los comentarios. ctgen lo analiza y
 * genera un ejecutable de tests real. Ver doc/ctgen.md.
 *
 * Generar y ejecutar a mano:
 *   gcc -O2 tools/ctgen.c -o ctgen
 *   ./ctgen --ctests . example/annotated.c -o annotated_tests --run
 */
#include <string.h>

/**
 * Suma de dos enteros.
 * @suite Aritmetica
 * @eq   suma(2, 3)   => 5
 * @eq   suma(-1, 1)  => 0
 * @test suma(0, 0) == 0
 */
static int suma(int a, int b) { return a + b; }

/**
 * Factorial iterativo.
 * @suite Aritmetica
 * @eq factorial(0) => 1
 * @eq factorial(5) => 120
 */
static int factorial(int n)
{
    int r = 1, i;
    for (i = 2; i <= n; i++)
        r *= i;
    return r;
}

/**
 * Indica si una cadena empieza por un prefijo.
 * @suite Cadenas
 * @case  prefijo_presente
 * @true  empieza_por("https://x", "https://")
 * @false empieza_por("ftp://x", "https://")
 */
static int empieza_por(const char *s, const char *pre)
{
    return strncmp(s, pre, strlen(pre)) == 0;
}

/**
 * Devuelve un saludo fijo (para demostrar @contains/@notnull).
 * @suite Cadenas
 * @notnull  saludo()
 * @contains saludo() => "mundo"
 */
static const char *saludo(void) { return "Hola, mundo"; }
