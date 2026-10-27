# Web Control Panel

The `web/` folder contains the project's web control panel, hosted on Firebase Hosting. It is deliberately simple: a login page and, after login, three gate cards and an indicator showing whether the ESP32 is online. The interface text is in Hungarian.

## Files

| File | Purpose |
|---|---|
| `index.html` | Login view and dashboard (three gate buttons, ESP32 status line) |
| `style.css` | Styling and mobile layout |
| `app.js` | Firebase login/logout and ESP32 status monitoring |
| `firebase-config.example.js` | Template for the Firebase web app configuration |

The Firebase configuration files are in the repository root: `firebase.json`, `.firebaserc` and `database.rules.json`.

## How it works

- `app.js` is an ES module. It loads the Firebase SDK (pinned to version 11.10.0) from Google's CDN, so no build step or `npm` is needed.
- The Firebase web configuration is imported at runtime from `firebase-config.local.js`. If that file is missing or still contains placeholders, the page starts in a preview mode where login is not available.
- Login uses Firebase Authentication (email + password).
- After login the page listens to `device/lastSeen` in the Realtime Database and refreshes the indicator every 5 seconds.

### ESP32 status indicator

The ESP32 writes a server-side timestamp (Unix epoch, milliseconds) to `device/lastSeen` every 10 seconds (see [`src/esp32_gate`](../src/esp32_gate/README.md)).

| Age of last heartbeat | Shown as |
|---|---|
| up to 45 seconds | Online |
| older than 45 seconds | Offline |
| no value in the database | No heartbeat yet |

### Gate buttons

The three buttons are currently **disabled**. The command path to the ESP32 is not implemented yet, so the page cannot trigger any RF transmission.

## Firebase setup

1. Create a Firebase project (the free Spark plan is enough) and register a **Web app**.
2. **Authentication → Sign-in method:** enable Email/Password.
3. **Authentication → Users:** create the users manually:
   - one account for each person who may use the page,
   - one account for the ESP32 (its email must match the one in `database.rules.json` and in `secrets.h`).
4. Check the Authentication settings and disable public sign-up if the option is available. The web API key is public by design, so otherwise anyone could create an account through the Firebase API.
5. **Realtime Database:** create a database and note its exact URL.
6. Copy `firebase-config.example.js` to `firebase-config.local.js` and fill in the values from the Firebase console (Project settings → Your apps), including the exact `databaseURL`.
7. Adjust `.firebaserc` to your project id (or run `firebase use --add`).

`firebase-config.local.js` is listed in `.gitignore`. It is still uploaded to Firebase Hosting on deploy, because the page needs it at runtime. The web configuration is visible to every visitor anyway; access is protected by Firebase Authentication and the database rules, not by hiding this file.

## Database rules

`database.rules.json` (repository root):

- everything is closed by default;
- any logged-in user may read `device/*`;
- only the ESP32 account (matched by its email address) may write `device/lastSeen`.

Before the control buttons are connected, the rules must be tightened so that only specific users can send commands.

## Local preview

Do not open the files through `file://`. Serve the folder over HTTP:

```bash
cd web
python -m http.server 8000
```

Then open `http://localhost:8000`. Without `firebase-config.local.js` you see the preview mode; with it, login and the status indicator work against your Firebase project.

## Deploy

Install the [Firebase CLI](https://firebase.google.com/docs/cli), log in, then from the repository root:

```bash
firebase deploy
```

This publishes the `web/` folder to Firebase Hosting and the rules from `database.rules.json`. `firebase.json` excludes dotfiles and `*.example.js` from the upload.

## Security notes

- Never commit Wi-Fi passwords, user passwords, the ESP32 device password, Admin SDK keys or working RF codes.
- Never allow public read or write access in the database rules.
- Keep stray copies of the configuration (for example `firebase_config`) out of this folder, because everything inside `web/` is deployed to the public site. 
