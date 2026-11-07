#pragma once
// =====================================================================
//  PCBuddy – Web-Oberfläche (komplett offline, keine externen Dateien)
// =====================================================================

// ---------------------------------------------------------------------
//  Gemeinsames CSS
// ---------------------------------------------------------------------
#define PCB_CSS R"CSS(
:root{--bg:#0d1014;--card:#161a21;--card2:#1c212a;--line:#272e39;--text:#e7eaef;--mut:#8a93a2;--acc:#3ecf9b;--acc-d:#2aa57a;--warn:#e5534b;--r:14px}
*{box-sizing:border-box}html{-webkit-text-size-adjust:100%}
body{margin:0;background:var(--bg);color:var(--text);font:14px/1.45 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
header{max-width:1000px;margin:0 auto;padding:18px 16px 10px;display:flex;align-items:center;gap:12px}
.logo{width:42px;height:42px;flex:none;border-radius:12px;background:var(--card);border:1px solid var(--line);display:flex;align-items:center;justify-content:center;gap:6px}
.logo i{width:9px;height:13px;border-radius:3px;background:var(--acc);display:block;animation:bl 5s infinite}
@keyframes bl{0%,92%,100%{transform:scaleY(1)}95%{transform:scaleY(.15)}}
h1{font-size:19px;margin:0;font-weight:700}.sub{color:var(--mut);font-size:12.5px}
.dot{display:inline-block;width:8px;height:8px;border-radius:50%;background:var(--mut);margin-right:6px;vertical-align:1px}.dot.on{background:var(--acc)}
main{max-width:1000px;margin:0 auto;padding:6px 16px 40px;display:grid;gap:14px;grid-template-columns:repeat(auto-fit,minmax(300px,1fr))}
.card{background:var(--card);border:1px solid var(--line);border-radius:var(--r);padding:16px;min-width:0}
.wide{grid-column:1/-1}
h2{font-size:12px;letter-spacing:.08em;text-transform:uppercase;color:var(--mut);margin:0 0 12px;font-weight:600}
h3{font-size:13px;margin:16px 0 8px;color:var(--text);font-weight:600}
label{display:block;font-size:12px;color:var(--mut);margin:10px 0 5px}
input,select,button{font:inherit;color:var(--text)}
input[type=text],input[type=password],input[type=number],input[type=time],input[type=date],select{width:100%;padding:9px 11px;background:var(--bg);border:1px solid var(--line);border-radius:10px;outline:none}
input:focus,select:focus{border-color:var(--acc)}
input[type=time],input[type=date]{color-scheme:dark;font:inherit;color:inherit;min-height:40px}
input[type=range]{width:100%;accent-color:var(--acc)}
.btn{display:inline-flex;align-items:center;justify-content:center;gap:6px;padding:9px 14px;border-radius:10px;border:1px solid var(--line);background:var(--card2);cursor:pointer;font-weight:600;font-size:13px;white-space:nowrap}
.btn:hover{border-color:#3a4352}.btn:active{transform:translateY(1px)}
.btn.pri{background:var(--acc);border-color:var(--acc);color:#05140e}.btn.pri:hover{background:var(--acc-d)}
.btn.bad{background:transparent;border-color:var(--warn);color:var(--warn)}
.btn:disabled{opacity:.5;cursor:default}
.grid{display:grid;gap:8px;grid-template-columns:repeat(auto-fill,minmax(96px,1fr))}
.row{display:grid;gap:10px;grid-template-columns:1fr 1fr}
.row3{display:grid;gap:10px;grid-template-columns:2fr 1fr 1fr}
.seg{display:grid;grid-template-columns:repeat(4,1fr);gap:6px;margin-top:8px}.btn.on{border-color:var(--acc);color:var(--acc)}
.flex{display:flex;gap:8px;flex-wrap:wrap;align-items:center}
.kv{display:grid;grid-template-columns:auto 1fr;gap:6px 14px;font-size:13px}.kv b{color:var(--mut);font-weight:500}.kv span{text-align:right;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.tg{display:flex;align-items:center;justify-content:space-between;gap:10px;padding:8px 0;border-bottom:1px solid var(--line);cursor:pointer}
.tg:last-child{border-bottom:0}.tg span{font-size:13.5px}.tg small{display:block;color:var(--mut);font-size:12px}
.tg input{appearance:none;-webkit-appearance:none;width:38px;height:22px;flex:none;border-radius:11px;background:var(--line);position:relative;cursor:pointer;transition:.15s}
.tg input:after{content:"";position:absolute;top:3px;left:3px;width:16px;height:16px;border-radius:50%;background:#cfd5de;transition:.15s}
.tg input:checked{background:var(--acc)}.tg input:checked:after{left:19px;background:#fff}
.chip{display:inline-block;padding:6px 10px;margin:4px 6px 0 0;border:1px solid var(--line);border-radius:999px;background:var(--bg);font-size:12.5px;cursor:pointer;color:var(--text)}
.chip:hover{border-color:var(--acc)}
.hint{color:var(--mut);font-size:12px;margin-top:6px}
pre{background:var(--bg);border:1px solid var(--line);border-radius:10px;padding:10px;overflow:auto;font-size:12px;margin:6px 0 0;white-space:pre-wrap;word-break:break-all}
code{font-family:ui-monospace,Consolas,monospace}
#toast{position:fixed;left:50%;bottom:18px;transform:translateX(-50%) translateY(80px);background:var(--card2);border:1px solid var(--line);padding:10px 16px;border-radius:10px;transition:.2s;font-size:13px;z-index:9;max-width:90vw}
#toast.show{transform:translateX(-50%) translateY(0)}#toast.err{border-color:var(--warn);color:var(--warn)}
@media(max-width:520px){.row,.row3{grid-template-columns:1fr}}
)CSS"

// ---------------------------------------------------------------------
//  WLAN-Setup-Portal (Access-Point-Modus)
// ---------------------------------------------------------------------
const char PORTAL_HTML[] PROGMEM = R"HTML(<!doctype html><html lang="de"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>PCBuddy Setup</title>
<style>)HTML" PCB_CSS R"HTML(main{max-width:480px;grid-template-columns:1fr}</style></head><body>
<header><div class="logo"><i></i><i></i></div><div><h1>PCBuddy Setup</h1><div class="sub">WLAN einrichten</div></div></header>
<main><div class="card">
<h2>1 · Netzwerk wählen</h2>
<div class="flex"><button class="btn" id="scanBtn" type="button" onclick="scan()">WLAN suchen</button><span class="hint" id="scanInfo"></span></div>
<div id="nets"></div>
<form id="f">
<label>WLAN-Name (SSID)</label><input type="text" name="ssid" id="ssid" required autocomplete="off">
<label>Passwort</label><input type="password" name="pass" id="pass" autocomplete="off">
<label class="tg" style="border:0;padding:4px 0"><span>Passwort anzeigen</span><input type="checkbox" onchange="pass.type=this.checked?'text':'password'"></label>
<h2 style="margin-top:18px">2 · Gerät</h2>
<label>Name im Netzwerk (.local)</label><input type="text" name="host" id="host" placeholder="pcbuddy">
<div class="hint">Danach erreichbar unter <b id="hostUrl">http://pcbuddy.local</b></div>
<label>Display-Typ</label><select name="oled" id="oled"><option value="0">0,96" OLED I2C (SSD1306)</option><option value="1">1,3" OLED I2C (SH1106)</option></select>
<div style="margin-top:16px"><button class="btn pri" style="width:100%" type="submit">Verbinden &amp; Neustart</button></div>
</form></div>
<div class="card" id="done" style="display:none"><h2>Gespeichert</h2><p id="doneTxt"></p></div>
</main><div id="toast"></div>
<script>
const $=s=>document.querySelector(s);
function toast(t,e){const x=$('#toast');x.textContent=t;x.className='show'+(e?' err':'');clearTimeout(x._t);x._t=setTimeout(()=>x.className='',2200)}
function bars(r){return r>-55?'▂▄▆█':r>-67?'▂▄▆':r>-78?'▂▄':'▂'}
async function scan(){const b=$('#scanBtn');b.disabled=true;$('#scanInfo').textContent='Suche…';
 try{for(let i=0;i<25;i++){const j=await (await fetch('/api/scan')).json();
  if(j.status==='done'){const n=$('#nets');n.innerHTML='';(j.networks||[]).forEach(w=>{const c=document.createElement('span');c.className='chip';c.textContent=w.ssid+'  '+bars(w.rssi)+(w.open?'':' 🔒');c.onclick=()=>{$('#ssid').value=w.ssid;$('#pass').focus()};n.appendChild(c)});
   $('#scanInfo').textContent=(j.networks||[]).length+' Netze gefunden';break}
  await new Promise(r=>setTimeout(r,700))}}catch(e){$('#scanInfo').textContent='Fehler beim Suchen'}b.disabled=false}
function upd(){const h=($('#host').value||'pcbuddy').toLowerCase().replace(/[^a-z0-9-]/g,'')||'pcbuddy';$('#hostUrl').textContent='http://'+h+'.local'}
$('#host').oninput=upd;
fetch('/api/settings').then(r=>r.json()).then(s=>{$('#host').value=s.host||'pcbuddy';$('#oled').value=s.oled;if(s.ssid)$('#ssid').value=s.ssid;upd()}).catch(()=>{});
$('#f').onsubmit=async e=>{e.preventDefault();try{const r=await fetch('/api/wifi',{method:'POST',body:new URLSearchParams(new FormData($('#f')))});const j=await r.json();
 if(j.ok){$('#done').style.display='';$('#doneTxt').innerHTML='PCBuddy startet neu und verbindet sich mit <b>'+$('#ssid').value.replace(/</g,'&lt;')+'</b>.<br><br>Verbinde dein Handy/PC wieder mit deinem normalen WLAN und öffne <b>http://'+j.host+'.local</b> oder die IP, die auf dem Display angezeigt wird.';$('#f').style.display='none'}else toast(j.err||'Fehler',1)}catch(e){toast('Fehler',1)}};
scan();
</script></body></html>)HTML";

// ---------------------------------------------------------------------
//  Dashboard (normaler Betrieb im Heimnetz) – in Kategorien (Tabs)
// ---------------------------------------------------------------------
#define DASH_CSS R"CSS(
.tabs{position:sticky;top:0;z-index:5;background:rgba(13,16,20,.94);backdrop-filter:blur(6px);border-bottom:1px solid var(--line)}
.tabs>div{max-width:1000px;margin:0 auto;padding:0 10px;display:flex;gap:2px;overflow-x:auto;scrollbar-width:none}
.tabs>div::-webkit-scrollbar{display:none}
.tab{padding:12px 12px 10px;border:0;border-bottom:2px solid transparent;background:none;color:var(--mut);font-weight:600;font-size:13.5px;cursor:pointer;white-space:nowrap}
.tab:hover{color:var(--text)}.tab.on{color:var(--text);border-bottom-color:var(--acc)}
.wrap{max-width:1000px;margin:0 auto;padding:14px 16px 90px}
.panel{display:none;gap:14px;grid-template-columns:repeat(auto-fit,minmax(300px,1fr))}.panel.on{display:grid}
.ptitle{grid-column:1/-1;margin:2px 0 -4px;font-size:13px;color:var(--mut)}
.savebar{display:none;position:fixed;left:50%;bottom:16px;transform:translateX(-50%);z-index:8;background:var(--card2);border:1px solid var(--acc);border-radius:12px;padding:8px 8px 8px 14px;gap:8px;align-items:center;box-shadow:0 8px 30px rgba(0,0,0,.45);width:min(560px,calc(100% - 32px))}
.savebar span{flex:1;font-size:13px}.savebar.show{display:flex}
.emo{display:grid;gap:8px;grid-template-columns:repeat(auto-fill,minmax(112px,1fr))}
.emo .btn{justify-content:flex-start;gap:8px;white-space:normal;text-align:left;line-height:1.2}.emo .btn i{font-style:normal;font-size:16px;width:20px;text-align:center}
.speed{display:flex;align-items:center;gap:10px}.speed b{min-width:24px;text-align:right}
.ck{display:flex;align-items:center;gap:7px;padding:7px 9px;border:1px solid var(--line);border-radius:10px;cursor:pointer;font-size:13px;background:var(--card2);user-select:none}
.ck input{accent-color:var(--acc);margin:0}
.bar{height:10px;border-radius:6px;background:var(--bg);border:1px solid var(--line);overflow:hidden;margin-top:10px}.bar i{display:block;height:100%;width:0;background:var(--acc);transition:width .2s}
.warn{border:1px solid var(--warn);color:#f0b4b0;border-radius:10px;padding:10px;font-size:12.5px;margin-top:8px}
.pl{display:flex;align-items:center;gap:12px;padding:11px 0;border-top:1px solid var(--line)}.pl:first-child{border-top:0}
.pl .tm{font-size:22px;font-weight:700;font-variant-numeric:tabular-nums;min-width:66px;line-height:1.1}.pl .tm small{display:block;font-size:11.5px;font-weight:500;color:var(--mut)}
.pl .tx{flex:1;min-width:0;cursor:pointer}.pl .tx b{display:block;overflow:hidden;text-overflow:ellipsis;white-space:nowrap;font-size:14px}
.pl .tx small{display:block;color:var(--mut);font-size:12px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}.pl.off .tm,.pl.off .tx{opacity:.45}
.tg.sw{border:0;padding:0}.empty{color:var(--mut);font-size:13px;padding:10px 0}
.days{display:grid;grid-template-columns:repeat(7,1fr);gap:5px}.days .btn{padding:9px 0}
.quick{display:flex;gap:6px;flex-wrap:wrap;margin-top:6px}.quick .chip{margin:0}
.modal{position:fixed;inset:0;z-index:20;background:rgba(0,0,0,.6);display:none;align-items:flex-start;justify-content:center;padding:28px 12px;overflow-y:auto}
.modal.open{display:flex}.modal .card{width:min(520px,100%);box-shadow:0 12px 40px rgba(0,0,0,.5)}
.dlgh{display:flex;align-items:center;justify-content:space-between;margin-bottom:4px}.dlgh h2{margin:0}.dlgh .x{padding:6px 10px}
.sndrow{display:flex;gap:6px}.sndrow select{flex:1}.sndrow .btn{padding:9px 12px}
#ringBar{display:none;margin-bottom:14px;border-color:var(--acc);align-items:center;gap:12px;flex-wrap:wrap}#ringBar.open{display:flex}
#ringBar b{flex:1;font-size:15px;min-width:160px}
input[type=file]{width:100%;padding:8px;background:var(--bg);border:1px dashed var(--line);border-radius:10px;color:var(--mut)}.ck i{font-style:normal}.ck.off{opacity:.45}
)CSS"

const char DASH_HTML[] PROGMEM = R"HTML(<!doctype html><html lang="de"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>PCBuddy</title>
<style>)HTML" PCB_CSS DASH_CSS R"HTML(</style></head><body>
<header><div class="logo"><i></i><i></i></div><div style="flex:1;min-width:0"><h1 id="title">PCBuddy</h1><div class="sub"><span class="dot" id="dot"></span><span id="subline">verbinde…</span></div></div></header>
<nav class="tabs"><div id="tabs">
<button class="tab" data-t="start">Start</button><button class="tab" data-t="face">Gesicht</button><button class="tab" data-t="text">Text</button><button class="tab" data-t="plan">Wecker &amp; Termine</button>
<button class="tab" data-t="disp">Anzeige</button><button class="tab" data-t="quiet">Ruhemodus</button><button class="tab" data-t="place">Ort &amp; Zeit</button>
<button class="tab" data-t="wifi">WLAN</button><button class="tab" data-t="sys">System</button></div></nav>
<div class="wrap">
<div class="card" id="ringBar"><b id="ringTxt">⏰ Wecker klingelt</b><button class="btn" id="ringSn" onclick="ringAct('snooze')">Schlummern</button><button class="btn pri" onclick="ringAct('stop')">Ausschalten</button></div>

<!-- ============ START ============ -->
<section class="panel" data-p="start">
<div class="card"><h2>Status</h2><div class="kv">
<b>Modus</b><span id="s_mode">–</span><b>Emotion</b><span id="s_emo">–</span><b>Uhrzeit</b><span id="s_time">–</span>
<b>Wetter</b><span id="s_wx">–</span><b>Ruhemodus</b><span id="s_quiet">–</span><b>Helligkeit</b><span id="s_br">–</span>
<b>Nächster Wecker</b><span id="s_al">–</span><b>Auto-Wechsel</b><span id="s_cy">–</span><b>WLAN</b><span id="s_wifi">–</span><b>Laufzeit</b><span id="s_up">–</span></div></div>
<div class="card"><h2>Helligkeit</h2>
<label>Jetzt <b id="lbV" style="color:var(--text)"></b></label>
<input type="range" id="liveBr" min="1" max="100" oninput="dimLive(this.value)" onchange="dimLive(this.value,1)">
<div class="seg"><button class="btn" onclick="dimSet(100)">100 %</button><button class="btn" onclick="dimSet(50)">50 %</button><button class="btn" onclick="dimSet(20)">20 %</button><button class="btn" onclick="dimSet(3)">Nacht</button></div>
<p class="hint" id="brHint"></p>
<label style="margin-top:14px">Ruhemodus</label>
<div class="seg" style="grid-template-columns:repeat(3,1fr)"><button class="btn" id="q_on" onclick="quiet('on')">An</button><button class="btn" id="q_off" onclick="quiet('off')">Aus</button><button class="btn" id="q_auto" onclick="quiet('auto')">Zeitplan</button></div></div>
<div class="card"><h2>Schnellzugriff</h2><div class="grid">
<button class="btn" onclick="act('/api/clock')">Uhr zeigen</button><button class="btn" onclick="act('/api/info')">Info-Folien</button>
<button class="btn" onclick="emo('heartbeat')">Herz</button><button class="btn" onclick="emo('laugh')">Lachen</button>
<button class="btn" onclick="act('/api/sleep')">Schlafen</button><button class="btn" onclick="act('/api/wake')">Aufwecken</button>
<button class="btn" id="qcy" onclick="cyToggle()">Durchlauf</button></div>
<label>Schnell-Text</label><div class="flex" style="flex-wrap:nowrap"><input type="text" id="qt" maxlength="200" placeholder="Bin gleich zurück!"><button class="btn pri" onclick="quickText()">Zeigen</button></div></div>
</section>

<!-- ============ GESICHT ============ -->
<section class="panel" data-p="face">
<div class="card wide"><h2>Emotionen</h2><div class="emo" id="emos"></div>
<div class="row" style="margin-top:6px"><div><label>Dauer</label><select id="emoMs"><option value="4000">4 Sekunden</option><option value="10000" selected>10 Sekunden</option><option value="60000">1 Minute</option><option value="0">dauerhaft</option></select></div><div></div></div></div>
<div class="card wide"><h2>Automatischer Wechsel</h2>
<div class="row3" style="grid-template-columns:1.3fr 1fr 1.3fr">
<div><label>Modus</label><select id="cyMode"><option value="0">Aus</option><option value="1">Durchlauf – eine nach der anderen</option><option value="2">Im Leerlauf – ab und zu eine</option></select></div>
<div><label>Reihenfolge</label><select id="cyOrder"><option value="0">der Reihe nach</option><option value="1">zufällig</option></select></div>
<div><label id="cySecL">Wechsel alle</label><div class="speed"><input type="range" id="cySec" min="2" max="60" oninput="cySecV.textContent=this.value+' s'"><b id="cySecV" style="min-width:36px">6 s</b></div></div></div>
<label>Welche Emotionen machen mit?</label><div class="emo" id="cyList"></div>
<div class="flex" style="margin-top:12px"><button class="btn" onclick="cyAll(1)">Alle</button><button class="btn" onclick="cyAll(0)">Keine</button><span style="flex:1"></span><button class="btn pri" onclick="cySave()">Übernehmen</button></div>
<p class="hint">Im Ruhemodus und beim Schlafen pausiert der Wechsel. Eine Emotion, die du oben selbst wählst, läuft erst ihre Dauer ab, danach geht der Durchlauf weiter.</p></div>
<div class="card"><h2>Gesten simulieren</h2><div class="grid">
<button class="btn" onclick="act('/api/tap')">Tippen</button><button class="btn" onclick="act('/api/double')">Doppeltipp</button>
<button class="btn" onclick="act('/api/long')">Lang (Love)</button><button class="btn" onclick="quiet('toggle')">3× (Ruhe)</button>
<button class="btn" onclick="act('/api/shake')">Schütteln</button><button class="btn" onclick="act('/api/rock')">Schaukeln</button></div>
<p class="hint">Wie am Gerät: Tippen = Reaktion, 2× = Info-Folien, 3× = Ruhemodus, lang = verliebt, schütteln = wütend, langsam hin und her = „Uh“.</p></div>
<div class="card"><h2>Aktionen</h2><div class="grid">
<button class="btn" onclick="act('/api/info')">Info-Folien</button><button class="btn" onclick="act('/api/clock')">Pac-Man-Uhr</button>
<button class="btn" onclick="act('/api/clock/demo')">Uhr-Animation</button><button class="btn" onclick="act('/api/sleep')">Schlafen</button>
<button class="btn" onclick="act('/api/wake')">Aufwecken</button><button class="btn" onclick="act('/api/vibrate?ms=350')">Vibrieren</button>
<button class="btn" onclick="act('/api/beep?f=2800&ms=120')">Piepen</button></div></div>
</section>

<!-- ============ TEXT ============ -->
<section class="panel" data-p="text">
<div class="card wide"><h2>Text aufs Display</h2>
<label>Text – Umlaute, ß, €, °, µ, § gehen</label><input type="text" id="mq" maxlength="200" placeholder="Grüße aus München – 23 °C!">
<div class="row" style="margin-top:10px"><div><label>Darstellung</label><select id="mqMode"><option value="once">Lauftext einmal</option><option value="loop">Lauftext Endlosschleife</option><option value="static">Stehend (15 s)</option><option value="static0">Stehend (bis Tippen)</option></select></div>
<div><label>Geschwindigkeit Lauftext</label><div class="speed"><input type="range" id="mqSpeed" min="1" max="10" oninput="speedLive(this.value)"><b id="spV">5</b></div></div></div>
<div class="flex" style="margin-top:14px"><button class="btn pri" onclick="sendText('text')">Anzeigen</button><button class="btn" onclick="sendText('notify')">Als Benachrichtigung (Ton + Vibration)</button><button class="btn" onclick="act('/api/wake')">Text beenden</button></div>
<p class="hint">Lange stehende Texte werden automatisch umgebrochen und kleiner geschrieben; passt es gar nicht, läuft er als Lauftext. Die Geschwindigkeit wird gespeichert und wirkt sofort.</p></div>
</section>

<!-- ============ WECKER & TERMINE ============ -->
<section class="panel" data-p="plan" style="grid-template-columns:repeat(auto-fit,minmax(340px,1fr))">
<div class="card wide warn" id="plTime" style="display:none;margin:0">Die Uhrzeit ist noch nicht synchronisiert (kein Internet?). Wecker und Termine klingeln erst, wenn PCBuddy die Uhrzeit kennt.</div>
<div class="card"><h2>Wecker</h2><div id="alList"></div>
<div class="flex" style="margin-top:10px"><button class="btn pri" id="alAdd" onclick="alEdit(-1)">+ Wecker</button><span class="hint" id="alCnt"></span></div>
<p class="hint">Klingelt auch im Ruhemodus, das Display wird dafür hell. Am Gerät: <b>tippen</b> oder <b>schütteln</b> = schlummern, <b>2× tippen</b> oder <b>lang drücken</b> = aus.</p></div>
<div class="card"><h2>Termine</h2><div id="evList"></div>
<div class="flex" style="margin-top:10px"><button class="btn pri" id="evAdd" onclick="evEdit(-1)">+ Termin</button><span class="hint" id="evCnt"></span></div>
<p class="hint">Zur Erinnerung zeigt PCBuddy dein Gesicht und den Termin und spielt die Melodie 3×. Tippen = gesehen.</p></div>
<div class="card wide"><h2>Einstellungen</h2><div class="row"><div>
<label style="margin-top:0">Wecker klingelt höchstens</label><select id="alMax" onchange="plSet('alarm_max',this.value)"><option value="1">1 Minute</option><option value="3">3 Minuten</option><option value="5">5 Minuten</option><option value="10">10 Minuten</option><option value="15">15 Minuten</option><option value="20">20 Minuten</option><option value="30">30 Minuten</option><option value="60">60 Minuten</option></select></div><div>
<label class="tg" style="margin-top:14px"><span>Termine im Ruhemodus mit Ton<small>Sonst nur Anzeige (Wecker klingeln immer)</small></span><input type="checkbox" id="evQuiet" onchange="plSet('ev_quiet',this.checked?1:0)"></label></div></div>
<p class="hint">Die nächsten Einträge stehen auch auf der Info-Folie „Wecker &amp; Termine“ (Doppeltipp). Ein- und ausschalten unter <a href="#disp" style="color:var(--acc)">Anzeige</a>.</p></div>
</section>

<div class="modal" id="alDlg" onclick="if(event.target===this)dlgClose()"><div class="card">
<div class="dlgh"><h2 id="alTitle">Wecker</h2><button class="btn x" type="button" title="Schließen" onclick="dlgClose()">✕</button></div>
<div class="row"><div><label>Uhrzeit</label><input type="time" id="a_time" required></div><div><label>Name</label><input type="text" id="a_label" maxlength="22" placeholder="Aufstehen"></div></div>
<label>Wochentage</label><div class="days" id="a_days"></div>
<div class="quick"><span class="chip" onclick="setDays(31)">Mo–Fr</span><span class="chip" onclick="setDays(96)">Wochenende</span><span class="chip" onclick="setDays(127)">Täglich</span><span class="chip" onclick="setDays(0)">Einmalig</span></div>
<p class="hint" id="a_dayHint"></p>
<div class="row"><div><label>Gesicht</label><select id="a_emo"></select></div><div><label>Schlummern</label><select id="a_snooze"><option value="0">aus</option><option value="5">5 Minuten</option><option value="10">10 Minuten</option><option value="15">15 Minuten</option></select></div></div>
<label>Ton</label><div class="sndrow"><select id="a_snd"></select><button class="btn" type="button" title="Probehören" onclick="sndPlay('a_snd')">▶</button><button class="btn" type="button" title="Stopp" onclick="sndStop()">■</button></div>
<label class="tg" style="margin-top:8px"><span>Vibration</span><input type="checkbox" id="a_vibe"></label>
<label class="tg"><span>Aktiv</span><input type="checkbox" id="a_en"></label>
<div class="flex" style="margin-top:14px"><button class="btn bad" id="a_del" onclick="alDel()">Löschen</button><button class="btn" id="a_test" onclick="plTest('alarm')">Testen</button><span style="flex:1"></span><button class="btn pri" onclick="alSave()">Speichern</button></div>
</div></div>

<div class="modal" id="evDlg" onclick="if(event.target===this)dlgClose()"><div class="card">
<div class="dlgh"><h2 id="evTitle">Termin</h2><button class="btn x" type="button" title="Schließen" onclick="dlgClose()">✕</button></div>
<label>Was?</label><input type="text" id="e_label" maxlength="32" placeholder="Zahnarzt">
<div class="row"><div><label>Datum</label><input type="date" id="e_date" required></div><div><label>Uhrzeit</label><input type="time" id="e_time" required></div></div>
<div class="row"><div><label>Wiederholen</label><select id="e_rep"><option value="0">nicht (einmalig)</option><option value="1">täglich</option><option value="2">wöchentlich</option><option value="3">monatlich</option><option value="4">jährlich (z. B. Geburtstag)</option></select></div>
<div><label>Erinnern</label><select id="e_lead"><option value="0">zur Uhrzeit</option><option value="5">5 min vorher</option><option value="10">10 min vorher</option><option value="15">15 min vorher</option><option value="30">30 min vorher</option><option value="60">1 Stunde vorher</option><option value="120">2 Stunden vorher</option><option value="240">4 Stunden vorher</option></select></div></div>
<div class="row"><div><label>Gesicht</label><select id="e_emo"></select></div><div><label>Ton</label><div class="sndrow"><select id="e_snd"></select><button class="btn" type="button" title="Probehören" onclick="sndPlay('e_snd')">▶</button></div></div></div>
<label class="tg" style="margin-top:8px"><span>Vibration</span><input type="checkbox" id="e_vibe"></label>
<label class="tg"><span>Aktiv</span><input type="checkbox" id="e_en"></label>
<div class="flex" style="margin-top:14px"><button class="btn bad" id="e_del" onclick="evDel()">Löschen</button><button class="btn" id="e_test" onclick="plTest('event')">Testen</button><span style="flex:1"></span><button class="btn pri" onclick="evSave()">Speichern</button></div>
</div></div>

<form id="cfg" autocomplete="off">
<!-- ============ ANZEIGE ============ -->
<section class="panel" data-p="disp">
<div class="card"><h2>Display</h2>
<label>Display-Typ</label><select name="oled"><option value="0">0,96" OLED I2C (SSD1306)</option><option value="1">1,3" OLED I2C (SH1106)</option></select>
<div class="hint">Gleiche Verdrahtung. Änderung startet das Gerät neu.</div>
<label>Helligkeit normal <span id="brV"></span></label><input type="range" name="bright" min="1" max="100" oninput="brV.textContent=this.value+' %'">
<label class="tg"><span>Um 180° drehen</span><input type="checkbox" name="flip"></label></div>
<div class="card"><h2>Info-Folien (Doppeltipp)</h2>
<label class="tg"><span>Wetter</span><input type="checkbox" name="sl_weather"></label>
<label class="tg"><span>Pac-Man-Uhr</span><input type="checkbox" name="sl_clock"></label>
<label class="tg"><span>Wecker &amp; Termine</span><input type="checkbox" name="sl_events"></label>
<label class="tg"><span>WLAN</span><input type="checkbox" name="sl_wifi"></label>
<label class="tg"><span>IP-Adresse</span><input type="checkbox" name="sl_ip"></label>
<label class="tg"><span>Firmware</span><input type="checkbox" name="sl_fw"></label>
<label class="tg"><span>Info per Doppeltipp sperren</span><input type="checkbox" name="lock_info"></label></div>
<div class="card"><h2>Ton, Vibration, Sensoren</h2>
<label class="tg"><span>Ton</span><input type="checkbox" name="sound"></label>
<label class="tg"><span>Vibration</span><input type="checkbox" name="vibe"></label>
<label class="tg"><span>Waka-Waka<small>Ton zur vollen Stunde, wenn Pac-Man die Stunde frisst</small></span><input type="checkbox" name="pac_sound"></label>
<label class="tg"><span>Neigungssensor (Schütteln &amp; Schaukeln)<small>Neigungssensor an GPIO 1 – weiter schütteln = Feuer, langsam schaukeln = „Uh“</small></span><input type="checkbox" name="shake_on"></label>
<label>Schüttel-Empfindlichkeit <span id="skV"></span></label><input type="range" name="shake_sens" min="1" max="5" oninput="skV.textContent=['','wenig','gering','mittel','hoch','sehr hoch'][this.value]">
<label>Auto-Schlaf nach</label><select name="sleep_min"><option value="0">nie</option><option value="2">2 min</option><option value="5">5 min</option><option value="10">10 min</option><option value="30">30 min</option><option value="60">60 min</option></select></div>
<div class="card"><h2>Sensor-Test (live)</h2><div class="kv">
<b>Neigungskontakt</b><span id="sn_tilt">–</span><b>Wechsel pro Sekunde</b><span id="sn_ps">–</span>
<b>Wechsel gesamt</b><span id="sn_tot">–</span><b>Touch (GPIO 7)</b><span id="sn_touch">–</span><b>Erkannt</b><span id="sn_last">–</span></div>
<p class="hint">Kippen: Der Kontakt sollte zwischen „geschlossen“ und „offen“ wechseln (SW-520D schaltet erst ab ca. 30–45° Neigung). Beim Schütteln sollten mehrere Wechsel pro Sekunde erscheinen. Wechselt nichts, prüfe die Verdrahtung: ein Pin an GPIO 1, der andere an GND.</p></div>
</section>

<!-- ============ RUHEMODUS ============ -->
<section class="panel" data-p="quiet">
<div class="card"><h2>Zeitplan</h2>
<label class="tg"><span>Automatisch nach Zeitplan</span><input type="checkbox" name="quiet_on"></label>
<div class="row"><div><label>Von</label><input type="time" name="quiet_from"></div><div><label>Bis</label><input type="time" name="quiet_to"></div></div>
<p class="hint">Über Mitternacht geht auch (z. B. 22:00–07:00). Manuell schalten: 3× tippen oder unten.</p>
<div class="seg" style="grid-template-columns:repeat(3,1fr)"><button type="button" class="btn" id="q2_on" onclick="quiet('on')">Jetzt an</button><button type="button" class="btn" id="q2_off" onclick="quiet('off')">Jetzt aus</button><button type="button" class="btn" id="q2_auto" onclick="quiet('auto')">Zeitplan</button></div></div>
<div class="card"><h2>Im Ruhemodus</h2>
<label>Helligkeit <span id="qbV"></span></label><input type="range" name="quiet_bright" min="1" max="100" oninput="qbV.textContent=this.value+' %'">
<label>Anzeige nach 20 s</label><select name="quiet_disp"><option value="0">Gesicht (nur gedimmt)</option><option value="1">Schlafendes Gesicht</option><option value="2">Pac-Man-Uhr</option><option value="3">Display aus</option></select>
<label class="tg"><span>Ton aus</span><input type="checkbox" name="quiet_mute"></label>
<label class="tg"><span>Vibration aus</span><input type="checkbox" name="quiet_novibe"></label></div>
</section>

<!-- ============ ORT & ZEIT ============ -->
<section class="panel" data-p="place">
<div class="card"><h2>Standort (Wetter)</h2>
<label>Ort suchen</label><div class="flex" style="flex-wrap:nowrap"><input type="text" id="cityQ" placeholder="z. B. München"><button class="btn" type="button" onclick="findCity()">Suchen</button></div>
<div id="cityRes"></div>
<label>Ortsname</label><input type="text" name="city">
<div class="row"><div><label>Breite</label><input type="text" name="lat"></div><div><label>Länge</label><input type="text" name="lon"></div></div></div>
<div class="card"><h2>Zeitzone</h2>
<label>Zeitzone</label><select id="tzsel"></select>
<input type="text" name="tz" id="tz" style="margin-top:8px;display:none" placeholder="POSIX-TZ, z. B. CET-1CEST,M3.5.0,M10.5.0/3">
<p class="hint">Die Uhrzeit kommt per Internet (NTP), Sommer-/Winterzeit wird automatisch umgestellt.</p></div>
<div class="card"><h2>Gerätename</h2>
<label>Name im Netzwerk (.local)</label><input type="text" name="host" placeholder="pcbuddy">
<p class="hint">Dashboard danach unter http://<i>name</i>.local</p></div>
</section>
<div class="savebar" id="savebar"><span>Ungespeicherte Änderungen</span><button class="btn" type="button" onclick="loadCfg()">Verwerfen</button><button class="btn pri" type="submit">Speichern</button></div>
</form>

<!-- ============ WLAN ============ -->
<section class="panel" data-p="wifi">
<div class="card"><h2>Verbindung</h2><div class="kv"><b>Verbunden mit</b><span id="w_ssid">–</span><b>Signal</b><span id="w_rssi">–</span><b>IP</b><span id="w_ip">–</span></div></div>
<div class="card"><h2>WLAN wechseln</h2>
<div class="flex"><button class="btn" id="scanBtn" type="button" onclick="scan()">Netze suchen</button><span class="hint" id="scanInfo"></span></div>
<div id="nets"></div>
<form id="wf"><label>SSID</label><input type="text" name="ssid" id="wssid" required><label>Passwort</label><input type="password" name="pass" id="wpass">
<div style="margin-top:12px"><button class="btn pri" type="submit">Verbinden &amp; Neustart</button></div></form></div>
</section>

<!-- ============ SYSTEM ============ -->
<section class="panel" data-p="sys">
<div class="card"><h2>Gerät</h2><div class="kv"><b>Firmware</b><span id="y_fw">–</span><b>Display</b><span id="y_oled">–</span><b>Freier Speicher</b><span id="y_heap">–</span><b>WLAN-Neuverbindungen</b><span id="y_wr">–</span><b>Laufzeit</b><span id="y_up">–</span></div></div>
<div class="card"><h2>Firmware-Update per WLAN</h2>
<div class="kv"><b>Installiert</b><span id="u_fw">–</span><b>Update möglich</b><span id="u_ota">–</span></div>
<div id="u_warn" class="warn" style="display:none"><b>Noch kein Update per WLAN möglich:</b> PCBuddy läuft mit einem Partitionsschema ohne zweiten App-Bereich (z.&nbsp;B. „Huge APP“).<br>
Einmal per USB flashen mit:<br>• <b>Board: „ESP32C3 Dev Module“</b> (der Eintrag „Nologo ESP32C3 Super Mini“ bietet das passende Schema nicht an)<br>
• <b>Partition Scheme: „Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)“</b><br>• USB CDC On Boot: Enabled<br>Die Einstellungen bleiben erhalten. Danach gehen alle weiteren Updates per WLAN.</div>
<label>Update-Datei (<code>PCBuddy_update_v….bin</code>)</label><input type="file" id="u_file" accept=".bin">
<div id="u_pwrow" style="display:none"><label>Update-Passwort</label><input type="password" id="u_pw" autocomplete="off"></div>
<div class="flex" style="margin-top:10px"><button class="btn pri" id="u_btn" onclick="doUpdate()">Update installieren</button><span class="hint" id="u_msg"></span></div>
<div class="bar"><i id="u_bar"></i></div>
<p class="hint">Die Einstellungen (WLAN, Ort, Ruhemodus …) bleiben erhalten. Während des Updates PCBuddy nicht ausschalten.
Auch in der Arduino IDE erscheint PCBuddy als Netzwerk-Port (<i>name</i>.local).</p>
<label>Update-Passwort festlegen (leer = keins)</label><div class="flex" style="flex-wrap:nowrap"><input type="password" id="u_newpw" maxlength="32" autocomplete="new-password"><button class="btn" onclick="setUpdPw()">Setzen</button></div></div>
<div class="card"><h2>Wartung</h2>
<div class="flex"><button class="btn" onclick="if(confirm('Neu starten?'))act('/api/restart')">Neustart</button>
<button class="btn" onclick="if(confirm('WLAN-Setup-Portal öffnen? Die Verbindung zum Dashboard geht dabei evtl. verloren.'))act('/api/portal')">Setup-Portal</button>
<button class="btn bad" onclick="if(confirm('Alle Einstellungen (WLAN, Ort, Zeitzone…) löschen und neu starten?'))act('/api/reset')">Werkseinstellungen</button></div>
<p class="hint">Am Gerät: 5 Sekunden halten öffnet ebenfalls das Setup-Portal.</p></div>
<div class="card wide"><h2>HTTP-API &amp; USB-Seriell</h2>
<p class="hint" style="margin-top:0">Alle Aktionen gehen auch per HTTP (GET oder POST) – z. B. für Skripte, Stream-Deck oder Home Assistant:</p>
<pre><code id="ex"></code></pre>
<p class="hint">USB-Seriell (115200 Baud): <code>emotion heartbeat</code>, <code>text Hallo</code>, <code>show Hallo</code>, <code>speed 7</code>, <code>dim 20</code>, <code>quiet on</code>, <code>clock demo</code>, <code>help</code></p></div>
</section>

</div><div id="toast"></div>
<script>
const $=s=>document.querySelector(s),$$=s=>document.querySelectorAll(s),F=document.forms.cfg;
const FORM_TABS=['disp','quiet','place'];
const EMO=[['neutral','😐','Neutral'],['cute','😊','Fröhlich'],['laugh','😆','Lachen'],['love','😍','Verliebt'],['heartbeat','💓','Pochendes Herz'],
['anime_love','💕','Anime-Love'],['mini_love','💗','Herzchen'],['kissou','😘','Küsschen'],['star','🤩','Begeistert'],['surprised','😮','Überrascht'],
['confused','🤔','Verwirrt'],['uh','🥴','Uh (wackelig)'],['dizzy','😵','Schwindelig'],['serious','😑','Ernst'],['angry','😠','Wütend'],['fire','🔥','Feuer'],
['sad','🙁','Traurig'],['crying','😢','Weinen'],['sleepy','😴','Müde'],['left','👈','Blick links'],['right','👉','Blick rechts']];
const TZ=[['CET-1CEST,M3.5.0,M10.5.0/3','Deutschland / Österreich / Schweiz (MEZ)'],['GMT0BST,M3.5.0/1,M10.5.0','Großbritannien (GMT/BST)'],['WET0WEST,M3.5.0/1,M10.5.0','Portugal (WEZ)'],['EET-2EEST,M3.5.0/3,M10.5.0/4','Osteuropa (OEZ)'],['MSK-3','Moskau (UTC+3)'],['<+03>-3','Türkei (UTC+3)'],['GST-4','VAE / Golf (UTC+4)'],['IST-5:30','Indien (UTC+5:30)'],['ICT-7','Thailand / Vietnam (UTC+7)'],['CST-8','China (UTC+8)'],['JST-9','Japan (UTC+9)'],['AEST-10AEDT,M10.1.0,M4.1.0/3','Australien Ost'],['NZST-12NZDT,M9.5.0,M4.1.0/3','Neuseeland'],['EST5EDT,M3.2.0,M11.1.0','USA Ost'],['CST6CDT,M3.2.0,M11.1.0','USA Zentral'],['MST7MDT,M3.2.0,M11.1.0','USA Mountain'],['PST8PDT,M3.2.0,M11.1.0','USA Pazifik'],['<-03>3','Brasilien / Argentinien (UTC-3)'],['UTC0','UTC'],['custom','Eigene (POSIX-String)…']];
let dirty=false,isQuiet=false,lastRing='';
function toast(t,e){const x=$('#toast');x.textContent=t;x.className='show'+(e?' err':'');clearTimeout(x._t);x._t=setTimeout(()=>x.className='',2000)}
async function act(u,quiet){try{const r=await fetch(u,{method:'POST',cache:'no-store'});if(!quiet)toast(r.ok?'OK':'Fehler '+r.status,!r.ok);setTimeout(poll,300);return r}catch(e){toast('Keine Verbindung',1)}}
function emo(k){act('/api/emotion?name='+k+'&ms='+$('#emoMs').value)}

// ---- Automatischer Wechsel ----
const FW_EMO=['neutral','serious','sleepy','crying','sad','cute','kissou','mini_love','anime_love','fire','angry','left','right','love','heartbeat','surprised','dizzy','laugh','star','confused','uh'];
const CY_NAMES=['Aus','Durchlauf','Im Leerlauf'];let cyMask=0xFFFFFFFF>>>0,cyCur=0;
function cyBuild(){const box=$('#cyList');box.innerHTML='';EMO.forEach(([k,i,n])=>{const idx=FW_EMO.indexOf(k);if(idx<0)return;
 const l=document.createElement('label');l.className='ck';const on=(cyMask>>>idx)&1;if(!on)l.classList.add('off');
 l.innerHTML='<input type="checkbox"'+(on?' checked':'')+'><i>'+i+'</i>'+n;const c=l.querySelector('input');
 c.onchange=()=>{cyMask=(c.checked?(cyMask|(1<<idx)):(cyMask&~(1<<idx)))>>>0;l.classList.toggle('off',!c.checked)};box.appendChild(l)})}
function cyAll(v){cyMask=v?0xFFFFFFFF>>>0:0;cyBuild()}
function cyFill(s){cyMask=(s.emo_mask>>>0);$('#cyMode').value=s.cycle_mode;$('#cyOrder').value=s.cycle_order;$('#cySec').value=s.cycle_sec;cySecV.textContent=s.cycle_sec+' s';cyLabel();cyBuild()}
function cyLabel(){$('#cySecL').textContent=$('#cyMode').value==='2'?'Pause im Leerlauf':'Wechsel alle'}
$('#cyMode').onchange=cyLabel;
async function cySave(){if(!cyMask&&$('#cyMode').value!=='0'){toast('Bitte mindestens eine Emotion wählen',1);return}
 const p=new URLSearchParams({cycle_mode:$('#cyMode').value,cycle_order:$('#cyOrder').value,cycle_sec:$('#cySec').value,emo_mask:String(cyMask>>>0)});
 try{const j=await (await fetch('/api/settings',{method:'POST',body:p})).json();toast(j.ok?('Auto-Wechsel: '+CY_NAMES[$('#cyMode').value]):'Fehler',!j.ok);poll()}catch(e){toast('Fehler',1)}}
async function cyToggle(){const r=await act('/api/cycle?mode='+(cyCur?'off':'run'),1);if(r&&r.ok){const j=await r.json();toast(j.cycle?'Durchlauf läuft':'Durchlauf aus');$('#cyMode').value=j.cycle;cyLabel()}}

// ---- Wecker & Termine ----
const DAYS=['Mo','Di','Mi','Do','Fr','Sa','So'],REP=['einmalig','täglich','wöchentlich','monatlich','jährlich'];
let PL={alarms:[],events:[],sounds:[]},edI=-1,edDays=0;
const esc=t=>String(t).replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
const emoName=k=>{const e=EMO.find(x=>x[0]===k);return e?e[1]+' '+e[2]:k};
function daysText(d){if(!d)return'einmalig';if(d===127)return'täglich';if(d===31)return'Mo–Fr';if(d===96)return'Sa + So';return DAYS.filter((x,i)=>d>>i&1).join(', ')}
function leadText(v){const o=$('#e_lead').querySelector('[value="'+v+'"]');return o?o.textContent:v+' min vorher'}
function fmtDate(s){const p=s.split('-');return p[2]+'.'+p[1]+'.'+p[0]}
function plRow(on,tm,sub,title,info,click,tog){const r=document.createElement('div');r.className='pl'+(on?'':' off');
 r.innerHTML='<div class="tm">'+tm+(sub?'<small>'+sub+'</small>':'')+'</div><div class="tx"><b>'+esc(title)+'</b>'+info.map(x=>'<small>'+x+'</small>').join('')+'</div><label class="tg sw"><input type="checkbox"'+(on?' checked':'')+'></label>';
 r.querySelector('.tx').onclick=click;r.querySelector('input').onchange=e=>tog(e.target.checked);return r}
function plRender(){const al=$('#alList'),ev=$('#evList');al.innerHTML='';ev.innerHTML='';
 PL.alarms.forEach(a=>{const sn=PL.snooze&&PL.snooze_idx===a.i;
  al.appendChild(plRow(a.en,a.time,'',a.label||'Wecker',[esc(daysText(a.days))+' · '+esc(PL.sounds[a.snd]),sn?'⏸ schlummert bis '+PL.snooze:(a.en?(a.next?'nächster: '+esc(a.next):''):'aus')],
   ()=>alEdit(a.i),v=>plQuick('alarm',a.i,v)))});
 PL.events.forEach(e=>{const past=e.en&&!e.next;
  ev.appendChild(plRow(e.en&&!past,e.time,fmtDate(e.date).slice(0,6),e.label||'Termin',[REP[e.rep]+(e.lead?' · '+leadText(e.lead):''),e.en?(past?'vorbei':'nächster: '+esc(e.next)):'aus'],
   ()=>evEdit(e.i),v=>plQuick('event',e.i,v)))});
 if(!PL.alarms.length)al.innerHTML='<div class="empty">Noch kein Wecker.</div>';
 if(!PL.events.length)ev.innerHTML='<div class="empty">Noch keine Termine.</div>';
 $('#alCnt').textContent=PL.alarms.length+' / '+PL.max_alarms;$('#evCnt').textContent=PL.events.length+' / '+PL.max_events;
 $('#alAdd').disabled=PL.alarms.length>=PL.max_alarms;$('#evAdd').disabled=PL.events.length>=PL.max_events;
 $('#plTime').style.display=PL.time_ok?'none':''}
async function plLoad(){try{PL=await (await fetch('/api/alarms',{cache:'no-store'})).json();
 for(const id of ['a_snd','e_snd']){const s=$('#'+id),v=s.value;if(s.options.length!==PL.sounds.length){s.innerHTML='';PL.sounds.forEach((n,i)=>{const o=document.createElement('option');o.value=i;o.textContent=n;s.appendChild(o)});if(v)s.value=v}}
 plRender()}catch(e){}}
async function plPost(u,p){try{const r=await fetch(u,{method:'POST',body:new URLSearchParams(p)});const j=await r.json();if(!j.ok)toast(j.err||'Fehler',1);return j}catch(e){toast('Keine Verbindung',1);return{}}}
async function plQuick(k,i,v){const j=await plPost('/api/'+k+'/set',{i,en:v?1:0});if(j.ok)toast(v?'Aktiviert':'Ausgeschaltet');plLoad();poll()}
function emoFill(id){const s=$('#'+id);if(s.options.length)return;EMO.forEach(([k,i,n])=>{const o=document.createElement('option');o.value=k;o.textContent=i+' '+n;s.appendChild(o)})}
function dayBtns(){const b=$('#a_days');b.innerHTML='';DAYS.forEach((d,i)=>{const x=document.createElement('button');x.type='button';x.className='btn'+(edDays>>i&1?' on':'');x.textContent=d;
 x.onclick=()=>{edDays^=1<<i;dayBtns()};b.appendChild(x)});$('#a_dayHint').textContent=edDays?'':'Einmalig: klingelt beim nächsten Mal und schaltet sich danach aus.'}
function setDays(v){edDays=v;dayBtns()}
function dlgClose(){$$('.modal').forEach(m=>m.classList.remove('open'));sndStop(1)}
function alEdit(i){emoFill('a_emo');edI=i;const a=i>=0?PL.alarms.find(x=>x.i===i):{time:'07:00',label:'',days:31,emo:'cute',snd:3,snooze:5,vibe:true,en:true};
 $('#alTitle').textContent=i>=0?'Wecker bearbeiten':'Neuer Wecker';$('#a_time').value=a.time;$('#a_label').value=a.label;edDays=a.days;dayBtns();
 $('#a_emo').value=a.emo;$('#a_snd').value=a.snd;$('#a_snooze').value=a.snooze;$('#a_vibe').checked=a.vibe;$('#a_en').checked=a.en;
 $('#a_del').style.display=$('#a_test').style.display=i>=0?'':'none';$('#alDlg').classList.add('open');$('#a_time').focus()}
function evEdit(i){emoFill('e_emo');edI=i;const n=new Date(),dd=n.getFullYear()+'-'+String(n.getMonth()+1).padStart(2,'0')+'-'+String(n.getDate()).padStart(2,'0');
 const e=i>=0?PL.events.find(x=>x.i===i):{label:'',date:dd,time:'12:00',rep:0,lead:15,emo:'star',snd:2,vibe:true,en:true};
 $('#evTitle').textContent=i>=0?'Termin bearbeiten':'Neuer Termin';$('#e_label').value=e.label;$('#e_date').value=e.date;$('#e_time').value=e.time;
 $('#e_rep').value=e.rep;$('#e_lead').value=e.lead;$('#e_emo').value=e.emo;$('#e_snd').value=e.snd;$('#e_vibe').checked=e.vibe;$('#e_en').checked=e.en;
 $('#e_del').style.display=$('#e_test').style.display=i>=0?'':'none';$('#evDlg').classList.add('open');$('#e_label').focus()}
async function alSave(){if(!$('#a_time').value){toast('Bitte Uhrzeit eingeben',1);return}
 const j=await plPost('/api/alarm/set',{i:edI,time:$('#a_time').value,label:$('#a_label').value.trim(),days:edDays,emo:$('#a_emo').value,snd:$('#a_snd').value,snooze:$('#a_snooze').value,vibe:$('#a_vibe').checked?1:0,en:$('#a_en').checked?1:0});
 if(j.ok){dlgClose();toast('Wecker gespeichert');await plLoad();poll()}}
async function evSave(){if(!$('#e_date').value||!$('#e_time').value){toast('Bitte Datum und Uhrzeit eingeben',1);return}
 const j=await plPost('/api/event/set',{i:edI,label:$('#e_label').value.trim(),date:$('#e_date').value,time:$('#e_time').value,rep:$('#e_rep').value,lead:$('#e_lead').value,emo:$('#e_emo').value,snd:$('#e_snd').value,vibe:$('#e_vibe').checked?1:0,en:$('#e_en').checked?1:0});
 if(j.ok){dlgClose();toast('Termin gespeichert');await plLoad();poll()}}
async function alDel(){if(!confirm('Wecker löschen?'))return;const j=await plPost('/api/alarm/del',{i:edI});if(j.ok){dlgClose();toast('Gelöscht');plLoad();poll()}}
async function evDel(){if(!confirm('Termin löschen?'))return;const j=await plPost('/api/event/del',{i:edI});if(j.ok){dlgClose();toast('Gelöscht');plLoad();poll()}}
async function plTest(k){dlgClose();const j=await plPost('/api/'+k+'/test',{i:edI});if(j.ok)toast('Test läuft – am Gerät tippen zum Beenden');poll()}
function sndPlay(id){fetch('/api/sound?id='+$('#'+id).value,{method:'POST'}).then(r=>r.json()).then(j=>{if(!j.ok)toast(j.err||'Fehler',1)}).catch(()=>toast('Keine Verbindung',1))}
function sndStop(q){fetch('/api/sound?stop=1',{method:'POST'}).catch(()=>{})}
async function plSet(k,v){const j=await plPost('/api/settings',{[k]:v});if(j.ok)toast('Gespeichert')}
async function ringAct(a){await act('/api/alarm/'+a,1);toast(a==='stop'?'Ausgeschaltet':'Schlummern');plLoad()}
document.addEventListener('keydown',e=>{if(e.key==='Escape')dlgClose()});

// ---- Tabs ----
function showTab(t){if(!$('.tab[data-t="'+t+'"]'))t='start';
 $$('.tab').forEach(b=>b.classList.toggle('on',b.dataset.t===t));$$('.panel').forEach(p=>p.classList.toggle('on',p.dataset.p===t));
 $('#savebar').classList.toggle('show',dirty&&FORM_TABS.includes(t));if(location.hash!=='#'+t)history.replaceState(null,'','#'+t);if(t==='plan')plLoad()}
$$('.tab').forEach(b=>b.onclick=()=>showTab(b.dataset.t));
function curTab(){return(location.hash||'#start').slice(1)}
function setDirty(v){dirty=v;$('#savebar').classList.toggle('show',v&&FORM_TABS.includes(curTab()))}
F.addEventListener('input',()=>setDirty(true));F.addEventListener('change',()=>setDirty(true));

EMO.forEach(([k,i,n])=>{const b=document.createElement('button');b.className='btn';b.innerHTML='<i>'+i+'</i>'+n;b.onclick=()=>emo(k);$('#emos').appendChild(b)});
TZ.forEach(([v,n])=>{const o=document.createElement('option');o.value=v;o.textContent=n;$('#tzsel').appendChild(o)});
$('#tzsel').onchange=()=>{const v=$('#tzsel').value;if(v==='custom'){$('#tz').style.display=''}else{$('#tz').style.display='none';$('#tz').value=v}setDirty(true)};

// ---- Text ----
function sendText(kind){const t=$('#mq').value.trim();if(!t){toast('Bitte Text eingeben',1);return}
 let m=$('#mqMode').value,ms='';if(m==='static0'){m='static';ms='&ms=0'}
 act('/api/'+kind+'?mode='+m+ms+'&speed='+$('#mqSpeed').value+'&text='+encodeURIComponent(t))}
function quickText(){const t=$('#qt').value.trim();if(!t){toast('Bitte Text eingeben',1);return}act('/api/text?mode=static&ms=0&text='+encodeURIComponent(t))}
let spT=0;function speedLive(v){spV.textContent=v;clearTimeout(spT);spT=setTimeout(()=>fetch('/api/settings',{method:'POST',body:new URLSearchParams({mq_speed:v})}).catch(()=>{}),250)}

// ---- Helligkeit / Ruhemodus ----
let dimT=0;
function dimLive(v,save){lbV.textContent=v+' %';clearTimeout(dimT);dimT=setTimeout(()=>{fetch('/api/dim?level='+v+(save?'&save=1':''),{method:'POST'}).catch(()=>{});
 const f=isQuiet?F.quiet_bright:F.bright;if(f&&!dirty){f.value=v;(isQuiet?qbV:brV).textContent=v+' %'}},save?0:120)}
function dimSet(v){$('#liveBr').value=v;dimLive(v,1);toast('Helligkeit '+v+' %')}
async function quiet(st){const r=await act('/api/quiet?state='+st,1);if(r&&r.ok){const j=await r.json();toast(j.quiet?'Ruhemodus an':'Ruhemodus aus')}}

// ---- Status ----
function up(s){const d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60);return(d?d+'d ':'')+h+'h '+m+'m'}
async function poll(){try{const s=await (await fetch('/api/status',{cache:'no-store'})).json();
 $('#dot').className='dot on';$('#title').textContent=s.name;$('#subline').textContent=s.host+'.local · '+s.ip;
 $('#s_mode').textContent=s.mode;$('#s_emo').textContent=(EMO.find(e=>e[0]===s.emotion)||[0,0,s.emotion])[2];$('#s_time').textContent=s.time||'nicht synchronisiert';
 $('#s_wx').textContent=s.wx?(s.wx.temp.toFixed(1)+' °C · '+s.wx.text+' · '+s.city):'noch keine Daten';
 isQuiet=!!s.quiet;$('#s_quiet').textContent=(s.quiet?'aktiv':'aus')+(s.quiet_mode==='auto'?' (Zeitplan)':' (manuell)');$('#s_br').textContent=s.bright+' %';
 ['on','off','auto'].forEach(k=>{$('#q_'+k).classList.toggle('on',s.quiet_mode===k);$('#q2_'+k).classList.toggle('on',s.quiet_mode===k)});
 if(document.activeElement!==$('#liveBr')){$('#liveBr').value=s.bright;lbV.textContent=s.bright+' %'}
 $('#brHint').textContent=s.quiet?'Ruhemodus aktiv – der Regler ändert die Ruhemodus-Helligkeit.':'Der Regler ändert die normale Helligkeit und wirkt sofort.';
 $('#s_al').textContent=s.next_alarm||'keiner';
 const rb=$('#ringBar');rb.classList.toggle('open',!!s.ringing);if(s.ringing){$('#ringTxt').textContent=(s.ringing==='alarm'?'⏰ Wecker klingelt':'📅 Termin-Erinnerung')+(PL.ring_label?': '+PL.ring_label:'');$('#ringSn').style.display=s.ringing==='alarm'?'':'none'}
 if(s.ringing!==lastRing){lastRing=s.ringing;plLoad()}else if(curTab()==='plan'&&!document.querySelector('.modal.open'))plLoad();
 cyCur=s.cycle||0;$('#s_cy').textContent=CY_NAMES[cyCur]||'–';$('#qcy').classList.toggle('on',cyCur===1);
 $('#s_wifi').textContent=s.ssid?(s.ssid+' ('+s.rssi+' dBm)'):'nicht verbunden';$('#s_up').textContent=up(s.uptime);
 $('#w_ssid').textContent=s.ssid||'–';$('#w_rssi').textContent=s.ssid?s.rssi+' dBm':'–';$('#w_ip').textContent=s.ip;
 $('#y_fw').textContent='v'+s.fw;$('#u_fw').textContent='v'+s.fw+(s.part?' ('+s.part+')':'');
 $('#u_ota').textContent=s.ota?'ja':'nein';$('#u_warn').style.display=s.ota?'none':'';$('#u_btn').disabled=!s.ota||updBusy;$('#u_pwrow').style.display=s.upd_pw?'':'none';$('#y_oled').textContent=s.oled?'1,3" SH1106':'0,96" SSD1306';$('#y_heap').textContent=Math.round(s.heap/1024)+' kB';$('#y_wr').textContent=(s.wifi_reconnects||0)+(s.wifi_reason?' (letzter Grund '+s.wifi_reason+')':'');$('#y_up').textContent=up(s.uptime);
 const H='http://'+s.host+'.local';$('#ex').textContent=[
 'curl "'+H+'/api/emotion?name=heartbeat&ms=5000"','curl "'+H+'/api/text?mode=static&text=Gr%C3%BC%C3%9Fe%20%E2%82%AC"',
 'curl "'+H+'/api/text?mode=loop&speed=8&text=Hallo"','curl "'+H+'/api/notify?text=Build%20fertig"','curl "'+H+'/api/dim?level=20"',
 'curl "'+H+'/api/quiet?state=on"     # on | off | auto | toggle','curl "'+H+'/api/clock/demo"','curl "'+H+'/api/cycle?mode=run&sec=5&order=random"   # run | idle | off','curl "'+H+'/api/alarm/set?time=06:45&days=31&label=Aufstehen&snd=7"   # Wecker Mo–Fr','curl "'+H+'/api/alarm/stop"','curl "'+H+'/api/status"'].join('\n');
}catch(e){$('#dot').className='dot';$('#subline').textContent='keine Verbindung'}}

// ---- Einstellungen ----
async function loadCfg(){try{const s=await (await fetch('/api/settings',{cache:'no-store'})).json();
 for(const el of F.elements){if(!el.name||!(el.name in s))continue;if(el.type==='checkbox')el.checked=!!s[el.name];else el.value=s[el.name]}
 $('#alMax').value=s.alarm_max;$('#evQuiet').checked=!!s.ev_quiet;
 cyFill(s);brV.textContent=s.bright+' %';qbV.textContent=s.quiet_bright+' %';skV.textContent=['','wenig','gering','mittel','hoch','sehr hoch'][s.shake_sens];$('#mqSpeed').value=s.mq_speed;spV.textContent=s.mq_speed;
 const known=TZ.some(t=>t[0]===s.tz);$('#tzsel').value=known?s.tz:'custom';$('#tz').style.display=known?'none':'';$('#wssid').value=s.ssid||'';
 setDirty(false)}catch(e){toast('Einstellungen konnten nicht geladen werden',1)}}
F.onsubmit=async e=>{e.preventDefault();const p=new URLSearchParams();
 for(const el of F.elements){if(!el.name)continue;p.append(el.name,el.type==='checkbox'?(el.checked?'1':'0'):el.value)}
 try{const j=await (await fetch('/api/settings',{method:'POST',body:p})).json();
  if(j.ok){toast(j.restart?'Gespeichert – Neustart…':'Gespeichert');setDirty(false);if(j.restart)setTimeout(()=>location.href='http://'+j.host+'.local/',7000);else{loadCfg();poll()}}else toast(j.err||'Fehler',1)}catch(e){toast('Fehler beim Speichern',1)}};
async function findCity(){const q=$('#cityQ').value.trim(),box=$('#cityRes');if(!q)return;box.innerHTML='<span class="hint">Suche…</span>';
 try{const j=await (await fetch('https://geocoding-api.open-meteo.com/v1/search?count=6&language=de&name='+encodeURIComponent(q))).json();box.innerHTML='';
  (j.results||[]).forEach(c=>{const b=document.createElement('span');b.className='chip';b.textContent=c.name+(c.admin1?', '+c.admin1:'')+' ('+(c.country_code||'')+')';
   b.onclick=()=>{F.city.value=c.name;F.lat.value=c.latitude.toFixed(4);F.lon.value=c.longitude.toFixed(4);box.innerHTML='';setDirty(true);toast('Übernommen – jetzt speichern')};box.appendChild(b)});
  if(!box.children.length)box.innerHTML='<span class="hint">Nichts gefunden</span>'}catch(e){box.innerHTML='<span class="hint">Ortssuche braucht Internet im Browser</span>'}}
$('#cityQ').addEventListener('keydown',e=>{if(e.key==='Enter'){e.preventDefault();findCity()}});

// ---- Firmware-Update ----
let updBusy=false;
function doUpdate(){const f=$('#u_file').files[0];if(!f){toast('Bitte .bin-Datei wählen',1);return}
 if(!/\.bin$/i.test(f.name)){toast('Das ist keine .bin-Datei',1);return}
 if(/merged|bootloader|partitions/i.test(f.name)&&!confirm('Der Dateiname sieht nach einer Komplett-/Bootloader-Datei aus. Für das WLAN-Update wird die normale App-Datei gebraucht. Trotzdem versuchen?'))return;
 if(!confirm('Firmware "'+f.name+'" ('+Math.round(f.size/1024)+' kB) installieren? PCBuddy startet danach neu.'))return;
 const fd=new FormData();fd.append('firmware',f,f.name);
 const x=new XMLHttpRequest();updBusy=true;$('#u_btn').disabled=true;$('#u_msg').textContent='Übertrage…';
 x.open('POST','/api/update?size='+f.size+'&pw='+encodeURIComponent($('#u_pw').value||''));
 x.upload.onprogress=e=>{if(e.lengthComputable){const p=Math.round(e.loaded*100/e.total);$('#u_bar').style.width=p+'%';$('#u_msg').textContent='Übertrage… '+p+' %'}};
 x.onload=()=>{let j={};try{j=JSON.parse(x.responseText)}catch(e){}
  if(x.status===200&&j.ok){$('#u_bar').style.width='100%';$('#u_msg').textContent='Installiert – PCBuddy startet neu…';waitReboot()}
  else{updBusy=false;$('#u_btn').disabled=false;$('#u_bar').style.width='0';$('#u_msg').textContent='Fehler: '+(j.err||x.status);toast('Update fehlgeschlagen',1)}};
 x.onerror=()=>{updBusy=false;$('#u_btn').disabled=false;$('#u_msg').textContent='Verbindung abgebrochen';toast('Verbindung abgebrochen',1)};
 x.send(fd)}
async function waitReboot(){const t0=Date.now();await new Promise(r=>setTimeout(r,5000));
 while(Date.now()-t0<60000){try{const s=await (await fetch('/api/status',{cache:'no-store'})).json();updBusy=false;
  $('#u_msg').textContent='Fertig – läuft jetzt mit v'+s.fw;toast('Update erfolgreich: v'+s.fw);$('#u_bar').style.width='0';poll();return}catch(e){}
  await new Promise(r=>setTimeout(r,1500))}
 updBusy=false;$('#u_msg').textContent='PCBuddy antwortet noch nicht – Seite neu laden'}
async function setUpdPw(){const v=$('#u_newpw').value;if(!confirm(v?'Update-Passwort setzen?':'Update-Passwort entfernen?'))return;
 try{const j=await (await fetch('/api/settings',{method:'POST',body:new URLSearchParams({upd_pw:v})})).json();toast(j.ok?(v?'Passwort gesetzt':'Passwort entfernt'):'Fehler',!j.ok);$('#u_newpw').value='';poll()}catch(e){toast('Fehler',1)}}

// ---- WLAN ----
function bars(r){return r>-55?'▂▄▆█':r>-67?'▂▄▆':r>-78?'▂▄':'▂'}
async function scan(){const b=$('#scanBtn');b.disabled=true;$('#scanInfo').textContent='Suche…';
 try{for(let i=0;i<25;i++){const j=await (await fetch('/api/scan')).json();
  if(j.status==='done'){const n=$('#nets');n.innerHTML='';(j.networks||[]).forEach(w=>{const c=document.createElement('span');c.className='chip';c.textContent=w.ssid+'  '+bars(w.rssi)+(w.open?'':' 🔒');c.onclick=()=>{$('#wssid').value=w.ssid;$('#wpass').focus()};n.appendChild(c)});
   $('#scanInfo').textContent=(j.networks||[]).length+' Netze';break}
  await new Promise(r=>setTimeout(r,700))}}catch(e){$('#scanInfo').textContent='Fehler'}b.disabled=false}
$('#wf').onsubmit=async e=>{e.preventDefault();if(!confirm('Mit "'+$('#wssid').value+'" verbinden und neu starten?'))return;
 try{const j=await (await fetch('/api/wifi',{method:'POST',body:new URLSearchParams(new FormData($('#wf')))})).json();toast(j.ok?'Gespeichert – Neustart…':'Fehler',!j.ok)}catch(e){toast('Fehler',1)}};
$('#mq').addEventListener('keydown',e=>{if(e.key==='Enter')sendText('text')});$('#qt').addEventListener('keydown',e=>{if(e.key==='Enter')quickText()});

// ---- Live-Sensor-Test (nur wenn der Tab "Anzeige" offen ist) ----
async function sensor(){if(curTab()!=='disp'||document.hidden)return;try{const s=await (await fetch('/api/sensor',{cache:'no-store'})).json();
 $('#sn_tilt').textContent=s.tilt<0?'nicht konfiguriert':(s.tilt?'offen (gekippt / HIGH)':'geschlossen (aufrecht / LOW)');
 $('#sn_ps').textContent=s.per_sec;$('#sn_tot').textContent=s.edges;$('#sn_touch').textContent=s.touch?'berührt':'–';
 $('#sn_last').textContent=s.last==='schuetteln'?'Schütteln!':s.last==='schaukeln'?'Schaukeln (Uh)':'–'}catch(e){}}
setInterval(sensor,300);
showTab(curTab());window.addEventListener('hashchange',()=>showTab(curTab()));
loadCfg();poll();setInterval(poll,3000);
</script></body></html>)HTML";
