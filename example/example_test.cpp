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
 * @file example_test.cpp
 * @brief Ejemplo de ctests.h en C++17 usando la API real del header.
 *
 * ctests es header-only y la misma API de macros (tt_suite, tt_run, EXPECT_*)
 * funciona en C y en C++. En C++ ademas se habilitan EXPECT_THROW y
 * EXPECT_NO_THROW para verificar excepciones.
 *
 * El header vive en la raiz del repo; compila desde example/ con -I.. y anade
 * ctests.c (la implementacion). Con CMake todo es automatico.
 * @code
 *   g++ -std=c++17 -I.. example/example_test.cpp ctests.c -o example_test -lm && ./example_test
 * @endcode
 */
#include "ctests.h"

#include <stdexcept>
#include <string>
#include <vector>

/* =============================================================================
 * Codigo de ejemplo a testear
 * ============================================================================= */

struct Respuesta {
    std::string texto;
    int         codigo = 0;
};

struct PreguntaTexto {
    std::string codigo;
    std::string valor;

    /* Devuelve nullptr si no hay valor; el llamador es dueno del puntero. */
    Respuesta *obtenerRespuesta() const {
        if (valor.empty()) return nullptr;
        return new Respuesta{valor, 1};
    }
};

static int dividir(int a, int b) {
    if (b == 0) throw std::runtime_error("division por cero");
    return a / b;
}

/* =============================================================================
 * Tests — cada uno es una funcion void(void), igual que en C
 * ============================================================================= */

static void test_valor_vacio_devuelve_null() {
    PreguntaTexto p{"t1", ""};
    EXPECT_NULL(p.obtenerRespuesta());
}

static void test_valor_relleno_devuelve_respuesta() {
    PreguntaTexto p{"t2", "hola"};
    Respuesta *r = p.obtenerRespuesta();
    EXPECT_NOT_NULL(r);
    EXPECT_EQ_STR(r->texto.c_str(), "hola");
    EXPECT_EQ_INT(r->codigo, 1);
    delete r;
}

static void test_division_entera() {
    EXPECT_EQ_INT(dividir(10, 2), 5);
}

static void test_division_por_cero_lanza() {
    EXPECT_THROW(dividir(1, 0), std::runtime_error);
}

static void test_operacion_segura_no_lanza() {
    EXPECT_NO_THROW(dividir(4, 2));
}

static void test_flotante_con_tolerancia() {
    EXPECT_NEAR(0.1 + 0.2, 0.3, 1e-9);
}

static void test_vector_tamano() {
    std::vector<int> v = {1, 2, 3};
    EXPECT_EQ_INT(static_cast<long long>(v.size()), 3);
}

static void test_string_contiene() {
    std::string s = "hello world";
    EXPECT_CONTAINS(s.c_str(), "world");
}

/* =============================================================================
 * MAIN
 * ============================================================================= */

int main() {
    tt_verbose(2);

    tt_suite("PreguntaTexto");
        tt_run("valor vacio devuelve null",        test_valor_vacio_devuelve_null);
        tt_run("valor relleno devuelve Respuesta", test_valor_relleno_devuelve_respuesta);

    tt_suite("Aritmetica");
        tt_run("division entera",                  test_division_entera);
        tt_run("division por cero lanza",          test_division_por_cero_lanza);
        tt_run("operacion segura no lanza",        test_operacion_segura_no_lanza);
        tt_run("flotante con tolerancia",          test_flotante_con_tolerancia);

    tt_suite("Contenedores");
        tt_run("vector tiene 3 elementos",         test_vector_tamano);
        tt_run("string contiene subcadena",        test_string_contiene);

    return tt_summary();
}
