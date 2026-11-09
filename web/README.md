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
- After login the page checks `allowedUsers/<uid>`. Only listed users see the device status and can use the buttons; others see their user id, so it can be added in the console.
- The page listens to `device` (`lastSeen`, `rssi`) and refreshes the indicator every 5 seconds.

### ESP32 status indicator

The ESP32 writes a server-side timestamp (Unix epoch, milliseconds) to `device/lastSeen` every 10 seconds (see [`src/esp32_gate`](../src/esp32_gate/README.md)).

| Age of last heartbeat | Shown as |
|---|---|
| up to 45 seconds | Online |
| older than 45 seconds | Offline |
| no value in the database | No heartbeat yet |

### Gate buttons

The buttons are active only when the user is allowed and the ESP32 is online.

1. The first tap asks for confirmation ("Biztos? Érintsd meg újra") for 3 seconds; the second tap sends the command.
2. The page creates `commands/<id>` with `gate` (`bal`, `jobb` or `kapu`), `status: "pending"`, a server timestamp and the user's id.
3. The ESP32 picks it up within about 2 seconds and updates the status; the page shows each step under the gate name.
4. The button stays disabled for 10 seconds after sending.

| Status | Shown as |
|---|---|
| `pending` | Elküldve, az ESP32-re vár… |
| `taken` | Az ESP32 átvette, adás… |
| `done` | Végrehajtva (time) |
| `expired` | Lejárt, nem hajtódott végre: reason |
| `failed` | Nem hajtódott végre: reason (e.g. cooldown, rate limit) |
| still `pending` after 25 s | Nem érkezett visszaigazolás az ESP32-től |

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
8. Allow your user to send commands: copy its **User UID** from Authentication → Users, then in **Realtime Database → Data** add a child `allowedUsers`, under it a child named after the UID with the value `true` (type boolean). The page also shows the UID of a logged-in user that is not allowed yet.

`firebase-config.local.js` is listed in `.gitignore`. It is still uploaded to Firebase Hosting on deploy, because the page needs it at runtime. The web configuration is visible to every visitor anyway; access is protected by Firebase Authentication and the database rules, not by hiding this file.

## Database rules

`database.rules.json` (repository root). Everything is closed by default; the rest is opened path by path:

| Path | Read | Write |
|---|---|---|
| `allowedUsers/<uid>` | that user only (to know whether it is allowed) | nobody from a client; edited in the console |
| `device/lastSeen`, `device/lastBoot`, `device/rssi` | allowed users | the ESP32 account only, numbers only |
| `commands` (query by `status`) | the ESP32 account | – |
| `commands/<id>` | the user who created it, and the ESP32 | allowed user: create only, with `status: "pending"`; ESP32: change the status |

Further checks on every command: `gate` is one of `bal`, `jobb`, `kapu`; `requestedBy` must be the writer's own id; `requestedAt` must be the server time (not older than 60 s); the status can only move forward (`pending` → `taken`/`expired`/`failed`, `taken` → `done`/`failed`); no extra fields; nobody can delete a command.

Test the rules after every change in the console: **Realtime Database → Rules → Rules Playground** (simulate a read or write as unauthenticated, as an allowed user and as another user).

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

To deploy only one part: `firebase deploy --only database` (rules) or `firebase deploy --only hosting` (web page).

This publishes the `web/` folder to Firebase Hosting and the rules from `database.rules.json`. `firebase.json` excludes dotfiles and `*.example.js` from the upload.

## Security notes

- Never commit Wi-Fi passwords, user passwords, the ESP32 device password, Admin SDK keys or working RF codes.
- Never allow public read or write access in the database rules.
- Keep stray copies of the configuration (for example `firebase_config`) out of this folder, because everything inside `web/` is deployed to the public site. 
