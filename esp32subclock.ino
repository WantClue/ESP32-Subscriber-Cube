#include <Arduino.h>
#include <TM1637TinyDisplay6.h>
#include <WiFiClientSecure.h>
#include <YoutubeApi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>


// Replace with your WiFi credentials
const char* ssid = "YourWifiSSID";
const char* password = "WIFIPASSWORD";

// Replace with your YouTube API key
const char* apiKey = "APIKEY";

// Replace with the channel ID of your Channel
const char* channelId = "CHANNELID";


// Replace these with the PIN numbers of your dev board.
const uint8_t CLK_PIN = 16;
const uint8_t DIO_PIN = 17;



TM1637TinyDisplay6 display(CLK_PIN, DIO_PIN);
WiFiClientSecure client;
YoutubeApi api(apiKey, client);

void setup() {

  Serial.begin(115200);
  Serial.print("Connecting WiFi ");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("."); delay(500);
  }
  Serial.println("");
  Serial.print("Connected: "); Serial.println(WiFi.localIP());
  display.begin();
  delay(1000);

}
void loop() {
  int subscriberCount = getSubscriberCount();
  Serial.print("Subscriber Count: ");
  Serial.println(subscriberCount);
  if (subscriberCount >= 0) {
    display.setBrightness(2);
    display.showNumber(subscriberCount, true);
  }

  delay(600000); // Fetch every 10 minute
  
}


int getSubscriberCount() {
  String url = "https://www.googleapis.com/youtube/v3/channels?part=statistics&id=" + String(channelId) + "&key=" + apiKey;

  HTTPClient http;
  http.begin(url);

  int httpResponseCode = http.GET();
  String response = http.getString();

  http.end();

  if (httpResponseCode == 200) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, response);

    int subscriberCount = doc["items"][0]["statistics"]["subscriberCount"].as<int>();
    return subscriberCount;
  } else {
    Serial.print("HTTP Request failed, error code: ");
    Serial.println(httpResponseCode);
    return -1; // Error code
  }
}