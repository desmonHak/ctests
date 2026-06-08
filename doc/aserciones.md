# Referencia de aserciones

Las aserciones son macros que usas **dentro** de una función de test. Hay dos
familias:

- **Hard** (`EXPECT_*`): la primera que falla detiene el test inmediatamente.
- **Soft** (`SOFT_EXPECT_*`): no detienen; acumulan el fallo y el test termina
  fallando con todos los mensajes juntos. Útiles para comprobar varios campos.

> Las macros que comparan enteros hacen *cast* a `long long`; las de flotantes a
> `double`. Pasa expresiones de tipos compatibles.

---

## Hard assertions (`EXPECT_*`)

### Booleanos

| Macro | Falla si | Mensaje |
|-------|----------|---------|
| `EXPECT_TRUE(c)`  | `c` es falso | `Expected TRUE: <c>` |
| `EXPECT_FALSE(c)` | `c` es verdadero | `Expected FALSE: <c>` |
| `EXPECT_MSG(c, ...)` | `c` es falso (mensaje `printf`-style propio) | el que tú escribas |

```c
EXPECT_TRUE(lista_vacia(l));
EXPECT_FALSE(error_ocurrido());
EXPECT_MSG(saldo >= 0, "saldo negativo: %d", saldo);
```

### Punteros

| Macro | Falla si |
|-------|----------|
| `EXPECT_NULL(p)`     | `p != NULL` |
| `EXPECT_NOT_NULL(p)` | `p == NULL` |
| `EXPECT_EQ_PTR(a, b)` | `a != b` (igualdad de punteros, formato `%p`) |

```c
EXPECT_NOT_NULL(buffer);
EXPECT_NULL(buscar(lista, "inexistente"));
EXPECT_EQ_PTR(nodo->siguiente, esperado);
```

### Enteros

| Macro | Falla si |
|-------|----------|
| `EXPECT_EQ_INT(a, b)`  | `a != b` |
| `EXPECT_NEQ_INT(a, b)` | `a == b` |

`EXPECT_EQ_INT` imprime un diff claro al fallar:

```text
  - Expected: 5
  + Received: 4
```

```c
EXPECT_EQ_INT(sumar(2, 3), 5);
EXPECT_NEQ_INT(id_nuevo(), id_viejo());
```

### Enteros sin signo

Como `EXPECT_EQ_INT` castea a `long long`, usa estas para valores que no caben con
signo (p. ej. `unsigned long long` cercanos a `2^64`):

| Macro | Falla si |
|-------|----------|
| `EXPECT_EQ_UINT(a, b)`  | `a != b` (comparados como `unsigned long long`) |
| `EXPECT_NEQ_UINT(a, b)` | `a == b` |

```c
EXPECT_EQ_UINT(hash, 0xFFFFFFFFFFFFFFFFULL);
```

### Comparaciones de orden

| Macro | Falla si |
|-------|----------|
| `EXPECT_GT(a, b)` | no `a > b` |
| `EXPECT_GE(a, b)` | no `a >= b` |
| `EXPECT_LT(a, b)` | no `a < b` |
| `EXPECT_LE(a, b)` | no `a <= b` |

```c
EXPECT_GT(longitud, 0);
EXPECT_LE(edad, 120);
```

### Flotantes

| Macro | Falla si |
|-------|----------|
| `EXPECT_NEAR(a, b, eps)` | `fabs(a - b) > eps` |

Nunca compares flotantes por igualdad exacta; usa una tolerancia:

```c
EXPECT_NEAR(0.1 + 0.2, 0.3, 1e-9);   /* 0.1+0.2 != 0.3 exacto en IEEE 754 */
```

### Cadenas (C, terminadas en `\0`)

| Macro | Falla si |
|-------|----------|
| `EXPECT_EQ_STR(a, b)`        | `strcmp(a, b) != 0` |
| `EXPECT_NEQ_STR(a, b)`       | `strcmp(a, b) == 0` |
| `EXPECT_CONTAINS(s, sub)`    | `sub` no aparece dentro de `s` |
| `EXPECT_STARTS_WITH(s, pre)` | `s` no empieza por `pre` |

```c
EXPECT_EQ_STR(usuario->nombre, "ana");
EXPECT_CONTAINS(log, "ERROR");
EXPECT_STARTS_WITH(url, "https://");
```

> En C++ usa `std::string::c_str()` para pasar `const char*`:
> `EXPECT_EQ_STR(s.c_str(), "hola");`

### Bloques de memoria

| Macro | Falla si |
|-------|----------|
| `EXPECT_EQ_MEM(a, b, n)` | `memcmp(a, b, n) != 0` |

Al fallar, indica el primer byte que difiere y sus valores. Útil para comparar
buffers, structs serializados o arrays binarios:

```c
unsigned char esperado[4] = {0xDE, 0xAD, 0xBE, 0xEF};
EXPECT_EQ_MEM(salida, esperado, sizeof(esperado));
/* fallo: memory differs at byte 2: 0x00 != 0xBE */
```

### Fallo incondicional

| Macro | Efecto |
|-------|--------|
| `EXPECT_FAIL(msg)` | Falla el test siempre, con el mensaje `msg` |

Útil en ramas que no deberían alcanzarse:

```c
switch (tipo) {
    case A: /* ... */ break;
    case B: /* ... */ break;
    default: EXPECT_FAIL("tipo desconocido");
}
```

### Alias `ASSERT_*`

Para quien viene de Google Test, cada `EXPECT_*` tiene un alias `ASSERT_*`
idéntico (`ASSERT_TRUE`, `ASSERT_EQ_INT`, `ASSERT_EQ_MEM`, `ASSERT_THROW`, …).

> Ojo: en ctests **`EXPECT_*` ya es "hard"** (detiene el test al fallar), así que
> `ASSERT_*` es exactamente lo mismo. La variante que *no* detiene es `SOFT_EXPECT_*`.

---

## Soft assertions (`SOFT_EXPECT_*`)

Mismas comprobaciones, pero acumulan en lugar de detener. Disponibles:

| Macro | Equivalente hard |
|-------|------------------|
| `SOFT_EXPECT_TRUE(c)`      | `EXPECT_TRUE` |
| `SOFT_EXPECT_FALSE(c)`     | `EXPECT_FALSE` |
| `SOFT_EXPECT_NULL(p)`      | `EXPECT_NULL` |
| `SOFT_EXPECT_NOT_NULL(p)`  | `EXPECT_NOT_NULL` |
| `SOFT_EXPECT_MSG(c, ...)`  | `EXPECT_MSG` |
| `SOFT_EXPECT_EQ_INT(a, b)` | `EXPECT_EQ_INT` |
| `SOFT_EXPECT_EQ_UINT(a, b)`| `EXPECT_EQ_UINT` |
| `SOFT_EXPECT_EQ_STR(a, b)` | `EXPECT_EQ_STR` |
| `SOFT_EXPECT_NEAR(a,b,e)`  | `EXPECT_NEAR` |
| `SOFT_EXPECT_GT(a, b)`     | `EXPECT_GT` |
| `SOFT_EXPECT_LT(a, b)`     | `EXPECT_LT` |

```c
static void test_respuesta(void) {
    Respuesta *r = obtener();
    EXPECT_NOT_NULL(r);                  /* precondicion: hard */

    SOFT_EXPECT_EQ_INT(r->codigo, 200);  /* todas se evaluan aunque alguna falle */
    SOFT_EXPECT_EQ_STR(r->estado, "OK");
    SOFT_EXPECT_GT(r->longitud, 0);

    free(r);
}
```

Si fallan varias, el informe del test las muestra todas:

```text
✗ test_respuesta 0ms
  - Expected: 200
  + Received: 404
  - Expected: "OK"
  + Received: "Not Found"
```

> El número máximo de soft-fallos que se acumulan por test es
> [`TT_MAX_SOFT`](configuracion.md) (16 por defecto); los que excedan ese límite
> se descartan silenciosamente.

---

## Aserciones de excepciones (solo C++)

Disponibles únicamente al compilar con un compilador C++ (`__cplusplus`).

| Macro | Falla si |
|-------|----------|
| `EXPECT_THROW(expr, ExType)` | `expr` **no** lanza una excepción de tipo `ExType` |
| `EXPECT_NO_THROW(expr)`      | `expr` lanza cualquier excepción |

```cpp
EXPECT_THROW(dividir(1, 0), std::runtime_error);
EXPECT_NO_THROW(dividir(4, 2));
```

Ver también: [referencia-api.md](referencia-api.md) · [configuracion.md](configuracion.md).
