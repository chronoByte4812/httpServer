add_library(HttpServerSrc-King STATIC
    HttpServer.cpp
    HttpRequest.cpp
    HttpResponse.cpp
    HttpServer.hpp
    HttpRequest.hpp
    HttpResponse.hpp
    util/HttpMethod.hpp
    util/HttpStatus.hpp
    util/HttpVersion.hpp
    util/MimeType.hpp
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

target_include_directories(HttpServerSrc-King PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/util
)