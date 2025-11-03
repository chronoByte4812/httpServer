// HttpServer-King.cpp : Defines the entry point for the application.
// Wednesday, 26 August 2025, 11:13 PM
// Edited on Sunday, 02 November 2025, 6:36 PMZ
// Dimmed to be heavily refactored later
//

// #define DEBUG_MODE
//  More #ifdef controls added later...

#include "./HttpServerSrc-King/HttpServer.hpp"
#include "./HttpServerSrc-King/util/HttpMethod.hpp"
#include "./HttpServerSrc-King/util/MimeType.hpp"
#include "./json.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>

namespace fs = std::filesystem;
using json = nlohmann::json;
std::string Page404 = "<h3 style='color: red;'>404 - File Not found</h3>";
std::string PageEmpty = "<h3 style='color: red;'>404 - File was found but is empty</h3>";
std::string Page403 = "<h3 style='color: red;'>403 - File Forbidden</h3>";
std::string Server_IP = "0.0.0.0";
std::string Server_Config_Path = (fs::current_path() / "ServerConfig.json").string();
std::vector<std::string> BlackListPaths;
bool useConfig = true;
bool useFileLogging = true;
bool hidePublicIp = false;
int Server_Port = 6432;
HttpServer httpServer_King;
std::regex privateIpRegexp(R"(^(?:10\.(?:25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]?[0-9])\.(?:25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]?[0-9])\.(?:25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]?[0-9])|192\.168\.(?:25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]?[0-9])\.(?:25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]?[0-9])|172\.(?:1[6-9]|2[0-9]|3[0-1])\.(?:25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]?[0-9])\.(?:25[0-5]|2[0-4][0-9]|1[0-9]{2}|[0-9]?[0-9]))$)");
std::array version = {
    1,
    0,
    0
};

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
    std::ofstream logFile("./ServerLogs.log", std::ios::app);
    std::tm now_tm;

#ifdef _WIN32
    localtime_s(&now_tm, &now_time_t);
#else
    localtime_r(&now_time_t, &now_tm);
#endif

    std::ostringstream currentTime;
    currentTime << std::put_time(&now_tm, "%d-%m-%y %H:%M:%S");

    std::string formattedLog = std::format("[{}] [{}] - {} \n", currentTime.str(), logType, message);
    std::cout << validTypes.at(logType) << formattedLog << "\u001b[0m" << std::endl;

    if (useFileLogging == true)
        logFile << formattedLog;

    logFile.close();
};

static void handleConfig()
{
    if (fs::exists(Server_Config_Path) && readFile(Server_Config_Path).empty() == false)
    {
        try
        {
            Write_log("INFO", "Config found!");

            std::ifstream dataFile(Server_Config_Path);
            json data = json::parse(dataFile);
            std::string Page404Custom = data.value("Page404Custom", "");
            std::string Page403Custom = data.value("Page403Custom", "");
            Server_IP = data.value("ip", Server_IP);
            Server_Port = data.value("port", Server_Port);
            BlackListPaths = data.value("BlackListPaths", BlackListPaths);
            useFileLogging = data.value("useFileLogging", useFileLogging);
            hidePublicIp = data.value("hidePublicIp", hidePublicIp);

            if (data.contains("MimeTypesCustom") && data["MimeTypesCustom"].is_object() && data["MimeTypesCustom"].size() > 0)
            {
                Write_log("INFO", "Custom mime types loaded.");

                MimeType::sMimeTypeMap.clear();

                for (auto &[ext, type] : data["MimeTypesCustom"].items())
                {
                    MimeType::sMimeTypeMap[ext] = type;
                };
            };

            if (!Page404Custom.empty())
            {
                if (fs::exists(Page404Custom))
                    Page404 = readFile(Page404Custom);
                else
                    Write_log("WARNING", "The provided 404 page was not found");
            };

            if (!Page403Custom.empty())
            {
                if (fs::exists(Page403Custom))
                    Page403 = readFile(Page403Custom);
                else
                    Write_log("WARNING", "The provided 403 page was not found");
            };

            Write_log("INFO", std::format("Custom 404 page is {}", fs::exists(Page404Custom) ? "enabled" : "disabled"));
            Write_log("INFO", std::format("Custom 403 page is {}", fs::exists(Page403Custom) ? "enabled" : "disabled"));
            Write_log("INFO", std::format("Logging output to file is {}", useFileLogging ? "enabled" : "disabled"));
            Write_log("INFO", std::format("Redact public IP logging is {}", hidePublicIp ? "enabled" : "disabled"));
        }
        catch (json::parse_error error)
        {
            Write_log("ERROR", std::format("Failed to parse config data: {}", error.what()));
        };
    }
    else
    {
        Write_log("INFO", "A server config data was not found. A new one has been written");
        std::ofstream file("ServerConfig.json");

        json configData = json::parse(R"(
			{
                "helpInfo": {
                    "BlackListPath": "An array that contains files and or paths that are forbidden to access",
                    "MimeTypesCustom": "An array that contains custom defined file mime types",
                    "useFileLogging": "A boolean to tell the server to write to a log file on the disk or not",
                    "Page403Custom": "A string path that tells the server to use a custom 403 page, avoid a forward slash at char[0]",
                    "Page404Custom": "A string path that tells the server to use a custom 403 page, avoid a forward slash at char[0]",
                    "ip": "A string that tells the server where to bind, use 0.0.0.0 to bind to all interfaces",
                    "port": "A number that tells the server what port to bind to, 80 is the standard for HTTP",
                    "hidePublicIp": "Tells the server to hide the public IP addresses of clients for privacy reasons"
                },
                "BlackListPaths": [
                    "/ServerConfig.json",
                    "/ServerLogs.log"
            ],
                "MimeTypesCustom": { },
                "useFileLogging": true,
                "hidePublicIp": false,
                "Page403Custom": "",
                "Page404Custom": "",
                "ip": "0.0.0.0",
                "port": 6432
            }
		)");

        file << configData.dump(4);
        file.close();
    };
};

int main(int argc, char *argv[])
{

#ifdef DEBUG_MODE
    Write_log("DEBUG", "Debug mode is enabled");
#endif

    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
        {
            char *arg = argv[i];

            if (std::strcmp(arg, "-help") == 0)
            {
                Write_log("INFO", "=======Help=======");
                Write_log("INFO", " -nc Flag that stops the server from using or creating a config file");
                Write_log("INFO", " -nl Flag that stopsa the server from writing to a log file");
                Write_log("INFO", "==================");

                return 0;
            };

            if (std::strcmp(arg, "-v") == 0)
            {
                Write_log("INFO", std::format("King HTTP Server Version: {}.{}.{}", version[0], version[1], version[2]));

                return 0;
            };

            if (std::strcmp(arg, "--nc") == 0)
                useConfig = false;

            if (std::strcmp(arg, "--nl") == 0)
                useFileLogging = false;
        };
    };

    if (useConfig == true)
        handleConfig();

    httpServer_King.use(R"(/.*)", HttpMethod::GET, [&](const HttpRequest &req, HttpResponse &res)
                        {
            std::string clientIp = req.getRemoteAddr();
            std::string path = req.getPath();
            std::string filePath = path == "/" ? fs::current_path().string() + "/index.html" : fs::current_path().string() + path;
            std::string file_contents = readFile(filePath);
            std::string content_type = MimeType::getMimeType(filePath);
            std::string method = HttpMethod::toString(req.getMethod());
            bool isBlackListed = false;
            HttpStatus::Code statusCode = HttpStatus::Code::InternalServerError;

            if (!std::regex_match(clientIp, privateIpRegexp) && hidePublicIp)
                clientIp = "(IP hidden)";

            for (const std::string& blacklistedPath : BlackListPaths)
            {
                if (path.rfind(blacklistedPath, 0) == 0)
                {
                    isBlackListed = true;
                    break;
                };
            };

			if (isBlackListed) { // File is blacklisted
                res.setStatus(HttpStatus::Code::Forbidden);
                res.setHeader("Content-Type", "text/html");
                res.send(Page403);

                statusCode = HttpStatus::Code::Forbidden;
            }
            else if (fs::exists(filePath)) // File is found
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

            Write_log("INFO", std::format("Client {} {} {} {}", clientIp, method, path, static_cast<int>(statusCode)));
        }
    );

    try
    {
        Write_log("INFO", std::format("Server listening on http://{}:{}", Server_IP, Server_Port));

        httpServer_King.listen(Server_IP.c_str(), Server_Port);
    }
    catch (std::exception &error)
    {
        Write_log("ERROR", std::format("An error occured while running the server: {}", error.what()));
    };
};