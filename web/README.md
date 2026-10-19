# Webes vezérlőfelület

A `web/` mappa a projekt egyszerű, mobilbarát webes kezelőfelületének forráskódját tartalmazza.

## Fájlok

- `index.html` – bejelentkezési oldal és a három kaput megjelenítő vezérlőfelület.
- `style.css` – megjelenés és mobilos elrendezés.
- `app.js` – Firebase-bejelentkezés és ESP32-státusz kijelzése.
- `firebase-config.example.js` – példa a Firebase webalkalmazás konfigurációjára.

## Helyi előnézet

A fájlokat ne közvetlenül `file://` URL-ről nyisd meg, mert a JavaScript modulok és a Firebase CDN-ből betöltött könyvtárai HTTP/HTTPS kiszolgálást igényelhetnek. Ha telepítve van a Python, nyiss terminált ebben a mappában, majd futtasd:

```bash
python -m http.server 8000
```

Ezután nyisd meg a `http://localhost:8000` címet a böngészőben. A látvány így ellenőrizhető Firebase-konfiguráció nélkül is. A tényleges bejelentkezéshez az alábbi beállítások kellenek.

## Firebase-beállítás

1. Hozz létre Firebase-projektet és regisztrálj benne Web Appot.
2. Kapcsold be az Authentication → Sign-in method → Email/Password szolgáltatást.
3. Hozd létre a használható felhasználókat a Firebase konzolban; nyilvános regisztráció nem része ennek az oldalnak.
4. Engedélyezd a Realtime Database-t, és hozz létre szigorú olvasási/írási szabályokat.
5. Másold a `firebase-config.example.js` fájlt `firebase-config.local.js` néven, és töltsd ki a Firebase konzolban látható konfigurációval, beleértve a pontos `databaseURL` értéket is.
6. A `firebase-config.local.js` ki van zárva a Gitből a gyökér `.gitignore` fájlban. A frontend konfigurációja ettől még látható lesz a böngészőben; a jogosultságokat a Firebase Authentication és a biztonsági szabályok biztosítják.

## ESP32 online állapot

A weboldal a Realtime Database `device/lastSeen` útvonalát figyeli. Az ESP32-programnak később legalább 15 másodpercenként frissítenie kell ezt az értéket Unix epoch **milliszekundumban**. A weboldal akkor jelez `Online` állapotot, ha az utolsó életjel legfeljebb 45 másodperces. Régi életjelet `Offline` állapotként jelez, hiányzó adatnál pedig `Még nincs életjel` látható.

A státuszfigyelés most csak frontend-előkészítés: amíg az ESP32 nem ír életjelet, nem lehet megbízhatóan online/offline állapotot megállapítani.

## Vezérlés állapota

A három vezérlőgomb szándékosan le van tiltva. Még nincs bekötve a távoli parancsküldés, ezért ezen az oldalon jelenleg nem indítható rádiós adás.

## Biztonság

- Ne commitold a Wi-Fi-jelszót, felhasználói jelszót, ESP32-titkot, Admin SDK-kulcsot vagy működő RF-kódot.
- Ne engedélyezz nyilvános adatbázis-írást.
- A vezérlőparancsok bekötése előtt külön parancsérvényesítést, egyszeri végrehajtást és újraindítás esetén biztonságos viselkedést kell megvalósítani.
