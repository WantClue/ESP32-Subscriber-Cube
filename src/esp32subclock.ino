#include <Arduino.h>
#include <TM1637TinyDisplay6.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>


// Replace with your WiFi credentials
const char* ssid = "YOURWIFI";
const char* password = "YOURPW";

// Replace with your YouTube API key
const char* apiKey = "YOURAPIKEY";

// Replace with the channel ID of your Channel
const char* channelId = "YOURCHANNELID";


// Replace these with the PIN numbers of your dev board.
const uint8_t CLK_PIN = 16;
const uint8_t DIO_PIN = 17;

// Restart the ESP32 after this many consecutive failed fetches
const uint8_t MAX_FAILURES = 5;

// WiFi connect timeout (ms) before giving up and rebooting
const uint32_t WIFI_TIMEOUT = 30000;


TM1637TinyDisplay6 display(CLK_PIN, DIO_PIN);

uint8_t consecutiveFailures = 0;

bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi ");

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > WIFI_TIMEOUT) {
      return false;
    }
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.print("Connected: ");
  Serial.println(WiFi.localIP());
  return true;
}

void setup() {

  Serial.begin(115200);

  display.begin();
  // Show dashes so we can tell the ESP32 (re)booted: if the screen ever
  // shows dashes for a long time, WiFi is down - not the counter logic.
  display.showString("------");

  if (!connectWiFi()) {
    Serial.println("WiFi connect failed, restarting...");
    delay(1000);
    ESP.restart();
  }

  delay(1000);
}
void loop() {
  // Make sure WiFi is still alive before fetching
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost, reconnecting...");
    WiFi.disconnect();
    if (!connectWiFi()) {
      Serial.println("Reconnect failed, restarting...");
      delay(1000);
      ESP.restart();
    }
  }

  int subscriberCount = getSubscriberCount();
  Serial.print("Subscriber Count: ");
  Serial.println(subscriberCount);
  if (subscriberCount >= 0) {
    consecutiveFailures = 0;
    display.setBrightness(2);
    display.showNumber(subscriberCount, true);
  } else {
    consecutiveFailures++;
    Serial.print("Consecutive failures: ");
    Serial.println(consecutiveFailures);
    if (consecutiveFailures >= MAX_FAILURES) {
      Serial.println("Too many failures, restarting...");
      delay(1000);
      ESP.restart();
    }
  }

  delay(600000); // Fetch every 10 minute
}


int getSubscriberCount() {
  String url = "https://www.googleapis.com/youtube/v3/channels?part=statistics&id=" + String(channelId) + "&key=" + apiKey;

  HTTPClient http;
  http.setTimeout(15000); // Never block longer than 15 seconds
  http.begin(url);

  int httpResponseCode = http.GET();
  String response = http.getString();

  http.end();

  if (httpResponseCode != 200) {
    Serial.print("HTTP Request failed, error code: ");
    Serial.println(httpResponseCode);
    return -1; // Error code
  }

  // Static document lives on the stack -> no heap fragmentation,
  // and we verify parsing actually succeeded.
  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, response);
  if (error) {
    Serial.print("JSON parse failed: ");
    Serial.println(error.c_str());
    return -1;
  }

  JsonVariant count = doc["items"][0]["statistics"]["subscriberCount"];
  if (count.isNull()) {
    Serial.println("subscriberCount missing in response");
    return -1;
  }

  return count.as<int>();
}
