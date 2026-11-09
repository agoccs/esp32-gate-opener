const loginView = document.querySelector("#loginView");
const dashboardView = document.querySelector("#dashboardView");
const loginForm = document.querySelector("#loginForm");
const loginButton = document.querySelector("#loginButton");
const authMessage = document.querySelector("#authMessage");
const logoutButton = document.querySelector("#logoutButton");
const emailInput = document.querySelector("#email");
const passwordInput = document.querySelector("#password");
const deviceStatusDot = document.querySelector("#deviceStatusDot");
const deviceStatusText = document.querySelector("#deviceStatusText");
const deviceSignal = document.querySelector("#deviceSignal");
const accessNote = document.querySelector("#accessNote");
const gateButtons = [...document.querySelectorAll(".gate-button[data-gate]")];

const SDK = "https://www.gstatic.com/firebasejs/11.10.0";
const ONLINE_MAX_AGE_MS = 45_000;   // heartbeat older than this = Offline
const CONFIRM_WINDOW_MS = 3_000;    // second tap must come within this time
const LOCK_AFTER_SEND_MS = 10_000;  // button stays disabled after sending
const NO_ANSWER_MS = 25_000;        // still pending after this = not delivered
const WATCH_MAX_MS = 40_000;        // stop following a command after this

let auth = null;
let authApi = null;
let databaseApi = null;
let database = null;
let firebaseReady = false;
let currentUser = null;
let allowed = false;
let stopStatusListener = null;
let statusRefreshTimer = null;
let lastSeenMs = null;
let deviceOnline = false;

const gateState = {};
for (const button of gateButtons) {
  gateState[button.dataset.gate] = { confirmTimer: null, lockedUntil: 0, busy: false, stopWatch: null, watchTimers: [] };
}

const REASONS = {
  "cooldown": "túl gyors ismétlés",
  "rate limit": "túl sok parancs egy percen belül",
  "no code": "nincs beállítva RF-kód",
  "too old": "túl későn ért oda",
  "before boot": "az ESP32 újraindulása előtt küldted",
  "unknown gate": "ismeretlen kapu",
};

function showMessage(message, type = "") {
  authMessage.textContent = message;
  authMessage.className = `message${type ? ` ${type}` : ""}`;
}

function showLogin() {
  loginView.hidden = false;
  dashboardView.hidden = true;
  currentUser = null;
  allowed = false;
  stopWatchingStatus();
  resetAllGates();
}

async function showDashboard(user) {
  loginView.hidden = true;
  dashboardView.hidden = false;
  currentUser = user;
  accessNote.hidden = true;
  allowed = await checkAccess(user);
  if (allowed) {
    startWatchingStatus();
  } else {
    setDeviceStatus("unknown", "Nincs jogosultság");
    accessNote.hidden = false;
    accessNote.textContent =
      "Ennek a fióknak még nincs vezérlési joga. A Firebase konzolban a Realtime Database " +
      `allowedUsers ága alá kell felvenni ezt az azonosítót true értékkel: ${user.uid}`;
  }
  updateButtons();
}

async function checkAccess(user) {
  if (!database || !databaseApi) return false;
  try {
    const snapshot = await databaseApi.get(databaseApi.ref(database, `allowedUsers/${user.uid}`));
    return snapshot.val() === true;
  } catch (error) {
    console.error("Access check failed:", error);
    return false;
  }
}

function setDeviceStatus(state, text) {
  deviceStatusDot.className = `status-dot ${state}`;
  deviceStatusText.textContent = text;
}

function refreshDeviceStatus() {
  if (lastSeenMs === null) {
    deviceOnline = false;
    setDeviceStatus("unknown", "Még nincs életjel");
  } else {
    const ageMs = Date.now() - lastSeenMs;
    deviceOnline = ageMs >= -10_000 && ageMs <= ONLINE_MAX_AGE_MS;
    if (deviceOnline) setDeviceStatus("online", "Online");
    else setDeviceStatus("offline", "Offline · nincs friss életjel");
  }
  updateButtons();
}

function stopWatchingStatus() {
  if (typeof stopStatusListener === "function") stopStatusListener();
  stopStatusListener = null;
  if (statusRefreshTimer !== null) clearInterval(statusRefreshTimer);
  statusRefreshTimer = null;
  lastSeenMs = null;
  deviceOnline = false;
  deviceSignal.textContent = "";
  setDeviceStatus("unknown", "Állapot még nem érhető el");
}

function startWatchingStatus() {
  stopWatchingStatus();
  if (!database || !databaseApi) {
    setDeviceStatus("unknown", "Még nincs beállítva");
    return;
  }

  setDeviceStatus("unknown", "Kapcsolat ellenőrzése…");
  stopStatusListener = databaseApi.onValue(
    databaseApi.ref(database, "device"),
    (snapshot) => {
      const device = snapshot.val() || {};
      const value = Number(device.lastSeen);
      lastSeenMs = Number.isFinite(value) && value > 0 ? value : null;
      const rssi = Number(device.rssi);
      deviceSignal.textContent = Number.isFinite(rssi) && rssi < 0 ? `· Wi-Fi: ${rssi} dBm` : "";
      refreshDeviceStatus();
    },
    (error) => {
      console.error("ESP32 status listener failed:", error);
      lastSeenMs = null;
      deviceOnline = false;
      setDeviceStatus("unknown", "Állapot nem érhető el");
      updateButtons();
    }
  );
  statusRefreshTimer = setInterval(refreshDeviceStatus, 5_000);
}

// ---------------------------------------------------------------- gates

function gateStateLine(gate) {
  return document.querySelector(`.gate-state[data-state-for="${gate}"]`);
}

function setGateText(gate, text, kind = "") {
  const line = gateStateLine(gate);
  if (!line) return;
  line.textContent = text;
  line.className = `gate-state${kind ? ` ${kind}` : ""}`;
}

function updateButtons() {
  const now = Date.now();
  for (const button of gateButtons) {
    const state = gateState[button.dataset.gate];
    const usable = allowed && deviceOnline && !state.busy && now >= state.lockedUntil;
    button.disabled = !usable;
    if (!usable && state.confirmTimer !== null) cancelConfirm(button);
  }
}

function cancelConfirm(button) {
  const state = gateState[button.dataset.gate];
  clearTimeout(state.confirmTimer);
  state.confirmTimer = null;
  button.classList.remove("confirm");
  button.textContent = "Nyitás / zárás";
}

function resetAllGates() {
  for (const button of gateButtons) {
    const gate = button.dataset.gate;
    const state = gateState[gate];
    cancelConfirm(button);
    stopWatchingCommand(gate);
    state.busy = false;
    state.lockedUntil = 0;
    setGateText(gate, "");
    button.disabled = true;
  }
}

function stopWatchingCommand(gate) {
  const state = gateState[gate];
  if (typeof state.stopWatch === "function") state.stopWatch();
  state.stopWatch = null;
  for (const timer of state.watchTimers) clearTimeout(timer);
  state.watchTimers = [];
}

function onGateClick(button) {
  const gate = button.dataset.gate;
  const state = gateState[gate];
  if (button.disabled) return;

  // First tap: ask for confirmation
  if (state.confirmTimer === null) {
    button.classList.add("confirm");
    button.textContent = "Biztos? Érintsd meg újra";
    state.confirmTimer = setTimeout(() => cancelConfirm(button), CONFIRM_WINDOW_MS);
    return;
  }

  // Second tap: send
  cancelConfirm(button);
  sendCommand(gate);
}

async function sendCommand(gate) {
  const state = gateState[gate];
  if (!currentUser || !database || !databaseApi) return;

  stopWatchingCommand(gate);
  state.busy = true;
  state.lockedUntil = Date.now() + LOCK_AFTER_SEND_MS;
  updateButtons();
  setGateText(gate, "Küldés…");

  let commandRef;
  try {
    commandRef = databaseApi.push(databaseApi.ref(database, "commands"));
    await databaseApi.set(commandRef, {
      gate,
      status: "pending",
      requestedAt: databaseApi.serverTimestamp(),
      requestedBy: currentUser.uid,
    });
  } catch (error) {
    console.error("Sending command failed:", error);
    setGateText(gate, "Nem sikerült elküldeni (jogosultság vagy hálózat).", "error");
    state.busy = false;
    updateButtons();
    return;
  }

  setGateText(gate, "Elküldve, az ESP32-re vár…");
  state.busy = false;
  updateButtons();
  setTimeout(updateButtons, LOCK_AFTER_SEND_MS + 100);

  let finished = false;
  state.stopWatch = databaseApi.onValue(
    commandRef,
    (snapshot) => {
      const command = snapshot.val();
      if (!command) return;
      const reason = REASONS[command.reason] || command.reason || "";
      switch (command.status) {
        case "pending":
          break;
        case "taken":
          setGateText(gate, "Az ESP32 átvette, adás…");
          break;
        case "done":
          finished = true;
          setGateText(gate, `Végrehajtva (${new Date().toLocaleTimeString("hu-HU")})`, "ok");
          stopWatchingCommand(gate);
          break;
        case "expired":
          finished = true;
          setGateText(gate, `Lejárt, nem hajtódott végre${reason ? `: ${reason}` : ""}.`, "error");
          stopWatchingCommand(gate);
          break;
        case "failed":
          finished = true;
          setGateText(gate, `Nem hajtódott végre${reason ? `: ${reason}` : ""}.`, "error");
          stopWatchingCommand(gate);
          break;
        default:
          break;
      }
    },
    (error) => console.error("Command listener failed:", error)
  );

  state.watchTimers.push(setTimeout(() => {
    if (!finished) setGateText(gate, "Nem érkezett visszaigazolás az ESP32-től.", "error");
  }, NO_ANSWER_MS));
  state.watchTimers.push(setTimeout(() => stopWatchingCommand(gate), WATCH_MAX_MS));
}

for (const button of gateButtons) {
  button.addEventListener("click", () => onGateClick(button));
}

// ---------------------------------------------------------------- login

function friendlyAuthError(error) {
  switch (error?.code) {
    case "auth/invalid-credential":
    case "auth/wrong-password":
    case "auth/user-not-found":
      return "Hibás e-mail-cím vagy jelszó.";
    case "auth/too-many-requests":
      return "Túl sok sikertelen próbálkozás. Próbáld meg később.";
    case "auth/network-request-failed":
      return "Hálózati hiba. Ellenőrizd az internetkapcsolatot.";
    case "auth/invalid-email":
      return "Adj meg érvényes e-mail-címet.";
    default:
      return "Nem sikerült bejelentkezni. Ellenőrizd a Firebase-beállításokat.";
  }
}

async function initializeFirebase() {
  let firebaseConfig;
  try {
    ({ firebaseConfig } = await import("./firebase-config.local.js"));
  } catch {
    showMessage("Előnézet: a valódi bejelentkezés még nincs beállítva.");
    return;
  }

  const requiredFields = ["apiKey", "authDomain", "projectId", "appId"];
  const configured = firebaseConfig && requiredFields.every((field) => {
    const value = firebaseConfig[field];
    return typeof value === "string" && value.trim().length > 0 && !value.includes("YOUR_");
  });

  if (!configured) {
    showMessage("Előnézet: a Firebase-konfiguráció hiányzik vagy nincs kitöltve.");
    return;
  }

  try {
    const appSdk = await import(`${SDK}/firebase-app.js`);
    authApi = await import(`${SDK}/firebase-auth.js`);
    const firebaseApp = appSdk.initializeApp(firebaseConfig);
    auth = authApi.getAuth(firebaseApp);

    if (firebaseConfig.databaseURL && !firebaseConfig.databaseURL.includes("YOUR_")) {
      databaseApi = await import(`${SDK}/firebase-database.js`);
      database = databaseApi.getDatabase(firebaseApp);
    }

    firebaseReady = true;
    authApi.onAuthStateChanged(auth, (user) => {
      if (user) showDashboard(user);
      else showLogin();
    });
    showMessage("");
  } catch (error) {
    console.error("Firebase initialization failed:", error);
    showMessage("Nem sikerült elindítani a Firebase-t. Ellenőrizd a konfigurációt.", "error");
  }
}

loginForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  const email = emailInput.value.trim();
  const password = passwordInput.value;

  if (!email || !password) {
    showMessage("Add meg az e-mail-címedet és a jelszavadat.", "error");
    return;
  }
  if (!firebaseReady || !auth || !authApi) {
    showMessage("Az oldal előnézeti módban van; a Firebase-bejelentkezést még be kell állítani.");
    return;
  }

  loginButton.disabled = true;
  loginButton.textContent = "Bejelentkezés…";
  showMessage("");
  try {
    await authApi.signInWithEmailAndPassword(auth, email, password);
    passwordInput.value = "";
  } catch (error) {
    showMessage(friendlyAuthError(error), "error");
  } finally {
    loginButton.disabled = false;
    loginButton.textContent = "Bejelentkezés";
  }
});

logoutButton.addEventListener("click", async () => {
  if (!auth || !authApi) return;
  try {
    await authApi.signOut(auth);
    passwordInput.value = "";
    showMessage("");
    showLogin();
  } catch (error) {
    console.error("Sign-out failed:", error);
    setDeviceStatus("unknown", "Kijelentkezés sikertelen");
  }
});

initializeFirebase();
