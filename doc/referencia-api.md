# Referencia de la API

Todas las funciones públicas de ctests. Son funciones `static` definidas en el
header (header-only), con prefijo `tt_`. Las macros de aserción tienen su propia
página: [aserciones.md](aserciones.md).

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

- Si `fn` **falla** → resultado `xfail`; **no** rompe la build.
- Si `fn` **pasa** → resultado `xpass`; **sí** cuenta como fallo (señal de que hay
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
