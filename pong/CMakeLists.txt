cmake_minimum_required(VERSION 3.20)
project(pong C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.12
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(SDL3)

add_executable(pong
    src/main.c
    src/constants.c
    src/ball.c
    src/paddle.c
    src/game.c
    src/renderer.c
)
target_link_libraries(pong PRIVATE SDL3::SDL3)

# Copy the SDL3 DLL next to the exe so `pong.exe` runs without installing SDL system-wide.
if(WIN32)
    add_custom_command(TARGET pong POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3>
            $<TARGET_FILE_DIR:pong>
    )
endif()
