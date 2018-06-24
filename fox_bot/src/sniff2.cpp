// #include <ArduinoJson.h>
// #include <ESP8266WiFi.h>
// #include <WiFiClient.h>
// #include <ESP8266HTTPClient.h>
// #include <vector>
//
// const char* apSsid     = "ap-ssid";
// const char* apPassword = "ap-password";
// const char* clientSsid     = "client-ssid";
// const char* clientPassword = "client-password";
//
// HTTPClient http;
//
// WiFiEventHandler probeRequestPrintHandler;
//
// String macToString(const unsigned char* mac) {
//   char buf[20];
//   snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
//            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
//   return String(buf);
// }
//
// std::vector<WiFiEventSoftAPModeProbeRequestReceived> myList;
//
// void onProbeRequestPrint(const WiFiEventSoftAPModeProbeRequestReceived& evt) {
//   myList.push_back(evt);
// }
//
// void setup() {
//   Serial.begin(115200);
//   Serial.println("Hello!");
//
//   // Don't save WiFi configuration in flash - optional
//   WiFi.persistent(false);
//
//   WiFi.mode(WIFI_AP_STA);
//   WiFi.softAP(apSsid, apPassword);
//   WiFi.begin(clientSsid, clientPassword);
//   while (WiFi.status() != WL_CONNECTED) {
//     Serial.print(".");
//     delay(100);
//   }
//   Serial.println("");
//   probeRequestPrintHandler = WiFi.onSoftAPModeProbeRequestReceived(&onProbeRequestPrint);
// }
//
// void loop() {
//   delay(3000);
//   String json = "";
//   DynamicJsonBuffer jsonBuffer;
//   JsonObject& root = jsonBuffer.createObject();
//   JsonArray& probes = root.createNestedArray("probes");
//   for(WiFiEventSoftAPModeProbeRequestReceived w : myList){
//     JsonObject& probe = probes.createNestedObject();
//     probe["address"] = macToString(w.mac);
//     probe["rssi"] = w.rssi;
//   }
//   myList.clear();
//   root.printTo(json);
//   Serial.println("json:" + json);
//
//   // http.begin("http://example.com/api/v1/probe");
//   // http.addHeader("Content-Type", "application/json");
//   // http.POST(json);
//   // http.end();
// }
