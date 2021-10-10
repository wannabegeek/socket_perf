cmake_minimum_required(VERSION 3.17)
project(socket_perf)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckSymbolExists)
INCLUDE(CheckCXXSymbolExists)

find_library(LIBRT_LIBRARIES rt)
IF (LIBRT_LIBRARIES)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LIBRT_LIBRARIES}")
ENDIF()

CHECK_SYMBOL_EXISTS(__GNU_LIBRARY__ "features.h" _GNU_SOURCE)
IF (_GNU_SOURCE)
    add_definitions(-D_GNU_SOURCE)
ENDIF()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)
CHECK_INCLUDE_FILES(pthread.h HAVE_PTHREAD_H)

set(CMAKE_REQUIRED_DEFINITIONS)

IF (DEFINED HAVE_PTHREAD_H)
    list(APPEND CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
    check_symbol_exists(pthread_setaffinity_np pthread.h HAVE_PTHREAD_SETAFFINITY_NP)
    check_symbol_exists(pthread_setname_np pthread.h HAVE_PTHREAD_SETNAME_NP)
    check_symbol_exists(pthread_getname_np pthread.h HAVE_PTHREAD_GETNAME_NP)
    list(REMOVE_ITEM CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
ENDIF()

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/config.h @ONLY)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic -Ofast -Wold-style-cast -Woverloaded-virtual -flto")

add_executable(socket_echo_server tcp/server/socket_echo_server.cpp)
add_executable(socket_echo_client tcp/client/socket_echo_client.cpp tcp/client/socket_processor.cpp tcp/client/socket_processor.h)
add_executable(internal_echo internal/internal_echo.cpp internal/internal_processor.cpp internal/internal_processor.h)
add_executable(jitter cpu_jitter/jitter.cpp)

TARGET_LINK_LIBRARIES(socket_echo_client pthread)
TARGET_LINK_LIBRARIES(internal_echo pthread utils)
TARGET_LINK_LIBRARIES(jitter pthread)

set(SOURCE_FILES utils/optimize.h utils/tfringbuffer.h utils/tfconsole_colors.h utils/tflogger.h utils/tfpool.h utils/tfspinlock.h utils/tfthread_utils.cpp utils/tfthread_utils.h)
add_library(utils STATIC ${SOURCE_FILES})
