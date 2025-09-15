// HttpServer-King.cpp : Defines the entry point for the application.
//

#include "./HttpServerSrc-King/HttpServer.hpp"
#include "./HttpServerSrc-King/util/HttpMethod.hpp"
#include "./HttpServerSrc-King/util/MimeType.hpp"
#include "./json.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;
using json = nlohmann::json;
std::string Page404 = "<h3 style='color: red;'>404 - File Not found</h3>";
std::string PageEmpty = "<h3 style='color: red;'>404 - File was found but is empty</h3>";
std::string Page403 = "<h3 style='color: red;'>403 - File Forbidden</h3>";
std::string Server_IP = "0.0.0.0";
std::string Server_Config_Path = (fs::current_path() / "ServerConfig.json").string();
std::vector<std::string> BlackListPaths;
bool useConfig = true;
int Server_Port = 6432;
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
    std::ofstream logFile("./ServerLogs.log", std::ios::app);
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

            if (data.contains("MimeTypesCustom") && data["MimeTypesCustom"].is_object() && data["MimeTypesCustom"].size() > 0)
            {
                Write_log("INFO", "Custom mime types loaded.");
                MimeType::sMimeTypeMap.clear();

                for (auto& [ext, type] : data["MimeTypesCustom"].items())
                {
                    MimeType::sMimeTypeMap[ext] = type;
                };
            };

            Server_IP = data.value("ip", Server_IP);
            Server_Port = data.value("port", Server_Port);
            BlackListPaths = data.value("BlackListPaths", BlackListPaths);

            if (!Page404Custom.empty())
            {
                if (fs::exists(Page404Custom))
                {
                    Write_log("INFO", "Custom 404 page successfully loaded!");
                    Page404 = readFile(Page404Custom);
                }
                else
                    Write_log("WARNING", "The provided 404 page was not found");
            };

            if (!Page403Custom.empty())
            {
                if (fs::exists(Page403Custom))
                {
                    Write_log("INFO", "Custom 403 page successfully loaded!");
                    Page403 = readFile(Page403Custom);
                }
                else
                    Write_log("WARNING", "The provided 403 page was not found");
            };
        }
        catch (std::exception &error)
        {
            Write_log("ERROR", "Failed to parse config data: " + std::string(error.what()));
        };
    }
    else
    {
        Write_log("INFO", "A server config data was not found. A new one has been written");
        std::ofstream file("ServerConfig.json");

        json configData = json::parse(R"(
			{
                "helpInfo": {
                    "ip": "The IP is what the Server will bind to in order to listen.",
                    "port": "The Port is what the Server will listen to in order to listen.",
                    "BlackListPaths": "The BlackListPaths is a list of paths that the Server will block all Clients access to.",
                    "MimeTypesCustom": "The MimeTypesCustom is a list of custom MimeTypes that the Server will use and overwrite default ones.",
                    "Page403Custom": "The Page403Custom is a custom page that the Server will use when a Client tries to access a BlackListed path.",
                    "Page404Custom": "The Page404Custom is a custom page that the Server will use when a Client tries to access a file that does not exist."
                },
                "BlackListPaths": [
                    "/ServerConfig.json",
                    "/ServerLogs.log"
            ],
                "MimeTypesCustom": {},
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
    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
        {
            char* arg = argv[i];

            if (std::strcmp(arg, "--noconfig") == 0)
                useConfig = false;
        };
    };

    if (useConfig == true)
        handleConfig();

    KingHttpServer.use("/", HttpMethod::GET, [](HttpRequest req, HttpResponse& res, const NextFn& next) {
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

    KingHttpServer.use(R"(/.*)", HttpMethod::GET, [](HttpRequest req, HttpResponse& res, const NextFn& next)
                       {
            std::string clientIp = req.getRemoteAddr();
            std::string path = req.getPath();
            std::string filePath = path == "/" ? fs::current_path().string() + "/index.html" : fs::current_path().string() + path;
            std::string ReadFile = readFile(filePath);
            std::string content_type = MimeType::getMimeType(filePath);
            std::string method = HttpMethod::toString(req.getMethod());
            bool isBlackListed = false;

            for (const std::string& blacklistedPath : BlackListPaths)
            {
                if (path.rfind(blacklistedPath, 0) == 0)
                {
                    isBlackListed = true;
                    break;
                }
            };

            if (isBlackListed) { // Private file.
                Write_log("INFO", "Client " + clientIp + " " + method + " " + path + " (403 Forbidden)");

                res.setStatus(HttpStatus::Forbidden);
                res.setHeader("Content-Type", "text/html");
                res.send(Page403);
            }
            else if (ReadFile.empty()) // File found but is empty
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
            } });

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