// HttpServer_dimmed-King.cpp : Defines the entry point for the application.
//

#include "../HttpServerSrc-King/HttpServer.hpp"
#include "../HttpServerSrc-King/util/HttpMethod.hpp"
#include "../HttpServerSrc-King/util/MimeType.hpp"
#include "../json.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;
using json = nlohmann::json;
std::string Page404 = "<h3 style='color: red;'>404 - File Not found or file is empty</h3>";
std::string PageEmpty = "<h3 style='color: red;'>404 - File was found but is empty</h3>";
std::string Page403 = "<h3 style='color: red;'>403 - File Forbidden</h3>";
std::string Server_IP = "0.0.0.0";
int Server_Port = 80;
HttpServer KingHttpServer;

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
    static const std::unordered_map<std::string, std::string> validTypes = {
        {"INFO", "\033[32m"},
        {"WARNING", "\033[33m"},
        {"ERROR", "\033[31m"},
        {"DEBUG", "\033[36m"}
    };

    std::time_t now_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm now_tm;

#ifdef _WIN32
    localtime_s(&now_tm, &now_time_t);
#else
    localtime_r(&now_time_t, &now_tm);
#endif

    std::ostringstream currentTime;
    currentTime << std::put_time(&now_tm, "%d-%m-%y %H:%M:%S");

    std::string formattedLog = "[" + currentTime.str() + "] [" + logType + "] - " + message + "\n";
    std::cout << validTypes.at(logType) << formattedLog << std::endl;
};

int main(int argc, char *argv[])
{
    KingHttpServer.use("/", HttpMethod::GET, [&](HttpRequest req, HttpResponse& res, const NextFn& next) {
        std::string clientIp = req.getRemoteAddr();
        std::string path = req.getPath();
        std::string filePath = fs::current_path().string() + "/index.html";
        std::string ReadFile = readFile(filePath);
        std::string content_type = MimeType::getMimeType(filePath);
        std::string method = HttpMethod::toString(req.getMethod());

        if (!fs::exists(filePath) || ReadFile.empty()) { // File doesn't exist or file is empty.

            Write_log("INFO", "Client " + clientIp + " " + method + " " + path + " (404 Not Found)");

            res.setStatus(HttpStatus::NotFound);
            res.setHeader("Content-Type", "text/html");
            res.send(Page404);
        }
        else { // The file exists.
            Write_log("INFO", "Client " + clientIp + " " + method + " " + path + " (200 OK)");

            res.setStatus(HttpStatus::OK);
            res.setHeader("Content-Type", content_type);
            res.send(ReadFile);
        }
    });

    KingHttpServer.use(R"(/.*)", HttpMethod::GET, [&](const HttpRequest req, HttpResponse& res, const NextFn& next)
        {
            std::string clientIp = req.getRemoteAddr();
            std::string path = req.getPath();
            std::string filePath = path == "/" ? fs::current_path().string() + "/index.html" : fs::current_path().string() + path;
            std::string ReadFile = readFile(filePath);
            std::string content_type = MimeType::getMimeType(filePath);
            std::string method = HttpMethod::toString(req.getMethod());

            if (ReadFile.empty()) // File found but is empty
            {
                Write_log("INFO", "Client " + clientIp + " " + method + " " + path + " (File found but empty)");

                res.setStatus(HttpStatus::NotFound);
                res.setHeader("Content-Type", "text/html");
                res.send(PageEmpty);
            }
            else if (!fs::exists(filePath)) { // File doesn't exist
                Write_log("INFO", "Client " + clientIp + " " + method + " " + path + " (404 Not Found)");

                res.setStatus(HttpStatus::NotFound);
                res.setHeader("Content-Type", "text/html");
                res.send(Page404);
            }
            else { // The file exists.
                Write_log("INFO", "Client " + clientIp + " " + method + " " + path + " (200 OK)");

                res.setStatus(HttpStatus::OK);
                res.setHeader("Content-Type", content_type);
                res.send(ReadFile);
            }
        });

    try
    {
        Write_log("INFO", "Server listening on " + Server_IP + ":" + std::to_string(Server_Port) + (Server_IP == "0.0.0.0" ? " All network interfaces." : ""));
        KingHttpServer.listen(Server_IP.c_str(), Server_Port);
    }
    catch (const std::exception &error)
    {
        std::cerr << "An error occured while running the server: " << error.what() << std::endl;
    };
};