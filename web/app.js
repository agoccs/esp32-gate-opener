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

let auth = null;
let authApi = null;
let databaseApi = null;
let database = null;
let firebaseReady = false;
let stopStatusListener = null;
let statusRefreshTimer = null;
let lastSeenMs = null;

function showMessage(message, type = "") {
  authMessage.textContent = message;
  authMessage.className = `message${type ? ` ${type}` : ""}`;
}

function showLogin() {
  loginView.hidden = false;
  dashboardView.hidden = true;
  stopWatchingStatus();
}

function showDashboard(user) {
  loginView.hidden = true;
  dashboardView.hidden = false;
  document.querySelector("#dashboardTitle").textContent = "Kapuk";
  if (user) startWatchingStatus();
}

function setDeviceStatus(state, text) {
  deviceStatusDot.className = `status-dot ${state}`;
  deviceStatusText.textContent = text;
}

function refreshDeviceStatus() {
  if (lastSeenMs === null) {
    setDeviceStatus("unknown", "Még nincs életjel");
    return;
  }

  // The ESP32 will later write /device/lastSeen as Unix epoch milliseconds.
  const ageMs = Date.now() - lastSeenMs;
  if (ageMs >= 0 && ageMs <= 45_000) {
    setDeviceStatus("online", "Online");
  } else {
    setDeviceStatus("offline", "Offline · nincs friss életjel");
  }
}

function stopWatchingStatus() {
  if (typeof stopStatusListener === "function") stopStatusListener();
  stopStatusListener = null;
  if (statusRefreshTimer !== null) clearInterval(statusRefreshTimer);
  statusRefreshTimer = null;
  lastSeenMs = null;
  setDeviceStatus("unknown", "Állapot még nem érhető el");
}

function startWatchingStatus() {
  stopWatchingStatus();
  if (!database || !databaseApi) {
    setDeviceStatus("unknown", "Még nincs beállítva");
    return;
  }

  setDeviceStatus("unknown", "Kapcsolat ellenőrzése…");
  const heartbeatRef = databaseApi.ref(database, "device/lastSeen");
  stopStatusListener = databaseApi.onValue(
    heartbeatRef,
    (snapshot) => {
      const value = snapshot.val();
      lastSeenMs = value === null ? null : Number(value);
      if (lastSeenMs !== null && !Number.isFinite(lastSeenMs)) lastSeenMs = null;
      refreshDeviceStatus();
    },
    (error) => {
      console.error("ESP32 status listener failed:", error);
      lastSeenMs = null;
      setDeviceStatus("unknown", "Állapot nem érhető el");
    }
  );
  statusRefreshTimer = setInterval(refreshDeviceStatus, 5_000);
}

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
    const appSdk = await import("https://www.gstatic.com/firebasejs/13.0.0/firebase-app.js");
    authApi = await import("https://www.gstatic.com/firebasejs/13.0.0/firebase-auth.js");
    const firebaseApp = appSdk.initializeApp(firebaseConfig);
    auth = authApi.getAuth(firebaseApp);

    if (firebaseConfig.databaseURL && !firebaseConfig.databaseURL.includes("YOUR_")) {
      databaseApi = await import("https://www.gstatic.com/firebasejs/13.0.0/firebase-database.js");
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
