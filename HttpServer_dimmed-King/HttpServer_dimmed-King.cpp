// HttpServer_dimmed-King.cpp : Defines the entry point for the application.
// ...
// Edited on ...
//

// #define DEBUG_MODE
// More #ifdef controls added later...

#include "HttpMethod.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "HttpServer.hpp"
#include "HttpStatus.hpp"
#include "json.h"
#include "MimeType.hpp"

#include <chrono>
#include <ctime>
#include <exception>
#include <filesystem>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

namespace fs = std::filesystem;
using json = nlohmann::json;
static std::mutex mutexLogging;
std::string Page404 = "<h3 style='color: red;'>404 - File Not found</h3>";
std::string PageEmpty = "<h3 style='color: red;'>404 - File was found but is empty</h3>";
std::string Page403 = "<h3 style='color: red;'>403 - File Forbidden</h3>";
std::string Server_IP = "0.0.0.0";
int Server_Port = 80;
HttpServer httpServer_King;

class ServerUtils
{
public:
    static std::string readFile(const std::string &filePath)
    {
        std::ostringstream contents;
        std::ifstream file(filePath, std::ios::in | std::ios::binary);

        if (!file)
            return "";

        contents << file.rdbuf();
        return contents.str();
    };

    static void Write_log(const std::string &logType, const std::string &message)
    {

        std::lock_guard<std::mutex> lock(mutexLogging);
        std::time_t now_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm now_tm;

#ifdef _WIN32
        localtime_s(&now_tm, &now_time_t);
#else
        localtime_r(&now_time_t, &now_tm);
#endif

        std::ostringstream currentTime;
        currentTime << std::put_time(&now_tm, "%d-%m-%y %H:%M:%S");
        std::string formattedLog = std::format("[{}] [{}] - {}", currentTime.str(), logType, message);

        std::cout << formattedLog << std::endl;
    };
};

int main(int argc, char *argv[])
{

    httpServer_King.use(R"(/.*)", HttpMethod::GET, [&](const HttpRequest &req, HttpResponse &res)
                        {
            std::string clientIp = req.getRemoteAddr();
            std::string path = req.getPath();
            std::string filePath = path == "/" ? fs::current_path().string() + "/index.html" : fs::current_path().string() + path;
            std::string file_contents = ServerUtils::readFile(filePath);
            std::string content_type = MimeType::getMimeType(filePath);
            std::string method = HttpMethod::toString(req.getMethod());
            bool isBlackListed = false;
            HttpStatus::Code statusCode = HttpStatus::Code::InternalServerError;

            if (fs::exists(filePath)) // File is found
            {
                if (file_contents.empty() && !fs::is_directory(filePath)) // File is empty
                {
                    res.setStatus(HttpStatus::Code::NotFound);
                    res.setHeader("Content-Type", "text/html");
                    res.send(PageEmpty);

                    statusCode = HttpStatus::Code::NotFound;
                }
                else if (fs::exists(filePath) && fs::is_directory(filePath)) // File is a directory
                {
                    res.setStatus(HttpStatus::Code::NotFound);
                    res.setHeader("Content-Type", "text/html");
                    res.send(Page404);

                    statusCode = HttpStatus::Code::NotFound;
                }
                else // File is ok and ready
                {
                    res.setStatus(HttpStatus::Code::OK);
                    res.setHeader("Content-Type", content_type);
                    res.send(file_contents);

                    statusCode = HttpStatus::Code::OK;
                };
            }
            else // File is not found
            {
                res.setStatus(HttpStatus::Code::NotFound);
                res.setHeader("Content-Type", "text/html");
                res.send(Page404);

                statusCode = HttpStatus::Code::NotFound;
            };

            ServerUtils::Write_log("INFO", std::format("Client {} {} {} {}", clientIp, method, path, static_cast<int>(statusCode)));
    });

    try
    {
        ServerUtils::ServerUtils::Write_log("INFO", std::format("Server listening on http://{}:{}", Server_IP, Server_Port));

        httpServer_King.listen(Server_IP.c_str(), Server_Port);
    }
    catch (std::exception &error)
    {
        ServerUtils::Write_log("ERROR", std::format("An error occured while running the server: {}", error.what()));
    }
};