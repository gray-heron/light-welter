cmake_minimum_required(VERSION 3.0)
project(light-welter)

include(CTest)
include(ExternalProject)
include(CMakeRC.cmake)
include(FindPkgConfig)

enable_testing()

# ==============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  add_compile_options(-fno-limit-debug-info)
endif()

include_directories(inc)
include_directories(${CMAKE_BINARY_DIR}/gen)
include_directories(${CMAKE_BINARY_DIR}/dependencies/include)

# ==============================================================================

set(pugixml_DIR /usr/share/libpugixml-dev/cmake/)

# ==============================================================================

find_package(Threads)
find_package(Boost COMPONENTS system filesystem unit_test_framework REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

ExternalProject_Add(sdl2-dependency
  URL https://github.com/libSDL2pp/libSDL2pp/archive/0.16.0.tar.gz
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/dependencies -DSDL2PP_WITH_MIXER=OFF -DSDL2PP_WITH_IMAGE=ON -DSDL2PP_STATIC=ON
)

ExternalProject_Add(pugixml-dependency
  URL https://github.com/zeux/pugixml/releases/download/v1.9/pugixml-1.9.tar.gz
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/dependencies -DSPDLOG_BUILD_EXAMPLES=0 -DSPDLOG_BUILD_BENCH=0 -DSPDLOG_BUILD_TESTING=0 -DSPDLOG_BUILD_TESTING=0
)

find_package(SDL2 REQUIRED)
PKG_SEARCH_MODULE(SDL2IMAGE REQUIRED SDL2_image>=2.0.0)
find_package(OpenGL REQUIRED)                                                          
find_package(GLEW REQUIRED)
find_package(assimp REQUIRED)
find_package(spdlog)
find_package(glm REQUIRED)

include_directories(${GLM_INCLUDE_DIRS})
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${ASSIMP_INCLUDE_DIRS})

link_directories(${ASSIMP_LIBRARY_DIRS})
link_directories(${GLM_LIBRARY_DIRS})
link_directories(${CMAKE_BINARY_DIR}/dependencies/lib)


# ==============================================================================
# resources
# ==============================================================================

cmrc_add_resource_library(resources res/default_configuration.xml)

# ==============================================================================
# cmake targets
# ==============================================================================

set(SRCS_NOMAIN 
  src/log.cpp
  src/config.cpp
  src/shader.cpp
  src/mesh.cpp
  src/texture.cpp
  src/view_raytracer.cpp
  src/view_opengl.cpp
  
  inc/config.h
  inc/exceptions.h
  inc/log.h
  inc/shader.h
  inc/mesh.h
  inc/texture.h
  inc/view_raytracer.h
  inc/view_opengl.h
  )

add_library (${PROJECT_NAME} STATIC ${SRCS_NOMAIN})

target_link_libraries(${PROJECT_NAME}
  ${Boost_LIBRARIES} pugixml SDL2pp ${SDL2IMAGE_LIBRARIES} ${SDL2_LIBRARIES} resources ${CMAKE_THREAD_LIBS_INIT} ${ASSIMP_LIBRARIES}
)

target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES}) 

add_executable(demo_app src/main.cpp)
target_link_libraries(demo_app ${PROJECT_NAME})

add_dependencies(${PROJECT_NAME} sdl2-dependency)
add_dependencies(${PROJECT_NAME} pugixml-dependency)

# ==============================================================================
# tests
# ==============================================================================
file(GLOB TEST_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} tests/*.cpp)

foreach(testSrc ${TEST_SRCS})
  get_filename_component(testName ${testSrc} NAME_WE)
  set(testName "test_${testName}")
  add_executable(${testName} "${testSrc}")
  target_link_libraries(${testName} ${PROJECT_NAME})

  set_target_properties(${testName} PROPERTIES 
      RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_BINARY_DIR}/tests)

  add_test(NAME ${testName} 
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMAND ${CMAKE_CURRENT_BINARY_DIR}/tests/${testName} )
endforeach(testSrc)