#pragma once
#include <pgmspace.h>

// ----------------------------------------------------------------
// Platzhalter (werden beim Ausliefern durch processor() ersetzt):
//   %WIFI_MAC_AP%      %WIFI_MAC_STA%     %WIFI_MODE%
//   %WIFI_USE_STATIC%  %WIFI_STATIC_IP%   %WIFI_SUBNET%
//   %SENSOR_ESP_IP%
//   %WLED_INNEN_IP%    %WLED_AUSSEN_IP%
//
// Aktuelle Sensordaten: Polling alle 5s von /api/data
// Verlaufsdaten:        /api/history?from=...&to=...&points=N
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
    font-size: 1.2em;
    margin: 0; padding: 10px;
    background: #1a1a2e;
    color: #eeeeff;
  }
  h2 { color: #aaaaff; margin: 12px 0 8px 0; }
  h4 { color: #aaaaff; margin: 12px 0 8px 0; }
  /* ---- Tabs ---- */
  .tab { overflow: hidden; background: #0d0d1a; border-bottom: 2px solid #3a3a8e; }
  .tab button {
    background: inherit; border: none; outline: none;
    cursor: pointer; padding: 12px 16px;
    color: #8888bb; font-size: 0.9em;
  }
  .tab button:hover  { background: #1a1a3e; color: #ffffff; }
  .tab button.active { background: #16213e; color: #ffffff; }
  .tabcontent { display: none; padding: 16px; }

  /* ---- Zweispaltig für Status ---- */
  .status-grid { display: flex; gap: 20px; flex-wrap: wrap; }
  .status-box {
    background: #16213e; border: 1px solid #3a3a8e;
    border-radius: 8px; padding: 12px 20px;
    min-width: 250px; flex: 1;
  }
  /* ---- für die Statistiken --- */
  tr.alt { background: #1a2a4e; }

  /* ---- Key-Value Zeilen ---- */
  .kv { display: flex; gap: 10px; align-items: baseline; margin-bottom: 10px; }
  .kv label { min-width: 135px; flex-shrink: 0; color: #8888bb; }

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
  .form-row input[type=password],
  .form-row input[type=datetime-local] {
    background: #0d0d1a; color: #eeeeff;
    border: 1px solid #3a3a8e; padding: 5px 10px;
    border-radius: 4px; font-size: 0.9em;
  }
  .form-row input[type=checkbox] { width: 20px; height: 20px; cursor: pointer; }

  /* ---- Buttons ---- */
  .btn {
    background: #2255cc; color: #fff; border: none;
    padding: 8px 20px; cursor: pointer; border-radius: 4px;
    font-size: 0.9em; margin: 6px 4px 6px 0;
  }
  .btn:hover { background: #44aaff; }
  .btn.active { background: #44aaff; }

  /* ---- Info ---- */
  .infoField {
    background: #1a3a1a; border: 2px solid #2e7d32;
    padding: 10px; border-radius: 4px;
    margin-top: 8px; font-size: 0.85em;
  }
  .invisible { display: none; }

  /* ---- History ---- */
  .history-controls {
    display: flex; gap: 10px; align-items: center;
    flex-wrap: wrap; margin-bottom: 16px;
    background: #16213e; border: 1px solid #3a3a8e;
    border-radius: 8px; padding: 10px;
  }
  .history-controls label { color: #8888bb; font-size: 0.9em; }
  .chart-wrap {
    background: #16213e; border: 1px solid #3a3a8e;
    border-radius: 8px; padding: 12px; margin-bottom: 16px;
    position: relative; height: 300px;
  }
  .chart-loading {
    position: absolute; top: 50%%; left: 50%%;
    transform: translate(-50%%, -50%%);
    color: #666688; font-size: 0.9em;
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
// Hilfsfunktionen
// ================================================================
function setText(id, v) { const el = document.getElementById(id); if (el) el.textContent = v; }
function getVal(id)      { return document.getElementById(id).value; }
function setVal(id, v)   { document.getElementById(id).value = v; }

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

// Statistiken laden und setzen 
async function loadStats() {
  const hours = getVal('statsHours');
  try {
    const r = await fetch('/api/stats?hours=' + hours);
    const d = await r.json();
    if (!d.valid) return;

    const s = (key, suffix, dec) => {
      setText('st-' + key + '-min', d[key.toUpperCase()].min.toFixed(dec) + suffix);
      setText('st-' + key + '-max', d[key.toUpperCase()].max.toFixed(dec) + suffix);
      setText('st-' + key + '-avg', d[key.toUpperCase()].avg.toFixed(dec) + suffix);
    };

    s('t',   ' °C', 1);
    s('h',   ' %%',  1);
    s('p',   ' hPa', 1);
    s('v',   ' V',  2);
    s('i',   ' A',  2);
    s('soc', ' %%',  1);
    s('pw',  ' W',  1);
    s('vs',  ' V',  2);

    // CO2 separat – int
    setText('st-co2-min', d.CO2.min + ' ppm');
    setText('st-co2-max', d.CO2.max + ' ppm');
    setText('st-co2-avg', d.CO2.avg + ' ppm');
    
    // MPPT1
    if (d.MPPT1_V) {
      setText('st-mppt1v-min',  d.MPPT1_V.min.toFixed(2)  + ' V');
      setText('st-mppt1v-max',  d.MPPT1_V.max.toFixed(2)  + ' V');
      setText('st-mppt1v-avg',  d.MPPT1_V.avg.toFixed(2)  + ' V');
      setText('st-mppt1pv-min', d.MPPT1_PV.min.toFixed(1) + ' W');
      setText('st-mppt1pv-max', d.MPPT1_PV.max.toFixed(1) + ' W');
      setText('st-mppt1pv-avg', d.MPPT1_PV.avg.toFixed(1) + ' W');
    }
    // MPPT2
    if (d.MPPT2_V) {
      setText('st-mppt2v-min',  d.MPPT2_V.min.toFixed(2)  + ' V');
      setText('st-mppt2v-max',  d.MPPT2_V.max.toFixed(2)  + ' V');
      setText('st-mppt2v-avg',  d.MPPT2_V.avg.toFixed(2)  + ' V');
      setText('st-mppt2pv-min', d.MPPT2_PV.min.toFixed(1) + ' W');
      setText('st-mppt2pv-max', d.MPPT2_PV.max.toFixed(1) + ' W');
      setText('st-mppt2pv-avg', d.MPPT2_PV.avg.toFixed(1) + ' W');
    }
    // Charger
    if (d.CHARGER_V) {
      setText('st-chargerv-min', d.CHARGER_V.min.toFixed(2) + ' V');
      setText('st-chargerv-max', d.CHARGER_V.max.toFixed(2) + ' V');
      setText('st-chargerv-avg', d.CHARGER_V.avg.toFixed(2) + ' V');
      setText('st-chargeri-min', d.CHARGER_I.min.toFixed(2) + ' A');
      setText('st-chargeri-max', d.CHARGER_I.max.toFixed(2) + ' A');
      setText('st-chargeri-avg', d.CHARGER_I.avg.toFixed(2) + ' A');
    }

  } catch(e) {}
}  
async function initStatsHours() {
    try {
        const r = await fetch('/api/capacity');
        const d = await r.json();
        const maxHours = d.maxHours;
        const sel = document.getElementById('statsHours');
        sel.innerHTML = '';
        [1, 4, 8, 12, 16, 24, maxHours].forEach(h => {
            const opt = document.createElement('option');
            opt.value = h;
            opt.textContent = h === maxHours ? 'Max (' + h + 'h)' : h + 'h';
            if (h === 12) opt.selected = true;
            sel.appendChild(opt);
        });
    } catch(e) {}
}

// ================================================================
// Polling – alle 2 Sekunden /api/data abrufen
// ================================================================
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
    // MPPT1
    if (d.mppt1 && d.mppt1.valid) {
      setBadge('mppt1V',     d.mppt1.V        + ' V',  'neutral');
      setBadge('mppt1I',     d.mppt1.I        + ' A',  'neutral');
      setBadge('mppt1PV',    d.mppt1.PV       + ' W',  'neutral');
      setBadge('mppt1State', d.mppt1.stateStr || '---', 'neutral');
      setBadge('mppt1Y',     d.mppt1.yield    + ' Wh', 'neutral');
    }
    // MPPT2
    if (d.mppt2 && d.mppt2.valid) {
      setBadge('mppt2V',     d.mppt2.V        + ' V',  'neutral');
      setBadge('mppt2I',     d.mppt2.I        + ' A',  'neutral');
      setBadge('mppt2PV',    d.mppt2.PV       + ' W',  'neutral');
      setBadge('mppt2State', d.mppt2.stateStr || '---', 'neutral');
      setBadge('mppt2Y',     d.mppt2.yield    + ' Wh', 'neutral');
    }
    // Charger
    if (d.charger && d.charger.valid) {
      setBadge('chargerV',     d.charger.V        + ' V',  'neutral');
      setBadge('chargerI',     d.charger.I        + ' A',  'neutral');
      setBadge('chargerState', d.charger.stateStr || '---', 'neutral');
    }

  } catch(e) { setText('statusUpdate', 'Fehler'); }
}
// Log-Polling separat
let logFrom = 0;
async function pollLog() {
    try {
        const r = await fetch('/api/log?from=' + logFrom);
        const d = await r.json();
        if (d.log && d.log.length > 0) {
            const c = document.getElementById('logContainer');
            d.log.forEach(line => {
                const div = document.createElement('div');
                div.textContent = line;
                c.appendChild(div);
            });
            c.scrollTop = c.scrollHeight;
        }
        if (typeof d.count !== 'undefined') logFrom = d.count;
    } catch(e) {}
}

// ================================================================
// History – Datepicker initialisieren (letzten 24h als Default)
// ================================================================
function initDatepickers() {
  const now  = new Date();
  const ago  = new Date(now - 24 * 3600 * 1000);

  function toLocal(d) {
    // datetime-local braucht Format YYYY-MM-DDTHH:MM
    return d.getFullYear() + '-' +
      String(d.getMonth()+1).padStart(2,'0') + '-' +
      String(d.getDate()).padStart(2,'0') + 'T' +
      String(d.getHours()).padStart(2,'0') + ':' +
      String(d.getMinutes()).padStart(2,'0');
  }

  ['klima','bat'].forEach(ch => {
    setVal('from_' + ch, toLocal(ago));
    setVal('to_'   + ch, toLocal(now));
  });
}

// ================================================================
// History laden und Charts zeichnen
// ================================================================
let chartKlima    = null;
let chartBatterie = null;

const CHART_DEFAULTS = {
  animation: false,
  responsive: true,
  maintainAspectRatio: false,
  plugins: { legend: { labels: { color: '#aaaaff', font: { size: 13 } } } },
  scales: {
    x: { ticks: { color: '#666688', maxTicksLimit: 10, font: { size: 11 } },
         grid: { color: '#2a2a4e' } },
    y: { ticks: { color: '#8888bb', font: { size: 11 } },
         grid: { color: '#2a2a4e' } }
  }
};

async function loadHistory(channel) {
  const fromId  = 'from_' + channel;
  const toId    = 'to_'   + channel;
  const from    = getVal(fromId);
  const to      = getVal(toId);
  const canvas  = document.getElementById('chart_' + channel);
  const loading = document.getElementById('loading_' + channel);

  if (!from || !to) { alert('Bitte Start und Ende wählen'); return; }

  const points = canvas.offsetWidth || 600;
  if (loading) loading.style.display = 'block';

  const controller = new AbortController();
  const timeout = setTimeout(() => controller.abort(), 25000); // 25s

  try {
    const r = await fetch(
      `/api/history?from=${encodeURIComponent(from)}&to=${encodeURIComponent(to)}&points=${points}`,
      { signal: controller.signal }
    );
    clearTimeout(timeout);
    const d = await r.json();

    if (loading) loading.style.display = 'none';

    if (d.error) { alert('Fehler: ' + d.error); return; }

    if (channel === 'klima') {
      if (chartKlima) chartKlima.destroy();
      chartKlima = new Chart(canvas, {
        type: 'line',
        options: CHART_DEFAULTS,
        data: {
          labels: d.labels,
          datasets: [
            { label: 'Temperatur (°C)', data: d.T,   borderColor: '#ff7043', tension: 0.3, pointRadius: 0 },
            { label: 'Feuchte (%%)',     data: d.H,   borderColor: '#42a5f5', tension: 0.3, pointRadius: 0 },
            { label: 'Luftdruck (hPa)', data: d.P,   borderColor: '#ab47bc', tension: 0.3, pointRadius: 0, hidden: true },
            { label: 'CO2 (ppm)',       data: d.CO2, borderColor: '#66bb6a', tension: 0.3, pointRadius: 0, hidden: true }
          ]
        }
      });
    } else {
      if (chartBatterie) chartBatterie.destroy();
      chartBatterie = new Chart(canvas, {
        type: 'line',
        options: CHART_DEFAULTS,
        data: {
          labels: d.labels,
          datasets: [
            { label: 'Spannung (V)', data: d.V,   borderColor: '#66bb6a', tension: 0.3, pointRadius: 0 },
            { label: 'SoC (%%)',      data: d.SOC, borderColor: '#ffa726', tension: 0.3, pointRadius: 0 },
            { label: 'Strom (A)',    data: d.I,   borderColor: '#ab47bc', tension: 0.3, pointRadius: 0 },
            { label: 'Leistung (W)', data: d.PW,  borderColor: '#ef5350', tension: 0.3, pointRadius: 0, hidden: true },
            { label: 'Starter (V)', data: d.VS, borderColor: '#26c6da', tension: 0.3, pointRadius: 0, hidden: true } 
          ]
        }
      });
    }

    // Info anzeigen
    setText('info_' + channel, `${d.total} Messwerte → ${d.labels.length} Punkte`);

  } catch(e) {
    if (loading) loading.style.display = 'none';
    alert('Verbindungsfehler');
  }
}

// ================================================================
// IPs speichern
// ================================================================
async function saveIPs() {
  const body = {
    sensor_esp_ip:  getVal('cfgSensorIP'),
    wled_innen_ip:  getVal('cfgWledInnen'),
    wled_aussen_ip: getVal('cfgWledAussen')
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
      subnet:        getVal('wifiSN'),
      gateway:       getVal('wifiGW'),   // neu
      dns:           getVal('wifiDNS')   // neu
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
// Start
// ================================================================
window.addEventListener('load', () => {
  document.getElementById('defaultTab').click();
  const cb = document.getElementById('wifiStatic');
  if (cb && cb.checked)
    document.getElementById('ip-fields').style.display = 'block';
  initDatepickers();
  poll();
  setInterval(poll, 2000);
  pollLog();
  setInterval(pollLog, 5000);
  initStatsHours();
  loadStats();
  setInterval(loadStats, 2000); // alle 2 Sekunden
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
  Update: <span id="statusUpdate">-</span>
</div>

<!-- Tab-Leiste -->
<div class="tab">
  <button class="tablink" id="defaultTab" onclick="openTab(event,'status')">Status</button>
  <button class="tablink"                 onclick="openTab(event,'histKlima')">Klima-Verlauf</button>
  <button class="tablink"                 onclick="openTab(event,'histBat')">Batterie-Verlauf</button>
  <button class="tablink"                 onclick="openTab(event,'beleuchtung')">Beleuchtung</button>
  <button class="tablink"                 onclick="openTab(event,'config')">Konfiguration</button>
  <button class="tablink"                 onclick="openTab(event,'help')">Hilfe</button>
</div>


<!-- ============================================================ -->
<!-- TAB: STATUS                                                 -->
<!-- ============================================================ -->
<div id="status" class="tabcontent">
  <div class="status-grid">
   <div class="status-box">
      <h2>Klima</h2>
      <div class="kv"><label>Temperatur:</label> <span class="badge neutral" id="valTemp">---</span></div>
      <div class="kv"><label>Feuchte:</label>    <span class="badge neutral" id="valHum">---</span></div>
      <div class="kv"><label>Luftdruck:</label>  <span class="badge neutral" id="valPress">---</span></div>
      <div class="kv"><label>CO2:</label>        <span class="badge neutral" id="valCO2">---</span></div>
    </div>
    <div class="status-box">
      <h2>Batterie (BMV712)</h2>
      <div class="kv"><label>Spannung:</label>        <span class="badge neutral" id="valV">---</span></div>
      <div class="kv"><label>Strom:</label>           <span class="badge neutral" id="valI">---</span></div>
      <div class="kv"><label>Leistung:</label>        <span class="badge neutral" id="valP">---</span></div>
      <div class="kv"><label>SoC:</label>             <span class="badge neutral" id="valSOC">---</span></div>
      <div class="kv"><label>Restlaufzeit:</label>    <span class="badge neutral" id="valTTG">---</span></div>
      <div class="kv"><label>Starterbatterie:</label> <span class="badge neutral" id="valVS">---</span></div>
    </div>
    <!-- Charger -->
    <div class="status-box">
      <h2>Ladegerät (IP22)</h2>
      <div class="kv"><label>Spannung:</label> <span class="badge neutral" id="chargerV">---</span></div>
      <div class="kv"><label>Strom:</label>    <span class="badge neutral" id="chargerI">---</span></div>
      <div class="kv"><label>Status:</label>   <span class="badge neutral" id="chargerState">---</span></div>
    </div>
    <!-- MPPT1 -->
    <div class="status-box">
      <h2>Solar MPPT1</h2>
      <div class="kv"><label>Spannung:</label>    <span class="badge neutral" id="mppt1V">---</span></div>
      <div class="kv"><label>Strom:</label>       <span class="badge neutral" id="mppt1I">---</span></div>
      <div class="kv"><label>Leistung PV:</label> <span class="badge neutral" id="mppt1PV">---</span></div>
      <div class="kv"><label>Status:</label>      <span class="badge neutral" id="mppt1State">---</span></div>
      <div class="kv"><label>Ertrag heute:</label><span class="badge neutral" id="mppt1Y">---</span></div>
    </div>

    <!-- MPPT2 -->
    <div class="status-box">
      <h2>Solar MPPT2</h2>
      <div class="kv"><label>Spannung:</label>    <span class="badge neutral" id="mppt2V">---</span></div>
      <div class="kv"><label>Strom:</label>       <span class="badge neutral" id="mppt2I">---</span></div>
      <div class="kv"><label>Leistung PV:</label> <span class="badge neutral" id="mppt2PV">---</span></div>
      <div class="kv"><label>Status:</label>      <span class="badge neutral" id="mppt2State">---</span></div>
      <div class="kv"><label>Ertrag heute:</label><span class="badge neutral" id="mppt2Y">---</span></div>
    </div>

    

  </div>
  <!-- Stats unterhalb der Sensordaten -->
  <div style="margin-top:16px">
    <div class="status-box">
      <h2>Statistik &nbsp;
        <span style="font-size:0.7em;color:#8888bb">letzte</span>
        <select id="statsHours" onchange="loadStats()" 
                style="background:#0d0d1a;color:#eeeeff;border:1px solid #3a3a8e;
                      padding:3px;border-radius:4px;font-size:0.8em">
          <option value="1">1h</option>
          <option value="4">4h</option>
          <option value="8">8h</option>
          <option value="12" selected>12h</option>
          <option value="16">16h</option>
          <option value="24">24h</option>
          <option value="42">42h</option>
        </select>
      </h2>
      <table style="width:100%;border-collapse:collapse;font-size:0.9em">
        <tr style="color:#8888bb">
          <td></td><td style="text-align:right">Min</td>
          <td style="text-align:right">Max</td>
          <td style="text-align:right">Avg</td>
        </tr>
        <tr><td>Temperatur</td>
          <td style="text-align:right" id="st-t-min">-</td>
          <td style="text-align:right" id="st-t-max">-</td>
          <td style="text-align:right" id="st-t-avg">-</td></tr>
        <tr class="alt"><td>Feuchte</td>
          <td style="text-align:right" id="st-h-min">-</td>
          <td style="text-align:right" id="st-h-max">-</td>
          <td style="text-align:right" id="st-h-avg">-</td></tr>
        <tr><td>Luftdruck</td>
          <td style="text-align:right" id="st-p-min">-</td>
          <td style="text-align:right" id="st-p-max">-</td>
          <td style="text-align:right" id="st-p-avg">-</td></tr>
        <tr class="alt"><td>CO2</td>
          <td style="text-align:right" id="st-co2-min">-</td>
          <td style="text-align:right" id="st-co2-max">-</td>
          <td style="text-align:right" id="st-co2-avg">-</td></tr>
        <tr><td>Spannung</td>
          <td style="text-align:right" id="st-v-min">-</td>
          <td style="text-align:right" id="st-v-max">-</td>
          <td style="text-align:right" id="st-v-avg">-</td></tr>
        <tr class="alt"><td>Strom</td>
          <td style="text-align:right" id="st-i-min">-</td>
          <td style="text-align:right" id="st-i-max">-</td>
          <td style="text-align:right" id="st-i-avg">-</td></tr>
        <tr><td>SoC</td>
          <td style="text-align:right" id="st-soc-min">-</td>
          <td style="text-align:right" id="st-soc-max">-</td>
          <td style="text-align:right" id="st-soc-avg">-</td></tr>
        <tr class="alt"><td>Leistung</td>
          <td style="text-align:right" id="st-pw-min">-</td>
          <td style="text-align:right" id="st-pw-max">-</td>
          <td style="text-align:right" id="st-pw-avg">-</td></tr>
        <tr><td>Starter</td>
          <td style="text-align:right" id="st-vs-min">-</td>
          <td style="text-align:right" id="st-vs-max">-</td>
          <td style="text-align:right" id="st-vs-avg">-</td></tr>
        <tr class="alt"><td>MPPT1 Spannung</td>
          <td style="text-align:right" id="st-mppt1v-min">-</td>
          <td style="text-align:right" id="st-mppt1v-max">-</td>
          <td style="text-align:right" id="st-mppt1v-avg">-</td></tr>
        <tr><td>MPPT1 Leistung</td>
          <td style="text-align:right" id="st-mppt1pv-min">-</td>
          <td style="text-align:right" id="st-mppt1pv-max">-</td>
          <td style="text-align:right" id="st-mppt1pv-avg">-</td></tr>
        <tr class="alt"><td>MPPT2 Spannung</td>
          <td style="text-align:right" id="st-mppt2v-min">-</td>
          <td style="text-align:right" id="st-mppt2v-max">-</td>
          <td style="text-align:right" id="st-mppt2v-avg">-</td></tr>
        <tr><td>MPPT2 Leistung</td>
          <td style="text-align:right" id="st-mppt2pv-min">-</td>
          <td style="text-align:right" id="st-mppt2pv-max">-</td>
          <td style="text-align:right" id="st-mppt2pv-avg">-</td></tr>
        <tr class="alt"><td>Charger Spannung</td>
          <td style="text-align:right" id="st-chargerv-min">-</td>
          <td style="text-align:right" id="st-chargerv-max">-</td>
          <td style="text-align:right" id="st-chargerv-avg">-</td></tr>
        <tr><td>Charger Strom</td>
          <td style="text-align:right" id="st-chargeri-min">-</td>
          <td style="text-align:right" id="st-chargeri-max">-</td>
          <td style="text-align:right" id="st-chargeri-avg">-</td></tr>
      </table>
    </div>
  </div>
</div>


<!-- ============================================================ -->
<!-- TAB: KLIMA-VERLAUF                                          -->
<!-- ============================================================ -->
<div id="histKlima" class="tabcontent">
  <div class="history-controls">
    <label>Von</label>
    <input type="datetime-local" id="from_klima">
    <label>Bis</label>
    <input type="datetime-local" id="to_klima">
    <button class="btn" onclick="loadHistory('klima')">Laden</button>
    <span id="info_klima" style="color:#666688;font-size:0.85em"></span>
  </div>
  <div class="chart-wrap">
    <canvas id="chart_klima"></canvas>
    <div class="chart-loading" id="loading_klima" style="display:none">Lade Daten...</div>
  </div>
  <p style="color:#666688;font-size:0.8em">
    Luftdruck und CO2 sind standardmäßig ausgeblendet – in der Legende anklicken zum Einblenden.
  </p>
</div>


<!-- ============================================================ -->
<!-- TAB: BATTERIE-VERLAUF                                       -->
<!-- ============================================================ -->
<div id="histBat" class="tabcontent">
  <div class="history-controls">
    <label>Von</label>
    <input type="datetime-local" id="from_bat">
    <label>Bis</label>
    <input type="datetime-local" id="to_bat">
    <button class="btn" onclick="loadHistory('bat')">Laden</button>
    <span id="info_bat" style="color:#666688;font-size:0.85em"></span>
  </div>
  <div class="chart-wrap">
    <canvas id="chart_bat"></canvas>
    <div class="chart-loading" id="loading_bat" style="display:none">Lade Daten...</div>
  </div>
  <p style="color:#666688;font-size:0.8em">
    Leistung ist standardmäßig ausgeblendet – in der Legende anklicken zum Einblenden.
  </p>
</div>

<!-- ============================================================ -->
<!-- TAB: Beleuchtung                                             -->
<!-- ============================================================ -->

<div id="beleuchtung" class="tabcontent">
    <div style="display:flex; gap:10px; flex-wrap:wrap;">
      <div style="flex:1; padding-left:2%%; min-width:300px;height:650px; border:1px solid #3a3a8e; border-radius:8px;">
        <h4>WLED Innen</h4> 
        <iframe src="http://%WLED_INNEN_IP%" 
            style="min-width:300px; width:90%%; height:90%%; ">        
      </iframe>
      </div>
      <div style="flex:1; padding-left:2%%; min-width:300px;height:650px; border:1px solid #3a3a8e; border-radius:8px;">
      <h4>WLED Außen</h4>
        <iframe src="http://%WLED_AUSSEN_IP%" 
            style="min-width:300px; width:90%%; height:90%%; ">
        </iframe>
      </div>
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
  
  <button class="btn" onclick="saveIPs()">Speichern / Neustart</button>
  <div class="infoField invisible" id="ipInfo">
    <strong>IPs / Intervall gespeichert.</strong> Neustart erfolgt gleich.
  </div>

  <h2>WLAN Konfiguration</h2>
  <div class="form-row"><label>SSID</label>    <input type="text"     id="wifiSsid" placeholder="Netzwerkname"></div>
  <div class="form-row"><label>Passwort</label><input type="password" id="wifiPass" placeholder="Passwort"></div>
  <div class="form-row">
    <label>Feste IP-Adresse</label>
    <input type="checkbox" id="wifiStatic" onchange="toggleIP(this)" %WIFI_USE_STATIC%>
  </div>
  <div id="ip-fields" style="display:none">
    <div class="form-row"><label>IP-Adresse</label><input type="text" id="wifiIP"  value="%WIFI_STATIC_IP%"></div>
    <div class="form-row"><label>Subnetz</label>   <input type="text" id="wifiSN"  value="%WIFI_SUBNET%"></div>
    <div class="form-row"><label>Gateway</label>   <input type="text" id="wifiGW"  value="%WIFI_GATEWAY%"></div>
    <div class="form-row"><label>DNS</label>       <input type="text" id="wifiDNS" value="%WIFI_DNS%"></div>
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
    <li>Tab <strong>Klima-Verlauf</strong>: Temperatur, Feuchte, Luftdruck, CO2 aus SD-Karte</li>
    <li>Tab <strong>Batterie-Verlauf</strong>: Spannung, Strom, SoC, Leistung aus SD-Karte</li>
    <li>Tab <strong>Konfiguration</strong>: IPs der Geräte, WLAN, feste IP</li>
    <li>Aktuelle Werte werden alle 5 Sekunden aktualisiert</li>
    <li>OTA-Update unter <a href="/update" style="color:#44aaff">/update</a></li>
  </ul>

  <h2>SD-Karte</h2>
  <div class="form-row">
    <label>Verzeichnis</label>
    <input type="text" id="sdDir" value="/2025" style="width:120px">
    <button class="btn" onclick="listSD()">Auflisten</button>
  </div>
  <div id="sdFiles" style="margin-top:8px;font-size:0.85em;line-height:1.8"></div>

  <h2>Datei hochladen</h2>
  <div class="form-row">
    <label>Zielverzeichnis</label>
    <input type="text" id="sdUploadDir" value="/2025" style="width:120px">
  </div>
  <input type="file" id="sdUploadFile" accept=".csv">
  <button class="btn" onclick="uploadSD()">Hochladen</button>
  <div class="status" id="sdUploadStatus" style="margin-top:6px;color:#ffff88"></div>

  <h2>Log-Nachrichten</h2>
  <button class="btn" onclick="document.getElementById('logContainer').innerHTML=''">Log leeren</button>
  <div id="logContainer"></div>
</div>

<script>
// SD-Funktionen (nach dem HTML damit IDs verfügbar sind)
async function listSD() {
  const dir = getVal('sdDir');
  try {
    const r = await fetch('/api/sd/list?dir=' + encodeURIComponent(dir));
    const d = await r.json();
    const div = document.getElementById('sdFiles');
    div.innerHTML = '';
    if (!d.files || d.files.length === 0) { div.textContent = 'Keine Dateien'; return; }
    d.files.forEach(f => {
      const line = document.createElement('div');
      if (f.dir) {
        line.textContent = '📁 ' + f.name;
      } else {
        const dlPath = encodeURIComponent(dir + '/' + f.name);
        line.innerHTML = `📄 ${f.name} (${(f.size/1024).toFixed(1)} KB) ` +
          `<a href="/api/sd/download?file=${dlPath}" style="color:#44aaff">Download</a>`;
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
      r.ok ? 'Erfolgreich hochgeladen!' : 'Fehler beim Hochladen';
  } catch(e) {
    document.getElementById('sdUploadStatus').textContent = 'Verbindungsfehler';
  }
}
</script>

</body>
</html>
)rawliteral";
