// Phone editor page served by web.cpp. Self-contained: the phone has no internet
// while it's on the pad's Wi-Fi, so no external scripts, fonts or images.
#pragma once

static const char WEB_PAGE[] = R"rawliteral(<!doctype html>
<html lang="en"><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<title>MacroDiver</title>
<style>
:root{--bg:#0b0d10;--card:#171b21;--card2:#212730;--line:#2c333d;--txt:#eef1f4;--dim:#8a929c;--y:#ffe710;--red:#e5533d;--green:#66c15a}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
body{margin:0;background:var(--bg);color:var(--txt);font:16px/1.35 system-ui,-apple-system,Segoe UI,Roboto,sans-serif;padding-bottom:88px}
header{position:sticky;top:0;z-index:5;background:var(--bg);border-bottom:1px solid var(--line);padding:12px 16px;display:flex;align-items:center;gap:10px}
header b{color:var(--y);letter-spacing:.08em;font-size:18px}
#dirty{color:var(--y);font-size:13px;margin-left:auto}
main{max-width:720px;margin:0 auto;padding:0 16px}
h2{font-size:13px;letter-spacing:.12em;text-transform:uppercase;color:var(--dim);margin:24px 0 10px}
h2 span{color:var(--txt);letter-spacing:0;text-transform:none}
button{font:inherit;color:var(--txt);background:var(--card2);border:1px solid var(--line);border-radius:10px;min-height:44px;padding:0 14px;cursor:pointer}
button:active{filter:brightness(1.3)}
button.primary{background:var(--y);color:#000;border-color:var(--y);font-weight:600}
button.danger{color:var(--red)}
button:disabled{opacity:.4}
input[type=text],input[type=search]{font:inherit;color:var(--txt);background:var(--card);border:1px solid var(--line);border-radius:10px;min-height:44px;padding:0 12px;width:100%}
input:focus{outline:2px solid var(--y);outline-offset:-1px}
.chips{display:flex;gap:8px;overflow-x:auto;padding-bottom:4px;scrollbar-width:none}
.chip{flex:none;border-radius:22px;white-space:nowrap}
.chip.sel{background:var(--y);color:#000;border-color:var(--y)}
.row{display:flex;gap:8px;margin-top:10px;flex-wrap:wrap}
.row>*{flex:1}
.seg{display:flex;gap:8px;margin-top:10px}
.seg button{flex:1}
.seg button.sel{border-color:var(--y);color:var(--y)}
.check{display:flex;align-items:center;gap:10px;margin-top:12px;min-height:44px}
.check input{width:22px;height:22px;accent-color:var(--y)}
ol{list-style:none;margin:0;padding:0}
.sep{color:var(--dim);font-size:12px;letter-spacing:.1em;text-transform:uppercase;margin:14px 0 6px}
.item{display:flex;align-items:center;gap:10px;background:var(--card);border:1px solid var(--line);border-left:5px solid var(--dim);border-radius:10px;padding:6px 6px 6px 10px;margin-bottom:8px}
.item .t{flex:1;min-width:0}
.item .n{white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.k{font-size:13px;letter-spacing:.05em}
.item button{min-width:44px;padding:0}
.ico{width:36px;height:36px;flex:none;border-radius:50%;background:#000;display:flex;align-items:center;justify-content:center;font-size:18px}
.ico img{width:32px;height:32px}
.empty{color:var(--dim);padding:12px 0}
#grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(150px,1fr));gap:8px;margin-top:10px}
.card{display:flex;align-items:center;gap:8px;text-align:left;padding:8px;min-height:56px;border-left:5px solid var(--dim)}
.card .n{font-size:14px;line-height:1.2}
.card.on{background:#3a3410;border-color:var(--y)}
.card.on .n::before{content:"\2713  ";color:var(--y)}
.msg{display:flex;gap:8px;margin-bottom:8px}
.msg button{min-width:44px;padding:0}
.note{color:var(--dim);font-size:13px;margin-top:6px}
footer{position:fixed;left:0;right:0;bottom:0;background:var(--bg);border-top:1px solid var(--line);padding:12px 16px calc(12px + env(safe-area-inset-bottom));display:flex;gap:10px;z-index:5}
footer button{flex:1}
#toast{position:fixed;left:50%;bottom:90px;transform:translateX(-50%);background:var(--card2);border:1px solid var(--line);border-radius:10px;padding:10px 16px;opacity:0;transition:opacity .2s;pointer-events:none;z-index:9;max-width:90vw}
#toast.show{opacity:1}
#loading{padding:40px 16px;text-align:center;color:var(--dim)}
.wcard{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:12px}
.wcard a{color:var(--y)}
.wcard .row{margin-top:10px}
#nets{display:flex;flex-direction:column;gap:8px;margin-top:10px}
#nets button{text-align:left;display:flex;justify-content:space-between}
#wifiForm input{margin-top:10px}
</style></head><body>
<header><b>MACRODIVER</b><span id="dirty"></span></header>
<div id="loading">Loading from the pad&hellip;</div>
<main id="app" hidden>
  <h2>Pad Wi-Fi</h2>
  <div class="wcard">
    <div id="wifiInfo">&hellip;</div>
    <div class="row"><button id="wifiChange">Change network</button></div>
    <div id="wifiForm" hidden>
      <div class="row"><button id="scanB">Scan networks</button></div>
      <div id="nets"></div>
      <input type="text" id="wssid" placeholder="Network name" autocomplete="off" autocapitalize="off">
      <input type="text" id="wpass" placeholder="Password" autocomplete="off" autocapitalize="off" spellcheck="false">
      <div class="row"><button id="wconnect" class="primary">Connect</button><button id="wforget" class="danger">Forget network</button></div>
    </div>
  </div>

  <h2>Builds</h2>
  <div id="tabs" class="chips"></div>
  <div class="row">
    <button id="newB">+ New</button><button id="dupB">Duplicate</button><button id="delB" class="danger">Delete</button>
  </div>

  <h2>Edit build</h2>
  <input type="text" id="bname" autocomplete="off">
  <div class="seg"><button data-l="0">Large &middot; 4 per page</button><button data-l="1">Dense &middot; 7 per page</button></div>
  <label class="check"><input type="checkbox" id="activeCk"> Show this build on the pad</label>

  <h2>On the pad <span id="count"></span></h2>
  <ol id="items"></ol>

  <h2>Add stratagems</h2>
  <input type="search" id="search" placeholder="Search by name&hellip;" autocomplete="off">
  <div id="cats" class="chips" style="margin-top:10px"></div>
  <div id="grid"></div>

  <h2>Chat messages</h2>
  <div id="msgs"></div>
  <button id="addMsg">+ Add message</button>
  <div class="note">Letters without accents only (they're typed on a keyboard). Add messages to a build from the &ldquo;Messages&rdquo; category above.</div>
</main>
<footer><button id="discard">Discard</button><button id="save" class="primary">Save to pad</button></footer>
<div id="toast"></div>
<script>
const MSG_CAT=9, $=id=>document.getElementById(id);
let S,B,M,active,sel=0,cat=0,query='',icons={},dirty=false;
const ARW={U:'↑',D:'↓',L:'←',R:'→'};
const arrows=k=>[...k].map(c=>ARW[c]||'').join(' ');
const ascii=s=>s.normalize('NFD').replace(/[̀-ͯ]/g,'').replace(/[^\x20-\x7e]/g,'');
const esc=s=>s.replace(/[&<>"]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));

function toast(t){const e=$('toast');e.textContent=t;e.classList.add('show');clearTimeout(e._t);e._t=setTimeout(()=>e.classList.remove('show'),2200)}
function setDirty(d){dirty=d;$('dirty').textContent=d?'● Unsaved changes':'';$('save').disabled=!d;$('discard').disabled=!d}
function change(){setDirty(true);render()}

// all pickable items: catalog + custom from the pad, messages from the editor
function all(){return S.strats.concat(M.filter(m=>m.text.trim()).map(m=>({id:m.id,c:MSG_CAT,n:m.text,k:'',i:-1})))}
function find(id){return all().find(s=>s.id===id)}
function color(c){return S.cats[c]?S.cats[c].c:'#8a929c'}
function icon(s){
  if(s.c===MSG_CAT)return '<div class="ico" style="color:'+color(MSG_CAT)+'">⌨</div>';
  return '<div class="ico">'+(icons[s.id]?'<img src="'+icons[s.id]+'" alt="">':'')+'</div>';
}

async function load(){
  try{
    S=await (await fetch('/api/state',{cache:'no-store'})).json();
  }catch(e){$('loading').textContent="Can't reach the pad. Reconnect your phone to the MacroDiver Wi-Fi and reload.";return}
  B=S.builds.map(b=>({name:b.name,layout:b.layout,ids:b.ids.slice()}));
  M=S.msgs.map(m=>({id:m.id,text:m.text}));
  active=S.active;sel=Math.min(sel,B.length-1);
  $('loading').hidden=true;$('app').hidden=false;
  setDirty(false);render();
  if(!Object.keys(icons).length)loadIcons();
  if(!W)wifiLoad();
}

async function loadIcons(){
  try{
    const buf=new Uint8Array(await (await fetch('/icons')).arrayBuffer()), px=S.iconSize, cv=document.createElement('canvas');
    cv.width=cv.height=px;const ctx=cv.getContext('2d');
    for(const s of S.strats){
      if(s.i<0)continue;
      const img=ctx.createImageData(px,px),o=s.i*px*px*3;
      for(let p=0;p<px*px;p++){
        const v=(buf[o+3*p]<<8)|buf[o+3*p+1];
        img.data[4*p]=(v>>11&31)*255/31;img.data[4*p+1]=(v>>5&63)*255/63;img.data[4*p+2]=(v&31)*255/31;img.data[4*p+3]=buf[o+3*p+2];
      }
      ctx.putImageData(img,0,0);icons[s.id]=cv.toDataURL();
    }
    render();
  }catch(e){}
}

function render(){
  const b=B[sel];
  $('tabs').innerHTML=B.map((x,i)=>'<button class="chip'+(i===sel?' sel':'')+'" data-i="'+i+'">'+(i===active?'★ ':'')+esc(x.name)+'</button>').join('');
  $('newB').disabled=B.length>=S.maxBuilds;$('dupB').disabled=B.length>=S.maxBuilds;$('delB').disabled=B.length<=1;
  if(document.activeElement!==$('bname'))$('bname').value=b.name;
  $('bname').maxLength=S.nameLen;
  document.querySelectorAll('.seg button').forEach(x=>x.classList.toggle('sel',+x.dataset.l===b.layout));
  $('activeCk').checked=sel===active;$('activeCk').disabled=sel===active;
  $('count').textContent=b.ids.length+' / '+S.maxSlots;

  const per=b.layout?7:4;let h='';
  b.ids.forEach((id,k)=>{
    const s=find(id);if(!s)return;
    if(k%per===0)h+='<li class="sep">Page '+(k/per+1)+'</li>';
    h+='<li class="item" style="border-left-color:'+color(s.c)+'">'+icon(s)+'<div class="t"><div class="n">'+esc(s.n)+'</div><div class="k" style="color:'+color(s.c)+'">'+(s.c===MSG_CAT?'chat message':arrows(s.k))+'</div></div>'
      +'<button data-a="up" data-k="'+k+'" '+(k?'':'disabled')+' aria-label="Move up">▲</button>'
      +'<button data-a="down" data-k="'+k+'" '+(k<b.ids.length-1?'':'disabled')+' aria-label="Move down">▼</button>'
      +'<button data-a="rm" data-k="'+k+'" class="danger" aria-label="Remove">✕</button></li>';
  });
  $('items').innerHTML=h||'<li class="empty">Empty. Add stratagems below.</li>';

  const used=new Set(all().map(s=>s.c));
  $('cats').innerHTML=S.cats.map((c,i)=>used.has(i)?'<button class="chip'+(i===cat&&!query?' sel':'')+'" data-c="'+i+'" style="border-left:5px solid '+c.c+'">'+esc(c.n)+'</button>':'').join('');
  const q=query.toLowerCase();
  const list=all().filter(s=>q?s.n.toLowerCase().includes(q):s.c===cat);
  $('grid').innerHTML=list.map(s=>'<button class="card'+(b.ids.includes(s.id)?' on':'')+'" data-id="'+s.id+'" style="border-left-color:'+color(s.c)+'">'+icon(s)+'<span class="n">'+esc(s.n)+'</span></button>').join('')||'<div class="empty">Nothing found.</div>';

  if(!$('msgs').contains(document.activeElement))
    $('msgs').innerHTML=M.map((m,j)=>'<div class="msg"><input type="text" data-j="'+j+'" maxlength="'+S.msgLen+'" value="'+esc(m.text)+'" placeholder="Message text"><button data-j="'+j+'" class="danger" aria-label="Delete message">✕</button></div>').join('');
  $('addMsg').disabled=M.length>=S.maxMsgs;
}

$('tabs').onclick=e=>{const i=e.target.closest('[data-i]');if(i){sel=+i.dataset.i;render()}};
$('newB').onclick=()=>{B.push({name:'Build '+(B.length+1),layout:0,ids:[]});sel=B.length-1;change();$('bname').focus();$('bname').select()};
$('dupB').onclick=()=>{const b=B[sel];B.push({name:(b.name.slice(0,S.nameLen-5)+' copy'),layout:b.layout,ids:b.ids.slice()});sel=B.length-1;change()};
$('delB').onclick=()=>{if(B.length<=1||!confirm('Delete build "'+B[sel].name+'"?'))return;B.splice(sel,1);if(active===sel)active=0;else if(active>sel)active--;sel=Math.max(0,sel-1);change()};
$('bname').oninput=e=>{const v=ascii(e.target.value);if(v!==e.target.value)e.target.value=v;B[sel].name=v;setDirty(true);render()};
$('bname').onblur=()=>{if(!B[sel].name.trim()){B[sel].name='Build '+(sel+1);render()}};
document.querySelector('.seg').onclick=e=>{const l=e.target.closest('[data-l]');if(l){B[sel].layout=+l.dataset.l;change()}};
$('activeCk').onchange=()=>{active=sel;change()};
$('items').onclick=e=>{
  const t=e.target.closest('[data-a]');if(!t)return;const ids=B[sel].ids,k=+t.dataset.k;
  if(t.dataset.a==='up')[ids[k-1],ids[k]]=[ids[k],ids[k-1]];
  else if(t.dataset.a==='down')[ids[k+1],ids[k]]=[ids[k],ids[k+1]];
  else ids.splice(k,1);
  change();
};
$('search').oninput=e=>{query=e.target.value.trim();render()};
$('cats').onclick=e=>{const c=e.target.closest('[data-c]');if(c){cat=+c.dataset.c;query='';$('search').value='';render()}};
$('grid').onclick=e=>{
  const c=e.target.closest('[data-id]');if(!c)return;const id=+c.dataset.id,ids=B[sel].ids,k=ids.indexOf(id);
  if(k>=0)ids.splice(k,1);else if(ids.length>=S.maxSlots){toast('Build is full ('+S.maxSlots+')');return}else ids.push(id);
  change();
};
$('msgs').oninput=e=>{const j=e.target.dataset.j;if(j===undefined)return;const v=ascii(e.target.value);if(v!==e.target.value)e.target.value=v;M[j].text=v;setDirty(true)};
$('msgs').onclick=e=>{
  const b=e.target.closest('button[data-j]');if(!b)return;const m=M[+b.dataset.j];
  if(!confirm('Delete message "'+m.text+'"?'))return;
  M.splice(+b.dataset.j,1);B.forEach(x=>x.ids=x.ids.filter(id=>id!==m.id));
  document.activeElement.blur();change();
};
$('msgs').addEventListener('focusout',()=>setTimeout(render,0));
$('addMsg').onclick=()=>{
  const id=Math.max(1999,...M.map(m=>m.id))+1;M.push({id,text:''});document.activeElement.blur();change();
  const inp=$('msgs').querySelector('input[data-j="'+(M.length-1)+'"]');if(inp)inp.focus();
};
$('discard').onclick=()=>{if(confirm('Discard all changes?'))load()};
$('save').onclick=async()=>{
  // drop empty messages and any references to them
  const keep=M.filter(m=>m.text.trim()),ok=new Set(keep.map(m=>m.id));
  B.forEach(b=>b.ids=b.ids.filter(id=>id<2000||ok.has(id)));
  $('save').disabled=true;
  try{
    const r=await fetch('/api/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({active,builds:B,msgs:keep})});
    const j=await r.json();
    if(!j.ok)throw new Error(j.err||'error');
    toast('Saved to the pad ✓');await load();
  }catch(e){toast("Not saved: can't reach the pad");$('save').disabled=false}
};
// ---- pad Wi-Fi ----
let W=null,formOpen=false,wifiMsg='';
async function wifiLoad(){
  try{W=await (await fetch('/api/wifi',{cache:'no-store'})).json()}catch(e){return false}
  renderWifi();return true;
}
function renderWifi(){
  const link=u=>'<a href="'+u+'">'+u+'</a>';
  let h;
  if(wifiMsg)h=wifiMsg;
  else if(W.online)h='Connected to <b>'+esc(W.home)+'</b>. From any device on that network, open<br>'+link('http://'+W.ip+'/')+' or '+link('http://'+W.host+'/');
  else if(W.state==='connecting')h='Connecting to <b>'+esc(W.home)+'</b>&hellip;';
  else if(W.home)h="Can't reach <b>"+esc(W.home)+'</b>. Check the password or pick another network.';
  else h='Pick your home Wi-Fi: the pad joins it and this editor stays available from your network.';
  $('wifiInfo').innerHTML=h;
  $('wifiForm').hidden=W.online&&!formOpen;
  $('wifiChange').parentNode.hidden=!W.online||formOpen;
  $('wforget').hidden=!W.home;
  if(!$('wssid').value&&W.home&&!W.online)$('wssid').value=W.home;
}
$('wifiChange').onclick=()=>{formOpen=true;renderWifi()};
$('scanB').onclick=async()=>{
  $('scanB').disabled=true;$('scanB').textContent='Scanning\u2026';
  for(let t=0;t<15;t++){
    let r;try{r=await (await fetch('/api/scan',{cache:'no-store'})).json()}catch(e){break}
    if(r.nets){
      r.nets.sort((a,b)=>b.rssi-a.rssi);
      $('nets').innerHTML=r.nets.map(n=>'<button data-s="'+esc(n.ssid)+'"><span>'+esc(n.ssid)+'</span><span>'+(n.lock?'\uD83D\uDD12 ':'')+(n.rssi>-60?'\u2582\u2584\u2586':n.rssi>-75?'\u2582\u2584':'\u2582')+'</span></button>').join('')||'<div class="empty">No network found.</div>';
      break;
    }
    await new Promise(r=>setTimeout(r,1000));
  }
  $('scanB').disabled=false;$('scanB').textContent='Scan again';
};
$('nets').onclick=e=>{const b=e.target.closest('[data-s]');if(!b)return;$('wssid').value=b.dataset.s;$('wpass').value='';$('wpass').focus()};
$('wconnect').onclick=async()=>{
  const ssid=$('wssid').value.trim(),pass=$('wpass').value;
  if(!ssid){toast('Enter or pick a network');return}
  try{await fetch('/api/wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid,pass})})}
  catch(e){toast("Can't reach the pad");return}
  formOpen=false;wifiMsg='Connecting to <b>'+esc(ssid)+'</b>&hellip;';renderWifi();
  for(let t=0;t<20;t++){
    await new Promise(r=>setTimeout(r,2000));
    if(!await wifiLoad()){
      wifiMsg='Your phone left the pad\'s setup network (normal when the pad switches channel).<br>Reconnect your phone to <b>'+esc(ssid)+'</b>, then open <b>http://macrodiver.local/</b> or the address shown in the pad menu (Phone editor).';
      $('wifiInfo').innerHTML=wifiMsg;return;
    }
    if(W.online){
      wifiMsg='';renderWifi();
      $('wifiInfo').innerHTML+='<br><br>Now switch your phone back to <b>'+esc(W.home)+'</b> and open the link above. Bookmark it.';
      return;
    }
    if(t>=10&&W.state==='setup')break;
  }
  wifiMsg='';renderWifi();
};
$('wforget').onclick=async()=>{
  if(!confirm('Forget '+W.home+'? The pad goes back to its own setup network.'))return;
  try{await fetch('/api/wifi/forget',{method:'POST'})}catch(e){}
  $('wssid').value='';$('wpass').value='';wifiLoad();
};
// coming back to the tab: refresh if nothing is being edited
document.addEventListener('visibilitychange',()=>{if(!document.hidden){wifiLoad();if(!dirty)load()}});

window.onbeforeunload=e=>{if(dirty){e.preventDefault();return ''}};
load();
</script>
</body></html>)rawliteral";
