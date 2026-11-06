#/*
#  ФАЙЛ: CMakeLists.txt
#
#  CMake конфигурация сборки: находит Geant4 и ROOT (если нужно), включает нужные
#  include/link, собирает пример exampleB1.
#*/

cmake_minimum_required(VERSION 3.16)
project(B1)

# --- Enforce C++ standard globally
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#----------------------------------------------------------------------------
# Find Geant4
option(WITH_GEANT4_UIVIS "Build example with Geant4 UI and Vis drivers" ON)
if(WITH_GEANT4_UIVIS)
  find_package(Geant4 REQUIRED ui_all vis_all)
else()
  find_package(Geant4 REQUIRED)
endif()

#----------------------------------------------------------------------------
# ROOT
find_package(ROOT REQUIRED)

#----------------------------------------------------------------------------
# Includes
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${Geant4_INCLUDE_DIRS})
include_directories(${ROOT_INCLUDE_DIRS})

#----------------------------------------------------------------------------
# Sources/headers
file(GLOB sources ${PROJECT_SOURCE_DIR}/src/*.cc)
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hh)

add_executable(exampleB1 exampleB1.cc ${sources} ${headers})
target_compile_features(exampleB1 PRIVATE cxx_std_17)

target_link_libraries(exampleB1 PRIVATE
  ${Geant4_LIBRARIES}
  ${ROOT_LIBRARIES}
)

#----------------------------------------------------------------------------
# Runtime files (macros, data). Копируем только если файл существует,
# чтобы configure_file не падал на отсутствующих скриптах.
set(RUNTIME_FILES
  mask-mura-61.txt
  init_vis.mac
  vis.mac
  GPS.mac
  run.mac
)

foreach(_f ${RUNTIME_FILES})
  if(EXISTS "${PROJECT_SOURCE_DIR}/${_f}")
    configure_file(
      "${PROJECT_SOURCE_DIR}/${_f}"
      "${PROJECT_BINARY_DIR}/${_f}"
      COPYONLY
    )
  endif()
endforeach()

#----------------------------------------------------------------------------
# Convenience target
add_custom_target(B1 DEPENDS exampleB1)

#----------------------------------------------------------------------------
# Install
install(TARGETS exampleB1 DESTINATION bin)
