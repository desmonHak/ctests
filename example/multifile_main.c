/*
 * VestaVM - Máquina Virtual Distribuida
 * Copyright © 2026 David López.T (DesmonHak) (Castilla y León, ES) — Licencia VMProject
 */

/**
 * @file multifile_main.c
 * @brief Punto de entrada del demo multi-archivo: agrega suites de otros .c.
 *
 * Cada suite vive en su propio archivo (multifile_aritmetica.c,
 * multifile_cadenas.c) e incluye "ctests.h". El estado es compartido, asi que
 * basta con llamarlas en orden y cerrar con tt_summary().
 *
 * @code
 *   gcc -std=c99 -I.. example/multifile_main.c example/multifile_aritmetica.c \
 *       example/multifile_cadenas.c ctests.c -o multifile -lm && ./multifile
 * @endcode
 */
#include "ctests.h"
#include "multifile.h"

int main(int argc, char **argv) {
    tt_parse_args(argc, argv);

    registrar_suite_aritmetica();
    registrar_suite_cadenas();

    return tt_summary();
}
