###########################################################

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

ProcessorCount(NPROC)

###########################################################

find_program(MOLD_LINKER mold)
find_program(LLD_LINKER  lld)
find_program(GOLD_LINKER gold)

if(MOLD_LINKER)
    message(STATUS "[Lumen] Linker: mold (${NPROC} threads)")
    add_link_options(-fuse-ld=mold -Wl,--threads,--thread-count=${NPROC})
elseif(LLD_LINKER)
    message(STATUS "[Lumen] Linker: lld")
    add_link_options(-fuse-ld=lld)
elseif(GOLD_LINKER)
    message(STATUS "[Lumen] Linker: gold")
    add_link_options(-fuse-ld=gold)
else()
    message(STATUS "[Lumen] Linker: system default")
endif()

###########################################################
