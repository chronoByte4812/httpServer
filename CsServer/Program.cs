using Newtonsoft.Json;
using System.Globalization;
using System.Net;

// C# no good compared to C++, an example is DLL's dependencies cannot be embeded into the executable binary.
// Another example, C# is significantly slower compared to C++.

namespace CsServer
{
    class Program
    {
        private static readonly Dictionary<string, string> MimeTypes = new()
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
                    {".chm", "application/vnd.ms-htmlhelp"},
                    {".woff", "font/woff"},
                    {".woff2", "font/woff2"}
        };

        private static readonly string ConfigFile = Path.Combine(Directory.GetCurrentDirectory(), "CsServerConfig.json");
        private static string Ip = "http://*:6432//";
        private static List<string> BlackListedPaths = [];
        private static string Custom404Page = "<h3 style='color: red;'>404 - Not found: That file not found on this server.</h3>";

        static string GetMimeType(string extension)
        {
            if (MimeTypes.TryGetValue(extension, out string? mimeType))
                return mimeType;
            return "application/octet-stream";
        }

        static string ReadFile(string filePath)
        {
            try
            {
                return File.ReadAllText(filePath);
            }
            catch
            {
                return string.Empty;
            };
        }

        static void Log(string message)
        {
            var time = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss", CultureInfo.InvariantCulture);
            var formattedMessage = $"[{time}] - {message}";
            Console.WriteLine(formattedMessage);
            File.AppendAllText("CsServerLog.log", formattedMessage + Environment.NewLine);
        }

        static void HandleConfig()
        {
            if (File.Exists(ConfigFile))
            {
                try
                {
                    Log("Config found!");

                    var configJson = File.ReadAllText(ConfigFile);
                    var configData = JsonConvert.DeserializeObject<dynamic>(configJson);
                    Ip = configData.hostName ?? Ip;
                    string port = configData.port?.ToString() ?? "6432";
                    BlackListedPaths = configData.blackListedPaths?.ToObject<List<string>>() ?? new List<string>();

                    string custom404Info = configData.custom404Path ?? string.Empty;
                    if (!string.IsNullOrEmpty(custom404Info) && File.Exists(custom404Info))
                    {
                        Custom404Page = ReadFile(custom404Info);
                    }
                    else if (!string.IsNullOrEmpty(custom404Info))
                    {
                        Log("The provided custom 404 path could not be resolved, Using default page.");
                    }
                }
                catch (Exception ex)
                {
                    Log("Failed to parse JSON data: " + ex.Message);
                }
            }
            else
            {
                Log("A configuration file was not found, the program will create one with default settings.");

                var defaultConfig = new
                {
                    hostName = "127.0.0.1",
                    port = 6432,
                    blackListedPaths = new List<string>(),
                    custom404Path = string.Empty
                };

                File.WriteAllText(ConfigFile, JsonConvert.SerializeObject(defaultConfig, Formatting.Indented));
            }
        }

        static void Main()
        {
            HandleConfig();
            HttpListener listener = new();
            listener.Prefixes.Add(Ip);

            try
            {
                listener.Start();
                Log($"Server listening on {Ip} All network interfaces");

                while (true)
                {
                    var context = listener.GetContext();
                    string clientIp = context.Request.RemoteEndPoint.ToString();
                    string requestedPath = context.Request.Url?.AbsolutePath ?? "/";

                    if (requestedPath == "/")
                    {
                        string filePath = Path.Combine(Directory.GetCurrentDirectory(), "index.html");

                        if (File.Exists(filePath))
                        {
                            string content = ReadFile(filePath);
                            context.Response.ContentType = "text/html";
                            context.Response.StatusCode = 200;
                            using (var writer = new StreamWriter(context.Response.OutputStream))
                            {
                                writer.Write(content);
                            }
                            Log($"Client {clientIp} requested {requestedPath} (200 OK)");
                        }
                        else
                        {
                            context.Response.StatusCode = 404;
                            context.Response.ContentType = "text/html";
                            using (var writer = new StreamWriter(context.Response.OutputStream))
                            {
                                writer.Write(Custom404Page);
                            }
                            Log($"Client {clientIp} requested {requestedPath} (404 Not Found)");
                        }
                    }
                    else
                    {
                        string filePath = Path.Combine(Directory.GetCurrentDirectory(), requestedPath.TrimStart('/'));
                        string absoluteFilePath = Path.GetFullPath(filePath);

                        foreach (var blackListedPath in BlackListedPaths)
                        {
                            if (absoluteFilePath.StartsWith(Path.GetFullPath(blackListedPath)))
                            {
                                context.Response.StatusCode = 403; // Forbidden
                                context.Response.ContentType = "text/html";
                                using (var writer = new StreamWriter(context.Response.OutputStream))
                                {
                                    writer.Write("<h3 style='color: red;'>403 - Forbidden: The requested file is not for the public.</h3>");
                                }
                                Log($"Client {clientIp} requested {requestedPath} (403 Forbidden)");
                                return;
                            }
                        }

                        if (File.Exists(filePath))
                        {
                            string extension = Path.GetExtension(filePath);
                            string mimeType = GetMimeType(extension);
                            string content = ReadFile(filePath);
                            context.Response.ContentType = mimeType;
                            context.Response.StatusCode = 200;
                            using (var writer = new StreamWriter(context.Response.OutputStream))
                            {
                                writer.Write(content);
                            }
                            Log($"Client {clientIp} requested {requestedPath} (200 OK)");
                        }
                        else
                        {
                            context.Response.StatusCode = 404;
                            context.Response.ContentType = "text/html";
                            using (var writer = new StreamWriter(context.Response.OutputStream))
                            {
                                writer.Write(Custom404Page);
                            }
                            Log($"Client {clientIp} requested {requestedPath} (404 Not Found)");
                        }
                    }

                    context.Response.OutputStream.Close();
                }
            }
            catch (Exception ex)
            {
                Log("An error occurred while running the server: " + ex.Message);
            }
        }
    }
}
