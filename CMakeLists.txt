cmake_minimum_required(VERSION 3.28)

project(
    LumenEngine
    VERSION 0.1.0
    DESCRIPTION "LumenEngine C++ Game Engine"
    LANGUAGES CXX
)

###########################################################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")

include(ProcessorCount)
include(LumenOptions)
include(LumenCompiler)
include(LumenDependencies)
include(LumenModule)
include(LumenShaders)
include(LumenBuild)

###########################################################
