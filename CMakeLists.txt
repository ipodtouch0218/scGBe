cmake_minimum_required(VERSION 3.16)
project(scGBe LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.x)
FetchContent_MakeAvailable(SFML)

# Dependency: wxWidgets
set(wxBUILD_SHARED OFF)
FetchContent_Declare(wxWidgets
   GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
   GIT_SHALLOW ON
)
FetchContent_MakeAvailable(wxWidgets)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/*.cpp)
include_directories(include include/interface include/gb include/gb/sound)

include_directories($ENV{mingw64}/include)
add_executable(scGBe ${sources})
target_link_libraries(scGBe PRIVATE sfml-audio)
target_link_libraries(scGBe PRIVATE wxcore)
target_compile_features(scGBe PRIVATE cxx_std_17)
target_compile_options(scGBe PRIVATE -O3)

if(WIN32)
    add_custom_command(
        TARGET scGBe
        COMMENT "Copy OpenAL DLL"
        PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SFML_SOURCE_DIR}/extlibs/bin/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/openal32.dll $<TARGET_FILE_DIR:scGBe>
        VERBATIM)
endif()

install(TARGETS scGBe)
