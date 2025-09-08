add_library(HttpServerSrc-King STATIC
    HttpServer.cpp
    HttpRequest.cpp
    HttpResponse.cpp
    WebSocket.cpp
    util/Base64.cpp
    WebSocket.hpp
    Common.hpp
    HttpServer.hpp
    HttpRequest.hpp
    HttpResponse.hpp
    util/HttpMethod.hpp
    util/HttpStatus.hpp
    util/HttpVersion.hpp
    util/MimeType.hpp
    util/Base64.hpp
)

# https://github.com/DarkGamerYT/http-server :3

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)
target_link_libraries(HttpServerSrc-King PUBLIC OpenSSL::SSL OpenSSL::Crypto)

target_include_directories(HttpServerSrc-King PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/util
)