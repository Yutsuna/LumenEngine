###########################################################

add_library(LumenCompiler INTERFACE)
add_library(Lumen::Compiler ALIAS LumenCompiler)

###########################################################

set(IS_GCC    $<CXX_COMPILER_ID:GNU>)
set(IS_CLANG  $<CXX_COMPILER_ID:Clang>)
set(IS_MSVC   $<CXX_COMPILER_ID:MSVC>)
set(IS_GNULIKE $<OR:${IS_GCC},${IS_CLANG}>)

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

###########################################################

target_compile_options(LumenCompiler INTERFACE
    $<${IS_GNULIKE}:
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wcast-align
        -Wunused
        -Woverloaded-virtual
        -Wconversion
        -Wsign-conversion
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
        -Wimplicit-fallthrough
        -Werror
    >
    $<${IS_MSVC}:
        /W4
        /permissive-
        /w14242 /w14254 /w14263 /w14265
        /w14287 /we4289 /w14296 /w14311
        /w14545 /w14546 /w14547 /w14549
        /w14555 /w14619 /w14640 /w14826
        /w14905 /w14906 /w14928
    >
)

target_compile_options(LumenCompiler INTERFACE
    $<$<AND:${IS_GNULIKE},$<CONFIG:Debug>>:        -Og -g3 -fno-omit-frame-pointer>
    $<$<AND:${IS_GNULIKE},$<CONFIG:Release>>:      -O3 -DNDEBUG>
    $<$<AND:${IS_GNULIKE},$<CONFIG:RelWithDebInfo>>: -O2 -g -DNDEBUG>
)

###########################################################

option(LUMEN_ENABLE_ASAN  "Enable AddressSanitizer (Debug only)"           OFF)
option(LUMEN_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer (Debug only)" OFF)

if(LUMEN_ENABLE_ASAN)
    target_compile_options(LumenCompiler INTERFACE
        $<$<AND:${IS_GNULIKE},$<CONFIG:Debug>>: -fsanitize=address -fno-omit-frame-pointer>
    )
    target_link_options(LumenCompiler INTERFACE
        $<$<AND:${IS_GNULIKE},$<CONFIG:Debug>>: -fsanitize=address>
    )
    message(STATUS "[Lumen] ASan: enabled")
endif()

if(LUMEN_ENABLE_UBSAN)
    target_compile_options(LumenCompiler INTERFACE
        $<$<AND:${IS_GNULIKE},$<CONFIG:Debug>>: -fsanitize=undefined>
    )
    target_link_options(LumenCompiler INTERFACE
        $<$<AND:${IS_GNULIKE},$<CONFIG:Debug>>: -fsanitize=undefined>
    )
    message(STATUS "[Lumen] UBSan: enabled")
endif()

###########################################################

option(LUMEN_ENABLE_LTO "Enable Link-Time Optimisation in Release" OFF)
if(LUMEN_ENABLE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT IPO_OK OUTPUT IPO_ERR)
    if(IPO_OK)
        set_property(TARGET LumenCompiler PROPERTY
            INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
        message(STATUS "[Lumen] LTO: enabled")
    else()
        message(WARNING "[Lumen] LTO requested but not supported: ${IPO_ERR}")
    endif()
endif()

###########################################################
