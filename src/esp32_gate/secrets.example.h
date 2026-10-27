// Masold ezt a fajlt "secrets.h" neven, es toltsd ki.
// A secrets.h-t NE tedd fel GitHubra (kerul a .gitignore-ba).
// Tipp: az ESP32 csak 2.4 GHz-es Wi-Fi-t tud.
// A jelszavakban ne legyen idezojel (") es perjel (\).

#define WIFI_SSID        "A_WIFI_NEVED"
#define WIFI_PASSWORD    "A_WIFI_JELSZAVAD"

// web/firebase-config.local.js -> apiKey
#define FIREBASE_API_KEY "AIza..."

// web/firebase-config.local.js -> databaseURL (a vegen NINCS perjel)
#define DATABASE_URL     "https://esp32-gate-opener-825-default-rtdb.europe-west1.firebasedatabase.app"

// A Firebase Authentication-ben az ESP32-nek letrehozott felhasznalo
#define DEVICE_EMAIL     "esp32@gate.local"
#define DEVICE_PASSWORD  "AZ_ESP32_JELSZAVA"
