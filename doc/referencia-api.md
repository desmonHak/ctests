# Referencia de la API

Todas las funciones públicas de ctests, con prefijo `tt_`. Están declaradas en
`ctests.h` y definidas en la implementación (`ctests.c`). Las macros de aserción
tienen su propia página: [aserciones.md](aserciones.md).

> Convención de nombres: lo que empieza por `tt_` es API pública; lo que empieza
> por `_tt_` / `_TT_` / `_TTC_` / `_TTS_` es interno — no lo uses directamente.

---

## Configuración

### `void tt_verbose(int v)`

Establece el nivel de verbosidad de la salida. Llámalo antes de ejecutar tests.

| `v` | Significado |
|-----|-------------|
| `0` | Silencioso — solo se imprimen los tests que fallan |
| `1` | Normal (valor por defecto) — se imprimen todos los tests |
| `2` | Detallado — añade las líneas *Slowest* / *Fastest* al resumen |

```c
tt_verbose(2);
```

### `void tt_color(int mode)`

Controla el uso de color ANSI en la salida.

| `mode` | Significado |
|--------|-------------|
| `-1` | Auto (por defecto): color solo si stdout es un terminal y `NO_COLOR` no está definida |
| `0`  | Forzar sin color |
| `1`  | Forzar con color |

```c
tt_color(0);   /* salida sin color, p. ej. para logs */
```

> En modo auto, ctests respeta la convención [`NO_COLOR`](https://no-color.org/):
> si esa variable de entorno existe, no emite color.

### `void tt_output_junit(const char *path)`

Activa la generación de un informe **JUnit XML** en `path`. Llámalo **antes** de
ejecutar los tests; el archivo se completa al llamar a `tt_summary()`. Lo
entienden GitHub Actions, GitLab CI, Jenkins, etc.

```c
tt_output_junit("results.xml");
```

### `void tt_parse_args(int argc, char **argv)`

Procesa los argumentos de la línea de comandos del binario de tests. Llámalo al
principio de `main()`.

| Opción | Efecto |
|--------|--------|
| `-f`, `--filter <texto>` | Ejecuta solo los tests cuyo `"suite > nombre"` contenga `<texto>` (sin distinguir mayúsculas) |
| `-v`, `--verbose <0\|1\|2>` | Equivale a `tt_verbose(n)` |
| `--junit <archivo>` | Equivale a `tt_output_junit(archivo)` |
| `--color` / `--no-color` | Fuerza color on/off |
| `-h`, `--help` | Muestra la ayuda y termina con `exit(0)` |

```c
int main(int argc, char **argv) {
    tt_parse_args(argc, argv);
    /* ... suites ... */
    return tt_summary();
}
```

---

## Suites

### `void tt_suite(const char *name)`

Inicia una nueva suite (grupo de tests). Hace *flush* automático de la suite
anterior (imprime su cabecera y resultados) antes de empezar la nueva. Además
**resetea** los hooks de setup/teardown: si la nueva suite los necesita, vuelve a
llamar a `tt_suite_hooks()`.

- `name` — nombre descriptivo del grupo (truncado a [`TT_NAME_MAX`](configuracion.md) bytes).

```c
tt_suite("Aritmetica");
```

### `void tt_suite_hooks(tt_fn setup_fn, tt_fn teardown_fn)`

Registra setup/teardown para la suite **actual**. Pasa `NULL` para desactivar
cualquiera de los dos.

- `setup_fn` — se ejecuta **antes** de cada test de la suite.
- `teardown_fn` — se ejecuta **después** de cada test, **incluso si el test falló**.

```c
tt_suite("Base de datos");
    tt_suite_hooks(setup, teardown);
```

> Debe llamarse **después** de `tt_suite()`, porque `tt_suite()` resetea los hooks.

### `void tt_suite_hooks_once(tt_fn before_all, tt_fn after_all)`

Registra hooks que corren **una sola vez** por suite (no por test). Pasa `NULL`
para desactivar cualquiera.

- `before_all` — se ejecuta antes del **primer** test de la suite.
- `after_all` — se ejecuta después del **último** test de la suite.

Ideal para tests de integración: arrancar/parar un servidor o una base de datos
una única vez por grupo. Llamar después de `tt_suite()`.

```c
tt_suite("Integracion");
    tt_suite_hooks_once(arrancar_servidor, parar_servidor);
    tt_run("ping", test_ping);
    tt_run("pong", test_pong);
```

> Combinables con `tt_suite_hooks()`: el orden por test es
> `before_all` (una vez) → `setup` → test → `teardown`, y `after_all` al cerrar la suite.

---

## Ejecutar tests

`tt_fn` es el tipo de una función de test: `typedef void (*tt_fn)(void);`

### `void tt_run(const char *name, tt_fn fn)`

Ejecuta `fn` como un caso de prueba, midiendo su duración y registrando el
resultado (pasa / falla / saltado).

- `name` — descripción del caso (truncada a `TT_NAME_MAX`).
- `fn` — la función de test.

Si una `EXPECT_*` falla dentro de `fn`, el test se detiene en ese punto y se marca
como fallido. Si hubo `SOFT_EXPECT_*` fallidas, el test falla al terminar con
todos los mensajes acumulados.

```c
tt_run("suma basica", test_suma_basica);
```

### `void tt_run_param(const char *name, tt_fn fn, const void *param)`

Ejecuta un test **data-driven**: fija la variable global `tt_param` con `param`
antes de llamar a `fn`, y la limpia al terminar. La función de test lee el dato
casteando `tt_param`.

- `name` — nombre del caso (incluye tú el índice/etiqueta si iteras una tabla).
- `fn` — función de test que lee `tt_param`.
- `param` — puntero al dato del caso.

```c
struct Caso { int a, b, esperado; };
static const struct Caso casos[] = {{1,1,2}, {2,3,5}};

static void test_suma(void) {
    const struct Caso *c = (const struct Caso *)tt_param;
    EXPECT_EQ_INT(c->a + c->b, c->esperado);
}

/* en main, dentro de una suite: */
size_t i;
for (i = 0; i < sizeof(casos)/sizeof(casos[0]); i++) {
    char nm[32];
    snprintf(nm, sizeof nm, "caso %u", (unsigned)i);
    tt_run_param(nm, test_suma, &casos[i]);
}
```

### `void tt_skip_test(const char *name, const char *reason)`

Registra un test como **saltado sin ejecutarlo**. Útil para funcionalidad aún no
implementada. No cuenta como fallo.

- `name` — nombre del test.
- `reason` — motivo del salto (puede ser `NULL`).

```c
tt_skip_test("exportar CSV", "pendiente de implementar");
```

### `void tt_xfail(const char *name, const char *reason, tt_fn fn)`

Ejecuta un test marcado como *expected failure* (se espera que falle, p. ej. por
un bug conocido).

- Si `fn` **falla** -> resultado `xfail`; **no** rompe la build.
- Si `fn` **pasa** -> resultado `xpass`; **sí** cuenta como fallo (señal de que hay
  que retirar el `xfail`).

```c
tt_xfail("calculo del IVA", "bug #42", test_iva);
```

---

## Control dentro de un test

### `tt_skip(reason)`  *(macro)*

Salta el test **actual** inmediatamente, desde dentro de su función. Nada después
de esta llamada se ejecuta. No cuenta como fallo.

- `reason` — cadena con el motivo.

```c
static void test_x(void) {
    if (!hay_red()) tt_skip("requiere conexion de red");
    EXPECT_TRUE(descargar());
}
```

---

## Resumen

### `int tt_summary(void)`

Hace *flush* de la última suite e imprime el resumen completo: tabla de fallos,
barra de progreso, conteos de *Test Files* / *Tests* / *Duration* y, en verbose ≥ 1,
*Slowest* / *Fastest*.

**Devuelve** `0` si todos los tests pasaron (los `xfail` no cuentan como fallo),
o `1` si hubo algún fallo o algún `xpass`. Propágalo desde `main`:

```c
int main(void) {
    /* ... tests ... */
    return tt_summary();
}
```

---

## Resumen de estados

| Estado | Símbolo | ¿Cuenta como fallo? | Cómo se produce |
|--------|:------:|:--------------------:|-----------------|
| pass   | ✓ | No  | el test terminó sin aserciones fallidas |
| fail   | ✗ | **Sí** | una `EXPECT_*` falló, o quedaron `SOFT_EXPECT_*` fallidas |
| skip   | ↷ | No  | `tt_skip_test()` o `tt_skip()` |
| xfail  | ∼ | No  | `tt_xfail()` y el test falló (lo esperado) |
| xpass  | ! | **Sí** | `tt_xfail()` pero el test pasó (inesperado) |

Ver también: [aserciones.md](aserciones.md) · [configuracion.md](configuracion.md).
