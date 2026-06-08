# =============================================================================
# Makefile de conveniencia para compilar y probar los EJEMPLOS de ctests.
#
#   make            # compila todos los ejemplos en bin/
#   make test       # compila y ejecuta cada ejemplo, verificando su exit code
#   make run        # ejecuta el ejemplo completo (salida visual con colores)
#   make clean      # borra bin/
#   make help       # ayuda
#
# Funciona en Windows (mingw32-make, recetas via cmd.exe) y en Linux/macOS
# (make, recetas via /bin/sh). No sustituye a CMake (la via de integracion en
# otros proyectos); es solo un atajo para desarrollar la propia libreria.
# =============================================================================

# make predefine CC=cc / CXX=g++; sustituimos solo si siguen en su valor por
# defecto (asi 'make CC=clang' o la variable de entorno CC siguen mandando).
ifeq ($(origin CC),default)
  CC := gcc
endif
ifeq ($(origin CXX),default)
  CXX := g++
endif
CFLAGS   ?= -std=c99 -Wall -Wextra -I.
CXXFLAGS ?= -std=c++17 -Wall -I.
LDLIBS   ?= -lm

BIN  := bin
IMPL := ctests.c
HDR  := ctests.h

# --- Diferencias de plataforma (cmd.exe vs /bin/sh) ------------------------
ifeq ($(OS),Windows_NT)
  # separador de ruta para EJECUTAR en cmd ('\'); $(_empty) evita que el
  # backslash final se interprete como continuacion de linea.
  _empty :=
  S      := \$(_empty)
  EXE    := .exe
  NULL   := nul
  MKDIR   = if not exist $(BIN) mkdir $(BIN)
  RMDIR   = if exist $(BIN) rmdir /s /q $(BIN)
  # cmd: si el programa NO falla, error; si falla (lo esperado), OK.
  CHECKFAIL = $(BIN)$(S)example_c$(EXE) >nul 2>&1 && (echo   [FAIL] example_c no fallo& exit 1) || echo   [OK]   example_c - fallo intencionado
else
  S      := /
  EXE    :=
  NULL   := /dev/null
  MKDIR   = mkdir -p $(BIN)
  RMDIR   = rm -rf $(BIN)
  CHECKFAIL = $(BIN)$(S)example_c$(EXE) >/dev/null 2>&1 && { echo "  [FAIL] example_c no fallo"; exit 1; } || echo "  [OK]   example_c - fallo intencionado"
endif

BINS := $(BIN)/quickstart$(EXE) \
        $(BIN)/advanced$(EXE) \
        $(BIN)/example_cpp$(EXE) \
        $(BIN)/multifile$(EXE) \
        $(BIN)/example_c$(EXE)

.PHONY: all examples test run gen clean help
.DEFAULT_GOAL := all

all: examples

examples: $(BINS)

$(BIN):
	$(MKDIR)

# gcc/g++ aceptan '/' en -o aunque se ejecute bajo cmd; solo la EJECUCION
# necesita '\' en Windows (variable $(S)).
$(BIN)/quickstart$(EXE): example/quickstart.c $(IMPL) $(HDR) | $(BIN)
	$(CC) $(CFLAGS) example/quickstart.c $(IMPL) -o $@ $(LDLIBS)

$(BIN)/advanced$(EXE): example/advanced.c $(IMPL) $(HDR) | $(BIN)
	$(CC) $(CFLAGS) example/advanced.c $(IMPL) -o $@ $(LDLIBS)

$(BIN)/example_cpp$(EXE): example/example_test.cpp $(IMPL) $(HDR) | $(BIN)
	$(CXX) $(CXXFLAGS) example/example_test.cpp $(IMPL) -o $@ $(LDLIBS)

$(BIN)/multifile$(EXE): example/multifile_main.c example/multifile_aritmetica.c \
                        example/multifile_cadenas.c $(IMPL) $(HDR) | $(BIN)
	$(CC) $(CFLAGS) example/multifile_main.c example/multifile_aritmetica.c \
	      example/multifile_cadenas.c $(IMPL) -o $@ $(LDLIBS)

$(BIN)/example_c$(EXE): example/example_test.c $(IMPL) $(HDR) | $(BIN)
	$(CC) $(CFLAGS) example/example_test.c $(IMPL) -o $@ $(LDLIBS)

# Ejecuta cada ejemplo en silencio y comprueba el exit code esperado.
# Los 4 primeros deben salir con 0 (si fallan, make aborta en esa linea);
# example_c falla A PROPOSITO (exit 1), asi que se verifica aparte.
test: examples
	@echo == Probando ejemplos ==
	@$(BIN)$(S)quickstart$(EXE)  >$(NULL) 2>&1 && echo   [OK]   quickstart
	@$(BIN)$(S)advanced$(EXE)    >$(NULL) 2>&1 && echo   [OK]   advanced
	@$(BIN)$(S)example_cpp$(EXE) >$(NULL) 2>&1 && echo   [OK]   example_cpp
	@$(BIN)$(S)multifile$(EXE)   >$(NULL) 2>&1 && echo   [OK]   multifile
	@$(CHECKFAIL)
	@echo == Todos los ejemplos OK ==

# Ejecuta el ejemplo completo en C mostrando la salida con color.
# El '-' inicial hace que make ignore el exit code (example_c sale 1 a proposito).
run: $(BIN)/example_c$(EXE)
	-@$(BIN)$(S)example_c$(EXE)

# Compila la herramienta ctgen.
$(BIN)/ctgen$(EXE): tools/ctgen.c | $(BIN)
	$(CC) -O2 -I. tools/ctgen.c -o $@

# Genera los tests de example/annotated.c con ctgen y los ejecuta.
gen: $(BIN)/ctgen$(EXE)
	$(BIN)$(S)ctgen$(EXE) --ctests . example/annotated.c -o $(BIN)$(S)annotated_tests --run

clean:
	@$(RMDIR)

help:
	@echo Objetivos:
	@echo   make            Compila los ejemplos en bin/
	@echo   make test       Compila y ejecuta los ejemplos (verifica exit codes)
	@echo   make run        Ejecuta el ejemplo completo (salida con color)
	@echo   make gen        Compila ctgen y genera+ejecuta los tests de annotated.c
	@echo   make clean      Borra bin/
	@echo Variables: CC, CXX, CFLAGS, CXXFLAGS, LDLIBS
