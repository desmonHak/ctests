# Configuración en tiempo de compilación

ctests no asigna memoria dinámica: todo su estado son buffers estáticos de tamaño
fijo. Esos tamaños se controlan con macros que puedes **redefinir antes de incluir
el header** o pasar al compilador con `-D`. Todas tienen un `#ifndef` de guarda, así
que solo necesitas definir las que quieras cambiar.

## Macros disponibles

| Macro | Por defecto | Qué controla |
|-------|:-----------:|--------------|
| `TT_MAX_TESTS`    | `512` | Máximo de tests por suite antes de hacer *flush*. Si una suite supera este número, los tests extra se ignoran. |
| `TT_MSG_MAX`      | `512` | Longitud máxima (bytes) del mensaje de error de una aserción. |
| `TT_NAME_MAX`     | `128` | Longitud máxima (bytes) del nombre de un test o suite. Los nombres más largos se truncan. |
| `TT_MAX_SOFT`     | `16`  | Máximo de *soft assertions* fallidas que se acumulan por test. Las que excedan se descartan. |
| `TT_MAX_FAILURES` | `256` | Máximo de fallos que se listan en la tabla resumen final. |

## Cómo redefinirlas

### Opción A — antes del `#include`

```c
#define TT_MAX_TESTS 2048
#define TT_NAME_MAX  256
#include "ctests.h"
```

### Opción B — desde la línea de compilación

```bash
gcc -std=c99 -DTT_MAX_TESTS=2048 -DTT_NAME_MAX=256 mis_tests.c -o mis_tests -lm
```

### Opción C — con CMake

```cmake
target_compile_definitions(mis_tests PRIVATE
    TT_MAX_TESTS=2048
    TT_NAME_MAX=256)
```

## Notas

- Son tamaños **por proceso/compilación**: afectan al consumo de memoria estática
  del binario de tests. Los valores por defecto son holgados para la mayoría de
  proyectos; súbelos solo si tienes suites muy grandes o nombres/mensajes largos.
- `TT_MAX_TESTS` es por **suite**, no global: como `tt_suite()` vacía el búfer al
  cambiar de grupo, el límite aplica a cada suite por separado.
- Como el header es *header-only* con estado estático, **inclúyelo en un único
  archivo de traducción** (normalmente el que tiene `main`). No lo incluyas en
  varios `.c`/`.cpp` que se enlacen juntos, o tendrás múltiples copias del estado.

## Personalización de aspecto

Los colores ANSI (`_TTC_*`) y los símbolos Unicode (`_TTS_*`) están definidos como
macros internas en el header. No forman parte de la API pública y pueden cambiar
entre versiones; si necesitas adaptarlos (por ejemplo, desactivar color en un
terminal sin soporte ANSI), edita el header directamente.

Ver también: [referencia-api.md](referencia-api.md) · [guia-de-uso.md](guia-de-uso.md).
