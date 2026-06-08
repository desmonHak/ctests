# Documentación de ctests

Índice de la documentación del proyecto.

| Documento | Para qué |
|-----------|----------|
| [instalacion.md](instalacion.md)       | Cómo añadir ctests a tu proyecto (CMake y manual) |
| [guia-de-uso.md](guia-de-uso.md)        | Tutorial de uso desde cero, concepto a concepto |
| [referencia-api.md](referencia-api.md)  | Referencia de todas las funciones públicas `tt_*` |
| [aserciones.md](aserciones.md)          | Referencia de todas las macros `EXPECT_*` y `SOFT_EXPECT_*` |
| [configuracion.md](configuracion.md)    | Macros de configuración en tiempo de compilación |

## Modelo mental en una frase

Escribes funciones `void test_xxx(void)` que contienen aserciones `EXPECT_*`;
las agrupas en **suites** con `tt_suite()` y las lanzas con `tt_run()`; al final
llamas a `tt_summary()`, que imprime el resumen y devuelve el código de salida.

```c
#include "ctests.h"

static void test_algo(void) { EXPECT_TRUE(1 + 1 == 2); }

int main(void) {
    tt_suite("mi grupo");
        tt_run("descripcion del caso", test_algo);
    return tt_summary();
}
```

## Cómo funciona por dentro

- Cada `EXPECT_*` que falla escribe un mensaje y hace `longjmp` de vuelta a
  `tt_run`, deteniendo el test en el primer fallo (*hard assertion*).
- Las `SOFT_EXPECT_*` no saltan: acumulan el mensaje y el test continúa; al
  terminar la función, si hubo algún fallo soft, el test se marca como fallido
  con **todos** los mensajes juntos.
- Los resultados se almacenan en un búfer por suite para poder imprimir la
  cabecera con los conteos exactos (`✓ Suite (N tests | M failed)`).
- No hay asignación dinámica ni hilos: todo el estado es estático. Por eso los
  tamaños máximos son [macros de configuración](configuracion.md).

> **Nota:** el corredor es de un solo hilo y usa estado global estático; no está
> pensado para ejecutar tests en paralelo dentro del mismo proceso.
