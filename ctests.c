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
 * @file ctests.c
 * @brief Unica unidad de traduccion que compila la implementacion de ctests.
 *
 * Anade este archivo a tu build (o enlaza el target CMake ctests::ctests) e
 * incluye "ctests.h" en tus archivos de test. Asi no necesitas definir
 * CTESTS_IMPLEMENTATION tu mismo.
 */
/* Silencia los avisos C4996 de MSVC sobre strncpy/fopen/etc. */
#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#  define _CRT_SECURE_NO_WARNINGS 1
#endif

#define CTESTS_IMPLEMENTATION
#include "ctests.h"
