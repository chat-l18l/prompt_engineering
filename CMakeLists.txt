cmake_minimum_required(VERSION 3.16)
project(opus_lan_chat C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes)

find_package(PkgConfig REQUIRED)
pkg_check_modules(OPUS REQUIRED opus)
pkg_check_modules(ALSA REQUIRED alsa)

add_executable(opus_lan_chat
    src/opus_lan_chat.c
)

target_include_directories(opus_lan_chat PRIVATE
    ${OPUS_INCLUDE_DIRS}
    ${ALSA_INCLUDE_DIRS}
)

target_link_directories(opus_lan_chat PRIVATE
    ${OPUS_LIBRARY_DIRS}
    ${ALSA_LIBRARY_DIRS}
)

target_link_libraries(opus_lan_chat PRIVATE
    ${OPUS_LIBRARIES}
    ${ALSA_LIBRARIES}
)

target_compile_options(opus_lan_chat PRIVATE
    ${OPUS_CFLAGS_OTHER}
    ${ALSA_CFLAGS_OTHER}
)
