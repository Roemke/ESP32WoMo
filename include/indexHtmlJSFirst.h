#pragma once
#include <pgmspace.h>

// ----------------------------------------------------------------
// Platzhalter (werden beim Ausliefern durch processor() ersetzt):
//   %WIFI_MAC_AP%      %WIFI_MAC_STA%     %WIFI_MODE%
//   %WIFI_USE_STATIC%  %WIFI_STATIC_IP%   %WIFI_SUBNET%
//   %SENSOR_ESP_IP%
//   %WLED_INNEN_IP%    %WLED_AUSSEN_IP%
//
// Sensordaten kommen per Polling alle 5s von /api/data
// ----------------------------------------------------------------

const char index_html[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang='de'>
<head>
<meta charset='utf-8'>
<meta name='viewport' content='width=device-width'>
<title>WoMo Display</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<style>
  body {
    font-family: Arial, sans-serif;
    font-size: 1.25em;
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

  /* ---- Zweispaltig für Status ---- */
  .status-grid { display: flex; gap: 20px; flex-wrap: wrap; }
  .status-box {
    background: #16213e; border: 1px solid #3a3a8e;
    border-radius: 8px; padding: 12px 20px;
    min-width: 300px; flex: 1;
  }

  /* ---- Key-Value Zeilen ---- */
  .kv { display: flex; gap: 10px; align-items: baseline; margin-bottom: 10px; }
  .kv label { min-width: 200px; flex-shrink: 0; color: #8888bb; }

  /* ---- Badges ---- */
  .badge {
    display: inline-block; padding: 3px 10px;
    border-radius: 10px; background: #2a2a4e;
    color: #eeeeff; font-weight: bold;
  }
  .badge.ok      { background: #2e7d32; color: #fff; }
  .badge.warn    { background: #f9a825; color: #000; }
  .badge.err     { background: #c62828; color: #fff; }
  .badge.neutral { background: #444466; color: #fff; }

  /* ---- Formular ---- */
  .form-row { display: flex; align-items: center; margin-bottom: 10px; gap: 10px; }
  .form-row label { min-width: 220px; flex-shrink: 0; color: #8888bb; }
  .form-row input[type=text],
  .form-row input[type=password] {
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
    padding: 10px; border-radius: 4px;
    margin-top: 8px; font-size: 0.85em;
  }
  .invisible { display: none; }

  /* ---- History Charts ---- */
  .chart-wrap {
    background: #16213e; border: 1px solid #3a3a8e;
    border-radius: 8px; padding: 12px; margin-bottom: 16px;
    height: 220px;
  }

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

  @media (max-width: 600px) { .status-grid { flex-direction: column; } }
</style>

<script>
// ================================================================
// History-Ringpuffer im Browser (max. 60 Einträge)
// ================================================================
const HISTORY_MAX = 60;
const hist = {
  labels: [],
  temp: [], hum: [], press: [],
  voltage: [], soc: [], current: []
};

function historyPush(d) {
  hist.labels.push(new Date().toLocaleTimeString());
  hist.temp.push(d.bme    && d.bme.valid     ? d.bme.T         : null);
  hist.hum.push(d.bme     && d.bme.valid     ? d.bme.H         : null);
  hist.press.push(d.bme   && d.bme.valid     ? d.bme.P         : null);
  hist.voltage.push(d.vedirect && d.vedirect.valid ? d.vedirect.V   : null);
  hist.soc.push(d.vedirect     && d.vedirect.valid ? d.vedirect.SOC : null);
  hist.current.push(d.vedirect && d.vedirect.valid ? d.vedirect.I   : null);
  if (hist.labels.length > HISTORY_MAX)
    Object.keys(hist).forEach(k => hist[k].shift());
  if (chartKlima)    chartKlima.update();
  if (chartBatterie) chartBatterie.update();
}

// ================================================================
// Charts
// ================================================================
let chartKlima    = null;
let chartBatterie = null;

function initCharts() {
  const gridColor = '#2a2a4e';
  const opts = {
    animation: false, responsive: true, maintainAspectRatio: false,
    plugins: { legend: { labels: { color: '#aaaaff' } } },
    scales: {
      x: { ticks: { color: '#666688', maxTicksLimit: 10 }, grid: { color: gridColor } },
      y: { ticks: { color: '#8888bb' },                    grid: { color: gridColor } }
    }
  };

  chartKlima = new Chart(document.getElementById('chartKlima'), {
    type: 'line', options: opts,
    data: {
      labels: hist.labels,
      datasets: [
        { label: 'Temperatur (°C)', data: hist.temp,  borderColor: '#ff7043', tension: 0.3, pointRadius: 2 },
        { label: 'Feuchte (%%)',     data: hist.hum,   borderColor: '#42a5f5', tension: 0.3, pointRadius: 2 }
      ]
    }
  });

  chartBatterie = new Chart(document.getElementById('chartBatterie'), {
    type: 'line', options: opts,
    data: {
      labels: hist.labels,
      datasets: [
        { label: 'Spannung (V)', data: hist.voltage, borderColor: '#66bb6a', tension: 0.3, pointRadius: 2 },
        { label: 'SoC (%%)',      data: hist.soc,     borderColor: '#ffa726', tension: 0.3, pointRadius: 2 },
        { label: 'Strom (A)',    data: hist.current, borderColor: '#ab47bc', tension: 0.3, pointRadius: 2 }
      ]
    }
  });
}

// ================================================================
// Polling – alle 5 Sekunden /api/data abrufen
// ================================================================
let logFrom = 0;

async function poll() {
  try {
    const r = await fetch('/api/data?logFrom=' + logFrom);
    const d = await r.json();

    setText('statusIP',     d.wifi || '-');
    setText('statusUpdate', new Date().toLocaleTimeString());

    // Klima
    if (d.bme && d.bme.valid) {
      setBadge('valTemp',  d.bme.T + ' °C',  tempClass(d.bme.T));
      setBadge('valHum',   d.bme.H + ' %%',   humClass(d.bme.H));
      setBadge('valPress', d.bme.P + ' hPa', 'neutral');
    }
    if (d.co2 && d.co2.valid)
      setBadge('valCO2', d.co2.ppm + ' ppm', co2Class(d.co2.ppm));

    // Batterie
    if (d.vedirect && d.vedirect.valid) {
      setBadge('valV',   d.vedirect.V   + ' V',  'neutral');
      setBadge('valI',   d.vedirect.I   + ' A',  'neutral');
      setBadge('valP',   d.vedirect.P   + ' W',  'neutral');
      setBadge('valSOC', d.vedirect.SOC + ' %%',  socClass(d.vedirect.SOC));
      setBadge('valTTG', ttgFormat(d.vedirect.TTG), 'neutral');
      setBadge('valVS',  d.vedirect.VS  + ' V',  'neutral');
    }

    historyPush(d);

    // Log
    if (d.log && d.log.length > 0) {
      const c = document.getElementById('logContainer');
      d.log.forEach(line => {
        const div = document.createElement('div');
        div.textContent = line;
        c.appendChild(div);
      });
      c.scrollTop = c.scrollHeight;
    }
    if (typeof d.logCount !== 'undefined') logFrom = d.logCount;

  } catch(e) { setText('statusUpdate', 'Fehler'); }
}

// ================================================================
// IPs speichern (kein Neustart nötig)
// ================================================================
async function saveIPs() {
  const body = {
    sensor_esp_ip:  getVal('cfgSensorIP'),
    wled_innen_ip:  getVal('cfgWledInnen'),
    wled_aussen_ip: getVal('cfgWledAussen'),
    sensor_poll_interval_ms: parseInt(getVal('cfgPollInterval'))
  };
  try {
    await fetch('/api/config', {
      method: 'POST', headers: {'Content-Type': 'application/json'},
      body: JSON.stringify(body)
    });
    document.getElementById('ipInfo').classList.remove('invisible');
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
      method: 'POST', headers: {'Content-Type': 'application/json'},
      body: JSON.stringify(body)
    });
    document.getElementById('wifiInfo').classList.remove('invisible');
  } catch(e) {}
}

// ================================================================
// Hilfsfunktionen
// ================================================================
function setText(id, v) { const el = document.getElementById(id); if (el) el.textContent = v; }
function getVal(id)      { return document.getElementById(id).value; }

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

function tempClass(t) { if (t < 5 || t > 35) return 'err'; if (t < 18 || t > 28) return 'warn'; return 'ok'; }
function humClass(h)  { return (h < 30 || h > 70) ? 'warn' : 'ok'; }
function socClass(s)  { if (s < 20) return 'err'; if (s < 50) return 'warn'; return 'ok'; }
function co2Class(p)  { if (p > 2000) return 'err'; if (p > 1000) return 'warn'; return 'ok'; }

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
  // Checkbox-Zustand aus Processor wiederherstellen
  const cb = document.getElementById('wifiStatic');
  if (cb && cb.checked)
    document.getElementById('ip-fields').style.display = 'block';
  initCharts();
  poll();
  setInterval(poll, 5000);
});

  async function listSD() {
      const dir = getVal('sdDir');
      try {
          const r = await fetch('/api/sd/list?dir=' + encodeURIComponent(dir));
          const d = await r.json();
          const div = document.getElementById('sdFiles');
          div.innerHTML = '';
          d.files.forEach(f => {
              const line = document.createElement('div');
              if (f.dir) {
                  line.textContent = '📁 ' + f.name;
              } else {
                  line.innerHTML = `📄 ${f.name} (${(f.size/1024).toFixed(1)} KB) ` +
                      `<a href="/api/sd/download?file=${encodeURIComponent(dir+'/'+f.name)}" ` +
                      `style="color:#44aaff">Download</a>`;
              }
              div.appendChild(line);
          });
      } catch(e) { document.getElementById('sdFiles').textContent = 'Fehler'; }
  }

  async function uploadSD() {
      const file = document.getElementById('sdUploadFile').files[0];
      if (!file) return;
      const dir  = getVal('sdUploadDir');
      const form = new FormData();
      form.append('file', file);
      try {
          const r = await fetch('/api/sd/upload?dir=' + encodeURIComponent(dir), {
              method: 'POST', body: form
          });
          document.getElementById('sdUploadStatus').textContent =
              r.ok ? 'Hochgeladen!' : 'Fehler';
      } catch(e) {
          document.getElementById('sdUploadStatus').textContent = 'Fehler';
      }
  }
</script>
</head>

<body>

<!-- Statusleiste -->
<div id="statusbar">
  IP: <span id="statusIP">...</span>
  &nbsp;|&nbsp;
  Modus: %WIFI_MODE%
  &nbsp;|&nbsp;
  Update: <span id="statusUpdate">-</span>
</div>

<!-- Tab-Leiste -->
<div class="tab">
  <button class="tablink" id="defaultTab" onclick="openTab(event,'status')">Status</button>
  <button class="tablink"                 onclick="openTab(event,'history')">Verlauf</button>
  <button class="tablink"                 onclick="openTab(event,'config')">Konfiguration</button>
  <button class="tablink"                 onclick="openTab(event,'help')">Hilfe</button>
</div>


<!-- ============================================================ -->
<!-- TAB: STATUS                                                 -->
<!-- ============================================================ -->
<div id="status" class="tabcontent">
  <div class="status-grid">

    <div class="status-box">
      <h2>Batterie (BMV712)</h2>
      <div class="kv"><label>Spannung:</label>        <span class="badge neutral" id="valV">---</span></div>
      <div class="kv"><label>Strom:</label>           <span class="badge neutral" id="valI">---</span></div>
      <div class="kv"><label>Leistung:</label>        <span class="badge neutral" id="valP">---</span></div>
      <div class="kv"><label>SoC:</label>             <span class="badge neutral" id="valSOC">---</span></div>
      <div class="kv"><label>Restlaufzeit:</label>    <span class="badge neutral" id="valTTG">---</span></div>
      <div class="kv"><label>Starterbatterie:</label> <span class="badge neutral" id="valVS">---</span></div>
    </div>

    <div class="status-box">
      <h2>Klima</h2>
      <div class="kv"><label>Temperatur:</label> <span class="badge neutral" id="valTemp">---</span></div>
      <div class="kv"><label>Feuchte:</label>    <span class="badge neutral" id="valHum">---</span></div>
      <div class="kv"><label>Luftdruck:</label>  <span class="badge neutral" id="valPress">---</span></div>
      <div class="kv"><label>CO₂:</label>        <span class="badge neutral" id="valCO2">---</span></div>
    </div>

  </div>
</div>


<!-- ============================================================ -->
<!-- TAB: VERLAUF                                                -->
<!-- ============================================================ -->
<div id="history" class="tabcontent">
  <h2>Klima-Verlauf</h2>
  <div class="chart-wrap">
    <canvas id="chartKlima"></canvas>
  </div>

  <h2>Batterie-Verlauf</h2>
  <div class="chart-wrap">
    <canvas id="chartBatterie"></canvas>
  </div>
</div>


<!-- ============================================================ -->
<!-- TAB: KONFIGURATION                                          -->
<!-- ============================================================ -->
<div id="config" class="tabcontent">

  <h2>Gerät</h2>
  <div class="kv"><label>MAC AP-Modus:</label>     <span>%WIFI_MAC_AP%</span></div>
  <div class="kv"><label>MAC Client-Modus:</label> <span>%WIFI_MAC_STA%</span></div>

  <h2>Geräte-IPs</h2>
  <div class="form-row"><label>Sensor-ESP IP</label>  <input type="text" id="cfgSensorIP"   value="%SENSOR_ESP_IP%"></div>
  <div class="form-row"><label>WLED Innen IP</label>  <input type="text" id="cfgWledInnen"  value="%WLED_INNEN_IP%"></div>
  <div class="form-row"><label>WLED Außen IP</label>  <input type="text" id="cfgWledAussen" value="%WLED_AUSSEN_IP%"></div>
  
  <h2>Polling</h2>
  <div class="form-row">
    <label>Sensor-Poll Intervall (ms)</label>
    <input type="number" id="cfgPollInterval" value="%SENSOR_POLL_INTERVAL%" min="1100" step="100">
  </div>
  <p style="color:#666688; font-size:0.85em; margin-top:-6px;">
    Mindestens 2000ms empfohlen. Kleinere Werte können die UI verlangsamen, Sensor liest mit 1000ms.
  </p>
  <button class="btn" onclick="saveIPs()">Speichern</button>
  <div class="infoField invisible" id="ipInfo">
    <strong>IPs / Intervall gespeichert.</strong> Änderungen werden beim nächsten Poll aktiv.
  </div>
  <h2>WLAN Konfiguration</h2>
  <div class="form-row"><label>SSID</label>    <input type="text"     id="wifiSsid" placeholder="Netzwerkname"></div>
  <div class="form-row"><label>Passwort</label><input type="password" id="wifiPass" placeholder="Passwort"></div>
  <div class="form-row">
    <label>Feste IP-Adresse</label>
    <input type="checkbox" id="wifiStatic" onchange="toggleIP(this)" %WIFI_USE_STATIC%>
  </div>
  <div id="ip-fields" style="display:none">
    <div class="form-row"><label>IP-Adresse</label><input type="text" id="wifiIP" value="%WIFI_STATIC_IP%"></div>
    <div class="form-row"><label>Subnetz</label>   <input type="text" id="wifiSN"  value="%WIFI_SUBNET%"></div>
  </div>
  <button class="btn" onclick="saveWifi()">WLAN Speichern &amp; Neustart</button>
  <div class="infoField invisible" id="wifiInfo">
    <strong>Daten gespeichert.</strong> Der ESP startet neu.
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
    <li>Tab <strong>Status</strong>: Aktuelle Messwerte vom Sensor-ESP</li>
    <li>Tab <strong>Verlauf</strong>: Grafische Darstellung der letzten 60 Messwerte</li>
    <li>Tab <strong>Konfiguration</strong>: IPs der Geräte, WLAN, feste IP</li>
    <li>Werte werden automatisch alle 5 Sekunden aktualisiert</li>
    <li>OTA-Update unter <a href="/update" style="color:#44aaff">/update</a></li>
  </ul>
  <h2>SD-Karte</h2>
  <div class="form-row">
    <label>Verzeichnis</label>
    <input type="text" id="sdDir" value="/2025">
    <button class="btn" onclick="listSD()">Auflisten</button>
  </div>
  <div id="sdFiles" style="margin-top:8px"></div>

  <h2>Datei hochladen</h2>
  <div class="form-row">
    <label>Zielverzeichnis</label>
    <input type="text" id="sdUploadDir" value="/2025">
  </div>
  <input type="file" id="sdUploadFile" accept=".csv">
  <button class="btn" onclick="uploadSD()">Hochladen</button>
  <div class="status" id="sdUploadStatus"></div>

  <h2>Log-Nachrichten</h2>
  <button class="btn" onclick="document.getElementById('logContainer').innerHTML=''">Log leeren</button>
  <div id="logContainer"></div>
</div>

</body>
</html>
)rawliteral";
