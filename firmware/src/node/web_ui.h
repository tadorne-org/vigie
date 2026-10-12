// Page d'administration, autonome (le téléphone connecté au point d'accès
// n'a pas Internet : ni CDN, ni police externe).
#pragma once
#include <pgmspace.h>

static const char kWebUi[] PROGMEM = R"HTML(<!doctype html>
<html lang="fr"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Vigie — administration</title>
<style>
:root{--bg:#f4f6f7;--card:#fff;--ink:#16232b;--mute:#5f6f78;--line:#dde3e6;--accent:#0b6e7f;--ok:#2f7d3a;--warn:#b26b00;--bad:#b3261e;--sim:#6b4fbb}
@media(prefers-color-scheme:dark){:root{--bg:#0f1417;--card:#172026;--ink:#e3eaee;--mute:#93a3ab;--line:#26323a;--accent:#4fb3c4;--ok:#6cc27a;--warn:#e0a040;--bad:#f07068;--sim:#a58ef0}}
*{box-sizing:border-box}body{margin:0;font:15px/1.45 system-ui,-apple-system,Segoe UI,Roboto,sans-serif;background:var(--bg);color:var(--ink)}
header{padding:14px 16px 0;display:flex;flex-wrap:wrap;gap:8px 14px;align-items:baseline}
h1{font-size:20px;margin:0;letter-spacing:.02em}h1 small{font-weight:400;color:var(--mute);font-size:13px;margin-left:6px}
.badges{display:flex;gap:6px;flex-wrap:wrap}.b{font-size:12px;padding:2px 8px;border-radius:99px;border:1px solid currentColor}
.b.sim{color:var(--sim)}.b.warn{color:var(--warn)}.b.bad{color:var(--bad)}.b.ok{color:var(--ok)}
nav{display:flex;gap:4px;padding:10px 16px 0;overflow-x:auto}nav button{flex:none}
main{padding:12px 16px 40px;max-width:980px}
button,input,select{font:inherit;color:inherit}
button{background:var(--card);border:1px solid var(--line);border-radius:8px;padding:7px 12px;cursor:pointer}
button.on,button.pri{background:var(--accent);border-color:var(--accent);color:#fff}
button.danger{border-color:var(--bad);color:var(--bad)}
a{color:inherit;text-decoration:none}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));gap:10px}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:12px 14px}
.k{font-size:12px;color:var(--mute);text-transform:uppercase;letter-spacing:.06em}
.v{font-size:26px;font-variant-numeric:tabular-nums;font-weight:600}.v small{font-size:14px;font-weight:400;color:var(--mute)}
.bar{height:6px;border-radius:3px;background:var(--line);margin-top:6px;overflow:hidden}.bar i{display:block;height:100%;background:var(--accent)}
table{border-collapse:collapse;width:100%;font-size:14px}td{padding:4px 0;border-bottom:1px solid var(--line)}td:last-child{text-align:right;font-variant-numeric:tabular-nums}
section{margin-top:14px}h2{font-size:15px;margin:18px 0 8px}
fieldset{border:1px solid var(--line);border-radius:12px;background:var(--card);margin:0 0 12px;padding:8px 14px 12px}
legend{font-weight:600;padding:0 6px}
label.f{display:grid;grid-template-columns:1fr minmax(120px,190px);gap:10px;align-items:center;padding:6px 0;border-bottom:1px solid var(--line)}
label.f:last-child{border:0}label.f span small{display:block;color:var(--mute);font-size:12px}
input[type=text],input[type=number],input[type=password],select{width:100%;padding:6px 8px;border:1px solid var(--line);border-radius:6px;background:var(--bg)}
input.err{border-color:var(--bad)}
.row{display:flex;gap:8px;flex-wrap:wrap;align-items:center}
.muted{color:var(--mute);font-size:13px}#msg{min-height:1.4em;margin-top:8px}
canvas{width:100%;height:260px;display:block}
.chartbar{display:flex;gap:6px;flex-wrap:wrap;margin-bottom:8px}
@media(max-width:520px){label.f{grid-template-columns:1fr}}
</style></head><body>
<header><h1>Vigie <small id="hname"></small></h1><div class="badges" id="badges"></div></header>
<nav id="tabs"><button data-t="dash" class="on">Tableau de bord</button><button data-t="hist">Historique</button><button data-t="conf">Réglages</button><button data-t="maint">Maintenance</button></nav>
<main>
<div id="dash">
 <div class="grid">
  <div class="card"><div class="k">Tension</div><div class="v" id="sv">–</div></div>
  <div class="card"><div class="k">Courant</div><div class="v" id="si">–</div></div>
  <div class="card"><div class="k">Puissance</div><div class="v" id="sp">–</div></div>
  <div class="card"><div class="k">Charge (SOC)</div><div class="v" id="ss">–</div><div class="bar"><i id="sbar" style="width:0"></i></div></div>
  <div class="card"><div class="k">Température</div><div class="v" id="st">–</div></div>
 </div>
 <section class="grid" style="grid-template-columns:repeat(auto-fill,minmax(290px,1fr))">
  <div class="card"><table id="tbat"></table></div>
  <div class="card"><table id="tsys"></table></div>
 </section>
</div>
<div id="hist" hidden>
 <div class="card"><div class="chartbar" id="cser"></div><div class="chartbar" id="crng"></div><canvas id="chart"></canvas><div class="muted" id="cinfo"></div></div>
 <h2>Exporter</h2>
 <div class="row"><a href="/log.csv"><button>Échantillons (CSV)</button></a><a href="/days.csv"><button>Résumés journaliers (CSV)</button></a><a href="/log.bin"><button>Journal brut (.bin)</button></a></div>
 <p class="muted">Le journal brut se décode avec <code>tools/vigie_log.py</code>. Les enregistrements antérieurs au recalage de l'horloge portent un temps relatif (« +123s ») : l'outil les recale d'après l'enregistrement <code>time_sync</code>.</p>
</div>
<div id="conf" hidden><form id="cform"></form>
 <div class="row"><button class="pri" id="csave">Enregistrer</button><span id="cmsg" class="muted"></span></div>
 <p class="muted">Clés : laisser vide pour conserver, « - » pour effacer. Les réglages s'appliquent immédiatement et survivent au sommeil.</p>
</div>
<div id="maint" hidden>
 <h2>Horloge</h2><div class="row"><button id="btime">Régler sur ce téléphone</button><span class="muted" id="tinfo"></span></div>
 <h2>État de charge</h2><div class="row"><input type="number" id="socv" min="0" max="100" step="1" value="100" style="width:90px"><button id="bsoc">Recaler le SOC</button><span class="muted">à faire batterie pleine, après une charge de quai</span></div>
 <h2>Radio</h2><div class="row"><button id="bpush">Pousser maintenant</button><button id="bsample">Mesurer maintenant</button></div>
 <div id="simbox"><h2>Simulateur</h2><div class="row"><select id="scn" style="width:auto"><option value="0">port</option><option value="1">navigation</option><option value="2">hivernage</option><option value="3">décharge 4 A</option><option value="4">quai (chargeur 15 A)</option></select><input type="number" id="simsoc" min="0" max="100" value="85" style="width:80px"><button id="bsim">Appliquer (scénario + SOC simulé)</button></div></div>
 <h2>Mise à jour du firmware</h2><div class="row"><input type="file" id="fw" accept=".bin"><button id="bota">Téléverser</button><span id="otamsg" class="muted"></span></div>
 <h2>Session</h2><div class="row"><button id="bext">Rester éveillé 15 min</button><button id="bsleep">Fermer et dormir</button><button id="breboot">Redémarrer</button></div>
 <h2>Effacement</h2><div class="row"><input type="text" id="erc" placeholder="taper EFFACER" style="width:150px"><button class="danger" id="berase">Effacer le journal</button></div>
</div>
<div id="msg" class="muted"></div>
</main>
<script>
const $=s=>document.querySelector(s),fmt=(x,d)=>x==null?'–':Number(x).toFixed(d);
let S={},cfg=[],series=null,ser='v',rng=24;
async function api(u,body){const r=await fetch(u,body?{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams(body)}:{});return r.json()}
function msg(t,bad){const m=$('#msg');m.textContent=t;m.style.color=bad?'var(--bad)':''}
document.querySelectorAll('#tabs button').forEach(b=>b.onclick=()=>{document.querySelectorAll('#tabs button').forEach(x=>x.classList.toggle('on',x==b));['dash','hist','conf','maint'].forEach(id=>$('#'+id).hidden=id!=b.dataset.t);if(b.dataset.t=='hist')loadSeries();if(b.dataset.t=='conf')loadConfig()});
function dur(s){if(s<0)return '–';if(s<90)return s+' s';if(s<5400)return Math.round(s/60)+' min';if(s<172800)return (s/3600).toFixed(1)+' h';return (s/86400).toFixed(1)+' j'}
const PS=['nominal','cadence réduite','survie','COUPURE'],MODES=['navigation','port','mouillage','hivernage'];
const AL=['','soc_warn','soc_alarm','volt_low','volt_critical','temp_high','overcurrent','no_charge_30d','boot_unexpected','sensor_lost','volt_cutoff'];
function rows(el,r){el.innerHTML=r.map(([k,v])=>`<tr><td>${k}</td><td>${v}</td></tr>`).join('')}
async function refresh(){try{S=await api('/api/status')}catch(e){msg('Nœud injoignable (endormi ?)',1);return}
 $('#hname').textContent=S.name+' · '+S.board+' · fw '+S.fw;
 $('#sv').innerHTML=fmt(S.v,2)+'<small> V</small>';$('#si').innerHTML=(S.i>0?'+':'')+fmt(S.i,2)+'<small> A</small>';
 $('#sp').innerHTML=fmt(S.p,1)+'<small> W</small>';$('#ss').innerHTML=fmt(S.soc,1)+'<small> %</small>';$('#sbar').style.width=S.soc+'%';
 $('#st').innerHTML=S.t==null?'–':fmt(S.t,1)+'<small> °C</small>';
 const al=AL.filter((a,k)=>k&&S.alerts&(1<<k));
 let b='';if(S.sim)b+='<span class="b sim">SIMULÉ · '+S.scenario+'</span>';if(!S.time_valid)b+='<span class="b warn">horloge non réglée</span>';
 if(!S.sensor_ok)b+='<span class="b bad">capteur muet</span>';if(S.power_state)b+='<span class="b '+(S.power_state>1?'bad':'warn')+'">'+PS[S.power_state]+'</span>';
 al.forEach(a=>b+='<span class="b bad">'+a+'</span>');b+='<span class="b ok">admin encore '+dur(S.admin_left)+'</span>';$('#badges').innerHTML=b;
 rows($('#tbat'),[['Capteur',S.sensor],['SOC fiable',S.soc_valid?'oui':'estimé'],['Entré aujourd\'hui',fmt(S.ahd_in,2)+' Ah'],['Sorti aujourd\'hui',fmt(S.ahd_out,2)+' Ah'],['Entré depuis mise en service',fmt(S.ah_in,1)+' Ah'],['Sorti depuis mise en service',fmt(S.ah_out,1)+' Ah'],['Mode',MODES[S.mode]],['Protection',PS[S.power_state]]]);
 const n=S.rec_next-S.rec_first;
 rows($('#tsys'),[['Heure (UTC)',S.time_valid?new Date(S.now*1000).toISOString().replace('T',' ').slice(0,19):'+'+S.now+' s'],['Depuis mise sous tension',dur(S.uptime)],['Journal',n+' / '+S.rec_cap+' ('+Math.round(100*n/S.rec_cap)+' %)'],['Résumés journaliers',S.days],['Prochaine mesure / push',dur(S.next_sample)+' / '+dur(S.next_push)],['ESP-NOW ok / échecs',S.en_ok+' / '+S.en_fail],['Dernier accusé',S.last_ack<0?'jamais':'il y a '+dur(S.last_ack)+' ('+S.rssi+' dBm)'],['Historique accusé','#'+S.acked+' / #'+(S.rec_next-1)],['Démarrages / réveils',S.boots+' / '+S.wakes],['MAC',S.mac],['Mémoire libre',Math.round(S.heap/1024)+' ko']]);
 $('#simbox').hidden=!S.sim;if(!window._scn&&S.sim_scn!=null){window._scn=1;$('#scn').value=S.sim_scn}$('#tinfo').textContent=S.time_valid?'':'horloge non réglée';
 if(!window._t){window._t=1;const d=Math.floor(Date.now()/1000)-S.now;if(!S.time_valid||Math.abs(d)>5){await api('/api/action',{do:'time',epoch:Math.floor(Date.now()/1000)});msg('Horloge réglée sur celle du téléphone.')}}
}
// Graphique
const SER={v:['Tension','V',2],i:['Courant','A',2],soc:['SOC','%',1]},RNG=[[6,'6 h'],[24,'24 h'],[72,'3 j'],[168,'7 j'],[720,'30 j']];
$('#cser').innerHTML=Object.entries(SER).map(([k,v])=>`<button data-s="${k}">${v[0]}</button>`).join('');
$('#crng').innerHTML=RNG.map(([h,l])=>`<button data-h="${h}">${l}</button>`).join('');
$('#cser').onclick=e=>{if(e.target.dataset.s){ser=e.target.dataset.s;draw()}};$('#crng').onclick=e=>{if(e.target.dataset.h){rng=+e.target.dataset.h;loadSeries()}};
async function loadSeries(){series=await api('/api/series?h='+rng);draw()}
function draw(){document.querySelectorAll('#cser button').forEach(b=>b.classList.toggle('on',b.dataset.s==ser));document.querySelectorAll('#crng button').forEach(b=>b.classList.toggle('on',+b.dataset.h==rng));
 const c=$('#chart'),dpr=devicePixelRatio||1,W=c.clientWidth,H=c.clientHeight;c.width=W*dpr;c.height=H*dpr;const g=c.getContext('2d');g.scale(dpr,dpr);g.clearRect(0,0,W,H);
 const cs=getComputedStyle(document.body),ink=cs.getPropertyValue('--mute'),line=cs.getPropertyValue('--line'),acc=cs.getPropertyValue('--accent');
 if(!series||!series.t.length){g.fillStyle=ink;g.fillText('Pas encore de données enregistrées.',10,20);$('#cinfo').textContent='';return}
 const t=series.t,y=series[ser],L=46,R=10,T=10,B=24;let y0=Math.min(...y),y1=Math.max(...y);if(ser=='i'){y0=Math.min(y0,0);y1=Math.max(y1,0)}if(y1-y0<1e-3){y0-=.5;y1+=.5}const pad=(y1-y0)*.08;y0-=pad;y1+=pad;
 const t0=t[0],t1=Math.max(t[t.length-1],t0+1),X=v=>L+(v-t0)/(t1-t0)*(W-L-R),Y=v=>T+(1-(v-y0)/(y1-y0))*(H-T-B);
 g.font='11px system-ui';g.strokeStyle=line;g.fillStyle=ink;g.lineWidth=1;
 for(let k=0;k<=4;k++){const v=y0+(y1-y0)*k/4,yy=Y(v);g.beginPath();g.moveTo(L,yy);g.lineTo(W-R,yy);g.stroke();g.fillText(v.toFixed(SER[ser][2]),4,yy+4)}
 if(ser=='i'){g.strokeStyle=ink;g.beginPath();g.moveTo(L,Y(0));g.lineTo(W-R,Y(0));g.stroke()}
 const abs=t0>1.7e9;for(let k=0;k<=4;k++){const v=t0+(t1-t0)*k/4,d=new Date(v*1000);g.fillText(abs?(rng>48?d.toLocaleDateString('fr',{day:'2-digit',month:'2-digit'})+' ':'')+d.toLocaleTimeString('fr',{hour:'2-digit',minute:'2-digit'}):'+'+Math.round(v/60)+' min',Math.min(X(v)-14,W-60),H-6)}
 g.strokeStyle=acc;g.lineWidth=1.6;g.beginPath();y.forEach((v,k)=>k?g.lineTo(X(t[k]),Y(v)):g.moveTo(X(t[k]),Y(v)));g.stroke();
 $('#cinfo').textContent=y.length+' points · '+SER[ser][0]+' min '+Math.min(...y).toFixed(SER[ser][2])+' / max '+Math.max(...y).toFixed(SER[ser][2])+' '+SER[ser][1]}
// Réglages
async function loadConfig(){cfg=await api('/api/config');const f=$('#cform');f.innerHTML='';const groups={};
 cfg.forEach(x=>(groups[x.g]=groups[x.g]||[]).push(x));
 for(const[g,xs]of Object.entries(groups)){const fs=document.createElement('fieldset');fs.innerHTML='<legend>'+g+'</legend>';
  xs.forEach(x=>{let inp;const u=x.u?' <small>'+x.u+'</small>':'';
   if(x.t=='bool')inp=`<input type="checkbox" name="${x.k}" ${x.v=='1'?'checked':''}>`;
   else if(x.secret)inp=`<input type="password" name="${x.k}" placeholder="${x.set?'définie'+(x.default?' (défaut !)':''):'aucune'}" autocomplete="off">`;
   else if(['u8','u16','u32','f32'].includes(x.t))inp=`<input type="number" name="${x.k}" value="${x.v}" min="${x.min}" max="${x.max}" step="${x.t=='f32'?'any':1}">`;
   else inp=`<input type="text" name="${x.k}" value="${x.v.replace(/"/g,'&quot;')}">`;
   fs.insertAdjacentHTML('beforeend',`<label class="f"><span>${x.l}${u}</span>${inp}</label>`)});f.appendChild(fs)}}
$('#csave').onclick=async()=>{const body={};document.querySelectorAll('#cform input').forEach(i=>{i.classList.remove('err');body[i.name]=i.type=='checkbox'?(i.checked?'1':'0'):i.value});
 const r=await api('/api/config',body);if(r.ok){$('#cmsg').textContent='Enregistré.';loadConfig();refresh()}else{$('#cmsg').textContent='Valeurs refusées : '+r.errors.join(', ');r.errors.forEach(k=>{const i=document.querySelector(`#cform [name="${k}"]`);i&&i.classList.add('err')})}};
// Maintenance
const act=async(p,ok)=>{const r=await api('/api/action',p);msg(r.ok?ok:'Refusé.',!r.ok);setTimeout(refresh,700)};
$('#btime').onclick=()=>act({do:'time',epoch:Math.floor(Date.now()/1000)},'Horloge réglée.');
$('#bsoc').onclick=()=>act({do:'soc',value:$('#socv').value},'SOC recalé.');
$('#bpush').onclick=()=>act({do:'push'},'Push lancé.');$('#bsample').onclick=()=>act({do:'sample'},'Mesure lancée.');
$('#bsim').onclick=()=>act({do:'sim',scn:$('#scn').value,soc:$('#simsoc').value},'Simulateur mis à jour.');
$('#bext').onclick=()=>act({do:'extend',s:900},'Session prolongée.');$('#bsleep').onclick=()=>act({do:'sleep'},'Le nœud se rendort ; le point d\'accès va disparaître.');
$('#breboot').onclick=()=>act({do:'reboot'},'Redémarrage…');
$('#berase').onclick=()=>act({do:'erase',confirm:$('#erc').value},'Journal effacé.');
$('#bota').onclick=()=>{const f=$('#fw').files[0];if(!f)return;const x=new XMLHttpRequest(),fd=new FormData();fd.append('fw',f);
 x.upload.onprogress=e=>$('#otamsg').textContent=Math.round(100*e.loaded/e.total)+' %';x.onload=()=>{$('#otamsg').textContent=x.status==200?'Installé, redémarrage…':'Échec : '+x.responseText};x.open('POST','/update');x.send(fd)};
refresh();setInterval(refresh,5000);
</script></body></html>)HTML";
