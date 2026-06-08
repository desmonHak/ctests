# ctgen — generación de tests desde anotaciones

`ctgen` es una herramienta (un solo `.c`, sin dependencias) que lee **anotaciones
en los comentarios** de tus archivos fuente y genera tests de ctests, produciendo
un **ejecutable real**. Es ideal para funciones puras: el test vive pegado a la
función que documenta y no se desincroniza.

> No sustituye a los tests escritos a mano: para fixtures, estado o efectos
> secundarios sigue usando la API normal. ctgen es *aditivo*.

## Cómo funciona

1. Anotas tus funciones con tags `@...` en comentarios de bloque (`/** ... */`).
2. `ctgen` analiza el archivo, extrae las anotaciones y el nombre de la función,
   y emite un `<archivo>.gen.c` que hace `#include` de tu fuente (así ve también
   las funciones `static`) y registra cada test con `TT_REGISTER`.
3. `ctgen` compila ese `.gen.c` (+ un `main` que llama a `tt_run_all()` + la
   implementación de ctests) en un ejecutable.

## Compilar la herramienta

```bash
gcc -std=c99 -O2 tools/ctgen.c -o ctgen      # en Windows: ctgen.exe
```

(También se compila sola con CMake — target `ctgen` — y con `make gen`.)

## Uso

```bash
# Generar y ejecutar en un paso:
./ctgen --ctests . example/annotated.c -o annotated_tests --run

# Solo generar los .gen.c (para integrarlos en tu propio build):
./ctgen --gen-only --emit-runner --outdir build/gen --ctests . src/mate.c
```

| Opción | Significado |
|--------|-------------|
| `-o <exe>` | Compila un ejecutable de tests (activa la compilación) |
| `--gen-only` | Solo genera los `.gen.c` (no compila) |
| `--emit-runner` | Emite también el `main` (`_ctgen_runner.gen.c`) en modo gen-only |
| `--outdir <dir>` | Carpeta de salida de los `.gen.c` (por defecto, junto a la fuente) |
| `--cc <cc>` | Compilador (por defecto `$CC`, o `g++` si hay `.cpp`, si no `gcc`) |
| `--ctests <dir>` | Carpeta con `ctests.h` y `ctests.c` (por defecto `.`) |
| `--keep` | No borra los `.gen.c` tras compilar |
| `--run` | Ejecuta el binario tras compilar |
| `-I<dir>` / `-D<macro>` | Se reenvían al compilador |

Puedes pasar **varias fuentes**; se combinan en un único ejecutable (cada `.gen.c`
incluye su propia fuente, así no hay choques entre funciones `static`).

## Anotaciones

Van en comentarios de bloque encima de la función. Cada tag de aserción genera
una llamada `EXPECT_*`. El separador `=>` divide la llamada del valor esperado
(así las comas de los argumentos no estorban).

### Control

| Tag | Efecto |
|-----|--------|
| `@suite <texto>` | Fija la suite para los bloques siguientes (texto libre) |
| `@case <texto>` | Nombre del test de este bloque (si no, el de la función) |
| `@skip <razón>` | El test se marca como saltado |

### Aserciones

| Tag | Genera |
|-----|--------|
| `@eq <call> => <v>` | `EXPECT_EQ(call, v)` — unificado: int/uint/str/float/ptr |
| `@ne <call> => <v>` | `EXPECT_NE(call, v)` |
| `@test <expr>` / `@true <expr>` | `EXPECT_TRUE(expr)` |
| `@false <expr>` | `EXPECT_FALSE(expr)` |
| `@null <expr>` / `@notnull <expr>` | `EXPECT_NULL` / `EXPECT_NOT_NULL` |
| `@contains <call> => "<s>"` | `EXPECT_CONTAINS(call, "s")` |
| `@near <call> => <v> +- <eps>` | `EXPECT_NEAR(call, v, eps)` |
| `@eq_int / @eq_uint / @eq_str / @eq_ptr <call> => <v>` | macros tipadas (explícitas) |

> `@eq`/`@ne` generan `EXPECT_EQ`/`EXPECT_NE`, que requieren C11 o C++ (el código
> generado se compila con el estándar por defecto del compilador, que ya es C11+).

Cada **bloque de comentario** con ≥1 aserción produce **un test**. Usa `@case`
para nombrarlo o agrupar varias aserciones bajo un nombre.

## Ejemplo

```c
/* mate.c — sin main, sin incluir ctests.h */
#include <string.h>

/**
 * @suite Aritmetica
 * @eq   suma(2, 3)  => 5
 * @eq   suma(-1, 1) => 0
 * @test suma(0, 0) == 0
 */
static int suma(int a, int b) { return a + b; }

/**
 * @suite Cadenas
 * @case  prefijo
 * @true  empieza_por("https://x", "https://")
 * @false empieza_por("ftp://x", "https://")
 */
static int empieza_por(const char *s, const char *p) { return strncmp(s, p, strlen(p)) == 0; }
```

```bash
./ctgen --ctests . mate.c -o mate_tests --run
```

```text
✓ Aritmetica (1 tests) 0ms
✓ Cadenas (1 tests) 0ms
     Tests  2 passed (2)
```

Ver la fuente real en [example/annotated.c](../example/annotated.c).

## Integración con CMake

ctgen se compila como target y genera tests que se registran en CTest:

```cmake
add_executable(ctgen tools/ctgen.c)

add_custom_command(
    OUTPUT  ${GEN}/mate.gen.c ${GEN}/_ctgen_runner.gen.c
    COMMAND ctgen --gen-only --emit-runner --outdir ${GEN}
            --ctests ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/mate.c
    DEPENDS ctgen mate.c VERBATIM)

add_executable(mate_tests ${GEN}/mate.gen.c ${GEN}/_ctgen_runner.gen.c)
target_link_libraries(mate_tests PRIVATE ctests::ctests)
add_test(NAME mate COMMAND mate_tests)
```

(Ver [example/CMakeLists.txt](../example/CMakeLists.txt) para el caso real.)
Con Make: `make gen` compila ctgen y genera+ejecuta los tests de `annotated.c`.

## Compatibilidad con Doxygen

Los comentarios de la **librería** (`ctests.h`) son Doxygen estándar y generan
documentación sin warnings. Las **anotaciones de ctgen** (`@eq`, `@suite`, …)
usan el prefijo `@`, igual que los comandos de Doxygen, así que por defecto
Doxygen avisaría `Found unknown command`. Para evitarlo, el [`Doxyfile`](../Doxyfile)
del proyecto las declara como `ALIASES`, de modo que Doxygen:

- no emite warnings, y
- las **renderiza** como casos de prueba en la documentación de cada función.

```doxyfile
ALIASES  = "eq=@li <b>EXPECT_EQ</b>"
ALIASES += "near=@li <b>EXPECT_NEAR</b>"
ALIASES += "suite=@par Suite de tests:^^"
# ... (uno por tag; ver Doxyfile)
```

Genera la doc con `doxygen Doxyfile` desde la raíz (salida en
`doc/doxygen_doc/html/`). Nota: `@test` es un comando propio de Doxygen (lista de
tests), por eso no se le pone alias.

## Limitaciones (por diseño)

- El generador **no entiende tipos de C**: por eso el tag elige la aserción
  (`@eq_int`, `@eq_str`, …). El `@test` universal sirve para cualquier expresión.
- Las expresiones se emiten **verbatim**: deben compilar en el `.gen.c`.
- El escáner de comentarios es textual; un `/*` dentro de una cadena podría
  confundirlo (raro en cabeceras anotadas).
- La fuente anotada **no debe tener `main()`** (ctgen aporta el suyo).
- El auto-registro usa constructores: GCC/Clang/MinGW probados; MSVC vía
  `.CRT$XCU` (mismo mecanismo que [`TEST()`](referencia-api.md), no ejercitado en CI).

Ver también: [referencia-api.md](referencia-api.md) · [aserciones.md](aserciones.md).
