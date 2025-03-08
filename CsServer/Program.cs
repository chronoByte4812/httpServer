using Newtonsoft.Json;
using System.Net;

class Program
{
    static string custom404Page = "<h3 style='color: red;'>404 - Not found: That file not found on this server.</h3>";
    static readonly string configFile = Path.Combine(Directory.GetCurrentDirectory(), "CsServerConfig.json");
    static List<string> blackListedPaths = [];
    static string ip = "127.0.0.1";
    static int port = 6432;

    static string GetMimeType(string extension)
    {
        var mimeTypes = new Dictionary<string, string>
        {
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

        return mimeTypes.ContainsKey(extension) ? mimeTypes[extension] : "application/octet-stream";
    }

    static string ReadFile(string filePath)
    {
        if (!File.Exists(filePath)) return "Error retrieving file contents";
        return File.ReadAllText(filePath);
    }

    static void Log(string message)
    {
        string timeString = DateTime.Now.ToString("dd-MM-yy HH:mm:ss");
        string formattedLog = $"[{timeString}] - {message}";
        Console.WriteLine(formattedLog);
        File.AppendAllText("./CsServerLog.log", formattedLog + Environment.NewLine);
    }

    static void HandleConfig()
    {
        if (File.Exists(configFile))
        {
            try
            {
                Log("Config found!");

                string data = File.ReadAllText(configFile);
                dynamic? config = JsonConvert.DeserializeObject(data);

                string newIp = config.hostName;
                string newPort = config.port.ToString();
                string custom404Info = config.custom404Path;
                blackListedPaths = config.blackListedPaths.ToObject<List<string>>();

                ip = string.IsNullOrEmpty(newIp) ? ip : newIp;
                port = string.IsNullOrEmpty(newPort) ? port : int.Parse(newPort);

                if (!string.IsNullOrEmpty(custom404Info) && File.Exists(custom404Info))
                    custom404Page = ReadFile(custom404Info);
                else if (!string.IsNullOrEmpty(custom404Info))
                    Log("The provided custom 404 path could not be resolved, Using default page.");
            }
            catch (Exception error)
            {
                Log($"Failed to parse JSON data: \n{error.Message}");
            }
        }
        else
        {
            Log("A configuration file was not found, The program will write one with the default settings.");
            var defaultConfig = new
            {
                comments = new
                {
                    _comment0 = "This is the current default configuration for the server.",
                    _comment1 = "You can change the ip and port as long as they are valid, if they are malformed in any way, the program will close (crash) with no warning or errors.",
                    _comment2 = "If you have a custom 404 page, great! set the relative file path, for example: errorPages/my404.html, if left blank or file doesn't exist, the server will use a predefined 404 page"
                },
                hostName = "127.0.0.1",
                port = 6432,
                blackListedPaths = new List<string>(),
                custom404Path = ""
            };

            string jsonConfig = JsonConvert.SerializeObject(defaultConfig, Formatting.Indented);
            File.WriteAllText(configFile, jsonConfig);
        }
    }

    static async Task StartServer()
    {
        HttpListener listener = new();
        listener.Prefixes.Add($"http://{ip}:{port}/");

        try
        {
            listener.Start();
            Log($"Server listening on {ip}:{port}");

            while (true)
            {
                HttpListenerContext context = await listener.GetContextAsync();
                HandleRequest(context);
            }
        }
        catch (Exception ex)
        {
            Log($"An error occurred while running the server program: {ex.Message}");
        }
    }

    static void HandleRequest(HttpListenerContext context)
    {
        string clientIp = context.Request.RemoteEndPoint.ToString();
        string requestedPath = context.Request.Url.AbsolutePath;
        string filePath = Path.Combine(Directory.GetCurrentDirectory(), requestedPath.TrimStart('/'));

        foreach (var blackListedPath in blackListedPaths)
        {
            if (filePath.StartsWith(blackListedPath, StringComparison.OrdinalIgnoreCase))
            {
                context.Response.StatusCode = (int)HttpStatusCode.Forbidden;
                context.Response.ContentType = "text/html";
                byte[] forbiddenResponse = System.Text.Encoding.UTF8.GetBytes("<h3 style='color: red;'>403 - Forbidden: The requested file is not for the public.</h3>");
                context.Response.OutputStream.Write(forbiddenResponse, 0, forbiddenResponse.Length);
                Log($"Client {clientIp} requested {requestedPath} (403 Forbidden)");
                return;
            }
        }

        if (File.Exists(filePath))
        {
            string fileContent = ReadFile(filePath);
            string mimeType = GetMimeType(Path.GetExtension(filePath));
            context.Response.ContentType = mimeType;
            byte[] contentBytes = System.Text.Encoding.UTF8.GetBytes(fileContent);
            context.Response.OutputStream.Write(contentBytes, 0, contentBytes.Length);
            Log($"Client {clientIp} requested {requestedPath} (200 OK)");
        }
        else
        {
            context.Response.StatusCode = (int)HttpStatusCode.NotFound;
            context.Response.ContentType = "text/html";
            byte[] notFoundResponse = System.Text.Encoding.UTF8.GetBytes(custom404Page);
            context.Response.OutputStream.Write(notFoundResponse, 0, notFoundResponse.Length);
            Log($"Client {clientIp} requested {requestedPath} (404 Not Found)");
        }
    }

    static void Main(string[] args)
    {
        HandleConfig();
        StartServer().GetAwaiter().GetResult();
    }
}
