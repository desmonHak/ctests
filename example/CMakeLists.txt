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

# example_test.c — recorrido completo en C99. Incluye fallos INTENCIONADOS
# para mostrar el formato de error, por lo que tt_summary() devuelve 1.
# Marcamos el test como "se espera que falle" para que CTest lo de por bueno.
add_executable(ctests_example_c example_test.c)
target_link_libraries(ctests_example_c PRIVATE ctests::ctests)
set_target_properties(ctests_example_c PROPERTIES C_STANDARD 99)
add_test(NAME example_c COMMAND ctests_example_c)
set_tests_properties(example_c PROPERTIES WILL_FAIL TRUE)
