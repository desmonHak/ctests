# Instalación e integración

`ctests` son dos archivos: `ctests.h` (API + macros) y `ctests.c` (la
implementación, que se compila una sola vez). Incluyes `ctests.h` en todos tus
archivos de test y compilas `ctests.c` una vez — o, si prefieres un único
archivo, usas el modo single-header (sección 4b). Hay cuatro formas de
integrarlo, de la más recomendable a la más simple.

> En sistemas tipo Unix/Linux la macro `EXPECT_NEAR` usa `fabs()` de `<math.h>`,
> así que hay que enlazar la librería matemática con `-lm`. El target de CMake
> `ctests::ctests` ya lo añade por ti; si compilas a mano, no lo olvides. En
> Windows/MinGW y macOS normalmente no hace falta.

---

## 1. FetchContent (recomendado)

No requiere instalar nada previamente: CMake clona ctests durante la configuración.

```cmake
cmake_minimum_required(VERSION 3.14)
project(mi_proyecto C)

include(FetchContent)
FetchContent_Declare(ctests
    GIT_REPOSITORY https://github.com/DesmonHak/ctests.git
    GIT_TAG        v1.0.0)          # fija una etiqueta/commit concreto
FetchContent_MakeAvailable(ctests)

add_executable(mis_tests mis_tests.c)
target_link_libraries(mis_tests PRIVATE ctests::ctests)

enable_testing()
add_test(NAME mis_tests COMMAND mis_tests)
```

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## 2. add_subdirectory (vendored / submódulo de git)

Si prefieres tener el código dentro de tu repositorio (por ejemplo como
submódulo de git en `extern/ctests`):

```bash
git submodule add https://github.com/DesmonHak/ctests.git extern/ctests
```

```cmake
add_subdirectory(extern/ctests)
target_link_libraries(mis_tests PRIVATE ctests::ctests)
```

Cuando ctests se añade como subproyecto, sus ejemplos **no** se compilan
(la opción `CTESTS_BUILD_EXAMPLES` queda en `OFF` automáticamente).

---

## 3. find_package (instalación en el sistema)

Primero instalas ctests una vez:

```bash
git clone https://github.com/DesmonHak/ctests.git
cd ctests
cmake -S . -B build
cmake --install build --prefix /ruta/de/instalacion
```

Esto copia `ctests.h` a `include/` y los archivos de paquete CMake a
`lib/cmake/ctests/`. Luego, desde cualquier proyecto:

```cmake
find_package(ctests 1.0 REQUIRED)
target_link_libraries(mis_tests PRIVATE ctests::ctests)
```

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/ruta/de/instalacion
```

(Si instalas en una ruta estándar del sistema, `CMAKE_PREFIX_PATH` no es necesario.)

---

## 4. Copia manual (sin CMake)

La librería son dos archivos: el header `ctests.h` (API + macros) y `ctests.c`
(la implementación, que se compila una sola vez). Tienes dos variantes:

### 4a. Header + implementación (recomendada)

Copia ambos archivos y añade `ctests.c` a tu build. Incluye `ctests.h` en
cuantos archivos de test quieras:

```bash
cp ctests.h ctests.c mi_proyecto/

# C
gcc -std=c99   mis_tests.c ctests.c -o mis_tests -lm
# C++
g++ -std=c++17 mis_tests.cpp ctests.c -o mis_tests -lm
```

### 4b. Single-header (solo `ctests.h`)

Si prefieres llevar un único archivo, copia solo `ctests.h` y define
`CTESTS_IMPLEMENTATION` en **exactamente uno** de tus `.c`/`.cpp` antes de incluirlo:

```c
/* en UN solo archivo de todo tu proyecto de tests */
#define CTESTS_IMPLEMENTATION
#include "ctests.h"
```

```bash
gcc -std=c99 mis_tests.c -o mis_tests -lm
```

En el resto de archivos de test incluye `ctests.h` con normalidad (sin la macro).
Define la macro en más de un archivo que se enlace junto → símbolos duplicados.

> Si los archivos están en otra carpeta, indica la ruta de include con `-I`:
> `gcc -std=c99 -Iextern/ctests mis_tests.c extern/ctests/ctests.c -o mis_tests -lm`

---

## Opciones de CMake

| Opción | Por defecto | Descripción |
|--------|-------------|-------------|
| `CTESTS_BUILD_EXAMPLES` | `ON` si ctests es el proyecto raíz, `OFF` como subproyecto | Compila y registra los ejemplos de [`example/`](../example) como tests de CTest |

```bash
cmake -S . -B build -DCTESTS_BUILD_EXAMPLES=OFF
```

## Versionado

El paquete declara compatibilidad `SameMajorVersion`: `find_package(ctests 1.0)`
acepta cualquier `1.x`, pero no `2.x`. Fija siempre un `GIT_TAG`/versión concreta
en producción.
