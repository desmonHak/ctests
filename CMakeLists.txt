# =============================================================================
# ctests — libreria de tests unitarios e integracion para C/C++
# =============================================================================
#
# Consumo en otros proyectos (ver doc/instalacion.md para los detalles):
#
#   # 1) FetchContent (recomendado, no requiere instalacion previa)
#   include(FetchContent)
#   FetchContent_Declare(ctests
#       GIT_REPOSITORY https://github.com/desmonHak/ctests.git
#       GIT_TAG        v1.0.0)
#   FetchContent_MakeAvailable(ctests)
#   target_link_libraries(mis_tests PRIVATE ctests::ctests)
#
#   # 2) add_subdirectory (si lo incluyes como submodulo/vendored)
#   add_subdirectory(extern/ctests)
#   target_link_libraries(mis_tests PRIVATE ctests::ctests)
#
#   # 3) find_package (tras 'cmake --install')
#   find_package(ctests 1.0 REQUIRED)
#   target_link_libraries(mis_tests PRIVATE ctests::ctests)
#
# En todos los casos: #include "ctests.h" en tus archivos de test (en cuantos
# quieras) y enlaza ctests::ctests. La implementacion (ctests.c) la compila
# este target; no necesitas definir CTESTS_IMPLEMENTATION.
# =============================================================================

cmake_minimum_required(VERSION 3.14)

project(ctests
    VERSION     1.0.0
    DESCRIPTION "Libreria de tests unitarios e integracion para C/C++"
    LANGUAGES   C CXX)

# --- Deteccion de uso como subproyecto -------------------------------------
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(CTESTS_IS_TOP_LEVEL ON)
else()
    set(CTESTS_IS_TOP_LEVEL OFF)
endif()

option(CTESTS_BUILD_EXAMPLES "Compilar los ejemplos de ctests" ${CTESTS_IS_TOP_LEVEL})

include(GNUInstallDirs)

# --- La biblioteca (estatica: compila la implementacion una vez) -----------
add_library(ctests STATIC ctests.c)
add_library(ctests::ctests ALIAS ctests)

target_include_directories(ctests PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")

# La macro EXPECT_NEAR usa fabs() de <math.h>, que se expande en el codigo del
# consumidor; en sistemas tipo Unix eso implica enlazar libm. Lo propagamos.
if(UNIX AND NOT APPLE)
    target_link_libraries(ctests PUBLIC m)
endif()

# --- Ejemplos / tests ------------------------------------------------------
if(CTESTS_BUILD_EXAMPLES)
    enable_testing()
    add_subdirectory(example)
endif()

# --- Instalacion y find_package() ------------------------------------------
include(CMakePackageConfigHelpers)

install(TARGETS ctests EXPORT ctestsTargets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/ctests.h"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(EXPORT ctestsTargets
        FILE        ctestsTargets.cmake
        NAMESPACE   ctests::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ctests")

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ctestsConfigVersion.cmake"
    VERSION       ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ctestsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ctestsConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ctests")

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ctestsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ctestsConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ctests")
