// ESP32 -> Firebase heartbeat
// sets device/lastSeen value every 10s (Firebase server time, ms).
// Webpage sets Online / Offline status according to this.

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "secrets.h"

const unsigned long HEARTBEAT_MS      = 10UL * 1000UL;
const unsigned long TOKEN_LIFETIME_MS = 50UL * 60UL * 1000UL;  // token valid for 60s
const unsigned long WIFI_RETRY_MS     = 10UL * 1000UL;

String idToken = "";
unsigned long tokenObtainedAt = 0;
unsigned long lastBeat = 0;
unsigned long lastWifiTry = 0;

bool signIn() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  String url = String("https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=") + FIREBASE_API_KEY;
  if (!http.begin(client, url)) return false;
  http.addHeader("Content-Type", "application/json");

  String body = String("{\"email\":\"") + DEVICE_EMAIL +
                "\",\"password\":\"" + DEVICE_PASSWORD +
                "\",\"returnSecureToken\":true}";
  int code = http.POST(body);
  String resp = http.getString();
  http.end();

  if (code != 200) {
    Serial.printf("Bejelentkezes SIKERTELEN (HTTP %d): %s\n", code, resp.c_str());
    return false;
  }

  int k = resp.indexOf("\"idToken\"");
  if (k < 0) return false;
  int colon = resp.indexOf(':', k);
  int q1 = resp.indexOf('"', colon + 1);
  int q2 = resp.indexOf('"', q1 + 1);
  if (colon < 0 || q1 < 0 || q2 < 0) return false;

  idToken = resp.substring(q1 + 1, q2);
  tokenObtainedAt = millis();
  Serial.println("Firebase bejelentkezes OK");
  return true;
}

bool sendHeartbeat() {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  String url = String(DATABASE_URL) + "/device/lastSeen.json?auth=" + idToken;
  if (!http.begin(client, url)) return false;
  http.addHeader("Content-Type", "application/json");

  int code = http.PUT("{\".sv\":\"timestamp\"}");  // server sets actual time
  http.end();

  if (code == 401 || code == 403) idToken = "";  // expired/wrong token -> resign-in
  if (code != 200) Serial.printf("Eletjel SIKERTELEN (HTTP %d)\n", code);
  return code == 200;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nESP32 kapuvezerlo - eletjel");

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastBeat = millis() - HEARTBEAT_MS;  // first heartbeat immadieately
}

void loop() {
  unsigned long now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    if (now - lastWifiTry >= WIFI_RETRY_MS) {
      lastWifiTry = now;
      Serial.println("Wi-Fi kapcsolodas...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
    return;
  }

  static bool wasConnected = false;
  if (!wasConnected) {
    wasConnected = true;
    Serial.print("Wi-Fi OK, IP: ");
    Serial.println(WiFi.localIP());
  }

  if (idToken.length() == 0 || now - tokenObtainedAt >= TOKEN_LIFETIME_MS) {
    if (!signIn()) {
      delay(5000);
      return;
    }
  }

  if (now - lastBeat >= HEARTBEAT_MS) {
    lastBeat = now;
    if (sendHeartbeat()) Serial.println("Eletjel elkuldve");
  }
}
