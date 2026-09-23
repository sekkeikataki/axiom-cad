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

include(FetchContent)

set(AXIOM_GLAD_DIR "${CMAKE_SOURCE_DIR}/third_party/glad")
if(NOT EXISTS "${AXIOM_GLAD_DIR}/src/gl.c")
  message(FATAL_ERROR
    "Vendored GLAD is missing (${AXIOM_GLAD_DIR}/src/gl.c). "
    "Run scripts/fetch-deps.sh from a network-connected clone.")
endif()

add_library(glad STATIC "${AXIOM_GLAD_DIR}/src/gl.c")
target_include_directories(glad PUBLIC "${AXIOM_GLAD_DIR}/include")
target_compile_features(glad PUBLIC c_std_99)

set(AXIOM_IMGUI_DIR "${CMAKE_SOURCE_DIR}/third_party/imgui")
if(NOT EXISTS "${AXIOM_IMGUI_DIR}/imgui.cpp")
  FetchContent_Declare(imgui
    URL https://github.com/ocornut/imgui/archive/refs/tags/v1.91.8.tar.gz
    URL_HASH SHA256=db3a2e02bfd6c269adf0968950573053d002f40bdfb9ef2e4a90bce804b0f286
  )
  FetchContent_GetProperties(imgui)
  if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
  endif()
  set(AXIOM_IMGUI_DIR "${imgui_SOURCE_DIR}")
endif()

add_library(imgui STATIC
  "${AXIOM_IMGUI_DIR}/imgui.cpp"
  "${AXIOM_IMGUI_DIR}/imgui_draw.cpp"
  "${AXIOM_IMGUI_DIR}/imgui_tables.cpp"
  "${AXIOM_IMGUI_DIR}/imgui_widgets.cpp"
  "${AXIOM_IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
  "${AXIOM_IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
)
target_include_directories(imgui PUBLIC
  "${AXIOM_IMGUI_DIR}"
  "${AXIOM_IMGUI_DIR}/backends"
)
target_link_libraries(imgui PUBLIC glad ${GLFW_LIBRARIES})
target_compile_definitions(imgui PUBLIC GLFW_INCLUDE_NONE)
target_include_directories(imgui PUBLIC ${GLFW_INCLUDE_DIRS} "${AXIOM_GLAD_DIR}/include")

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
install(FILES
  examples/bracket.axm examples/pulley.axm examples/housing.axm
  examples/flange.axm examples/assembly.axm examples/handle.axm
  examples/tray.axm examples/spring.axm examples/gear.axm
  examples/ribbed.axm examples/elbow.axm examples/coupling.axm
  examples/pillow.axm examples/vblock.axm examples/hanger.axm
  examples/box-fillet.axm
  DESTINATION share/axiom/examples OPTIONAL)
