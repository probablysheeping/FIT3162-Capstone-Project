cmake_minimum_required(VERSION 3.20)
project(ConvexPolygonIoU)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(SFML 3 REQUIRED COMPONENTS System Window Graphics)

# Add OpenGL for macOS
if(APPLE)
    find_package(OpenGL REQUIRED)
endif()

# ImGui source files
set(IMGUI_SOURCES
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui-SFML.cpp
)

# Project source files
set(PROJECT_SOURCES
    main.cpp
    polygon.cpp
    saving.cpp
    filelocationchooser.cpp
    logging.cpp
    ${IMGUI_SOURCES}
)

# Enable Objective-C++ for macOS file dialogs
if(APPLE)
    set_source_files_properties(filelocationchooser.cpp PROPERTIES
        COMPILE_FLAGS "-x objective-c++"
    )
endif()

# Project headers
set(PROJECT_HEADERS
    vectordefs.h
    polygon.h
    saving.h
    filelocationchooser.h
    logging.h
    imgui/imgui.h
    imgui/imgui-SFML.h
    imgui/imgui_internal.h
    imgui/imconfig.h
)

# Create the executable
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${PROJECT_HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)

# Link libraries
if(APPLE)
    target_link_libraries(${PROJECT_NAME} 
        SFML::System
        SFML::Window  
        SFML::Graphics
        ${OPENGL_LIBRARIES}
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework Foundation"
    )
else()
    target_link_libraries(${PROJECT_NAME} 
        SFML::System
        SFML::Window
        SFML::Graphics
    )
endif()

# Compiler-specific options
if(APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        GL_SILENCE_DEPRECATION
    )
endif()

# Set output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Copy any resource files if they exist
file(GLOB RESOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.ttf" "${CMAKE_CURRENT_SOURCE_DIR}/*.png")
if(RESOURCE_FILES)
    file(COPY ${RESOURCE_FILES} DESTINATION ${CMAKE_BINARY_DIR}/bin/)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
