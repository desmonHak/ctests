# Instalación e integración

`ctests` es header-only: en esencia solo necesitas que el compilador encuentre
`ctests.h`. Hay cuatro formas de conseguirlo, de la más recomendable a la más simple.

> En sistemas tipo Unix/Linux el header usa `fabs()` de `<math.h>`, así que hay
> que enlazar la librería matemática con `-lm`. El target de CMake `ctests::ctests`
> ya lo añade por ti; si compilas a mano, no lo olvides. En Windows/MinGW y macOS
> normalmente no hace falta.

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

La opción más simple: copia `ctests.h` a tu proyecto y compílalo directamente.

```bash
cp ctests.h mi_proyecto/

# C
gcc -std=c99   mi_proyecto/mis_tests.c   -o mis_tests -lm
# C++
g++ -std=c++17 mi_proyecto/mis_tests.cpp -o mis_tests
```

Si el header está en otra carpeta, indica la ruta de include con `-I`:

```bash
gcc -std=c99 -Iextern/ctests mis_tests.c -o mis_tests -lm
```

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
