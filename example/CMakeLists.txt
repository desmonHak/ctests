# Ejemplos de ctests. Cada ejecutable enlaza el target de la libreria y se
# registra como test de CTest para poder lanzarlos con `ctest`.

# quickstart.c — el ejemplo minimo (debe pasar al 100%)
add_executable(ctests_quickstart quickstart.c)
target_link_libraries(ctests_quickstart PRIVATE ctests::ctests)
add_test(NAME quickstart COMMAND ctests_quickstart)

# advanced.c — features avanzadas: data-driven, hooks once, aserciones nuevas
add_executable(ctests_advanced advanced.c)
target_link_libraries(ctests_advanced PRIVATE ctests::ctests)
add_test(NAME advanced COMMAND ctests_advanced)

# autoregister.c — macro TEST() (auto-registro) + captura de stdout.
# El auto-registro usa constructores; en MSVC va por .CRT$XCU (no ejercitado en
# CI), asi que solo lo construimos con GCC/Clang/MinGW para mantener el CI verde.
if(NOT MSVC)
    add_executable(ctests_autoregister autoregister.c)
    target_link_libraries(ctests_autoregister PRIVATE ctests::ctests)
    add_test(NAME autoregister COMMAND ctests_autoregister)
endif()

# example_test.cpp — recorrido por la API en C++17 (debe pasar al 100%)
add_executable(ctests_example_cpp example_test.cpp)
target_link_libraries(ctests_example_cpp PRIVATE ctests::ctests)
set_target_properties(ctests_example_cpp PROPERTIES
    CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
add_test(NAME example_cpp COMMAND ctests_example_cpp)

# multifile — tests repartidos en varios archivos que comparten estado
add_executable(ctests_multifile
    multifile_main.c
    multifile_aritmetica.c
    multifile_cadenas.c)
target_link_libraries(ctests_multifile PRIVATE ctests::ctests)
add_test(NAME multifile COMMAND ctests_multifile)

# --- ctgen: generacion de tests desde anotaciones ------------------------
# Compilamos la herramienta ctgen y la usamos para generar tests a partir de
# example/annotated.c. El auto-registro del codigo generado usa constructores
# (en MSVC, .CRT$XCU, no ejercitado en CI), asi que el test solo se construye
# con GCC/Clang/MinGW. La herramienta ctgen si se compila siempre.
add_executable(ctgen ${CMAKE_SOURCE_DIR}/tools/ctgen.c)

if(NOT MSVC)
    set(_ctgen_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
    file(MAKE_DIRECTORY "${_ctgen_dir}")
    add_custom_command(
        OUTPUT  "${_ctgen_dir}/annotated.gen.c" "${_ctgen_dir}/_ctgen_runner.gen.c"
        COMMAND ctgen --gen-only --emit-runner
                --outdir "${_ctgen_dir}" --ctests "${CMAKE_SOURCE_DIR}"
                "${CMAKE_CURRENT_SOURCE_DIR}/annotated.c"
        DEPENDS ctgen "${CMAKE_CURRENT_SOURCE_DIR}/annotated.c"
        COMMENT "ctgen: generando tests desde annotated.c"
        VERBATIM)
    add_executable(ctests_annotated
        "${_ctgen_dir}/annotated.gen.c"
        "${_ctgen_dir}/_ctgen_runner.gen.c")
    target_link_libraries(ctests_annotated PRIVATE ctests::ctests)
    # ctgen genera EXPECT_EQ (@eq) que requiere C11; fijarlo aunque el proyecto
    # se configure con un estandar C anterior (p.ej. el job C99 del CI).
    set_target_properties(ctests_annotated PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
    add_test(NAME annotated COMMAND ctests_annotated)
endif()

# example_test.c — recorrido completo en C99. Incluye fallos INTENCIONADOS
# para mostrar el formato de error, por lo que tt_summary() devuelve 1.
# Marcamos el test como "se espera que falle" para que CTest lo de por bueno.
add_executable(ctests_example_c example_test.c)
target_link_libraries(ctests_example_c PRIVATE ctests::ctests)
set_target_properties(ctests_example_c PROPERTIES C_STANDARD 99)
add_test(NAME example_c COMMAND ctests_example_c)
set_tests_properties(example_c PROPERTIES WILL_FAIL TRUE)
