# Web Control Panel

The `web/` folder contains the projects web-based control panel's code.

## Files

- `index.html` – login page and control panel for my three virtual gate opener button (set according to yours).
- `style.css` – Style and mobile layout.
- `app.js` – Firebase-login and ESP status checker.
- `firebase-config.example.js` – example for the Firebase web-app configuration.

## Local preview

Do not open the files directly from a `file://` URL, as JavaScript modules and Firebase libraries loaded from the CDN may require HTTP/HTTPS serving. If you have Python installed, open a terminal in this folder and run:

```bash
python -m http.server 8000
```

Then open `http://localhost:8000` in your browser. This way, you can check how it looks even without a Firebase configuration. For the actual login, the following settings are required.

## Firebase-setup

1. Make a Firebase-project and register a Web App.
2. Turn on Authentication → Sign-in method → Email/Password.
3. Create users for running tests inside Firebase console; public registration is disabled.
4. Allow Realtime Database, and create strict read/write rules.
5. Copy `firebase-config.example.js` file as `firebase-config.local.js` and fill up with configuration seen in Firebase console, using exact `databaseURL` values.
6. The `firebase-config.local.js` is not inside the Git as it is in `.gitignore` file. The front-end configuration is visible in browser; the login authentication managed by Firebase Authentication.

## ESP32 online status

The website monitors the Realtime Database `device/lastSeen` path. The ESP32 program must later update this value at least every 15 seconds in Unix epoch **milliseconds**. The website indicates an `Online` status if the last heartbeat is at most 45 seconds old. An old heartbeat is indicated as an `Offline` status, and if data is missing, `No heartbeat yet` is displayed.

Status monitoring is currently just frontend preparation: until the ESP32 writes a heartbeat, it is not possible to reliably determine the online/offline status.

## Control status

The three control buttons are intentionally disabled. Remote command sending is not yet connected, so radio transmission cannot currently be initiated on this page.

## Security

- Do not commit the Wi-Fi password, user password, ESP32 secret, Admin SDK key, or a working RF code.

- Do not allow public database writing.

- Before connecting control commands, separate command validation, single execution, and safe behavior upon restart must be implemented.
