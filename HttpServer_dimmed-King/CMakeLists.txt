add_executable(HttpServer_dimmed-King "HttpServer_dimmed-King.cpp" "../json.h")

if (MSVC)
    add_definitions(-DNOMINMAX)
endif()

target_link_libraries(HttpServer_dimmed-King PRIVATE HttpServerSrc-King)
find_package(OpenSSL REQUIRED)
target_link_libraries(HttpServer-King PUBLIC OpenSSL::SSL OpenSSL::Crypto)
target_compile_features(HttpServer-King PUBLIC cxx_std_23)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(TARGET HttpServer_dimmed-King PROPERTY CXX_STANDARD 23)