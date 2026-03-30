cmake_minimum_required(VERSION 3.16)
project(PhysicsExperiment)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)
find_program(GLSLC_EXECUTABLE glslc)
find_program(GLSLANG_VALIDATOR_EXECUTABLE glslangValidator)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
add_executable(app ${APP_SOURCES})

target_include_directories(app PRIVATE src)

target_link_libraries(app PRIVATE Vulkan::Vulkan)
if (UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_include_directories(app PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(app PRIVATE ${X11_LIBRARIES})
    if (X11_Xext_LIB)
        target_link_libraries(app PRIVATE ${X11_Xext_LIB})
    endif()
endif()

file(GLOB_RECURSE VULKAN_SHADER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/shaders/vulkan/*.vert"
    "${CMAKE_SOURCE_DIR}/shaders/vulkan/*.frag")

set(SHADER_OUTPUTS)

if (GLSLC_EXECUTABLE OR GLSLANG_VALIDATOR_EXECUTABLE)
    foreach(SHADER_SOURCE ${VULKAN_SHADER_SOURCES})
        file(RELATIVE_PATH SHADER_RELATIVE_PATH "${CMAKE_SOURCE_DIR}/shaders" "${SHADER_SOURCE}")
        set(SHADER_OUTPUT "${CMAKE_BINARY_DIR}/shaders/${SHADER_RELATIVE_PATH}.spv")
        get_filename_component(SHADER_OUTPUT_DIR "${SHADER_OUTPUT}" DIRECTORY)

        if (GLSLC_EXECUTABLE)
            add_custom_command(
                OUTPUT ${SHADER_OUTPUT}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
                COMMAND ${GLSLC_EXECUTABLE} ${SHADER_SOURCE} -o ${SHADER_OUTPUT}
                DEPENDS ${SHADER_SOURCE}
                COMMENT "Compiling shader ${SHADER_RELATIVE_PATH}"
                VERBATIM)
        else()
            add_custom_command(
                OUTPUT ${SHADER_OUTPUT}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_OUTPUT_DIR}
                COMMAND ${GLSLANG_VALIDATOR_EXECUTABLE} -V ${SHADER_SOURCE} -o ${SHADER_OUTPUT}
                DEPENDS ${SHADER_SOURCE}
                COMMENT "Compiling shader ${SHADER_RELATIVE_PATH}"
                VERBATIM)
        endif()

        list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT})
    endforeach()

    add_custom_target(compile_shaders DEPENDS ${SHADER_OUTPUTS})
else()
    add_custom_target(compile_shaders
        COMMAND ${CMAKE_COMMAND} -E echo "No shader compiler found. Install glslc or glslangValidator to build SPIR-V shaders."
        VERBATIM)
endif()
