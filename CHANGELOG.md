# Changelog

Formato basado en [Keep a Changelog](https://keepachangelog.com/es/1.1.0/);
versionado [SemVer](https://semver.org/lang/es/).

## [1.0.0] - 2026-06-09

Primera versión estable.

### Núcleo de tests

- Suites (`tt_suite`), tests (`tt_run`), data-driven (`tt_run_param` + `tt_param`).
- Fixtures por test (`tt_suite_hooks`) y once por suite (`tt_suite_hooks_once`).
- `tt_skip` / `tt_skip_test` / `tt_xfail`; soft assertions; verbosidad; resumen
  visual con barra de progreso, slowest/fastest.
- Auto-registro de tests con `TEST(suite, nombre)` + `tt_run_all()`
  (constructor en GCC/Clang/MinGW, `.CRT$XCU` en MSVC).

### Aserciones

- **API unificada** `EXPECT_EQ` / `EXPECT_NE` para int/uint/cadena/flotante
  (tolerancia relativa) / puntero, vía `_Generic` (C11) o plantillas (C++17).
- Macros tipadas como fallback C99 e interno: `EXPECT_EQ_INT/UINT/PTR/STR/MEM`,
  `EXPECT_NEAR`/`NEAR_REL`/`EQ_FLOAT`, `EXPECT_IN_RANGE`, `EXPECT_ARRAY_EQ`,
  `EXPECT_TRUE/FALSE`, `EXPECT_NULL/NOT_NULL`, `EXPECT_CONTAINS`/`STARTS_WITH`,
  `EXPECT_MSG`, orden (`GT/GE/LT/LE`). Variantes `SOFT_*` y alias `ASSERT_*`.
- En C++: `EXPECT_THROW` / `EXPECT_NO_THROW`.
- Todo fallo se prefija con `archivo:línea`.

### Robustez y salida

- Captura de crashes (`SIGSEGV`/`SIGFPE`/…) con `sig(set)jmp`: el test se reporta
  y la suite continúa (`tt_catch_crashes`).
- Timeout por test en POSIX (`tt_timeout`, `--timeout`).
- Captura de `stdout`/`stderr` (`tt_capture_begin`/`tt_capture_end`).
- Color con auto-detección de terminal + `NO_COLOR`; VT100/UTF-8 en Windows.
- Informes **JUnit XML** y **TAP**; opciones de línea de comandos (`tt_parse_args`).

### Herramientas y empaquetado

- `ctgen`: genera tests desde anotaciones `@tag` en los comentarios y produce un
  ejecutable real. Soporta archivos y **carpetas** (`-r`), tests complejos
  (`@let`/`@cleanup`, `@body`, `@suite_setup`/`@suite_teardown`).
- CMake: target `ctests::ctests` (STATIC) con `install` y `find_package`;
  Makefile (`test`/`run`/`gen`); CI (Linux/macOS/Windows, C99, ASan/UBSan,
  install+find_package).
- Documentación en `doc/` y compatibilidad Doxygen (ALIASES para los tags de ctgen).

[1.0.0]: https://github.com/desmonHak/ctests/releases/tag/v1.0.0
