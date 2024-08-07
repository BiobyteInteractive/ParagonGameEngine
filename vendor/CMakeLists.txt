cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

include(FetchContent)
include(ExternalProject)

FetchContent_Declare(
  efsw
  GIT_REPOSITORY https://github.com/SpartanJ/efsw.git
)
FetchContent_MakeAvailable(efsw)

FetchContent_Declare(
  toml11
  GIT_REPOSITORY https://github.com/ToruNiina/toml11.git
)
FetchContent_MakeAvailable(toml11)

FetchContent_Declare(
  enet
  GIT_REPOSITORY https://github.com/lsalzman/enet.git
)
FetchContent_MakeAvailable(enet)

FetchContent_Declare(
  flecs
  GIT_REPOSITORY https://github.com/SanderMertens/flecs.git
)
FetchContent_MakeAvailable(flecs)

fetchcontent_declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
)
fetchcontent_makeavailable(glm)

fetchcontent_declare(
  glfw3
  GIT_REPOSITORY https://github.com/glfw/glfw.git
)
fetchcontent_makeavailable(glfw3)

fetchcontent_declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
)
fetchcontent_makeavailable(assimp)

fetchcontent_declare(
  nanosvg
  GIT_REPOSITORY https://github.com/memononen/nanosvg.git
)
FetchContent_Populate(nanosvg)

fetchcontent_declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
)
fetchcontent_makeavailable(stb)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        origin/docking
  SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendor/imgui
)

FetchContent_Populate(imgui)

# Create a custom CMakeLists.txt for imgui
file(WRITE ${CMAKE_SOURCE_DIR}/vendor/imgui/CMakeLists.txt [[
cmake_minimum_required(VERSION 3.11)
project(imgui)

link_directories(
    "${CMAKE_SOURCE_DIR}/build/_deps/glfw3-build/src/Debug"
)

file(GLOB_RECURSE IMGUI_SRC
    "imconfig.h"
    "imgui_demo.cpp"
    "imgui_draw.cpp"
    "imgui_internal.h"
    "imgui_tables.cpp"
    "imgui_widgets.cpp"
    "imgui.cpp"
    "imgui.h"
    "imstb_rectpack.h"
    "imstb_textedit.h"
    "imstb_truetype.h"

    "backends/imgui_impl_glfw.cpp"
    "backends/imgui_impl_glfw.h"

    "backends/imgui_impl_opengl3_loader.h"
    "backends/imgui_impl_opengl3.cpp"
    "backends/imgui_impl_opengl3.h"
)

add_library(imgui STATIC ${IMGUI_SRC})

add_dependencies(imgui glfw)

target_include_directories(imgui 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${glfw3_SOURCE_DIR}/include
)

target_link_libraries(imgui
    PUBLIC glfw3dll
)
]])

add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/imgui)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/glad)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/finch)

set_target_properties(glfw                                PROPERTIES FOLDER "Dependencies/GLFW3")
set_target_properties(uninstall                           PROPERTIES FOLDER "Dependencies/GLFW3")
set_target_properties(update_mappings                     PROPERTIES FOLDER "Dependencies/GLFW3")
set_target_properties(assimp                              PROPERTIES FOLDER "Dependencies")
set_target_properties(efsw                                PROPERTIES FOLDER "Dependencies/efsw")
set_target_properties(efsw-static                         PROPERTIES FOLDER "Dependencies/efsw")
set_target_properties(enet                                PROPERTIES FOLDER "Dependencies")
set_target_properties(finch                               PROPERTIES FOLDER "Dependencies")
set_target_properties(flecs                               PROPERTIES FOLDER "Dependencies/flecs")
set_target_properties(flecs_static                        PROPERTIES FOLDER "Dependencies/flecs")
set_target_properties(glad                                PROPERTIES FOLDER "Dependencies")
set_target_properties(glm                                 PROPERTIES FOLDER "Dependencies")
set_target_properties(imgui                               PROPERTIES FOLDER "Dependencies")
set_target_properties(unit                                PROPERTIES FOLDER "Dependencies")
set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER "Dependencies")
set_target_properties(zlibstatic                          PROPERTIES FOLDER "Dependencies")