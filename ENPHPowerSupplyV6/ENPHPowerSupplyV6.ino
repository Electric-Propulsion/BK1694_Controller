#include <SPI.h>
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <ArduinoJson.h>

using namespace websockets;

// -----------------------------------------
// Wi-Fi Credentials
// -----------------------------------------
const char* ssid     = "EPNet";
const char* password = "electron";

// Static IP Configuration
IPAddress local_IP(192, 168, 0, 156);  // Set your desired static IP
IPAddress gateway(192, 168, 0, 1);    // Set your network's gateway
IPAddress subnet(255, 255, 255, 0);   // Set your subnet mask

// -----------------------------------------
// WebSocket Server
// -----------------------------------------
WebsocketsServer server;

// -----------------------------------------
// MCP4151 SPI pins on ESP32-C6
// -----------------------------------------
static const int CS_PIN   = 4;   // MCP4151 CS
static const int MOSI_PIN = 23;  // SPI MOSI
static const int MISO_PIN = -1;  // Not used (write-only)
static const int SCK_PIN  = 19;  // SPI clock

// Additional pin for enable control
static const int ENABLE_PIN = 15;  // Pin 15 to be controlled by the `enable` command

// Holds the pot wiper value (0..255)
int currentValue = 0;

// ------------------------------------------------------------
// Initialize SPI and configure the MCP4151 CS pin as output.
// Configure ENABLE_PIN as output.
// ------------------------------------------------------------
void initMCP4151() {
  pinMode(CS_PIN, OUTPUT);
  digitalWrite(CS_PIN, HIGH);  // Keep CS high when idle

  // Initialize SPI on the chosen pins
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
}

// ------------------------------------------------------------
// Set the MCP4151 wiper to an 8-bit value (0..255).
//
// MCP41x1 chips have 256 taps, so every integer in [0..255]
// corresponds to one step of the internal resistor array.
// We send two 8-bit frames:
//   1) Command Byte =>  0x00  (address=0, write command=00)
//   2) Data Byte    =>  your wiper position
// ------------------------------------------------------------
void setMCP4151Wiper(uint8_t wiperValue) {
  // Ensure 0..255
  wiperValue = constrain(wiperValue, 0, 255);

  // Command byte for Wiper 0 + Write = 0x00
  uint8_t commandByte = 0x00;
  uint8_t dataByte    = wiperValue;

  SPI.beginTransaction(SPISettings(1'000'000, MSBFIRST, SPI_MODE0));

  digitalWrite(CS_PIN, LOW);
  SPI.transfer(commandByte);
  SPI.transfer(dataByte);
  digitalWrite(CS_PIN, HIGH);

  SPI.endTransaction();

  Serial.printf("MCP4151: wiper set to %u\n", wiperValue);
}

// ------------------------------------------------------------
// Handle Incoming WebSocket Messages
// ------------------------------------------------------------
void handleWebSocketClient(WebsocketsClient& client) {
  if (client.available()) {
    auto message = client.readBlocking();
    Serial.printf("Received message: %s\n", message.data().c_str());

    // Parse JSON
    StaticJsonDocument<200> jsonDoc;
    DeserializationError error = deserializeJson(jsonDoc, message.data());
    if (error) {
      Serial.printf("JSON parse error: %s\n", error.f_str());
      return;
    }

    if (jsonDoc.containsKey("command")) {
      const char* command = jsonDoc["command"];

      if (strcmp(command, "setValue") == 0) {
        // Expect an integer "value" 0..255
        if (jsonDoc.containsKey("value") && jsonDoc["value"].is<int>()) {
          int val = jsonDoc["value"];
          val = constrain(val, 0, 255);
          currentValue = val;

          // Update the MCP4151
          setMCP4151Wiper(currentValue);

          // Respond
          StaticJsonDocument<100> response;
          response["status"]  = "ok";
          response["command"] = "setValue";
          response["value"]   = currentValue;

          String responseString;
          serializeJson(response, responseString);
          client.send(responseString);
        } else {
          Serial.println("Invalid or missing 'value' field in JSON");
        }
      }
      else if (strcmp(command, "enable") == 0) {
        // Expect a boolean "value" (true/false)
        if (jsonDoc.containsKey("value") && jsonDoc["value"].is<bool>()) {
            bool enableState = jsonDoc["value"];

            // Reverse logic: set Pin 15 LOW if enableState is true, HIGH if false
            digitalWrite(ENABLE_PIN, enableState ? LOW : HIGH);

            // Respond
            StaticJsonDocument<100> response;
            response["status"]  = "ok";
            response["command"] = "enable";
            response["value"]   = enableState;

            String responseString;
            serializeJson(response, responseString);
            client.send(responseString);

            Serial.printf("Pin 15 (ENABLE_PIN) set to %s\n", enableState ? "LOW" : "HIGH");
        } else {
            Serial.println("Invalid or missing 'value' field in JSON");
        }
      }
      else if (strcmp(command, "getStatus") == 0) {
        // Return currentValue
        StaticJsonDocument<100> response;
        response["status"]  = "ok";
        response["command"] = "getStatus";
        response["value"]   = currentValue;
        response["enable"]  = digitalRead(ENABLE_PIN) == LOW;

        String responseString;
        serializeJson(response, responseString);
        client.send(responseString);

        Serial.printf("Sent currentValue: %d\n", currentValue);
      }
      else {
        // Unknown command
        StaticJsonDocument<100> response;
        response["status"]  = "error";
        response["message"] = "Unknown command";

        String responseString;
        serializeJson(response, responseString);
        client.send(responseString);

        Serial.printf("Unknown command received: %s\n", command);
      }
    }
  }
}

// ------------------------------------------------------------
// Setup
// ------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting WebSocket + MCP4151 (8-bit) demo...");

  // 1. Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nFailed to connect to WiFi");
    while (true) {}
  }
  Serial.println("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());

  // 2. Initialize ENABLE_PIN (independent from MCP4151)
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, LOW);  // Set initial state to LOW (default disabled)

  // 3. Initialize SPI + MCP4151
  initMCP4151();

  // Optional: Log the initial enable state
  Serial.println("ENABLE_PIN initialized to LOW (default disabled)");

  // 4. Start WebSocket server
  server.listen(7777);
  Serial.println("WebSocket Server listening on port 7777.");
}


// ------------------------------------------------------------
// Main Loop
// ------------------------------------------------------------
void loop() {
  auto client = server.accept();
  if (client.available()) {
    handleWebSocketClient(client);
  }
}
