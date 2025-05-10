#include "./httplib.h"
#include "./json.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;
using json = nlohmann::json;
httplib::Server svr;
std::string custom404Page = "<h3 style='color: red;'>404 - Not found: That file not found on this server.</h3>";
std::string configFile = (fs::current_path() / "CppServerConfig.json").string();
std::vector<std::string> blackListedPaths;
std::string ip = "0.0.0.0";
size_t port = 6432;
std::map<std::string, std::string, std::less<std::string>, std::allocator<std::pair<const std::string, std::string>>> mime_types = {
	{".html", "text/html"},
	{".css", "text/css"},
	{".js", "text/javascript"},
	{".json", "application/json"},
	{".xml", "application/xml"},
	{".svg", "image/svg+xml"},
	{".png", "image/png"},
	{".ico", "image/x-icon"},
	{".gif", "image/gif"},
	{".jpg", "image/jpeg"},
	{".jpeg", "image/jpeg"},
	{".zip", "application/zip"},
	{".tar", "application/x-tar"},
	{".log", "text/plain"},
	{".gz", "application/gzip"},
	{".cpp", "text/plain"},
	{".h", "text/plain"},
};

static std::string readFile(const std::string &filePath)
{
	std::ostringstream contents;
	std::ifstream file(filePath, std::ios::in | std::ios::binary);

	if (!file)
		return "Error retrieving file contents";

	contents << file.rdbuf();
	return contents.str();
};

static void log(const std::string &logType, const std::string &message)
{
	static const std::unordered_map<std::string, std::string> validTypes = {
		{"INFO", "\033[32m"},
		{"WARNING", "\033[33m"},
		{"ERROR", "\033[31m"},
		{"DEBUG", "\033[36m"}};

	std::time_t now_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::ofstream logFile("./CppServerLog.log", std::ios::app);
	std::tm now_tm;
#ifdef _WIN32
	localtime_s(&now_tm, &now_time_t);
#else
	localtime_r(&now_time_t, &now_tm);
#endif
	std::ostringstream currentTime;
	currentTime << std::put_time(&now_tm, "%d-%m-%y %H:%M:%S");

	std::string formattedLog = "[" + currentTime.str() + "] [" + logType + "] - " + message + "\n";
	std::cout << validTypes.at(logType) << formattedLog << "\033[0m";
	logFile << formattedLog;

	logFile.close();
};

static void handleConfig()
{
	if (fs::exists(configFile))
	{
		try
		{
			log("INFO", "Config found!");
			std::ifstream dataFile(configFile);
			json data = json::parse(dataFile);

			if (data.contains("customMimeTypes") && !data["customMimeTypes"].empty())
			{
				mime_types.clear();

				for (const auto &mime : data["customMimeTypes"])
				{
					if (mime.is_object() && mime.size() == 1)
					{
						for (auto it = mime.begin(); it != mime.end(); ++it)
						{
							mime_types[it.key()] = it.value();
						};
					};
				};
			};

			ip = data.value("ip", ip);
			port = data.value("port", port);
			blackListedPaths = data.value("blackListedPaths", blackListedPaths);
			std::string custom404Path = data.value("custom404Path", "");

			if (custom404Path.empty())
				return;
			if (fs::exists(custom404Path))
			{
				log("INFO", "Custom 404 page successfully loaded!");
				custom404Page = readFile(custom404Path);
			}
			else
				log("INFO", "The given 404 page was not found");
		}
		catch (std::exception &error)
		{
			log("ERROR", "Failed to parse JSON data: " + std::string(error.what()));
		};
	}
	else
	{
		log("INFO", "The configuration file was not found, the program will write one with the default settings. Restart the server each time you change the config...");
		std::ofstream file("CppServerConfig.json");

		json configData = json::parse(R"(
			{
				"comments": {
					"_comment0": "This is the current default configuration for the server.",
					"_comment1": "You can change the server options with this file, it'll overwrite default configurations. But if the server for some reason exists with no warning, a probable cause is the port assigned is in use or another option is malformed, try another port",
					"_comment2": "You can also add a custom 404 error page too! you set the relative file path as follows: /path/to/404.html and or ./my404.html",
					"_comment3": "Blacklisted files and folders can be added as well, you set them as follows: /somefolder/ and or /private.txt, add as many as you like!",
					"_comment4": "Custom mime types are a way to define what the server should serve what as, for example a .png file would be servered as image/png"
				},
				"ip": "0.0.0.0",
				"port": 6432,
				"blackListedPaths": [
					"/CppServerConfig.json",
					"/CppServerLog.log"
				],
				"customMimeTypes": [],
				"custom404Path": ""
			}
		)");

		// Actually writing JSON data to the fuckin file!
		file << configData.dump(4);
		file.close();
	};
};

int main(int argc, char *argv[])
{
	handleConfig();

	svr.Get("/", [&](const httplib::Request &req, httplib::Response &res) // Root path (./index.html).
			{
			std::string clientIp = req.remote_addr;
			std::string filePath = (fs::absolute(fs::current_path() / "index.html").string());

			if (fs::exists(filePath))
			{
				res.set_content(readFile(filePath), "text/html");
				log("INFO" , "Client " + clientIp + " " + req.method + " " + req.path + " (200 OK)");
			}
			else
			{
				res.status = httplib::StatusCode::NotFound_404;
				log("INFO" , "Client " + clientIp + " " + req.method + " " + req.path + "index.html (404 Not Found)");
				res.set_content("<h3 style='color: red;'>404 - The main index.html doesn't exist on this server.</h3>", "text/html");
			}; });

	svr.Get(".*", [&](const httplib::Request &req, httplib::Response &res)
			{
    std::string clientIp = req.remote_addr;
    std::string filePath = fs::current_path().string() + req.path;
	std::string content_type = httplib::detail::find_content_type(filePath, mime_types, "application/octet-stream");
    bool isBlackListed = false;
    
    for (const std::string& blacklistedPath : blackListedPaths)
    {
        if (req.path.rfind(blacklistedPath, 0) == 0)
        {
			isBlackListed = true;
            break;
        }
    };
	
    if (fs::exists(filePath) && !isBlackListed)
    {
        res.set_content(readFile(filePath), content_type);
        log("INFO", "Client " + clientIp + " " + req.method + " " + req.path + " (200 OK)");
    }
	else if (isBlackListed)
	{
        log("INFO", "Client " + clientIp + " " + req.method + " " + req.path + " (403 Forbidden)");
        res.status = httplib::StatusCode::Forbidden_403;
		res.set_content("<h3 style='color: red;'>403 - Forbidden <a href='/'>go back home</a></h3>", "text/html");
	}
    else
    {
        res.status = httplib::StatusCode::NotFound_404;
        log("INFO", "Client " + clientIp + " " + req.method + " " + req.path + " (404 Not Found)");
        res.set_content(custom404Page, "text/html");
    }; });

	try
	{
		log("INFO", "Server listening on " + ip + ":" + std::to_string(port) + (ip == "0.0.0.0" ? " All network interfaces." : ""));
		svr.listen(ip, port);
	}
	catch (std::exception &error)
	{
		log("ERROR", "The provided port is in use! choose another one");
	}
};