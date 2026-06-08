/*
 * VestaVM - Máquina Virtual Distribuida
 * Copyright © 2026 David López.T (DesmonHak) (Castilla y León, ES) — Licencia VMProject
 */

/**
 * @file multifile.h
 * @brief Declara las suites definidas en otros archivos del demo multi-archivo.
 *
 * Demuestra que ctests permite repartir los tests entre varias unidades de
 * traduccion: cada archivo incluye "ctests.h" y comparte el mismo estado.
 */
#ifndef MULTIFILE_DEMO_H
#define MULTIFILE_DEMO_H

void registrar_suite_aritmetica(void);   /* definida en multifile_aritmetica.c */
void registrar_suite_cadenas(void);      /* definida en multifile_cadenas.c   */

#endif
