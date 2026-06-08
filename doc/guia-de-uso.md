# Guía de uso

Tutorial progresivo de ctests. Cada sección añade un concepto sobre el anterior.

## 1. El esqueleto

Un programa de tests con ctests siempre tiene la misma forma:

```c
#include "ctests.h"

/* 1) Funciones de test: void(void), con aserciones dentro */
static void test_algo(void) {
    EXPECT_EQ_INT(2 + 2, 4);
}

int main(void) {
    /* 2) Agrupa en suites */
    tt_suite("Aritmetica");
        tt_run("dos mas dos", test_algo);

    /* 3) Resumen + codigo de salida (0 = todo ok, 1 = hubo fallos) */
    return tt_summary();
}
```

- Una **función de test** no recibe argumentos ni devuelve nada. Las aserciones
  comunican el resultado por dentro.
- `tt_suite("nombre")` abre un grupo. Cada `tt_run("descripcion", funcion)`
  posterior pertenece a esa suite hasta el siguiente `tt_suite`.
- `tt_summary()` debe ser lo último: imprime el resumen y devuelve el código de
  salida que tu `main` debe propagar (`return tt_summary();`).

## 2. Aserciones (hard)

Dentro de un test usas macros `EXPECT_*`. La primera que falla **detiene** el
test (las siguientes líneas no se ejecutan) y lo marca como fallido:

```c
static void test_usuario(void) {
    Usuario *u = crear_usuario("ana");
    EXPECT_NOT_NULL(u);                 /* si es NULL, paramos aqui */
    EXPECT_EQ_STR(u->nombre, "ana");
    EXPECT_EQ_INT(u->edad, 0);
    EXPECT_GT(u->id, 0);
    liberar_usuario(u);
}
```

La lista completa está en [aserciones.md](aserciones.md). Las más usadas:

| Macro | Comprueba |
|-------|-----------|
| `EXPECT_TRUE(c)` / `EXPECT_FALSE(c)` | condición booleana |
| `EXPECT_EQ_INT(a, b)` | igualdad de enteros (con diff `Expected/Received`) |
| `EXPECT_NEAR(a, b, eps)` | flotantes dentro de una tolerancia |
| `EXPECT_EQ_STR(a, b)` / `EXPECT_CONTAINS(s, sub)` | cadenas |
| `EXPECT_NULL(p)` / `EXPECT_NOT_NULL(p)` | punteros |

## 3. Soft assertions

A veces quieres comprobar varios campos y ver **todos** los que fallan, no solo
el primero. Para eso están las `SOFT_EXPECT_*`: no detienen el test; acumulan los
fallos y, al terminar la función, si hubo alguno, el test falla mostrándolos todos.

```c
static void test_validacion(void) {
    Respuesta *r = obtener();
    EXPECT_NOT_NULL(r);                    /* hard: sin r no tiene sentido seguir */

    SOFT_EXPECT_EQ_INT(r->codigo, 200);    /* aunque falle... */
    SOFT_EXPECT_EQ_STR(r->estado, "OK");   /* ...esta tambien se evalua */
    SOFT_EXPECT_GT(r->longitud, 0);

    free(r);
}
```

> Combina ambas: usa *hard* para precondiciones (si no se cumplen, lo demás
> no tiene sentido) y *soft* para verificar múltiples propiedades independientes.
> El límite de soft-fallos acumulados por test es [`TT_MAX_SOFT`](configuracion.md).

## 4. Fixtures: setup y teardown

Para preparar y limpiar estado alrededor de cada test de una suite, registra
hooks con `tt_suite_hooks(setup, teardown)` **después** de abrir la suite:

```c
static Conexion *db = NULL;

static void setup(void)    { db = db_open(":memory:"); }
static void teardown(void) { db_close(db); db = NULL; }   /* se llama aunque el test falle */

static void test_insert(void) { EXPECT_EQ_INT(db_insert(db, "x"), 0); }
static void test_count(void)  { EXPECT_EQ_INT(db_count(db), 0); }

int main(void) {
    tt_suite("Base de datos");
        tt_suite_hooks(setup, teardown);   /* aplica a los tt_run de esta suite */
        tt_run("insertar", test_insert);
        tt_run("contar",   test_count);
    return tt_summary();
}
```

- `setup` se ejecuta **antes** de cada `tt_run` de la suite.
- `teardown` se ejecuta **después** de cada test, **incluso si falló** (ideal para
  liberar memoria, cerrar ficheros, etc.).
- Cada `tt_suite()` **resetea** los hooks. Si la siguiente suite también los
  necesita, vuelve a llamar a `tt_suite_hooks()`.

## 5. Saltar tests (skip)

Hay dos maneras de marcar un test como saltado (no cuenta como fallo):

**a) Sin ejecutarlo en absoluto** — `tt_skip_test(nombre, razon)`:

```c
tt_skip_test("exportar a CSV", "pendiente de implementar");
```

**b) Decidirlo dentro del test** — `tt_skip(razon)` (p. ej. según el entorno):

```c
static void test_solo_linux(void) {
#ifndef __linux__
    tt_skip("solo aplica en Linux");
#endif
    EXPECT_TRUE(funcion_de_linux());
}
```

`tt_skip()` corta el test inmediatamente: nada después se ejecuta.

## 6. Fallos esperados (xfail)

Cuando conoces un bug que aún no puedes arreglar, marca el test como *expected
failure* con `tt_xfail(nombre, razon, funcion)`:

```c
tt_xfail("calculo roto del IVA", "bug #42 sin corregir", test_iva);
```

- Si el test **falla** → se reporta como `xfail` (morado) y **no** rompe la build.
- Si el test **pasa** → se reporta como `xpass` (¡el bug se arregló y nadie quitó
  el xfail!) y **sí** cuenta como fallo, recordándote actualizarlo.

## 7. Verbosidad

`tt_verbose(nivel)` controla cuánto se imprime (llámalo antes de los tests):

| Nivel | Efecto |
|-------|--------|
| `0` | Silencioso: solo se muestran los tests que fallan |
| `1` | Normal (por defecto): muestra todos los tests |
| `2` | Detallado: como `1` + estadísticas de test más lento / más rápido |

```c
int main(void) {
    tt_verbose(2);
    tt_suite("...");
    /* ... */
    return tt_summary();
}
```

## 8. Tests de integración

ctests no distingue "unitario" de "integración" a nivel de API: la diferencia la
pones tú en lo que ejercitas. Para integración, usa **fixtures** para levantar y
derribar el sistema bajo prueba, y agrupa los casos en una suite propia:

```c
static Servidor *srv;
static void up(void)   { srv = servidor_arrancar(8080); }
static void down(void) { servidor_parar(srv); }

static void test_ping(void) {
    Respuesta *r = http_get("http://localhost:8080/ping");
    EXPECT_NOT_NULL(r);
    EXPECT_EQ_INT(r->status, 200);
    EXPECT_CONTAINS(r->body, "pong");
    respuesta_free(r);
}

int main(void) {
    tt_suite("Integracion HTTP");
        tt_suite_hooks(up, down);
        tt_run("GET /ping responde 200 pong", test_ping);
    return tt_summary();
}
```

## 9. Uso en CI

`tt_summary()` devuelve `0` si todo pasó y `1` si hubo cualquier fallo (incluido
un `xpass`). Como `main` propaga ese valor, tu sistema de CI detecta el fallo sin
configuración adicional. Con CMake/CTest:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure   # exit != 0 si algun test falla
```

## Siguiente paso

- [referencia-api.md](referencia-api.md) — todas las funciones `tt_*`.
- [aserciones.md](aserciones.md) — todas las macros de aserción.
- [configuracion.md](configuracion.md) — ajustar límites en compilación.
