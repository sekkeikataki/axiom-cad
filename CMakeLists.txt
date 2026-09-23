cmake_minimum_required(VERSION 3.16)
project(axiom LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options(-O3 -ffast-math)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)

add_library(glad STATIC third_party/glad/src/gl.c)
target_include_directories(glad PUBLIC third_party/glad/include)
target_compile_features(glad PUBLIC c_std_99)

add_library(imgui STATIC
  third_party/imgui/imgui.cpp
  third_party/imgui/imgui_draw.cpp
  third_party/imgui/imgui_tables.cpp
  third_party/imgui/imgui_widgets.cpp
  third_party/imgui/backends/imgui_impl_glfw.cpp
  third_party/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
  third_party/imgui
  third_party/imgui/backends
)
target_link_libraries(imgui PUBLIC glad ${GLFW_LIBRARIES})
target_compile_definitions(imgui PUBLIC GLFW_INCLUDE_NONE)
target_include_directories(imgui PUBLIC ${GLFW_INCLUDE_DIRS} third_party/glad/include)

add_executable(axiom
  src/main.cpp
  src/mesh.cpp
  src/document.cpp
  src/solve.cpp
  src/render.cpp
  src/app.cpp
  src/material.cpp
  src/icons.cpp
  src/io.cpp
  src/slicer.cpp
  src/engineer.cpp
  src/inspect.cpp
)
find_package(ZLIB)
if(ZLIB_FOUND)
  target_link_libraries(axiom PRIVATE ZLIB::ZLIB)
  target_compile_definitions(axiom PRIVATE AXIOM_HAVE_ZLIB)
endif()
target_include_directories(axiom PRIVATE src third_party/glad/include ${GLFW_INCLUDE_DIRS})
target_compile_definitions(axiom PRIVATE GLFW_INCLUDE_NONE)
target_link_libraries(axiom PRIVATE imgui glad ${GLFW_LIBRARIES} GL dl pthread)
target_link_directories(axiom PRIVATE ${GLFW_LIBRARY_DIRS})

install(TARGETS axiom RUNTIME DESTINATION bin)
install(FILES examples/bracket.axm examples/pulley.axm examples/housing.axm examples/flange.axm examples/assembly.axm examples/handle.axm examples/tray.axm examples/spring.axm examples/gear.axm examples/ribbed.axm examples/elbow.axm examples/coupling.axm examples/pillow.axm examples/vblock.axm examples/hanger.axm DESTINATION share/axiom/examples OPTIONAL)
