# ESP32 Gate Firmware

The final ESP32 firmware of the project. It connects to Wi-Fi and Firebase, reports its online status, receives the commands of the three web buttons and transmits the matching 433.92 MHz RF code.

## Files

| File | Purpose |
|---|---|
| `esp32_gate.ino` | The firmware (Arduino IDE sketch; the folder name must match the sketch name) |
| `google_roots.h` | Google root certificates used to verify the HTTPS connections (public data) |
| `secrets.example.h` | Template for the Wi-Fi and Firebase settings → copy to `secrets.h` |
| `rf_codes.example.h` | Template for the RF protocol, timing and gate codes → copy to `rf_codes.h` |

`secrets.h` and `rf_codes.h` are private and must never be published.

## How it works

1. **Wi-Fi:** station mode; if the connection drops, it retries every 10 seconds.
2. **Clock:** the time is set from NTP (`pool.ntp.org`, `time.google.com`). Certificate validation needs the real date, so no HTTPS request is made before this succeeds.
3. **Sign-in:** Firebase Authentication REST API (`signInWithPassword`) with the device account; the ID token is renewed after 50 minutes or after an HTTP 401/403.
4. **Heartbeat (every 10 s):** one multi-path update writes `device/lastSeen` (server timestamp) and `device/rssi` (Wi-Fi signal, dBm); the first one after boot also writes `device/lastBoot`.
5. **Commands (every 2 s):** `GET /commands.json?orderBy="status"&equalTo="pending"`. For each pending command, oldest first:

| Check | Result if it fails |
|---|---|
| `gate` is `bal`, `jobb` or `kapu` | `failed` (unknown gate) |
| sent after this boot | `expired` (before boot) |
| not older than 20 s | `expired` (too old) |
| RF code configured for the gate | `failed` (no code) |
| no transmission on the same gate in the last 10 s | `failed` (cooldown) |
| at most 6 transmissions in the last 60 s | `failed` (rate limit) |

If all checks pass, the ESP32 writes `taken` **before** transmitting, sends the RF code, then writes `done`. If writing `taken` fails, nothing is transmitted. A restart between the two steps can lose a command but can never repeat it.

6. **Watchdog:** if no heartbeat succeeds for 10 minutes (Wi-Fi, internet or Firebase problem), the ESP32 restarts itself.

**Secure connections:** every HTTPS connection is verified with `setCACert()` against the Google Trust Services roots in `google_roots.h` (GTS Root R1, R3 and R4, valid until 2036). The database connection is kept open and reused (`setReuse`), so polling does not open a new TLS session every 2 seconds.

## Requirements

- Arduino IDE with the ESP32 board package (esp32 by Espressif Systems)
- Libraries from the Library Manager: **ArduinoJson** (by Benoit Blanchon, version 7.x) and **rc-switch** (by sui77)
- RF433-JTX transmitter: DATA → GPIO 25, VCC → 5V/VIN, GND → GND, external 433 MHz antenna

## Setup

### 1. Firebase

Complete the Firebase setup in [`../../web/README.md`](../../web/README.md), including the ESP32 user in Firebase Authentication and your own user in `allowedUsers`. The ESP32 user's email must be the one in `database.rules.json`.

### 2. Secrets

Copy `secrets.example.h` to `secrets.h` and fill it in:

| Setting | Where to find it |
|---|---|
| `WIFI_SSID`, `WIFI_PASSWORD` | Your 2.4 GHz Wi-Fi network (the ESP32 does not support 5 GHz) |
| `FIREBASE_API_KEY` | `apiKey` in `web/firebase-config.local.js` |
| `DATABASE_URL` | `databaseURL` in the same file, without a trailing slash |
| `DEVICE_EMAIL`, `DEVICE_PASSWORD` | The ESP32 user created in Firebase Authentication |

Do not use double quotes (`"`) or backslashes (`\`) in the passwords.

### 3. RF codes

Copy `rf_codes.example.h` to `rf_codes.h` and enter the values measured from your own remote control (see [`../receiver`](../receiver/README.md) and [`../../analysis`](../../analysis/README.md)): protocol, bit length, pulse length, repeat count and one code per gate.

Keep `RF_DRY_RUN true` for the first tests: the ESP32 then handles commands normally but only prints which code it would send. Set it to `false` only when you are ready to test at the gates.

### 4. Upload

1. Open `esp32_gate.ino` in the Arduino IDE, select the ESP32 board and port, upload.
2. Open the Serial Monitor at **115200 baud**.

Expected output:

```
ESP32 kapuvezerlo - eletjel + parancsok
RF: beallitva, PROBA uzem (nincs adas)
Wi-Fi kapcsolodas...
Wi-Fi OK, IP: 192.168.x.x, jelerosseg: -61 dBm
Ido szinkronizalas... OK
Firebase bejelentkezes OK
Eletjel elkuldve
Parancs -Nx...: bal
  -> taken (HTTP 200)
  [PROBA] RF adas kihagyva: bal, kod ..., 12 bit
  -> done (HTTP 200)
```

## Troubleshooting

| Symptom | Likely cause |
|---|---|
| Compile error `ArduinoJson.h` or `RCSwitch.h: No such file` | Library not installed (see Requirements) |
| Compile error `rf_codes.h: No such file` | Copy `rf_codes.example.h` to `rf_codes.h` |
| Stuck on `Wi-Fi kapcsolodas...` | Wrong SSID/password, or a 5 GHz-only network |
| `Ido szinkronizalas SIKERTELEN` repeats | No internet access, or NTP is blocked on the network |
| `Bejelentkezes SIKERTELEN (HTTP 400)` | Wrong API key, or wrong device email/password |
| `Eletjel SIKERTELEN (HTTP 401/403)` | The device email does not match `database.rules.json`, or the rules were not deployed |
| `Parancs lekerdezes SIKERTELEN (HTTP 400)` with "Index not defined" | `.indexOn` for `commands` missing: deploy `database.rules.json` |
| `Kapcsolati hiba` with a `TLS:` message | Certificate check failed: wrong clock, a network that intercepts HTTPS, or Google moved to a root not in `google_roots.h` |
| Every command `failed (no code)` | `rf_codes.h` still has zeros |
| Web page shows *Offline* while the ESP32 reports success | Wrong system time on the viewing device |

## Known limitations

- Polling every 2 seconds gives up to 2–3 seconds of delay. Check the daily download in the Firebase console (Realtime Database → Usage); the free plan allows 10 GB per month.
- Firmware updates need a USB cable; there is no remote update.
- The trusted root certificates are compiled into the firmware. They are valid until 2036; if Google switches to a different root, `google_roots.h` must be updated.
- The device password is stored in the ESP32 flash in plain form.
- The page cannot see whether a gate is open or closed.

## Safety

Only use this with gates and devices you own or are authorized to control. Never publish working access codes or device-specific RF parameters. A gate moved remotely cannot be seen: make sure nobody is in its way.
