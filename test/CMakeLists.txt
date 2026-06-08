# Meta-tests: la propia libreria ctests se prueba a si misma.
#   meta_ok     -> todo pasa (exit 0); ejercita el despacho de EXPECT_EQ (C11).
#   meta_fail   -> una asercion falla (exit 1, WILL_FAIL).
#   meta_xpass  -> un xfail que pasa cuenta como fallo (exit 1, WILL_FAIL).

add_executable(ctests_meta_ok meta_ok.c)
target_link_libraries(ctests_meta_ok PRIVATE ctests::ctests)
set_target_properties(ctests_meta_ok PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
add_test(NAME meta_ok COMMAND ctests_meta_ok)

add_executable(ctests_meta_fail meta_fail.c)
target_link_libraries(ctests_meta_fail PRIVATE ctests::ctests)
add_test(NAME meta_fail COMMAND ctests_meta_fail)
set_tests_properties(meta_fail PROPERTIES WILL_FAIL TRUE)

add_executable(ctests_meta_xpass meta_xpass.c)
target_link_libraries(ctests_meta_xpass PRIVATE ctests::ctests)
add_test(NAME meta_xpass COMMAND ctests_meta_xpass)
set_tests_properties(meta_xpass PROPERTIES WILL_FAIL TRUE)
