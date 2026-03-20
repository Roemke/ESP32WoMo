#pragma once
#include <pgmspace.h>

// ----------------------------------------------------------------
// Platzhalter (werden beim Ausliefern durch processor() ersetzt):
//   %WIFI_MAC_AP%   %WIFI_MAC_STA%   %WIFI_MODE%
//   %BME_SDA%       %BME_SCL%        %BME_ADDR%
//   %BME_INTERVAL%  %VE_RX%          %VE_BAUD%
//
// Messwerte kommen per Polling alle 5s von /api/data
// Konfigurationswerte werden beim Laden per /api/config geholt
// ----------------------------------------------------------------

const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang='de'>
<head>
<meta charset='utf-8'>
<meta name='viewport' content='width=device-width'>
<title>Sensor ESP</title>
<style>
  body {
    font-family: Arial, sans-serif;
    font-size: 1.4em;
    margin: 0; padding: 10px;
    background: #1a1a2e;
    color: #eeeeff;
  }
  h2 { color: #aaaaff; margin: 12px 0 8px 0; }

  /* ---- Tabs ---- */
  .tab { overflow: hidden; background: #0d0d1a; border-bottom: 2px solid #3a3a8e; }
  .tab button {
    background: inherit; border: none; outline: none;
    cursor: pointer; padding: 12px 20px;
    color: #8888bb; font-size: 1em;
  }
  .tab button:hover  { background: #1a1a3e; color: #ffffff; }
  .tab button.active { background: #16213e; color: #ffffff; }
  .tabcontent { display: none; padding: 16px; }

  /* ---- Key-Value Zeilen ---- */
  .kv { display: flex; gap: 10px; align-items: baseline; margin-bottom: 10px; }
  .kv label { min-width: 200px; flex-shrink: 0; color: #8888bb; }

  /* ---- Badges ---- */
  .badge {
    display: inline-block; padding: 3px 10px;
    border-radius: 10px; background: #2a2a4e; color: #eeeeff; font-weight: bold;
  }
  .badge.ok      { background: #2e7d32; color: #fff; }
  .badge.warn    { background: #f9a825; color: #000; }
  .badge.err     { background: #c62828; color: #fff; }
  .badge.neutral { background: #444466; color: #fff; }

  /* ---- Formular ---- */
  .form-row { display: flex; align-items: center; margin-bottom: 10px; gap: 10px; }
  .form-row label { min-width: 200px; flex-shrink: 0; color: #8888bb; }
  .form-row input[type=text],
  .form-row input[type=password],
  .form-row input[type=number] {
    background: #0d0d1a; color: #eeeeff;
    border: 1px solid #3a3a8e; padding: 5px 10px;
    border-radius: 4px; font-size: 0.9em; width: 220px;
  }
  .form-row input[type=checkbox] { width: 20px; height: 20px; cursor: pointer; }

  /* ---- Buttons ---- */
  .btn {
    background: #2255cc; color: #fff; border: none;
    padding: 8px 20px; cursor: pointer; border-radius: 4px;
    font-size: 0.9em; margin: 6px 4px 6px 0;
  }
  .btn:hover { background: #44aaff; }

  /* ---- Info ---- */
  .infoField {
    background: #1a3a1a; border: 2px solid #2e7d32;
    padding: 10px; border-radius: 4px; margin-top: 8px; font-size: 0.85em;
  }
  .invisible { display: none; }

  /* ---- Log ---- */
  #logContainer {
    height: 260px; overflow-y: auto;
    background: #0d0d1a; padding: 10px;
    font-family: monospace; font-size: 0.8em; color: #aaffaa;
    border: 1px solid #3a3a8e; border-radius: 4px; margin-top: 8px;
  }
  #logContainer div { border-bottom: 1px solid #1a1a2e; padding: 2px 0; }

  /* ---- Statusleiste ---- */
  #statusbar { font-size: 0.75em; color: #666688; margin-bottom: 10px; }

  #ip-fields { margin-left: 10px; }
</style>

<script>
// ================================================================
// Polling – alle 5 Sekunden /api/data abrufen
// ================================================================
let logFrom = 0;
async function poll() {
  try {
    const r = await fetch('/api/data?logFrom=' + logFrom);
    const d = await r.json();
console.log(JSON.stringify(d.bme));
    setText('statusIP',     d.wifi || '-');
    setText('statusUpdate', new Date().toLocaleTimeString());

    // BME280
    if (d.bme && d.bme.valid) {
      setBadge('valTemp',  d.bme.T + ' C',   tempClass(d.bme.T));
      setBadge('valHum',   d.bme.H + ' %%',  humClass(d.bme.H));
      setBadge('valPress', d.bme.P + ' hPa', 'neutral');
    }

    // VE.Direct / BMV712
    if (d.vedirect && d.vedirect.valid) {
      setBadge('valV',   d.vedirect.V   + ' V',  'neutral');
      setBadge('valI',   d.vedirect.I   + ' A',  'neutral');
      setBadge('valP',   d.vedirect.P   + ' W',  'neutral');
      setBadge('valSOC', d.vedirect.SOC + ' %%', socClass(d.vedirect.SOC));
      setBadge('valTTG', ttgFormat(d.vedirect.TTG), 'neutral');
      setBadge('valVS',  d.vedirect.VS  + ' V',  'neutral');
    }

    // Log – neue Zeilen anhängen
    if (d.log && d.log.length > 0) {
            const c = document.getElementById('logContainer');
            d.log.forEach(line => {
                const div = document.createElement('div');
                div.textContent = line;
                c.appendChild(div);
            });
            c.scrollTop = c.scrollHeight;
        }
        if (typeof d.logCount !== 'undefined')
            logFrom = d.logCount;
    } catch(e) {
        setText('statusUpdate', 'Fehler');
  }
}

// ================================================================
// Konfiguration laden beim Start
// ================================================================
async function loadConfig() {
  try {
    const r = await fetch('/api/config');
    const d = await r.json();
    setVal('cfg-bme-sda',      d.bme_sda);
    setVal('cfg-bme-scl',      d.bme_scl);
    setVal('cfg-bme-addr',     d.bme_addr);
    setVal('cfg-bme-interval', d.bme_interval_ms);
    setVal('cfg-ve-rx',        d.vedirect_rx);
    setVal('cfg-ve-baud',      d.vedirect_baud);
  } catch(e) {}
}

// ================================================================
// Konfiguration speichern
// ================================================================
async function saveConfig() {
  const body = {
    bme_sda:         parseInt(getVal('cfg-bme-sda')),
    bme_scl:         parseInt(getVal('cfg-bme-scl')),
    bme_addr:        parseInt(getVal('cfg-bme-addr')),
    bme_interval_ms: parseInt(getVal('cfg-bme-interval')),
    vedirect_rx:     parseInt(getVal('cfg-ve-rx')),
    vedirect_baud:   parseInt(getVal('cfg-ve-baud'))
  };
  try {
    await fetch('/api/config', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify(body)
    });
    document.getElementById('cfgInfo').classList.remove('invisible');
  } catch(e) {}
}

// ================================================================
// WiFi speichern
// ================================================================
async function saveWifi() {
  const body = {
    ssid:          getVal('wifiSsid'),
    password:      getVal('wifiPass'),
    use_static_ip: document.getElementById('wifiStatic').checked,
    static_ip:     getVal('wifiIP'),
    subnet:        getVal('wifiSN')
  };
  try {
    await fetch('/api/config/wifi', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify(body)
    });
    document.getElementById('wifiInfo').classList.remove('invisible');
  } catch(e) {}
}

// ================================================================
// Hilfsfunktionen
// ================================================================
function setText(id, v)   { const el = document.getElementById(id); if (el) el.textContent = v; }
function getVal(id)        { return document.getElementById(id).value; }
function setVal(id, v)     { document.getElementById(id).value = v; }

function setBadge(id, text, cls) {
  const el = document.getElementById(id);
  if (!el) return;
  el.textContent = text;
  el.className = 'badge ' + (cls || 'neutral');
}

function ttgFormat(min) {
  if (min < 0) return '---';
  return Math.floor(min / 60) + 'h ' + (min %% 60) + 'm';
}

function tempClass(t) {
  if (t < 5 || t > 35) return 'err';
  if (t < 18 || t > 28) return 'warn';
  return 'ok';
}
function humClass(h) {
  if (h < 30 || h > 70) return 'warn';
  return 'ok';
}
function socClass(s) {
  if (s < 20) return 'err';
  if (s < 50) return 'warn';
  return 'ok';
}

function toggleIP(cb) {
  document.getElementById('ip-fields').style.display = cb.checked ? 'block' : 'none';
}

function openTab(evt, name) {
  document.querySelectorAll('.tabcontent').forEach(t => t.style.display = 'none');
  document.querySelectorAll('.tab button').forEach(b => b.classList.remove('active'));
  document.getElementById(name).style.display = 'block';
  evt.currentTarget.classList.add('active');
}

window.addEventListener('load', () => {
  document.getElementById('defaultTab').click();  
  poll();
  setInterval(poll, 5000);
});
</script>
</head>

<body>

<!-- Statusleiste -->
<div id="statusbar">
  IP: <span id="statusIP">...</span>
  &nbsp;|&nbsp;
  Modus: %WIFI_MODE%
  &nbsp;|&nbsp;
  letztes Update: <span id="statusUpdate">-</span>
</div>

<!-- Tab-Leiste -->
<div class="tab">
  <button class="tablink" id="defaultTab" onclick="openTab(event,'sensoren')">Sensoren</button>
  <button class="tablink"                 onclick="openTab(event,'config')">Konfiguration</button>
  <button class="tablink"                 onclick="openTab(event,'help')">Hilfe</button>
</div>


<!-- ============================================================ -->
<!-- TAB: SENSOREN                                               -->
<!-- ============================================================ -->
<div id="sensoren" class="tabcontent">
  <h2>Klima (BME280)</h2>
  <div class="kv"><label>Temperatur:</label>     <span class="badge neutral" id="valTemp">---</span></div>
  <div class="kv"><label>Feuchte:</label>        <span class="badge neutral" id="valHum">---</span></div>
  <div class="kv"><label>Luftdruck:</label>      <span class="badge neutral" id="valPress">---</span></div>

  <h2>Batterie (BMV712)</h2>
  <div class="kv"><label>Spannung:</label>       <span class="badge neutral" id="valV">---</span></div>
  <div class="kv"><label>Strom:</label>          <span class="badge neutral" id="valI">---</span></div>
  <div class="kv"><label>Leistung:</label>       <span class="badge neutral" id="valP">---</span></div>
  <div class="kv"><label>SoC:</label>            <span class="badge neutral" id="valSOC">---</span></div>
  <div class="kv"><label>Restlaufzeit:</label>   <span class="badge neutral" id="valTTG">---</span></div>
  <div class="kv"><label>Starterbatterie:</label><span class="badge neutral" id="valVS">---</span></div>
</div>


<!-- ============================================================ -->
<!-- TAB: KONFIGURATION                                          -->
<!-- ============================================================ -->
<div id="config" class="tabcontent">

  <h2>Gerät</h2>
  <div class="kv"><label>MAC AP-Modus:</label>     <span>%WIFI_MAC_AP%</span></div>
  <div class="kv"><label>MAC Client-Modus:</label> <span>%WIFI_MAC_STA%</span></div>

  <h2>Sensor-Pins</h2>
    <div class="form-row"><label>BME280 SDA (GPIO)</label>   <input type="number" id="cfg-bme-sda"      value="%BME_SDA%"></div>
    <div class="form-row"><label>BME280 SCL (GPIO)</label>   <input type="number" id="cfg-bme-scl"      value="%BME_SCL%"></div>
    <div class="form-row"><label>BME280 Adresse</label>      <input type="number" id="cfg-bme-addr"     value="%BME_ADDR%"></div>
    <div class="form-row"><label>Messintervall (ms)</label>  <input type="number" id="cfg-bme-interval" value="%BME_INTERVAL%"></div>
    <div class="form-row"><label>VE.Direct RX (GPIO)</label> <input type="number" id="cfg-ve-rx"        value="%VE_RX%"></div>
    <div class="form-row"><label>VE.Direct Baud</label>      <input type="number" id="cfg-ve-baud"      value="%VE_BAUD%"></div>
    <button class="btn" onclick="saveConfig()">Speichern &amp; Neustart</button>
    <div class="infoField invisible" id="cfgInfo">
        <strong>Konfiguration gespeichert.</strong> Der ESP startet neu.
    </div>

  <h2>WLAN Konfiguration</h2>
  <div class="form-row"><label>SSID</label>    <input type="text"     id="wifiSsid" placeholder="Netzwerkname"></div>
  <div class="form-row"><label>Passwort</label><input type="password" id="wifiPass" placeholder="Passwort"></div>
  <div class="form-row">
    <label>Feste IP-Adresse</label>
    <input type="checkbox" id="wifiStatic" onchange="toggleIP(this)">
  </div>
  <div id="ip-fields" style="display:none">
    <div class="form-row"><label>IP-Adresse</label><input type="text" id="wifiIP" value="192.168.1.100"></div>
    <div class="form-row"><label>Subnetz</label>   <input type="text" id="wifiSN"  value="255.255.255.0"></div>
  </div>
  <button class="btn" onclick="saveWifi()">WLAN Speichern &amp; Neustart</button>
  <div class="infoField invisible" id="wifiInfo">
    <strong>Daten gespeichert.</strong> Der ESP startet neu und verbindet sich mit dem neuen Netzwerk.
  </div>

  <h2>System</h2>
  <button class="btn" onclick="fetch('/api/reboot',{method:'POST'})">ESP Neustart</button>
  <a href="/update"><button class="btn">OTA Update</button></a>
</div>


<!-- ============================================================ -->
<!-- TAB: HILFE                                                  -->
<!-- ============================================================ -->
<div id="help" class="tabcontent">
  <h2>Hilfe</h2>
  <ul>
    <li>Tab <strong>Sensoren</strong>: Aktuelle Messwerte von BME280 und Victron BMV712</li>
    <li>Tab <strong>Konfiguration</strong>: GPIO-Pins, Messintervall, WLAN, IP-Einstellungen</li>
    <li>Werte werden automatisch alle 5 Sekunden aktualisiert</li>
    <li>OTA-Update unter <a href="/update" style="color:#44aaff">/update</a></li>
  </ul>

  <h2>Log-Nachrichten</h2>
  <button class="btn" onclick="document.getElementById('logContainer').innerHTML=''">Log leeren</button>
  <div id="logContainer"></div>
</div>

</body>
</html>
)rawliteral";
