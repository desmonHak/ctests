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
 * @file example_test.c
 * @brief Ejemplo completo de ctests.h en C puro (C99).
 *
 * Demuestra: suites, skip, xfail, setup/teardown, soft-assertions,
 *            verbose mode y todas las macros EXPECT_*.
 *
 * El header vive en la raiz del repo; al compilar desde example/ se indica
 * con -I.. (o deja que CMake configure el include path automaticamente).
 * @code
 *   gcc -std=c99 -I.. example/example_test.c -o example_test -lm && ./example_test
 * @endcode
 */
#include "ctests.h"
#include <stdlib.h>
#include <string.h>

/* =============================================================================
 * Codigo de ejemplo a testear
 * ============================================================================= */

typedef struct { char codigo[64]; char valor[256]; } PreguntaTexto;
typedef struct { char texto[256]; int codigo; } Respuesta;

static Respuesta *obtener_respuesta(const PreguntaTexto *p) {
    Respuesta *r;
    if (p->valor[0] == '\0') return NULL;
    r = (Respuesta *)malloc(sizeof(Respuesta));
    strncpy(r->texto, p->valor, sizeof(r->texto) - 1);
    r->texto[sizeof(r->texto)-1] = '\0';
    r->codigo = 1;
    return r;
}

static int dividir(int a, int b, int *err) {
    if (b == 0) { *err = 1; return 0; }
    *err = 0;
    return a / b;
}

static int vector_suma(const int *v, int n) {
    int i, s = 0;
    for (i = 0; i < n; i++) s += v[i];
    return s;
}

/* =============================================================================
 * Fixture global — ejemplo de setup/teardown
 * ============================================================================= */

static Respuesta *_fixture_resp = NULL;

static void fixture_setup(void) {
    PreguntaTexto p = {"t_fixture", "datos de prueba"};
    _fixture_resp = obtener_respuesta(&p);
}

static void fixture_teardown(void) {
    free(_fixture_resp);
    _fixture_resp = NULL;
}

/* =============================================================================
 * Suite: PreguntaTexto
 * ============================================================================= */

static void test_vacio_null(void) {
    PreguntaTexto p = {"t1", ""};
    EXPECT_NULL(obtener_respuesta(&p));
}

static void test_relleno_respuesta(void) {
    PreguntaTexto p = {"t2", "hola mundo"};
    Respuesta *r = obtener_respuesta(&p);
    EXPECT_NOT_NULL(r);
    EXPECT_EQ_STR(r->texto, "hola mundo");
    EXPECT_EQ_INT(r->codigo, 1);
    free(r);
}

static void test_contiene_texto(void) {
    PreguntaTexto p = {"t3", "el gato y el perro"};
    Respuesta *r = obtener_respuesta(&p);
    EXPECT_NOT_NULL(r);
    EXPECT_CONTAINS(r->texto, "gato");
    EXPECT_STARTS_WITH(r->texto, "el gato");
    free(r);
}

/* =============================================================================
 * Suite: Aritmetica
 * ============================================================================= */

static void test_suma_basica(void) {
    EXPECT_EQ_INT(1 + 1, 2);
}

static void test_division_ok(void) {
    int err;
    EXPECT_EQ_INT(dividir(10, 2, &err), 5);
    EXPECT_EQ_INT(err, 0);
}

static void test_division_cero(void) {
    int err;
    dividir(1, 0, &err);
    EXPECT_EQ_INT(err, 1);
}

static void test_flotante(void) {
    /* 0.1 + 0.2 no es exactamente 0.3 en IEEE 754: necesita tolerancia */
    EXPECT_NEAR(0.1 + 0.2, 0.3, 1e-9);
}

static void test_orden(void) {
    EXPECT_GT(10, 5);
    EXPECT_GE(7,  7);
    EXPECT_LT(3,  4);
    EXPECT_LE(5,  5);
}

static void test_rango(void) {
    int x = 42;
    EXPECT_GE(x, 0);
    EXPECT_LE(x, 100);
}

/* TEST QUE FALLA A PROPOSITO — muestra formato de error */
static void test_fallo_demo(void) {
    EXPECT_EQ_INT(2 + 2, 5);   /* 4 != 5 — falla aqui */
}

/* =============================================================================
 * Suite: Arrays
 * ============================================================================= */

static void test_suma_vector(void) {
    int v[] = {1, 2, 3, 4, 5};
    EXPECT_EQ_INT(vector_suma(v, 5), 15);
}

static void test_vector_un_elem(void) {
    int v[] = {99};
    EXPECT_EQ_INT(vector_suma(v, 1), 99);
}

static void test_vector_vacio(void) {
    EXPECT_EQ_INT(vector_suma(NULL, 0), 0);
}

/* =============================================================================
 * Suite: Strings
 * ============================================================================= */

static void test_str_igual(void)    { EXPECT_EQ_STR("hola", "hola"); }
static void test_str_distinta(void) { EXPECT_NEQ_STR("hola", "mundo"); }
static void test_str_contiene(void) { EXPECT_CONTAINS("the quick brown fox", "brown"); }
static void test_str_starts(void)   { EXPECT_STARTS_WITH("hello world", "hello"); }

/* =============================================================================
 * Suite: Booleanos
 * ============================================================================= */

static void test_verdadero(void) { EXPECT_TRUE(1 == 1); }
static void test_falso(void)     { EXPECT_FALSE(1 == 2); }

/* =============================================================================
 * Suite: Nuevas capacidades
 * ============================================================================= */

/* -- skip desde dentro del test -- */
static void test_funcionalidad_futura(void) {
    tt_skip("aun no implementado — ticket #42");
    /* nada de lo que hay aqui se ejecuta */
    EXPECT_EQ_INT(1, 2);
}

/* -- xfail: test esperado a fallar (bug conocido) -- */
static void test_bug_conocido(void) {
    /* Simula un calculo incorrecto que sabemos que esta roto */
    int resultado = 0 / 1;  /* deberia ser 0 pero supongamos bug */
    EXPECT_EQ_INT(resultado, 999);   /* falla como se espera */
}

/* -- Soft assertions: acumulan todos los fallos -- */
static void test_multiple_campos(void) {
    PreguntaTexto p = {"t4", "respuesta valida"};
    Respuesta *r = obtener_respuesta(&p);
    EXPECT_NOT_NULL(r);  /* hard: si es NULL paramos aqui */

    /* Soft: comprueba varios campos y reporta todos los que fallen */
    SOFT_EXPECT_EQ_INT(r->codigo, 1);
    SOFT_EXPECT_EQ_STR(r->texto, "respuesta valida");
    SOFT_EXPECT_GT(strlen(r->texto), 0);
    SOFT_EXPECT_TRUE(r->texto[0] != '\0');

    /* Esta soft-assertion falla a proposito para mostrar el mecanismo */
    SOFT_EXPECT_EQ_INT(r->codigo, 99);  /* 1 != 99 */
    SOFT_EXPECT_EQ_STR(r->texto, "otro texto");  /* strings distintos */

    free(r);
    /* Al salir la funcion, ctests detecta los soft-failures y falla el test
       con TODOS los mensajes combinados, no solo el primero. */
}

/* -- Setup/teardown: usa el fixture global -- */
static void test_fixture_no_null(void) {
    EXPECT_NOT_NULL(_fixture_resp);
}

static void test_fixture_codigo(void) {
    EXPECT_EQ_INT(_fixture_resp->codigo, 1);
}

static void test_fixture_texto(void) {
    EXPECT_EQ_STR(_fixture_resp->texto, "datos de prueba");
}

/* =============================================================================
 * MAIN
 * ============================================================================= */

int main(void) {

    /* tt_verbose(0): silencioso — solo fallos
     * tt_verbose(1): normal (defecto)
     * tt_verbose(2): detallado + slowest/fastest */
    tt_verbose(2);

    /* --- Suite basica --- */
    tt_suite("PreguntaTexto");
        tt_run("valor vacio devuelve NULL",        test_vacio_null);
        tt_run("valor relleno devuelve Respuesta", test_relleno_respuesta);
        tt_run("respuesta contiene y empieza con", test_contiene_texto);

    /* --- Aritmetica --- */
    tt_suite("Aritmetica");
        tt_run("suma basica",                      test_suma_basica);
        tt_run("division entera correcta",         test_division_ok);
        tt_run("division por cero reporta error",  test_division_cero);
        tt_run("flotante con tolerancia",          test_flotante);
        tt_run("comparaciones de orden",           test_orden);
        tt_run("rango valido 0-100",               test_rango);
        tt_run("fallo intencionado (demo)",        test_fallo_demo);

    /* --- Arrays --- */
    tt_suite("Arrays");
        tt_run("suma vector [1..5]",               test_suma_vector);
        tt_run("vector de un elemento",            test_vector_un_elem);
        tt_run("vector vacio devuelve 0",          test_vector_vacio);

    /* --- Strings --- */
    tt_suite("Strings");
        tt_run("igualdad de cadenas",              test_str_igual);
        tt_run("desigualdad de cadenas",           test_str_distinta);
        tt_run("cadena contiene subcadena",        test_str_contiene);
        tt_run("cadena empieza con prefijo",       test_str_starts);

    /* --- Booleanos --- */
    tt_suite("Booleanos");
        tt_run("condicion verdadera",              test_verdadero);
        tt_run("condicion falsa",                  test_falso);

    /* --- Nuevas capacidades --- */
    tt_suite("Nuevas capacidades");

        /* skip: test marcado como a saltear sin ejecutar */
        tt_skip_test("modulo exportacion CSV", "pendiente de implementar");

        /* skip desde dentro */
        tt_run("funcionalidad futura (skip interno)", test_funcionalidad_futura);

        /* xfail: se espera que falle */
        tt_xfail("bug conocido en calculo (#42)", "bug #42 sin corregir", test_bug_conocido);

        /* soft assertions: acumula multiples fallos */
        tt_run("validacion multiple campos (soft)", test_multiple_campos);

    /* --- Setup / Teardown por suite --- */
    tt_suite("Fixture con setup/teardown");
        tt_suite_hooks(fixture_setup, fixture_teardown);
        tt_run("fixture no es NULL",       test_fixture_no_null);
        tt_run("fixture codigo == 1",      test_fixture_codigo);
        tt_run("fixture texto correcto",   test_fixture_texto);

    return tt_summary();
}
