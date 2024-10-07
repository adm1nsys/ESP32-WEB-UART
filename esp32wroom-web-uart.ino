#include <WiFi.h>
#include <WebServer.h>
#include <HardwareSerial.h>

// Setting up Wi-Fi
const char* ssid = "SSID";
const char* password = "PASSWORD";
WebServer server(80);

// Setting up UART1
HardwareSerial MySerial(1);  // Use UART1
int RX_PIN = 16;  // RX pin for UART1
int TX_PIN = 17;  // TX pin for UART1

String logs = "";  // Line for storing logs
bool newData = false;  // Flag for checking new data

void setup() {
  Serial.begin(115200);  // Arduino IDE Debug Console
  MySerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);  // UART for LuckFox
  
  // Подключение к Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());

  // Setting up the web interface
  server.on("/", handleRoot);  // Home page
  server.on("/logs", handleLogs);  // Logs on request
  server.on("/send", handleSend);  // Command to send data via UART
  server.begin();
  Serial.println("HTTP server started");
}

// Function to clear control characters from strings
String cleanLog(String log) {
  // Remove terminal control characters and sequences (e.g. [2004h and [2004l)
  log.replace("\033[", "");  // Remove the initial characters of control codes
  log.replace("?2004h", "");
  log.replace("?2004l", "");
  return log;
}

void loop() {
  // Working with a web server
  server.handleClient();
  
  // Reading data from LuckFox via UART
  if (MySerial.available()) {
    while (MySerial.available()) {
      char data = MySerial.read();  // Reading data one character at a time

      // If it's a tab, replace it with spaces.
      if (data == '\t') {
        logs += "    ";  // Add 4 spaces instead of tabs
      }
      // If it is an escape sequence, skip it.
      else if (data == 27) {  // Escape code - 27 in ASCII
        while (MySerial.available() && MySerial.peek() != '[') {
          MySerial.read();
        }
      }
      else {
        logs += data;  // Adding a symbol to the logs
      }

      // Adding a line break in case of a newline character
      if (data == '\n') {
        logs += "<br>";  // Adding a line break in case of a newline character
      }
    }
    logs = cleanLog(logs);  // Clearing logs from control characters
    newData = true;  // Set the new data flag
    Serial.println("Received from LuckFox: " + logs);  // Output data to console
  }
}

// Processing a web request (main page)
void handleRoot() {
  String html = "<html>\n";
  html += "<head>\n";
  html += "<style>\n";
  html += "body { background-color: #2e2e2e; color: white; font-family: monospace; }\n";
  html += "#logs { white-space: pre-wrap; height: 90vh; overflow-y: scroll; background-color: #1e1e1e; padding: 10px; border: 1px solid #444; }\n";
  html += "#input-form { display: flex; align-items: center; }\n";
  html += "input[type='text'] { flex: 1; padding: 10px; background-color: #1e1e1e; color: white; border: 1px solid #444; margin-right: 10px; }\n";
  html += "button { padding: 10px; background-color: #444; color: white; border: none; cursor: pointer; }\n";
  html += "button:hover { background-color: #666; }\n";
  html += "</style>\n";
  html += "</head>\n";
  html += "<body>\n";
  html += "<h1>LuckFox UART Logs</h1>\n";
  html += "<div id='logs'></div>\n";
  html += "<div id='input-form'>\n";
  html += "<input type='text' id='command-input' placeholder='Enter command here'>\n";
  html += "<button onclick='sendCommand()'>Send</button>\n";
  html += "</div>\n";
  html += "<script>\n";
  html += "let logsCache = '';\n";  // Buffer for logs
  html += "function fetchLogs() {\n";
  html += "  fetch('/logs').then(r => r.text()).then(t => {\n";
  html += "    if (t !== logsCache) {\n";  // If the logs have changed
  html += "      document.getElementById('logs').innerHTML = logsCache + t;\n";
  html += "      logsCache = t;\n";  // Save current logs to buffer
  html += "    }\n";
  html += "  });\n";
  html += "}\n";
  html += "function sendCommand() {\n";
  html += "  const command = document.getElementById('command-input').value;\n";
  html += "  fetch('/send?command=' + encodeURIComponent(command))\n";
  html += "    .then(() => {\n";
  html += "      document.getElementById('command-input').value = '';\n";
  html += "      fetchLogs();\n";
  html += "    });\n";
  html += "}\n";
  html += "setInterval(fetchLogs, 2000);\n";
  html += "</script>\n";
  html += "</body>\n";
  html += "</html>\n";

  server.send(200, "text/html", html);
}

// Sending logs to a web page
void handleLogs() {
  if (newData) {  // We send only if there is new data
    server.send(200, "text/plain", logs);  // Sending accumulated logs
    newData = false;  // Drop the flag
  } else {
    server.send(200, "text/plain", "");  // No new data
  }
}

// Processing a command from the web interface and sending it via UART
void handleSend() {
  if (server.hasArg("command")) {
    String command = server.arg("command");
    Serial.println("Sending command to LuckFox: " + command);
    
    MySerial.println(command);  // Sending a command to LuckFox via UART
    server.send(200, "text/plain", "Command sent: " + command);
  } else {
    server.send(200, "text/plain", "No command received");
  }
}