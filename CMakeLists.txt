cmake_minimum_required(VERSION 3.16)
project(gbemu LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# set(CMAKE_C_COMPILER "C:/msys64/mingw64/bin/gcc.exe")
# set(CMAKE_CXX_COMPILER "C:/msys64/mingw64/bin/g++.exe")
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.x)
FetchContent_MakeAvailable(SFML)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/*.cpp)

include_directories(include include/sound)
include_directories(C:/msys64/mingw64/include)
add_executable(gbemu ${sources})
target_link_libraries(gbemu PRIVATE sfml-graphics sfml-audio)
target_compile_features(gbemu PRIVATE cxx_std_17)

if(WIN32)
    add_custom_command(
        TARGET gbemu
        COMMENT "Copy OpenAL DLL"
        PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SFML_SOURCE_DIR}/extlibs/bin/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/openal32.dll $<TARGET_FILE_DIR:gbemu>
        VERBATIM)
endif()

install(TARGETS gbemu)
