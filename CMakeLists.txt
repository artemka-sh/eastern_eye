cmake_minimum_required(VERSION 4.0) # C++23 требует минимум 3.20+, 3.22 — золотой стандарт

project(eastern_eye VERSION 1.0)

# --- 1. Глобальные настройки проекта ---
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Настройка ccache для ускорения пересборки
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

# --- 2. Поиск зависимостей ---
find_package(OpenCV REQUIRED COMPONENTS core imgproc video tracking)
add_subdirectory(deps/reflect-cpp)

set(HTTPLIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/deps/cpp-httplib/httplib.h")
if(NOT EXISTS "${HTTPLIB_PATH}")
    file(DOWNLOAD "https://raw.githubusercontent.com/yhirose/cpp-httplib/refs/heads/master/httplib.h" "${HTTPLIB_PATH}")
endif()
# --- 3. Определение исходников ---
file(GLOB_RECURSE SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

# --- 4. Создание исполняемого файла ---
add_executable(${PROJECT_NAME} ${SRC_FILES})

target_precompile_headers(${PROJECT_NAME} PRIVATE deps/cpp-httplib/httplib.h)

# --- 5. Настройка параметров таргета (инклуды и опции) ---
target_include_directories(${PROJECT_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${OpenCV_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/deps/cpp-httplib/
)

target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
        -march=native
        -mtune=native
        -O3
        -ffast-math
        -Wall
        -Wextra
        >
)

# --- 6. Линковка библиотек ---
# Переменная ${OpenCV_LIBS} уже содержит все найденные компоненты (включая tracking)
target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS} reflectcpp)

# --- 6. Создание симлинков для build-а ---
execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/res ${CMAKE_BINARY_DIR}/res
)
