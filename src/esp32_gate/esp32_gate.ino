// ESP32 -> Firebase heartbeat
// sets device/lastSeen value every 10s (Firebase server time, ms).
// Webpage sets Online / Offline status according to this.
//
// HTTPS connections are verified against the Google root certificates
// (google_roots.h). Certificate validation needs the correct date, so the
// clock is set from NTP after the Wi-Fi connection is up.

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <time.h>
#include "secrets.h"
#include "google_roots.h"

const unsigned long HEARTBEAT_MS      = 10UL * 1000UL;
const unsigned long TOKEN_LIFETIME_MS = 50UL * 60UL * 1000UL;  // token valid for 60 minutes
const unsigned long WIFI_RETRY_MS     = 10UL * 1000UL;

String idToken = "";
unsigned long tokenObtainedAt = 0;
unsigned long lastBeat = 0;
unsigned long lastWifiTry = 0;
bool timeSynced = false;

// Real date/time is needed to check the certificate validity period.
bool syncTime() {
  configTime(0, 0, "pool.ntp.org", "time.google.com");
  Serial.print("Ido szinkronizalas");
  for (int i = 0; i < 30; i++) {
    if (time(nullptr) > 1700000000) {  // later than Nov 2023 = clock is set
      Serial.println(" OK");
      return true;
    }
    Serial.print(".");
    delay(500);
  }
  Serial.println(" SIKERTELEN");
  return false;
}

// Prints why an HTTPS request failed (negative code = connection/TLS error).
void logTlsError(WiFiClientSecure &client, HTTPClient &http, int code) {
  if (code >= 0) return;
  char err[100] = "";
  client.lastError(err, sizeof(err));
  Serial.printf("  Kapcsolati hiba: %s | TLS: %s\n", http.errorToString(code).c_str(), err);
}

bool signIn() {
  WiFiClientSecure client;
  client.setCACert(GOOGLE_ROOT_CAS);  // verify the server certificate
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
    logTlsError(client, http, code);
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
  client.setCACert(GOOGLE_ROOT_CAS);  // verify the server certificate
  HTTPClient http;

  String url = String(DATABASE_URL) + "/device/lastSeen.json?auth=" + idToken;
  if (!http.begin(client, url)) return false;
  http.addHeader("Content-Type", "application/json");

  int code = http.PUT("{\".sv\":\"timestamp\"}");  // server sets actual time
  http.end();

  if (code == 401 || code == 403) idToken = "";  // expired/wrong token -> resign-in
  if (code != 200) {
    Serial.printf("Eletjel SIKERTELEN (HTTP %d)\n", code);
    logTlsError(client, http, code);
  }
  return code == 200;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nESP32 kapuvezerlo - eletjel");

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastBeat = millis() - HEARTBEAT_MS;  // first heartbeat immediately
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

  // No HTTPS before the clock is set, otherwise certificate checks fail.
  if (!timeSynced) {
    timeSynced = syncTime();
    if (!timeSynced) {
      delay(5000);
      return;
    }
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
