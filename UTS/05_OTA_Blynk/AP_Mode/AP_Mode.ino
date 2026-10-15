#include <WiFi.h>
#include <WiFiAP.h>
#include <WebServer.h>
/* INCLUDE ESP2SOTA LIBRARY */
#include <ESP2SOTA.h>

const char* ssid = "ESP2SOTA";
const char* password = "123456788abc";

WebServer server(80);

void setup(void) {
  pinMode(2, OUTPUT);
  Serial.begin(115200);
  delay(1000);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);  

  IPAddress IP(10, 10, 10, 1);
  IPAddress NMask(255, 255, 255, 0);
  WiFi.softAPConfig(IP, IP, NMask);

  WiFi.softAP(ssid, password);
  delay(500);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  server.on("/myurl", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", "Hello there!");
  });

  ESP2SOTA.begin(&server);
  server.begin();
  Serial.println("HTTP server & ESP2SOTA siap.");
}

void loop(void) {
  /* HANDLE UPDATE REQUESTS */
  server.handleClient();

  /* YOUR LOOP CODE HERE */


  digitalWrite(2, HIGH);
  delay(1250);
  digitalWrite(2, LOW);
  delay(1250);

  delay(5);
}