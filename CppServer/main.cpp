#include "./httplib.h";
#include "./json.h";
#include <chrono>;
#include <filesystem>;
#include <fstream>;
#include <iostream>;

namespace fs = std::filesystem;
using json = nlohmann::json;
httplib::Server svr;
std::string custom404Page = "<h3 style='color: red;'>404 - Not found: That file not found on this server.</h1>";
std::string configFile = (fs::current_path() / "CppServerConfig.json").string();
std::vector<std::string> blackListedPaths;
std::string ip = "127.0.0.1";
int port = 6432;

static std::string getMimeType(const std::string &extension)
{
	static const std::unordered_map<std::string, std::string> mime_types = {
		{".html", "text/html"},
		{".htm", "text/html"},
		{".css", "text/css"},
		{".js", "application/javascript"},
		{".json", "application/json"},
		{".txt", "text/plain"},
		{".csv", "text/csv"},
		{".xml", "application/xml"},
		{".jpeg", "image/jpeg"},
		{".png", "image/png"},
		{".ico", "image/x-icon"},
		{".gif", "image/gif"},
		{".bmp", "image/bmp"},
		{".svg", "image/svg+xml"},
		{".webp", "image/webp"},
		{".mp4", "video/mp4"},
		{".webm", "video/webm"},
		{".mp3", "audio/mp3"},
		{".wav", "audio/wav"},
		{".pdf", "application/pdf"},
		{".zip", "application/zip"},
		{".gz", "application/gzip"},
		{".woff", "font/woff"},
		{".woff2", "font/woff2"}
	};

	auto it = mime_types.find(extension);
	if (it != mime_types.end())
	return it -> second;
	else return "application/octet-stream";
};

static std::string readFile(const std::string &filePath)
{
	std::ifstream file(filePath, std::ios::in | std::ios::binary);

	if (!file)
	{
		return "Error retrieving file contents";
	};

	std::ostringstream contents;
	contents << file.rdbuf();
	return contents.str();
};

static void log(const std::string& message) {
	auto now = std::chrono::system_clock::now();
	std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
	std::tm now_tm;
#	ifdef _WIN32
	localtime_s(&now_tm, &now_time_t);
	#else
	localtime_r(&now_time_t, &now_tm);
	#endif
	std::ostringstream timeStream;
	timeStream << std::put_time(&now_tm, "%d-%m-%y %H:%M:%S");
	std::string timeString = timeStream.str();
	std::string formattedLog = "[" + timeString + "] - " + message;
	std::cout << formattedLog << std::endl;
	std::ofstream logFile("./CppServerLog.log", std::ios::app);
	logFile << formattedLog << std::endl;
	logFile.close();
};

static void handleConfig()
{
	// This function will automaically look for a little file called CppServerConfig.json and parse it to find configs for the server settings. Though where called, the function can be commented out.
	if (fs::exists(configFile)) {
		try
		{
			log("Config found!\n");
			std::ifstream dataFile("./CppServerConfig.json");
			json data = json::parse(dataFile);
			std::string newIp = data["hostName"];
			std::string newPort = std::to_string(data["port"].get<int>());
			std::string custom404Info = data["custom404Path"];
		    std::vector<std::string> blackListedPaths = data["blackListedPaths"];

			ip = newIp.empty() ? ip : newIp;
			port = newPort.empty() ? port : std::stoi(newPort);

			if (!custom404Info.empty() && fs::exists(custom404Info)) custom404Page = readFile(custom404Info);
			else if (!custom404Info.empty()) log("The provided custom 404 path could not be resolved, Using default page.");
		}
		catch (const std::exception& error)
		{
			log("Failed to parse JSON data: \n" + std::string(error.what()) + "\n");
		};
	}
	else {
		log("A configuration file was not found, The program will write one with the default settings.\n");
		std::ofstream file("CppServerConfig.json");

		json configData = json::parse(R"(
			{   
				"comments": {
                "_comment0" : "This is the current default configuration for the server.",
				"_comment1" : "You can change the ip and port as long as they are valid, if they are malformed in any way, the program will close (crash) with no warning or errors.",
				"_comment2" : "If you have a custom 404 page, great! set the relative file path, for example: errorPages/my404.html, if left blank or file doesn't exist, the server will use a predefined 404 page"
            },
				"hostName": "127.0.0.1",
				"port": 6432,
				"blackListedPaths": [],
				"custom404Path": ""
			}
		)");

		// Actually writing JSON data to the fuckin file!
		file << configData.dump(4);
	};
};

int main(char *argv[], int argc)
{
	handleConfig();
	svr.Get("/", [&](const httplib::Request &req, httplib::Response &res) // Root path (./index.html).
			{
			std::string client_ip = req.remote_addr;
			std::string filePath = (fs::absolute(fs::current_path() / "index.html").string());

			if (fs::exists(filePath)) {
				res.set_content(readFile(filePath), "text/html");
				log("Client " + client_ip + " requested " + req.path + " (200 OK)\n");
			}
			else {
				res.status = httplib::StatusCode::NotFound_404;
				log("Client " + client_ip + " requested " + req.path + " (404 Not Found)\n");
				res.set_content(custom404Page, "text/html");
		} 
	});

	svr.Get(".*", [&](const httplib::Request &req, httplib::Response &res) { // Any file (*/*.*).
		std::string client_ip = req.remote_addr;
		std::string filePath = fs::current_path().string() + req.path;
		std::filesystem::path file(filePath);

		for (const std::string& blackListedPath : blackListedPaths) {
			if (fs::absolute(fs::current_path() / req.path).string().find(fs::absolute(blackListedPath).string()) == 0) {
				res.status = httplib::StatusCode::Forbidden_403;
				res.set_content("<h3 style='color: red;'>403 - Forbidden: The requested file is not for the public.</h3>", "text/html");
				log("Client " + client_ip + " requested " + req.path + " (403 Forbidden)\n");
				return;
			}
		};

		if (fs::exists(filePath))
		{
			res.set_content(readFile(filePath), getMimeType(file.extension().string()));
			log("Client " + client_ip + " requested " + req.path + " (200 OK)\n");
		}
		else
		{
			res.status = httplib::StatusCode::NotFound_404;
			log("Client " + client_ip + " requested " + req.path + " (404 Not Found)\n");
			res.set_content(custom404Page, "text/html");
		};
	});

	svr.Post("/submit", [](const httplib::Request& req, httplib::Response& res) {
		std::string name0 = req.get_param_value("name");
		std::string email0 = req.get_param_value("email");

		std::cout << "Name: " + name0 + " Email: " + email0 + "\n";

		res.set_content("Got the info!", "text/plain");
		res.set_redirect("/");
		});

	try
	{
		log("Server listening on " + ip + ":" + std::to_string(port) + (ip == "0.0.0.0" ? " All network interfaces.\n" : "\n"));
		svr.listen(ip, port);
	}
	catch (const std::exception &error)
	{
		log("An error occured while running the server program: " + std::string(error.what()));
	}
};