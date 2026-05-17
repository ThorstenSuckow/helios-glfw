cmake_minimum_required(VERSION 4.0)
project(helios-glfw LANGUAGES C CXX)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

include(CTest)
include(FetchContent)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(CMAKE_IFC_OUTPUT_DIRECTORY)
    file(MAKE_DIRECTORY "${CMAKE_IFC_OUTPUT_DIRECTORY}")
endif()

set(HELIOS_ENGINE_LOCAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../helios-engine" CACHE PATH "Local path to helios-engine repository")
set(HELIOS_ECS_LOCAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../helios-ecs" CACHE PATH "Local path to helios-ecs repository")
if(NOT TARGET helios::engine)
    if(EXISTS "${HELIOS_ENGINE_LOCAL_PATH}/CMakeLists.txt")
        add_subdirectory("${HELIOS_ENGINE_LOCAL_PATH}" "${CMAKE_BINARY_DIR}/_deps/helios-engine-build")
    else()
        FetchContent_Declare(helios_engine
                GIT_REPOSITORY https://github.com/thorstensuckow/helios-engine.git
                GIT_TAG main
        )
        FetchContent_MakeAvailable(helios_engine)
    endif()
endif()

if(EXISTS "${HELIOS_ECS_LOCAL_PATH}/src/_module.ixx")
    set(HELIOS_ECS_SOURCE_DIR "${HELIOS_ECS_LOCAL_PATH}")
else()
    FetchContent_Declare(helios_ecs
            GIT_REPOSITORY https://github.com/thorstensuckow/helios-ecs.git
            GIT_TAG main
    )
    FetchContent_GetProperties(helios_ecs)
    if(NOT helios_ecs_POPULATED)
        FetchContent_Populate(helios_ecs)
    endif()
    set(HELIOS_ECS_SOURCE_DIR "${helios_ecs_SOURCE_DIR}")
endif()

if(NOT TARGET thirdparty::glfw OR NOT TARGET thirdparty::glad)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.4
    )
    FetchContent_Declare(glad
            GIT_REPOSITORY https://github.com/Dav1dde/glad.git
            GIT_TAG v2.0.8
            SOURCE_SUBDIR cmake
    )
    FetchContent_MakeAvailable(glfw glad)

    if(NOT TARGET thirdparty::glad)
        glad_add_library(glad_gl_core_46 STATIC REPRODUCIBLE LOADER API gl:core=4.6)
        add_library(thirdparty::glad ALIAS glad_gl_core_46)
    endif()

    if(NOT TARGET thirdparty::glfw)
        add_library(thirdparty::glfw ALIAS glfw)
    endif()
endif()

file(GLOB_RECURSE CPP_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

file(GLOB_RECURSE MODULE_INTERFACE_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.ixx")

list(FILTER CPP_FILES EXCLUDE REGEX "/CMakeFiles/|/CMakeTmp/|/cmake_build_|/build/")
list(FILTER MODULE_INTERFACE_FILES EXCLUDE REGEX "/CMakeFiles/|/CMakeTmp/|/cmake_build_|/build/")

add_library(helios_glfw STATIC)
add_library(helios::glfw ALIAS helios_glfw)
set_property(TARGET helios_glfw PROPERTY CXX_SCAN_FOR_MODULES ON)
target_compile_features(helios_glfw PUBLIC cxx_std_23)

target_sources(helios_glfw PRIVATE ${CPP_FILES})

if(MODULE_INTERFACE_FILES)
    target_sources(helios_glfw PUBLIC
            FILE_SET modules TYPE CXX_MODULES
            BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src"
            FILES ${MODULE_INTERFACE_FILES}
    )
endif()

target_link_libraries(helios_glfw
        PUBLIC
        $<BUILD_INTERFACE:helios::engine>
        $<INSTALL_INTERFACE:helios::engine>
        PRIVATE
        $<BUILD_INTERFACE:thirdparty::glad>
        $<BUILD_INTERFACE:thirdparty::glfw>
)

target_include_directories(helios_glfw PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${HELIOS_ECS_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

set(HELIOS_GLFW_ENABLE_PACKAGE_DEFAULT OFF)
if(PROJECT_IS_TOP_LEVEL)
    set(HELIOS_GLFW_ENABLE_PACKAGE_DEFAULT ON)
endif()
option(HELIOS_GLFW_ENABLE_PACKAGE "Enable install/export package files" ${HELIOS_GLFW_ENABLE_PACKAGE_DEFAULT})

if(HELIOS_GLFW_ENABLE_PACKAGE)
    install(TARGETS helios_glfw
            EXPORT helios-glfwTargets
            ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            FILE_SET modules DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    )

    set(HELIOS_GLFW_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/helios-glfw")

    configure_package_config_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/cmake/helios-glfwConfig.cmake.in"
            "${CMAKE_CURRENT_BINARY_DIR}/helios-glfwConfig.cmake"
            INSTALL_DESTINATION "${HELIOS_GLFW_INSTALL_CMAKEDIR}"
    )

    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/helios-glfwConfigVersion.cmake"
            VERSION "0.1.0"
            COMPATIBILITY SameMajorVersion
    )

    install(EXPORT helios-glfwTargets
            FILE helios-glfwTargets.cmake
            NAMESPACE helios::
            DESTINATION "${HELIOS_GLFW_INSTALL_CMAKEDIR}"
    )

    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/helios-glfwConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/helios-glfwConfigVersion.cmake"
            DESTINATION "${HELIOS_GLFW_INSTALL_CMAKEDIR}"
    )
endif()

if(BUILD_TESTING)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.17.0
    )
    FetchContent_MakeAvailable(googletest)

    file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.test.cpp")

    add_executable(helios-glfw-tests ${TEST_SOURCES})
    target_link_libraries(helios-glfw-tests PRIVATE helios_glfw GTest::gtest_main)

    include(GoogleTest)
    gtest_discover_tests(helios-glfw-tests)
endif()
