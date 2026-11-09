// ESP32 gate opener: heartbeat + commands from the web page + RF transmission
//
// - Every 10 s it writes device/lastSeen (server time), device/rssi and, once
//   after boot, device/lastBoot. The web page shows Online/Offline from this.
// - Every 2 s it asks the database for commands with status "pending".
//   Each command is handled at most once: status "taken" is written BEFORE
//   transmitting, so a restart in between can lose a command but never repeat it.
// - Safety checks: commands older than 20 s or sent before this boot expire,
//   10 s cooldown per gate, at most 6 transmissions per minute.
// - HTTPS connections are verified against the Google root certificates
//   (google_roots.h); the clock is set from NTP first, because certificate
//   validation needs the real date.
//
// Private settings: secrets.h (Wi-Fi, Firebase) and rf_codes.h (RF codes).
// Neither is published; see secrets.example.h and rf_codes.example.h.
//
// Libraries (Arduino IDE -> Library Manager): ArduinoJson (7.x), rc-switch.

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <RCSwitch.h>
#include <time.h>
#include <sys/time.h>
#include "secrets.h"
#include "rf_codes.h"
#include "google_roots.h"

#ifndef RF_DRY_RUN
#define RF_DRY_RUN true   // safe default: only print, never transmit
#endif

// ---- Settings ----
const int TX_PIN = 25;                                         // RF433-JTX DATA pin
const unsigned long HEARTBEAT_MS      = 10UL * 1000UL;
const unsigned long POLL_MS           = 2UL * 1000UL;
const unsigned long TOKEN_LIFETIME_MS = 50UL * 60UL * 1000UL;  // token valid for 60 minutes
const unsigned long WIFI_RETRY_MS     = 10UL * 1000UL;
const unsigned long WATCHDOG_MS       = 10UL * 60UL * 1000UL;  // restart if no heartbeat for 10 min
const double        COMMAND_MAX_AGE_MS = 20000.0;              // older commands expire
const unsigned long GATE_COOLDOWN_MS  = 10UL * 1000UL;         // per gate
const int           MAX_RUNS_PER_MIN  = 6;                     // all gates together
const int           MAX_BATCH         = 10;                    // commands handled per poll

struct Gate {
  const char *id;
  unsigned long code;
  unsigned long lastRunMs;
  bool hasRun;
};

Gate gates[] = {
  {"bal",  RF_CODE_BAL,  0, false},
  {"jobb", RF_CODE_JOBB, 0, false},
  {"kapu", RF_CODE_KAPU, 0, false},
};
const int GATE_COUNT = sizeof(gates) / sizeof(gates[0]);

// Defined here, before any function: the Arduino IDE adds function
// prototypes above the first function, and they need this type.
struct PendingCommand {
  String id;
  String gate;
  double requestedAt;
};

WiFiClientSecure dbClient;  // one TLS connection to the database, reused
HTTPClient dbHttp;          // must stay global, otherwise the connection is closed
RCSwitch rf;

String idToken = "";
unsigned long tokenObtainedAt = 0;
bool timeSynced = false;
double bootEpochMs = 0;      // NTP time when the clock was first set
bool bootReported = false;
bool wasConnected = false;
bool rfConfigured = false;
unsigned long lastBeat = 0;
unsigned long lastBeatOk = 0;
unsigned long lastPoll = 0;
unsigned long lastWifiTry = 0;
unsigned long runTimes[MAX_RUNS_PER_MIN];
int runCount = 0;
int runIdx = 0;

// ---------------------------------------------------------------- helpers

double nowEpochMs() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return (double)tv.tv_sec * 1000.0 + (double)(tv.tv_usec / 1000);
}

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

void logTlsError(WiFiClientSecure &client, int code) {
  if (code >= 0) return;
  char err[100] = "";
  client.lastError(err, sizeof(err));
  Serial.printf("  Kapcsolati hiba: %s | TLS: %s\n", HTTPClient::errorToString(code).c_str(), err);
}

bool validId(const String &s) {
  if (s.length() == 0 || s.length() > 40) return false;
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (!(isalnum((unsigned char)c) || c == '-' || c == '_')) return false;
  }
  return true;
}

Gate *findGate(const String &id) {
  for (int i = 0; i < GATE_COUNT; i++) {
    if (id == gates[i].id) return &gates[i];
  }
  return nullptr;
}

bool rateLimited() {
  if (runCount < MAX_RUNS_PER_MIN) return false;
  unsigned long oldest = runTimes[runIdx];  // ring buffer: next slot = oldest entry
  return millis() - oldest < 60000UL;
}

void recordRun() {
  runTimes[runIdx] = millis();
  runIdx = (runIdx + 1) % MAX_RUNS_PER_MIN;
  if (runCount < MAX_RUNS_PER_MIN) runCount++;
}

// ---------------------------------------------------------------- Firebase

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
    logTlsError(client, code);
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

// One request to the Realtime Database over the reused connection.
// path: "/" for the root, "/commands" etc. Returns the HTTP code.
int dbRequest(const char *method, const String &path, const String &query,
              const String &body, String *response) {
  String url = String(DATABASE_URL) + path + ".json?auth=" + idToken + query;
  if (!dbHttp.begin(dbClient, url)) return -1;
  if (body.length() > 0) dbHttp.addHeader("Content-Type", "application/json");
  int code = dbHttp.sendRequest(method, body);
  if (response != nullptr && code > 0) *response = dbHttp.getString();
  dbHttp.end();

  if (code < 0) {
    logTlsError(dbClient, code);
    dbClient.stop();  // start with a fresh connection next time
  }
  if (code == 401 || code == 403) idToken = "";  // expired/wrong token -> sign in again
  return code;
}

bool sendHeartbeat() {
  // Multi-path update: each path is checked by its own database rule.
  String body = String("{\"device/lastSeen\":{\".sv\":\"timestamp\"},\"device/rssi\":") + String((int)WiFi.RSSI());
  if (!bootReported) body += ",\"device/lastBoot\":{\".sv\":\"timestamp\"}";
  body += "}";

  int code = dbRequest("PATCH", "/", "", body, nullptr);
  if (code != 200) {
    Serial.printf("Eletjel SIKERTELEN (HTTP %d)\n", code);
    return false;
  }
  bootReported = true;
  return true;
}

bool setStatus(const String &id, const char *status, const char *reason) {
  String body = String("{\"status\":\"") + status + "\",\"handledAt\":{\".sv\":\"timestamp\"}";
  if (reason != nullptr) body += String(",\"reason\":\"") + reason + "\"";
  body += "}";

  int code = dbRequest("PATCH", "/commands/" + id, "", body, nullptr);
  Serial.printf("  -> %s%s%s%s (HTTP %d)\n", status,
                reason ? " (" : "", reason ? reason : "", reason ? ")" : "", code);
  return code == 200;
}

// ---------------------------------------------------------------- RF

void transmit(const Gate &g) {
  if (RF_DRY_RUN) {
    Serial.printf("  [PROBA] RF adas kihagyva: %s, kod %lu, %d bit\n", g.id, g.code, RF_BIT_LENGTH);
    return;
  }
  Serial.printf("  RF adas: %s\n", g.id);
  rf.send(g.code, RF_BIT_LENGTH);
}

// ---------------------------------------------------------------- commands

void handleCommand(const PendingCommand &c) {
  Serial.printf("Parancs %s: %s\n", c.id.c_str(), c.gate.c_str());

  Gate *g = findGate(c.gate);
  if (g == nullptr) { setStatus(c.id, "failed", "unknown gate"); return; }
  if (c.requestedAt < bootEpochMs) { setStatus(c.id, "expired", "before boot"); return; }
  if (nowEpochMs() - c.requestedAt > COMMAND_MAX_AGE_MS) { setStatus(c.id, "expired", "too old"); return; }
  if (!rfConfigured || g->code == 0) { setStatus(c.id, "failed", "no code"); return; }
  if (g->hasRun && millis() - g->lastRunMs < GATE_COOLDOWN_MS) { setStatus(c.id, "failed", "cooldown"); return; }
  if (rateLimited()) { setStatus(c.id, "failed", "rate limit"); return; }

  // Mark as taken first. If this fails, do NOT transmit.
  if (!setStatus(c.id, "taken", nullptr)) {
    Serial.println("  Nem sikerult atvenni, nincs adas");
    return;
  }

  transmit(*g);
  g->lastRunMs = millis();
  g->hasRun = true;
  recordRun();

  setStatus(c.id, "done", nullptr);
}

void pollCommands() {
  String body;
  int code = dbRequest("GET", "/commands",
                       "&orderBy=%22status%22&equalTo=%22pending%22&limitToFirst=10",
                       "", &body);
  if (code != 200) {
    Serial.printf("Parancs lekerdezes SIKERTELEN (HTTP %d) %s\n", code, body.c_str());
    return;
  }
  if (body == "null" || body == "{}") return;  // nothing pending

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.printf("Hibas valasz (JSON): %s\n", err.c_str());
    return;
  }

  PendingCommand list[MAX_BATCH];
  int n = 0;
  for (JsonPair kv : doc.as<JsonObject>()) {
    if (n >= MAX_BATCH) break;
    String id = kv.key().c_str();
    if (!validId(id)) continue;
    list[n].id = id;
    list[n].gate = kv.value()["gate"] | "";
    list[n].requestedAt = kv.value()["requestedAt"] | 0.0;
    n++;
  }

  // Oldest first
  for (int i = 1; i < n; i++) {
    PendingCommand tmp = list[i];
    int j = i - 1;
    while (j >= 0 && list[j].requestedAt > tmp.requestedAt) {
      list[j + 1] = list[j];
      j--;
    }
    list[j + 1] = tmp;
  }

  for (int i = 0; i < n; i++) handleCommand(list[i]);
}

// ---------------------------------------------------------------- main

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nESP32 kapuvezerlo - eletjel + parancsok");

  rfConfigured = (RF_PROTOCOL > 0 && RF_BIT_LENGTH > 0 && RF_PULSE_US > 0);
  if (rfConfigured) {
    rf.enableTransmit(TX_PIN);
    rf.setProtocol(RF_PROTOCOL);
    rf.setPulseLength(RF_PULSE_US);  // after setProtocol, which resets it
    rf.setRepeatTransmit(RF_REPEAT);
  }
  Serial.printf("RF: %s, %s\n", rfConfigured ? "beallitva" : "NINCS beallitva (rf_codes.h)",
                RF_DRY_RUN ? "PROBA uzem (nincs adas)" : "eles adas");

  dbClient.setCACert(GOOGLE_ROOT_CAS);  // verify the database certificate
  dbHttp.setReuse(true);
  dbHttp.setConnectTimeout(8000);
  dbHttp.setTimeout(8000);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastBeat = millis() - HEARTBEAT_MS;  // first heartbeat immediately
}

void loop() {
  unsigned long now = millis();

  // Software watchdog: no successful heartbeat for 10 minutes -> restart
  if (now - lastBeatOk >= WATCHDOG_MS) {
    Serial.println("10 perce nincs sikeres eletjel, ujrainditas...");
    delay(200);
    ESP.restart();
  }

  if (WiFi.status() != WL_CONNECTED) {
    if (wasConnected) {
      wasConnected = false;
      dbClient.stop();
      Serial.println("Wi-Fi kapcsolat megszakadt");
    }
    if (now - lastWifiTry >= WIFI_RETRY_MS) {
      lastWifiTry = now;
      Serial.println("Wi-Fi kapcsolodas...");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }
    return;
  }

  if (!wasConnected) {
    wasConnected = true;
    Serial.print("Wi-Fi OK, IP: ");
    Serial.print(WiFi.localIP());
    Serial.printf(", jelerosseg: %d dBm\n", WiFi.RSSI());
  }

  // No HTTPS before the clock is set, otherwise certificate checks fail.
  if (!timeSynced) {
    timeSynced = syncTime();
    if (!timeSynced) {
      delay(5000);
      return;
    }
    bootEpochMs = nowEpochMs();  // commands sent before this moment are ignored
  }

  if (idToken.length() == 0 || now - tokenObtainedAt >= TOKEN_LIFETIME_MS) {
    if (!signIn()) {
      delay(5000);
      return;
    }
  }

  if (now - lastBeat >= HEARTBEAT_MS) {
    lastBeat = now;
    if (sendHeartbeat()) {
      lastBeatOk = millis();
      Serial.println("Eletjel elkuldve");
    }
  }

  if (idToken.length() > 0 && millis() - lastPoll >= POLL_MS) {
    lastPoll = millis();
    pollCommands();
  }
}
