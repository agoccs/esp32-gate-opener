# ESP32 Gate Firmware (Wi-Fi + Firebase) 

This sketch connects the ESP32 to Wi-Fi and Firebase. At the moment it implements the **online-status heartbeat**: every 10 seconds the ESP32 reports to the Realtime Database that it is alive, and the web control panel shows it as *Online*.

RF transmission is **not** part of this sketch yet. The working RF transmitter test is in [`../transmitter`](../transmitter/README.md); merging it with this firmware, together with the command path from the web buttons, is the next development step.

## Files

| File | Purpose |
|---|---|
| `esp32_gate.ino` | The firmware (Arduino IDE sketch; the folder name must match the sketch name) |
| `google_roots.h` | Google root certificates used to verify the HTTPS connections (public data) |
| `secrets.example.h` | Template for all private settings |

## How it works

1. **Wi-Fi:** the ESP32 connects in station mode. If the connection drops, it retries every 10 seconds.
2. **Clock:** after the Wi-Fi connection is up, the time is set from NTP (`pool.ntp.org`, `time.google.com`). Certificate validation needs the real date, so no HTTPS request is made before this succeeds.
3. **Sign-in:** it logs in to Firebase Authentication through the REST API (`signInWithPassword`) with its own device account and receives an ID token.
4. **Token refresh:** the token is valid for 60 minutes. The sketch signs in again after 50 minutes, or immediately if the database answers with HTTP 401/403.
5. **Heartbeat:** every 10 seconds it sends `PUT <DATABASE_URL>/device/lastSeen.json?auth=<token>` with the value `{".sv":"timestamp"}`. The Firebase server replaces it with the current Unix time in milliseconds. The timestamp is written by the server, so the heartbeat itself does not depend on the ESP32 clock.
6. **Web page:** the control panel watches `device/lastSeen` and shows *Online* while the value is not older than 45 seconds.

**Secure connections:** every HTTPS connection is verified with `setCACert()` against the Google Trust Services roots in `google_roots.h` (GTS Root R1, R3 and R4, valid until 2036). If the server cannot prove its identity, the connection is refused and nothing is sent, so the device password and the token never reach an impostor.

Only the libraries built into the ESP32 Arduino core are used: `WiFi`, `WiFiClientSecure` and `HTTPClient`.

## Setup

### 1. Firebase

Complete the Firebase setup in [`../../web/README.md`](../../web/README.md), including a dedicated **ESP32 user** in Firebase Authentication (email + password). Its email address must be the one allowed to write `device/lastSeen` in `database.rules.json`.

### 2. Secrets

1. Copy `secrets.example.h` to `secrets.h` in this folder.
2. Fill in the values:

| Setting | Where to find it |
|---|---|
| `WIFI_SSID`, `WIFI_PASSWORD` | Your 2.4 GHz Wi-Fi network (the ESP32 does not support 5 GHz) |
| `FIREBASE_API_KEY` | `apiKey` in `web/firebase-config.local.js` |
| `DATABASE_URL` | `databaseURL` in the same file, without a trailing slash |
| `DEVICE_EMAIL`, `DEVICE_PASSWORD` | The ESP32 user created in Firebase Authentication |

Do not use double quotes (`"`) or backslashes (`\`) in the passwords: the sign-in request is built as a plain JSON string without escaping.

`secrets.h` is in `.gitignore` and must never be committed.

### 3. Upload

1. Open `esp32_gate.ino` in the Arduino IDE with the ESP32 board package installed.
2. Select your ESP32 board and port, then upload.
3. Open the Serial Monitor at **115200 baud**.

Expected output (the messages are in Hungarian without accents):

```
ESP32 kapuvezerlo - eletjel
Wi-Fi kapcsolodas...
Wi-Fi OK, IP: 192.168.x.x
Ido szinkronizalas... OK
Firebase bejelentkezes OK
Eletjel elkuldve
```

Within a few seconds the web page should switch to *Online*. The ESP32 can be seen as offline again 45 seconds after you unplug it.

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| Stuck on `Wi-Fi kapcsolodas...` | Wrong SSID/password, or a 5 GHz-only network |
| `Bejelentkezes SIKERTELEN (HTTP 400)` | Wrong API key, or wrong device email/password |
| `Eletjel SIKERTELEN (HTTP 401/403)` | The device email does not match the one in `database.rules.json`, or the rules were not deployed |
| `Eletjel SIKERTELEN (HTTP 404)` or connection error | Wrong `DATABASE_URL` (check region and trailing slash) |
| `Ido szinkronizalas SIKERTELEN` repeats | No internet access, or NTP is blocked on the network |
| `Kapcsolati hiba` with a `TLS:` message (HTTP code negative) | The certificate check failed: wrong clock, a network that intercepts HTTPS, or Google moved to a root that is not in `google_roots.h` |
| Web page shows *Offline* while the ESP32 reports success | Wrong system time on the viewing device |

## Known limitations

- The trusted root certificates are compiled into the firmware. They are valid until 2036, but if Google ever switches to a different root, `google_roots.h` has to be updated and the firmware re-uploaded.
- The device password is stored in the ESP32 flash in plain form.
- No RF transmission and no command handling yet.

## Safety

Only use this with gates and devices you own or are authorized to control. Never publish working access codes or device-specific RF parameters.
