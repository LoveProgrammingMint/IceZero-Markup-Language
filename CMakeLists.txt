cmake_minimum_required(VERSION 3.25)

project(IceZeroMarkupLanguage VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(IZML_BUILD_TOOLS "Build izmlc and izmlgenc" ON)

set(IZML_ROOT       "${CMAKE_CURRENT_SOURCE_DIR}")
set(IZML_COMMON     "${IZML_ROOT}/common")
set(IZML_PARSER     "${IZML_ROOT}/parser")
set(IZML_COMPILER   "${IZML_ROOT}/compiler")
set(IZML_GENERATOR  "${IZML_ROOT}/generator")
set(IZML_RUNTIME    "${IZML_ROOT}/runtime")
set(IZML_DATA       "${IZML_ROOT}/data")
set(IZML_BIN_DIR    "${CMAKE_BINARY_DIR}/bin")
set(IZML_LIB_DIR    "${CMAKE_BINARY_DIR}/lib")

file(MAKE_DIRECTORY "${IZML_BIN_DIR}" "${IZML_LIB_DIR}")

function(izml_place target)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${IZML_BIN_DIR}"
        LIBRARY_OUTPUT_DIRECTORY "${IZML_BIN_DIR}"
        ARCHIVE_OUTPUT_DIRECTORY "${IZML_LIB_DIR}"
    )
endfunction()

# ---------------------------------------------------------------------------
#  izml-parser.dll — IZML -> IZLR
# ---------------------------------------------------------------------------
set(IZML_PARSER_SOURCES
    "${IZML_PARSER}/src/izml_ast.cpp"
    "${IZML_PARSER}/src/izml_lexer.cpp"
    "${IZML_PARSER}/src/izml_parser.cpp"
    "${IZML_PARSER}/src/izml_sema.cpp"
    "${IZML_PARSER}/src/izml_api.cpp"
)

if(WIN32)
    list(APPEND IZML_PARSER_SOURCES "${IZML_PARSER}/src/dllmain.cpp")
endif()

add_library(izml-parser SHARED ${IZML_PARSER_SOURCES})
target_include_directories(izml-parser
    PUBLIC
        "${IZML_PARSER}/include"
        "${IZML_COMMON}/include"
)
target_compile_definitions(izml-parser PRIVATE IZMLPARSER_EXPORTS)
set_target_properties(izml-parser PROPERTIES PREFIX "" OUTPUT_NAME "izml-parser")
izml_place(izml-parser)

# ---------------------------------------------------------------------------
#  izml-runtime.dll — IZRC C ABI over the native runtime
# ---------------------------------------------------------------------------
add_library(izml-runtime SHARED
    "${IZML_RUNTIME}/src/izml_runtime.cpp"
)
target_include_directories(izml-runtime PUBLIC "${IZML_RUNTIME}/include")
target_compile_definitions(izml-runtime PRIVATE IZMLRUNTIME_EXPORTS)
set_target_properties(izml-runtime PROPERTIES PREFIX "" OUTPUT_NAME "izml-runtime")
izml_place(izml-runtime)

if(IZML_BUILD_TOOLS)
    # -----------------------------------------------------------------------
    #  izmlc.exe — IZML/IZLR -> IZRC
    # -----------------------------------------------------------------------
    add_executable(izmlc
        "${IZML_COMPILER}/src/main.cpp"
        "${IZML_COMPILER}/src/izrc_writer.cpp"
    )
    target_include_directories(izmlc PRIVATE
        "${IZML_COMPILER}/src"
        "${IZML_PARSER}/include"
        "${IZML_COMMON}/include"
    )
    target_link_libraries(izmlc PRIVATE izml-parser)
    izml_place(izmlc)

    # -----------------------------------------------------------------------
    #  izmlgenc.exe — IZLR -> configured language
    # -----------------------------------------------------------------------
    add_executable(izmlgenc
        "${IZML_GENERATOR}/src/main.cpp"
        "${IZML_GENERATOR}/src/lang_config.cpp"
        "${IZML_GENERATOR}/src/template_value.cpp"
        "${IZML_GENERATOR}/src/template_expr.cpp"
        "${IZML_GENERATOR}/src/template_render.cpp"
        "${IZML_GENERATOR}/src/template_generator.cpp"
    )
    target_include_directories(izmlgenc PRIVATE
        "${IZML_GENERATOR}/src"
        "${IZML_PARSER}/include"
        "${IZML_COMMON}/include"
    )
    target_link_libraries(izmlgenc PRIVATE izml-parser)
    target_compile_definitions(izmlgenc PRIVATE IZML_RUNTIME_DIR="${IZML_RUNTIME}")
    izml_place(izmlgenc)

    add_custom_command(TARGET izmlgenc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${IZML_DATA}/izmlgen/c-cpp.izmlgen-lang"
            "${IZML_DATA}/izmlgen/csharp.izmlgen-lang"
            "${IZML_DATA}/izmlgen/python3.izmlgen-lang"
            "${IZML_BIN_DIR}"
        COMMENT "Staging IZML language configs"
        VERBATIM
    )
endif()