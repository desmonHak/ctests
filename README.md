<div align="center">

# ctests

**Librería de tests unitarios e integración para C y C++**

[![CI](https://github.com/desmonHak/ctests/actions/workflows/ci.yml/badge.svg)](https://github.com/desmonHak/ctests/actions/workflows/ci.yml)
![C99](https://img.shields.io/badge/C-99-blue)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)
![Licencia](https://img.shields.io/badge/licencia-VMProject-green)

C99 · C++17 · Sin dependencias · Multiplataforma · Salida con color

</div>

---

`ctests` es una micro-librería de pruebas en un solo header
([`ctests.h`](ctests.h)). Te da suites, aserciones, fixtures, _skip_, _xfail_,
_soft assertions_, tests parametrizados y un resumen visual con barra de
progreso — todo sin dependencias externas, en C puro o en C++.

```text
✓ Aritmetica (7 tests | 1 failed) 0ms
    ✓ suma basica 0ms
    ✗ fallo intencionado (demo) 0ms
      - Expected: 5
      + Received: 4

  [████████████████████████████░░░░]  21/23  (91%)

 Test Files  2 failed | 5 passed (7)
     Tests  2 failed | 1 xfail | 2 skipped | 21 passed (26)
  Duration  0.4ms
```

## Características

- **Fácil de integrar**: un header + una implementación. Con CMake es un
  `target_link_libraries(... ctests::ctests)`; sin CMake, copias dos archivos.
- **Multi-archivo**: incluye `ctests.h` en cuantos archivos de test quieras; el
  estado es compartido. Tus tests no tienen que vivir en un único `.c`.
- **C y C++**: la misma API funciona en C99 y C++17. En C++ se añaden
  `EXPECT_THROW` / `EXPECT_NO_THROW`.
- **Aserciones expresivas y simples**: un solo `EXPECT_EQ(a, b)` para enteros,
  cadenas, flotantes y punteros (C11/C++ vía `_Generic`/plantillas); más orden,
  memoria, rango, booleanos y mensajes personalizados.
- **Soft assertions**: acumulan todos los fallos de un test en vez de parar en el primero.
- **Fixtures**: `setup`/`teardown` por test y hooks `once` por suite.
- **Skip y xfail**: marca tests pendientes o fallos conocidos sin romper la build.
- **Tests parametrizados** (data-driven) con `tt_run_param`, y **auto-registro**
  opcional con `TEST(suite, nombre) { ... }` (sin enumerarlos en `main`).
- **Robustez**: un crash en un test (segfault, etc.) se reporta y la suite
  continúa; `timeout` por test en POSIX; captura de `stdout`/`stderr`.
- **Mensajes con `archivo:línea`** clicables en el editor.
- **Pensada para CI**: salida JUnit XML y TAP, código de salida correcto, filtrado
  y opciones por línea de comandos (`--filter`, `--junit`, `--tap`, `--no-color`…).
- **Salida robusta**: color con auto-detección de terminal (respeta `NO_COLOR`),
  VT100/UTF-8 activados solos en la consola de Windows.

## Inicio rápido

```c
#include "ctests.h"

static int sumar(int a, int b) { return a + b; }

static void test_suma(void) {
    EXPECT_EQ(sumar(2, 3), 5);   // un solo EXPECT_EQ para todos los tipos (C11/C++)
}

int main(void) {
    tt_suite("sumar");
        tt_run("2 + 3 == 5", test_suma);
    return tt_summary();
}
```

La implementación vive en `ctests.c`. Tienes dos formas de compilar:

```bash
# Opción 1 — añade ctests.c a tu build (recomendado)
gcc -std=c99   mis_tests.c ctests.c -o mis_tests -lm && ./mis_tests
g++ -std=c++17 mis_tests.cpp ctests.c -o mis_tests -lm && ./mis_tests

# Opción 2 — "single header": define CTESTS_IMPLEMENTATION en UN archivo
#   #define CTESTS_IMPLEMENTATION
#   #include "ctests.h"
gcc -std=c99 mis_tests.c -o mis_tests -lm && ./mis_tests
```

> En sistemas tipo Unix añade `-lm` (la macro `EXPECT_NEAR` usa `fabs`).
> En Windows/MinGW no es necesario. CMake lo gestiona automáticamente.

## Integración con CMake

`ctests` expone el target `ctests::ctests`. La vía recomendada es **FetchContent**:

```cmake
include(FetchContent)
FetchContent_Declare(ctests
    GIT_REPOSITORY https://github.com/desmonHak/ctests.git
    GIT_TAG        v1.0.0)
FetchContent_MakeAvailable(ctests)

add_executable(mis_tests mis_tests.c)     # o varios archivos .c/.cpp
target_link_libraries(mis_tests PRIVATE ctests::ctests)

enable_testing()
add_test(NAME mis_tests COMMAND mis_tests)
```

Incluye `ctests.h` en tus archivos de test; CMake compila la implementación por
ti (no necesitas `CTESTS_IMPLEMENTATION`). También puedes usar `add_subdirectory()`
o `find_package(ctests)` tras instalar. Detalles en **[doc/instalacion.md](doc/instalacion.md)**.

## Documentación

| Documento | Contenido |
|-----------|-----------|
| [doc/instalacion.md](doc/instalacion.md)      | Formas de integrar ctests (FetchContent, add_subdirectory, find_package, copia manual, single-header) |
| [doc/guia-de-uso.md](doc/guia-de-uso.md)      | Tutorial: suites, fixtures, skip, xfail, soft, parametrizados, multi-archivo, CLI, CI |
| [doc/referencia-api.md](doc/referencia-api.md)| Cada función pública (`tt_*`) documentada |
| [doc/aserciones.md](doc/aserciones.md)        | Cada macro `EXPECT_*` / `SOFT_EXPECT_*` / `ASSERT_*` con ejemplos |
| [doc/configuracion.md](doc/configuracion.md)  | Macros de compilación (`TT_MAX_TESTS`, `TT_MSG_MAX`, …) |
| [doc/ctgen.md](doc/ctgen.md)                  | Generación de tests desde anotaciones `@tag` con la herramienta `ctgen` |

## Ejemplos

En [`example/`](example/):

| Archivo | Descripción |
|---------|-------------|
| [quickstart.c](example/quickstart.c)              | El ejemplo mínimo posible |
| [example_test.c](example/example_test.c)          | Recorrido completo en C99 (suites, fixtures, skip, xfail, soft) |
| [example_test.cpp](example/example_test.cpp)      | Recorrido en C++17 incluyendo aserciones de excepciones |
| [advanced.c](example/advanced.c)                  | Tests parametrizados, hooks `once`, aserciones nuevas, CLI |
| [autoregister.c](example/autoregister.c)          | Macro `TEST()` (auto-registro) + captura de `stdout` |
| [multifile_*.c](example/)                         | Tests repartidos en varios archivos que comparten estado |
| [annotated.c](example/annotated.c)                | Fuente anotada con `@tag`; los tests los genera `ctgen` |

Tienes dos formas de compilarlos y probarlos:

```bash
# Opción A — Makefile (rápido, sin configurar CMake)
make            # compila los ejemplos en bin/
make test       # los ejecuta y verifica su código de salida
make run        # ejecuta el ejemplo completo (salida con color)

# Opción B — CMake + CTest (también valida instalación/consumo)
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

> El `Makefile` funciona en Windows (`mingw32-make`) y en Linux/macOS (`make`).
> En Windows usa `mingw32-make` en lugar de `make`.

## Línea de comandos

El binario de tests acepta opciones si llamas a `tt_parse_args(argc, argv)`:

```bash
./mis_tests --filter Aritmetica   # solo suites/tests que contengan "Aritmetica"
./mis_tests --junit results.xml   # genera informe JUnit XML para CI
./mis_tests --tap results.tap     # genera informe TAP
./mis_tests --no-color            # desactiva el color
./mis_tests --no-catch            # no captura crashes (útil al depurar)
./mis_tests --timeout 5           # límite de 5 s por test (solo POSIX)
./mis_tests --verbose 2           # nivel de detalle
./mis_tests --help
```

## Generación de tests desde anotaciones (ctgen)

Para funciones puras puedes **anotar** la función y dejar que la herramienta
`ctgen` genere los tests y un ejecutable real:

```c
/**
 * @suite Aritmetica
 * @eq_int suma(2, 3) => 5
 * @test   suma(0, 0) == 0
 */
static int suma(int a, int b) { return a + b; }
```

```bash
gcc -O2 tools/ctgen.c -o ctgen
./ctgen --ctests . mate.c -o mate_tests --run     # un archivo
./ctgen --ctests . -r src -o tests --run          # toda una carpeta (recursivo)
```

`ctgen` acepta archivos **o carpetas**: procesa los `.c`/`.cpp` anotados que
encuentre y los combina en un único ejecutable (con `make gen` se hace sobre
`example/`). Soporta tests complejos: `@let`/`@cleanup`, `@body … @endbody` y
fixtures de suite. Detalles, tags y la integración CMake/Make en
**[doc/ctgen.md](doc/ctgen.md)**.

## Requisitos

- Compilador C99 (gcc, clang, MSVC) o C++17.
- CMake ≥ 3.14 (opcional, solo para la integración/ejemplos).
- Terminal con soporte ANSI para el color (en Windows 10+ se activa solo).

## Licencia

Copyright © 2026 David López.T (**DesmonHak**) — Castilla y León, ES.
Licencia **VMProject**: uso libre **no comercial** con atribución obligatoria;
prohibido el lucro sin permiso escrito del autor. Ver [LICENSE.md](LICENSE.md).
