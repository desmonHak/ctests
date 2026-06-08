<div align="center">

# ctests

**Librería _header-only_ de tests unitarios e integración para C y C++**

C99 · C++17 · Sin dependencias · Un solo archivo · Salida con color

</div>

---

`ctests` es una micro-librería de pruebas contenida en un único header
([`ctests.h`](ctests.h)). Copias el archivo —o lo enlazas con CMake— y ya tienes
suites, aserciones, fixtures, _skip_, _xfail_, _soft assertions_ y un resumen
visual con barra de progreso, todo sin librerías externas.

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

- **Header-only**: un solo `#include "ctests.h"`, sin compilar ni enlazar nada extra.
- **C y C++**: la misma API funciona en C99 y C++17. En C++ se añaden `EXPECT_THROW` / `EXPECT_NO_THROW`.
- **Aserciones expresivas**: enteros, flotantes (con tolerancia), cadenas, punteros, orden y booleanos.
- **Soft assertions**: acumulan todos los fallos de un test en vez de parar en el primero.
- **Fixtures**: `setup`/`teardown` por suite.
- **Skip y xfail**: marca tests pendientes o fallos conocidos sin romper la build.
- **Salida rica**: barra de progreso _live_, colores ANSI, símbolos Unicode, tests más lento/rápido.
- **Código de salida**: `tt_summary()` devuelve `0` si todo pasó y `1` si hubo fallos — listo para CI.

## Inicio rápido

```c
#include "ctests.h"

static int sumar(int a, int b) { return a + b; }

static void test_suma(void) {
    EXPECT_EQ_INT(sumar(2, 3), 5);
}

int main(void) {
    tt_suite("sumar");
        tt_run("2 + 3 == 5", test_suma);
    return tt_summary();
}
```

```bash
# C
gcc -std=c99   misuite.c   -o misuite -lm && ./misuite
# C++
g++ -std=c++17 misuite.cpp -o misuite      && ./misuite
```

> En sistemas tipo Unix añade `-lm` (el header usa `fabs` de `<math.h>`).
> En Windows/MinGW no es necesario. CMake lo gestiona automáticamente.

## Integración con CMake

`ctests` expone el target `ctests::ctests`. La vía recomendada es **FetchContent**:

```cmake
include(FetchContent)
FetchContent_Declare(ctests
    GIT_REPOSITORY https://github.com/DesmonHak/ctests.git
    GIT_TAG        v1.0.0)
FetchContent_MakeAvailable(ctests)

add_executable(mis_tests mis_tests.c)
target_link_libraries(mis_tests PRIVATE ctests::ctests)

enable_testing()
add_test(NAME mis_tests COMMAND mis_tests)
```

También puedes usar `add_subdirectory()` (vendored/submódulo) o `find_package(ctests)`
tras instalar. Todos los detalles en **[doc/instalacion.md](doc/instalacion.md)**.

## Documentación

| Documento | Contenido |
|-----------|-----------|
| [doc/instalacion.md](doc/instalacion.md)      | Las cuatro formas de integrar ctests (FetchContent, add_subdirectory, find_package, copia manual) |
| [doc/guia-de-uso.md](doc/guia-de-uso.md)      | Tutorial paso a paso: suites, fixtures, skip, xfail, soft assertions, verbosidad |
| [doc/referencia-api.md](doc/referencia-api.md)| Cada función pública (`tt_*`) documentada |
| [doc/aserciones.md](doc/aserciones.md)        | Cada macro `EXPECT_*` / `SOFT_EXPECT_*` con ejemplos |
| [doc/configuracion.md](doc/configuracion.md)  | Macros de compilación (`TT_MAX_TESTS`, `TT_MSG_MAX`, …) |

## Ejemplos

En [`example/`](example/):

| Archivo | Descripción |
|---------|-------------|
| [quickstart.c](example/quickstart.c)            | El ejemplo mínimo posible |
| [example_test.c](example/example_test.c)        | Recorrido completo en C99 (suites, fixtures, skip, xfail, soft) |
| [example_test.cpp](example/example_test.cpp)    | Recorrido en C++17 incluyendo aserciones de excepciones |

Compílalos y ejecútalos con CMake:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Requisitos

- Compilador C99 (gcc, clang, MSVC) o C++17.
- CMake ≥ 3.14 (opcional, solo para la integración/ejemplos).
- Una terminal con soporte ANSI para ver los colores (la mayoría; en Windows 10+ la consola moderna los soporta).

## Licencia

Copyright © 2026 David López.T (**DesmonHak**) — Castilla y León, ES.
Licencia **VMProject**: uso libre **no comercial** con atribución obligatoria;
prohibido el lucro sin permiso escrito del autor. Ver [LICENSE.md](LICENSE.md).
