# CMakeList.txt : CMake project for HttpServer-King, include source and define
# project specific logic here.
#

cmake_minimum_required (VERSION 3.16)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("HttpServer-King")

if (MSVC)
    add_definitions(-DNOMINMAX)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Add source to this project's executable.
add_executable(HttpServer-King "HttpServer-King.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET HttpServer-King PROPERTY CXX_STANDARD 23)
endif()

find_package(OpenSSL REQUIRED)
target_link_libraries(HttpServer-King PUBLIC OpenSSL::SSL OpenSSL::Crypto)
target_compile_features(HttpServer-King PUBLIC cxx_std_23)

add_subdirectory("HttpServerSrc-King")

target_link_libraries(HttpServer-King PRIVATE HttpServerSrc-King)

add_subdirectory("HttpServer_dimmed-King")