cmake_minimum_required(VERSION 3.15)
project(gpda_cpp LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options(/W4 /O2 /EHsc /permissive-)
else()
    add_compile_options(-Wall -Wextra -O2)
endif()

# ANTLR4 C++ runtime (static)
set(ANTLR4_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../tools/antlr4-cpp-runtime)
if (EXISTS ${ANTLR4_ROOT}/runtime/src)
    set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "")
    set(ANTLR4_INSTALL OFF CACHE BOOL "")
    set(WITH_STATIC_CRT OFF CACHE BOOL "")
    add_subdirectory(${ANTLR4_ROOT} ${CMAKE_CURRENT_BINARY_DIR}/antlr4)
    set(ANTLR4_AVAILABLE TRUE)
endif()

# --- Scannerless parser library ---
add_library(scannerless STATIC scannerless.cpp)
target_include_directories(scannerless PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_scannerless test_scannerless.cpp)
target_link_libraries(test_scannerless PRIVATE scannerless)

# --- Tokenized parser library ---
add_library(tokenized STATIC tokenized.cpp)
target_include_directories(tokenized PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_tokenized test_tokenized.cpp)
target_link_libraries(test_tokenized PRIVATE tokenized)

# --- Hello-world (kept for sanity check) ---
add_executable(hello hello.cpp)

# --- Benchmarks ---
add_executable(run_ours_scannerless
    benchmarks/drivers/run_ours_scannerless.cpp
    benchmarks/generated/json_scannerless.cpp)
target_link_libraries(run_ours_scannerless PRIVATE scannerless)
target_include_directories(run_ours_scannerless PRIVATE
    benchmarks/drivers)

# flex+bison: they output C, so suppress MSVC warnings.
add_executable(run_flex_bison
    benchmarks/drivers/run_flex_bison.cpp
    benchmarks/generated/json.tab.c
    benchmarks/generated/json.yy.c)
target_include_directories(run_flex_bison PRIVATE
    benchmarks/drivers
    benchmarks/generated)
if (MSVC)
    # generated flex code uses POSIX `unistd.h` conventions; silence noise
    target_compile_definitions(run_flex_bison PRIVATE YY_NO_UNISTD_H)
    set_source_files_properties(
        benchmarks/generated/json.tab.c
        benchmarks/generated/json.yy.c
        PROPERTIES COMPILE_FLAGS "/W0")
endif()

# Our tokenized parser + flex lexer
add_executable(run_ours_tokenized
    benchmarks/drivers/run_ours_tokenized.cpp
    benchmarks/generated/json_tokenized.cpp
    benchmarks/generated/json.tab.c
    benchmarks/generated/json.yy.c)
target_link_libraries(run_ours_tokenized PRIVATE tokenized)
target_include_directories(run_ours_tokenized PRIVATE
    benchmarks/drivers
    benchmarks/generated)
if (MSVC)
    target_compile_definitions(run_ours_tokenized PRIVATE YY_NO_UNISTD_H)
    set_source_files_properties(
        benchmarks/generated/json.tab.c
        benchmarks/generated/json.yy.c
        PROPERTIES COMPILE_FLAGS "/W0")
endif()

# ANTLR4 JSON benchmark driver
if (ANTLR4_AVAILABLE)
    add_executable(run_antlr
        benchmarks/drivers/run_antlr.cpp
        benchmarks/generated/antlr/JSONLexer.cpp
        benchmarks/generated/antlr/JSONParser.cpp)
    target_include_directories(run_antlr PRIVATE
        benchmarks/drivers
        benchmarks/generated/antlr
        ${ANTLR4_ROOT}/runtime/src)
    target_link_libraries(run_antlr PRIVATE antlr4_static)
    if (MSVC)
        target_compile_options(run_antlr PRIVATE /W0)
    endif()
endif()

# ============================================================================
# Arithmetic benchmarks — unambiguous (precedence levels) vs ambiguous
# (single-level with %left / alt-ordering / GPDA's all-paths exploration).
# ============================================================================

# --- Ours: scannerless, unambiguous arith ---
add_executable(run_ours_scannerless_arith
    benchmarks/drivers/run_ours_scannerless_arith.cpp
    benchmarks/generated/arith_scannerless.cpp)
target_link_libraries(run_ours_scannerless_arith PRIVATE scannerless)
target_include_directories(run_ours_scannerless_arith PRIVATE
    benchmarks/drivers)
target_compile_definitions(run_ours_scannerless_arith PRIVATE
    GRAMMAR_FN=build_arith_graph
    PARSER_NAME="ours_scannerless_arith")

# --- Ours: scannerless, ambiguous arith ---
add_executable(run_ours_scannerless_arith_ambig
    benchmarks/drivers/run_ours_scannerless_arith.cpp
    benchmarks/generated/arith_ambig_scannerless.cpp)
target_link_libraries(run_ours_scannerless_arith_ambig PRIVATE scannerless)
target_include_directories(run_ours_scannerless_arith_ambig PRIVATE
    benchmarks/drivers)
target_compile_definitions(run_ours_scannerless_arith_ambig PRIVATE
    GRAMMAR_FN=build_arith_ambig_graph
    PARSER_NAME="ours_scannerless_arith_ambig")

# --- flex+bison arith (generates arith_parse + arith_lex) ---
add_executable(run_flex_bison_arith
    benchmarks/drivers/run_flex_bison_arith.cpp
    benchmarks/generated/arith.tab.c
    benchmarks/generated/arith.yy.c)
target_include_directories(run_flex_bison_arith PRIVATE
    benchmarks/drivers
    benchmarks/generated)
if (MSVC)
    target_compile_definitions(run_flex_bison_arith PRIVATE YY_NO_UNISTD_H)
    set_source_files_properties(
        benchmarks/generated/arith.tab.c
        benchmarks/generated/arith.yy.c
        PROPERTIES COMPILE_FLAGS "/W0")
endif()

# --- Ours: tokenized, unambiguous arith (uses flex arith lexer) ---
add_executable(run_ours_tokenized_arith
    benchmarks/drivers/run_ours_tokenized_arith.cpp
    benchmarks/generated/arith_tokenized.cpp
    benchmarks/generated/arith.tab.c
    benchmarks/generated/arith.yy.c)
target_link_libraries(run_ours_tokenized_arith PRIVATE tokenized)
target_include_directories(run_ours_tokenized_arith PRIVATE
    benchmarks/drivers
    benchmarks/generated)
target_compile_definitions(run_ours_tokenized_arith PRIVATE
    GRAMMAR_FN=build_arith_tok_graph
    PARSER_NAME="ours_tokenized_arith")
if (MSVC)
    target_compile_definitions(run_ours_tokenized_arith PRIVATE YY_NO_UNISTD_H)
    set_source_files_properties(
        benchmarks/generated/arith.tab.c
        benchmarks/generated/arith.yy.c
        PROPERTIES COMPILE_FLAGS "/W0")
endif()

# --- Ours: tokenized, ambiguous arith ---
add_executable(run_ours_tokenized_arith_ambig
    benchmarks/drivers/run_ours_tokenized_arith.cpp
    benchmarks/generated/arith_ambig_tokenized.cpp
    benchmarks/generated/arith.tab.c
    benchmarks/generated/arith.yy.c)
target_link_libraries(run_ours_tokenized_arith_ambig PRIVATE tokenized)
target_include_directories(run_ours_tokenized_arith_ambig PRIVATE
    benchmarks/drivers
    benchmarks/generated)
target_compile_definitions(run_ours_tokenized_arith_ambig PRIVATE
    GRAMMAR_FN=build_arith_ambig_tok_graph
    PARSER_NAME="ours_tokenized_arith_ambig")
if (MSVC)
    target_compile_definitions(run_ours_tokenized_arith_ambig PRIVATE
        YY_NO_UNISTD_H)
    set_source_files_properties(
        benchmarks/generated/arith.tab.c
        benchmarks/generated/arith.yy.c
        PROPERTIES COMPILE_FLAGS "/W0")
endif()

# --- ANTLR4 arith driver ---
if (ANTLR4_AVAILABLE)
    add_executable(run_antlr_arith
        benchmarks/drivers/run_antlr_arith.cpp
        benchmarks/generated/antlr/ArithLexer.cpp
        benchmarks/generated/antlr/ArithParser.cpp
        benchmarks/generated/antlr/ArithBaseListener.cpp
        benchmarks/generated/antlr/ArithListener.cpp)
    target_include_directories(run_antlr_arith PRIVATE
        benchmarks/drivers
        benchmarks/generated/antlr
        ${ANTLR4_ROOT}/runtime/src)
    target_link_libraries(run_antlr_arith PRIVATE antlr4_static)
    if (MSVC)
        target_compile_options(run_antlr_arith PRIVATE /W0)
    endif()
endif()
