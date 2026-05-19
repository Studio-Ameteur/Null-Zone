cmake_minimum_required(VERSION 3.10)
project(NullZone CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_definitions(-DWINVER=0x0501 -D_WIN32_WINNT=0x0501)
    add_definitions(-DSDL_MAIN_HANDLED)
endif()

set(SOURCES
    src/main.cpp
    NullZone.rc
)

add_executable(NullZone WIN32 ${SOURCES})

target_include_directories(NullZone PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/SDL2/furto
)

if(MINGW)
    target_link_directories(NullZone PRIVATE
        ${CMAKE_SOURCE_DIR}/SDL2/lib
    )
    target_link_libraries(NullZone PRIVATE
        mingw32
        SDL2main
        SDL2
        SDL2_mixer
        SDL2_ttf
        SDL2_image
        freetype
        harfbuzz
        graphite2
        brotlidec
        brotlicommon
        bz2
        png
        z
        jpeg
        dwrite
        rpcrt4
        opengl32
        gdi32
        winmm
        ole32
        shell32
        version
        imm32
        setupapi
        -static-libgcc
        -static-libstdc++
        -static
        -lpthread
    )
    target_link_options(NullZone PRIVATE
        -Wl,--subsystem,windows
    )
endif()

target_compile_options(NullZone PRIVATE
    -O2
    -Wall
    -Wno-unused-variable
    -Wno-unused-function
    -mwindows
)

set_target_properties(NullZone PROPERTIES
    OUTPUT_NAME "NullZone"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_custom_command(TARGET NullZone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/COPYRIGHT.txt
        ${CMAKE_BINARY_DIR}/bin/COPYRIGHT.txt
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/CREDITS.txt
        ${CMAKE_BINARY_DIR}/bin/CREDITS.txt
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/bin/assets
)
