cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

# Определяем опции AddressSanitizer
set(ASAN_FLAGS "-fsanitize=address -fno-omit-frame-pointer")

# Проверим, поддерживает ли компилятор AddressSanitizer
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(${ASAN_FLAGS} COMPILER_SUPPORTS_ASAN)

if(COMPILER_SUPPORTS_ASAN)
    message(STATUS "AddressSanitizer поддерживается компилятором")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ASAN_FLAGS}")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${ASAN_FLAGS}")
else()
    message(STATUS "AddressSanitizer не поддерживается компилятором")
endif()


# Включим отладочную информацию и установите уровень оптимизации
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Установим режим сборки по умолчанию в Debug
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

get_property(importTargets DIRECTORY "${CMAKE_SOURCE_DIR}" PROPERTY IMPORTED_TARGETS)
message(STATUS "${importTargets}")

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(game_server
	src/main.cpp
	src/logger.cpp	
	src/logger.h
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/model.cpp
	src/model.h
	src/tagged.h
	src/boost_json.cpp
	src/json_loader.cpp
	src/json_loader.h
	src/request_handler.cpp
	src/request_handler.h
	src/content.cpp
	src/content.h
	src/aux.cpp	
	src/aux.h

)
target_include_directories(game_server PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server PRIVATE Threads::Threads CONAN_PKG::boost)