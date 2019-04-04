
# ==============================================================================
# cmake setup
# ==============================================================================

cmake_minimum_required(VERSION 3.0)
project(light-welter)

include(CTest)
include(ExternalProject)
include(CMakeRC.cmake)
include(FindPkgConfig)

enable_testing()

# ==============================================================================
# compiler setup
# ==============================================================================

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  add_compile_options(-fno-limit-debug-info)
endif()
add_compile_options(-Wall)

include_directories(inc)
include_directories(${CMAKE_BINARY_DIR}/gen)
include_directories(${CMAKE_BINARY_DIR}/dependencies/include)

# ==============================================================================
# external dependencies
# ==============================================================================

find_package(Threads)
find_package(Boost COMPONENTS system filesystem unit_test_framework REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

ExternalProject_Add(sdl2-dependency
  URL https://github.com/libSDL2pp/libSDL2pp/archive/0.16.0.tar.gz
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/dependencies -DSDL2PP_WITH_MIXER=OFF -DSDL2PP_WITH_IMAGE=ON -DSDL2PP_STATIC=OFF -DSDL2PP_WITH_EXAMPLES=OFF -DSDL2PP_WITH_TESTS=OFF -DSDL2PP_ENABLE_LIVE_TESTS=OFF)

ExternalProject_Add(pugixml-dependency
  URL https://github.com/zeux/pugixml/releases/download/v1.9/pugixml-1.9.tar.gz
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/dependencies
)

ExternalProject_Add(spdlog-dependency
  URL https://github.com/gabime/spdlog/archive/v1.1.0.tar.gz
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/dependencies -DSPDLOG_BUILD_EXAMPLES=OFF -DSPDLOG_BUILD_BENCH=OFF -DSPDLOG_BUILD_TESTING=OFF -DSPDLOG_BUILD_TESTING=OFF
)

ExternalProject_Add(glm-dependency
  URL https://github.com/g-truc/glm/releases/download/0.9.9.4/glm-0.9.9.4.zip
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/dependencies -DGLM_TEST_ENABLE=OFF -DBUILD_SHARED_LIBS=OFF
)

find_package(SDL2 REQUIRED)
PKG_SEARCH_MODULE(SDL2IMAGE REQUIRED SDL2_image>=2.0.0)
find_package(OpenGL REQUIRED)                                                          
find_package(GLEW REQUIRED)
find_package(assimp REQUIRED)

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
# workarounds
# ==============================================================================

add_compile_options(-DGLM_FORCE_XYZW_ONLY)

# ==============================================================================
# build config
# ==============================================================================

set(SRCS_NOMAIN 
  src/log.cpp
  src/config.cpp
  src/shader.cpp
  src/mesh.cpp
  src/texture.cpp
  src/view_raytracer.cpp
  src/view_opengl.cpp
  src/raycaster.cpp
  src/pathtracer.cpp
  
  inc/config.h
  inc/exceptions.h
  inc/log.h
  inc/shader.h
  inc/mesh.h
  inc/texture.h
  inc/pathtracer.h
  inc/raycaster.h
  inc/view_raytracer.h
  inc/view_opengl.h
  )

add_library (${PROJECT_NAME} STATIC ${SRCS_NOMAIN})

target_link_libraries(${PROJECT_NAME}
  ${Boost_LIBRARIES} pugixml SDL2pp resources ${SDL2IMAGE_LIBRARIES} ${SDL2_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${ASSIMP_LIBRARIES}
)

target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES}) 

add_executable(demo_app src/main.cpp)
target_link_libraries(demo_app ${PROJECT_NAME})

add_dependencies(${PROJECT_NAME} sdl2-dependency)
add_dependencies(${PROJECT_NAME} pugixml-dependency)
add_dependencies(${PROJECT_NAME} spdlog-dependency)
add_dependencies(${PROJECT_NAME} glm-dependency)

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