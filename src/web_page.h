// Phone editor page served by web.cpp. Self-contained: the phone has no internet
// while it's on the pad's Wi-Fi, so no external scripts, fonts or images.
// Only "Check for updates" goes online (GitHub API + raw SVGs of the icon library,
// both allow cross-origin reads), when the phone is on the home network.
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
select{font:inherit;color:var(--txt);background:var(--card);border:1px solid var(--line);border-radius:10px;min-height:44px;padding:0 8px;width:100%}
.imp{display:block;background:var(--card2);border:1px solid var(--line);border-radius:10px;padding:10px;margin-top:10px}
.imp summary{min-height:32px;cursor:pointer}
.nr{border-top:1px solid var(--line);margin-top:10px;padding-top:10px}
.hd{display:flex;align-items:center;gap:10px}
.hd input[type=checkbox]{width:22px;height:22px;flex:none;accent-color:var(--y)}
.hd img{width:44px;height:44px;flex:none;background:#000;border-radius:50%;padding:4px}
.hd input[type=text]{flex:1;min-width:0}
.pad{display:flex;gap:6px;margin-top:8px;align-items:center}
.pad button{min-width:42px;padding:0}
.pad .code{flex:1;font-size:18px;letter-spacing:.08em;color:var(--y)}
.pad .code .note{font-size:13px;letter-spacing:0}
.fx{margin:6px 0 10px;padding-left:20px}
.file{display:flex;flex:none;align-items:center;border:1px solid var(--line);border-radius:10px;min-height:44px;padding:0 14px;background:var(--card);cursor:pointer}
#updStatus{color:var(--y);margin-top:8px}
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

  <h2>New stratagems &amp; icons</h2>
  <div class="wcard">
    <div>Looks in the community icon library for new warbonds and icon fixes, and adds them with their icons. Needs internet: the pad and this phone on your home Wi-Fi.</div>
    <div class="row"><button id="updCheck">Check for updates</button></div>
    <div id="updStatus"></div>
    <div id="updOut"></div>
  </div>

  <h2>Custom stratagems <span id="cCount"></span></h2>
  <div id="customs"></div>
  <div id="ced" class="imp" hidden>
    <div class="hd"><img id="cedImg" alt=""><input type="text" id="cedName" placeholder="Name" autocomplete="off"></div>
    <div class="pad"><select id="cedCat"></select><label class="file">Image&hellip;<input type="file" id="cedFile" accept="image/*,.svg" hidden></label></div>
    <div id="cedPad"></div>
    <div class="row"><button id="cedSave" class="primary">Save to pad</button><button id="cedCancel">Cancel</button><button id="cedDel" class="danger">Delete</button></div>
  </div>
  <button id="addCustom">+ New custom stratagem</button>
  <div class="note">Read codes from the in-game stratagem menu. Custom stratagems are saved right away, separately from &ldquo;Save to pad&rdquo;.</div>

  <h2>Chat messages</h2>
  <div id="msgs"></div>
  <button id="addMsg">+ Add message</button>
  <div class="note">Letters without accents only (they're typed on a keyboard). Add messages to a build from the &ldquo;Messages&rdquo; category above.</div>
</main>
<footer><button id="discard">Discard</button><button id="save" class="primary">Save to pad</button></footer>
<div id="toast"></div>
<script>
const MSG_CAT=9, $=id=>document.getElementById(id);
let S,B,M,active,sel=0,cat=0,query='',icons={},iconVer=null,dirty=false;
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
  setDirty(false);render();renderCustoms();updRender();
  if(S.iconVer!==iconVer){iconVer=S.iconVer;icons={};loadIcons()}
  if(!W)wifiLoad();
}

async function loadIcons(){
  try{
    const buf=new Uint8Array(await (await fetch('/icons?v='+S.iconVer)).arrayBuffer()), px=S.iconSize, cv=document.createElement('canvas');
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
    render();renderCustoms();
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
// ---- custom stratagems and updates from the icon library ----
const LIB='nvigneux/Helldivers-2-Stratagems-icons-svg';
const libUrl=p=>'https://raw.githubusercontent.com/'+LIB+'/HEAD/'+p.split('/').map(encodeURIComponent).join('/')+'.svg';
const norm=s=>s.toLowerCase().normalize('NFD').replace(/[^a-z0-9]/g,'');
const catOpts=c=>S.cats.slice(0,MSG_CAT).map((x,i)=>'<option value="'+i+'"'+(i===c?' selected':'')+'>'+esc(x.n)+'</option>').join('');
const codePad=k=>'<div class="pad"><span class="code">'+(k?arrows(k):'<span class="note">no code yet</span>')+'</span>'
  +'UDLR'.split('').map(d=>'<button data-d="'+d+'">'+ARW[d]+'</button>').join('')+'<button data-d="x" aria-label="Delete last arrow">&#9003;</button></div>';
const codeStep=(k,d)=>d==='x'?k.slice(0,-1):(k.length<S.codeLen?k+d:k);
let L=null,N={},FX=[],R={},svgT={},svgU={},openF=new Set(),working=false;  // library, new/fixes found, import rows, svg caches

function guessCat(n){
  n=n.toLowerCase();
  if(/^orbital/.test(n))return 0;
  if(/^eagle/.test(n))return 1;
  if(/exosuit|frv|vehicle|tank|maelstrom|bastion/.test(n))return 4;
  if(/sentry|tower/.test(n))return 5;
  if(/mine|emplacement|relay|battlement/.test(n))return 6;
  if(/pack|guard dog|backpack|shield/.test(n))return 3;
  if(/drill|beacon|flag|reinforce|resupply|hellbomb|probe|upload|container/.test(n))return 7;
  return 2;
}
function hiddenSet(){try{return new Set(JSON.parse(localStorage.getItem('mdHidden')||'[]'))}catch(e){return new Set()}}
function saveHidden(h){try{localStorage.setItem('mdHidden',JSON.stringify([...h]))}catch(e){}}
async function post(u,o){
  const r=await fetch(u,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(o)}),j=await r.json();
  if(!j.ok)throw new Error(j.err||'error');return j;
}
async function svgText(p){
  if(!svgT[p]){const r=await fetch(libUrl(p));if(!r.ok)throw new Error('icon download failed');svgT[p]=await r.text()}
  return svgT[p];
}
// image -> pad icon: 64 px then 44 px, RGB565 big-endian + alpha (icons.h, same as tools/icongen)
async function raster(blob){
  const u=URL.createObjectURL(blob),img=new Image();
  try{
    await new Promise((ok,ko)=>{img.onload=ok;img.onerror=()=>ko(new Error('unreadable image'));img.src=u});
    const out=new Uint8Array(64*64*3+44*44*3);let o=0;
    for(const px of [64,44]){
      const cv=document.createElement('canvas');cv.width=cv.height=px;const c=cv.getContext('2d');
      const w=img.naturalWidth||px,h=img.naturalHeight||px,k=Math.min(px/w,px/h);
      c.drawImage(img,(px-w*k)/2,(px-h*k)/2,w*k,h*k);
      const d=c.getImageData(0,0,px,px).data;
      for(let i=0;i<px*px;i++){
        const v=((d[4*i]>>3)<<11)|((d[4*i+1]>>2)<<5)|(d[4*i+2]>>3);
        out[o++]=v>>8;out[o++]=v&255;out[o++]=d[4*i+3];
      }
    }
    return out;
  }finally{URL.revokeObjectURL(u)}
}
const libIcon=async p=>raster(new Blob([await svgText(p)],{type:'image/svg+xml'}));
async function upIcon(id,sha,px){
  const fd=new FormData();fd.append('f',new Blob([px]),'icon.bin');
  const j=await (await fetch('/api/icon?id='+id+'&sha='+encodeURIComponent(sha),{method:'POST',body:fd})).json();
  if(!j.ok)throw new Error(j.err||'icon not saved');
}
function busy(on,msg){working=on;$('updStatus').textContent=msg||'';$('updCheck').disabled=on;document.querySelectorAll('#updOut button').forEach(b=>b.disabled=on)}
function needClean(){if(dirty)toast('Save or discard your build changes first');return dirty}

$('updCheck').onclick=async()=>{
  if(needClean())return;
  busy(true,'Checking the icon library…');
  try{
    const r=await fetch('https://api.github.com/repos/'+LIB+'/git/trees/HEAD?recursive=1');
    if(!r.ok)throw new Error(r.status===403||r.status===429?'GitHub is limiting requests: try again in an hour.':'GitHub error '+r.status);
    L=(await r.json()).tree.filter(x=>x.type==='blob'&&x.path.endsWith('.svg')).map(x=>{
      const p=x.path.slice(0,-4),k=p.lastIndexOf('/');return {p,sha:x.sha,folder:k<0?'':p.slice(0,k),name:p.slice(k+1)};
    });
    busy(false);
  }catch(e){busy(false,e instanceof TypeError?"Can't reach GitHub. The phone needs internet: use your home Wi-Fi, not the pad's setup network.":e.message)}
  updRender();
};

function updRender(){
  if(!L)return;
  const hid=hiddenSet(),byPath={},loose={};
  S.strats.forEach(s=>{if(s.p)byPath[s.p]=s;else loose[norm(s.n)]=s});
  FX=[];N={};let nHid=0;
  for(const f of L){
    const s=byPath[f.p]||loose[norm(f.name)];  // by library path, else by name (no icon yet)
    if(s){if(s.h!==f.sha&&!(s.h==='user'&&s.p))FX.push({f,s});continue}
    if(hid.has(f.p)){nHid++;continue}
    (N[f.folder]=N[f.folder]||[]).push(f);
  }
  const folders=Object.keys(N).sort(),room=S.maxCustom-S.strats.filter(s=>s.id>=1000).length;
  let h='';
  if(FX.length)h+='<div class="imp"><b>Icons to download</b><ul class="fx">'+FX.map(x=>'<li>'+esc(x.s.n)+' <span class="note">'+(x.s.h?'newer version':'missing')+'</span></li>').join('')
    +'</ul><button id="fixAll" class="primary">Download '+FX.length+' icon'+(FX.length>1?'s':'')+'</button></div>';
  for(const fd of folders){
    h+='<details class="imp" data-f="'+esc(fd)+'"'+(openF.has(fd)||folders.length===1?' open':'')+'><summary><b>'+esc(fd||'Library')+'</b> &middot; '+N[fd].length+' new</summary>';
    for(const f of N[fd]){
      const r=R[f.p]=R[f.p]||{name:ascii(f.name).slice(0,S.nameLen),cat:guessCat(f.name),code:'',on:true};
      h+='<div class="nr" data-p="'+esc(f.p)+'"><div class="hd"><input type="checkbox" data-a="on"'+(r.on?' checked':'')+' aria-label="Add this one">'
        +'<img data-svg="'+esc(f.p)+'" alt=""><input type="text" data-a="name" maxlength="'+S.nameLen+'" value="'+esc(r.name)+'"></div>'
        +'<div class="pad"><select data-a="cat">'+catOpts(r.cat)+'</select><button data-a="hide">Hide</button></div>'+codePad(r.code)+'</div>';
    }
    h+='<div class="row"><button class="primary" data-imp="'+esc(fd)+'">Add to pad</button></div></details>';
  }
  if(folders.length)h+='<div class="note">Enter codes from the in-game stratagem menu, now or later (a stratagem without a code can\'t be sent). Room for '+room+' more custom stratagems.</div>';
  if(!FX.length&&!folders.length)h+='<div class="note">Up to date: all '+L.length+' library icons are on the pad'+(nHid?' or hidden':'')+'.</div>';
  if(nHid)h+='<div class="row"><button id="unhide">Show '+nHid+' hidden</button></div>';
  $('updOut').innerHTML=h;
  document.querySelectorAll('#updOut img[data-svg]').forEach(async img=>{
    const p=img.dataset.svg;
    try{if(!svgU[p])svgU[p]=URL.createObjectURL(new Blob([await svgText(p)],{type:'image/svg+xml'}));img.src=svgU[p]}catch(e){}
  });
  if(working)busy(true,$('updStatus').textContent);
}

$('updOut').addEventListener('toggle',e=>{const d=e.target.dataset.f;if(d!==undefined)e.target.open?openF.add(d):openF.delete(d)},true);
$('updOut').oninput=e=>{
  const row=e.target.closest('[data-p]');if(!row)return;const r=R[row.dataset.p],a=e.target.dataset.a;
  if(a==='name'){const v=ascii(e.target.value);if(v!==e.target.value)e.target.value=v;r.name=v}
  else if(a==='on')r.on=e.target.checked;
  else if(a==='cat')r.cat=+e.target.value;
};
$('updOut').onclick=async e=>{
  const t=e.target.closest('button');if(!t||working)return;
  const row=t.closest('[data-p]');
  if(t.dataset.d&&row){const r=R[row.dataset.p];r.code=codeStep(r.code,t.dataset.d);t.parentNode.outerHTML=codePad(r.code);return}
  if(t.dataset.a==='hide'){const h=hiddenSet();h.add(row.dataset.p);saveHidden(h);updRender();return}
  if(t.id==='unhide'){saveHidden(new Set());updRender();return}
  if(needClean())return;
  if(t.id==='fixAll'){
    let n=0;
    try{
      for(const x of FX){
        busy(true,'Downloading icon for '+x.s.n+'…');
        await upIcon(x.s.id,x.f.sha,await libIcon(x.f.p));
        if(x.s.id>=1000&&!x.s.p)await post('/api/custom',{id:x.s.id,name:x.s.n,cat:x.s.c,code:x.s.k,src:x.f.p});  // track it from now on
        n++;
      }
      busy(false);toast(n+' icon'+(n>1?'s':'')+' updated ✓');
    }catch(x){busy(false,'Stopped: '+x.message)}
    await load();return;
  }
  if(t.dataset.imp!==undefined){
    const list=(N[t.dataset.imp]||[]).filter(f=>R[f.p].on);
    if(!list.length){toast('Nothing ticked');return}
    let n=0;
    try{
      for(const f of list){
        const r=R[f.p];
        busy(true,'Adding '+(r.name||f.name)+'…');
        const px=await libIcon(f.p);  // before creating it: a bad icon leaves nothing behind
        const j=await post('/api/custom',{id:0,name:r.name.trim()||ascii(f.name),cat:r.cat,code:r.code,src:f.p});
        await upIcon(j.id,f.sha,px);
        delete R[f.p];n++;
      }
      busy(false);toast(n+' added to the pad ✓');
    }catch(x){busy(false,'Stopped after '+n+': '+x.message)}
    await load();
  }
};

function renderCustoms(){
  const C=S.strats.filter(s=>s.id>=1000);
  $('cCount').textContent=C.length+' / '+S.maxCustom;
  $('customs').innerHTML=C.map(s=>'<div class="item" style="border-left-color:'+color(s.c)+'">'+icon(s)+'<div class="t"><div class="n">'+esc(s.n)+'</div><div class="k" style="color:'+(s.k?color(s.c):'var(--dim)')+'">'+(s.k?arrows(s.k):'no code yet')+'</div></div>'
    +'<button data-e="'+s.id+'" aria-label="Edit">&#9998;</button></div>').join('')||'<div class="empty">None yet. Add one here, or with &ldquo;Check for updates&rdquo;.</div>';
  $('addCustom').disabled=C.length>=S.maxCustom;
}
let E=null;  // custom stratagem in the editor
function cedOpen(s){
  if(needClean())return;
  E=s?{id:s.id,name:s.n,cat:s.c,code:s.k,px:null,img:icons[s.id]||''}:{id:0,name:'',cat:8,code:'',px:null,img:''};
  cedRender();$('ced').hidden=false;$('addCustom').hidden=true;$('cedName').focus();
}
function cedClose(){$('ced').hidden=true;$('addCustom').hidden=false;E=null}
function cedRender(){
  $('cedName').value=E.name;$('cedName').maxLength=S.nameLen;$('cedCat').innerHTML=catOpts(E.cat);$('cedPad').innerHTML=codePad(E.code);
  $('cedImg').src=E.img||'';$('cedImg').style.visibility=E.img?'visible':'hidden';$('cedDel').hidden=!E.id;
}
$('customs').onclick=e=>{const b=e.target.closest('[data-e]');if(b)cedOpen(S.strats.find(s=>s.id===+b.dataset.e))};
$('addCustom').onclick=()=>cedOpen(null);
$('cedPad').onclick=e=>{const d=e.target.closest('[data-d]');if(d){E.code=codeStep(E.code,d.dataset.d);$('cedPad').innerHTML=codePad(E.code)}};
$('cedName').oninput=e=>{const v=ascii(e.target.value);if(v!==e.target.value)e.target.value=v;E.name=v};
$('cedCat').onchange=e=>E.cat=+e.target.value;
$('cedFile').onchange=async e=>{
  const f=e.target.files[0];e.target.value='';if(!f)return;
  try{E.px=await raster(f);E.img=URL.createObjectURL(f);cedRender()}catch(x){toast("Can't read that image")}
};
$('cedCancel').onclick=cedClose;
$('cedSave').onclick=async()=>{
  if(!E.name.trim()){toast('Enter a name');return}
  $('cedSave').disabled=true;
  try{
    const j=await post('/api/custom',{id:E.id,name:E.name,cat:E.cat,code:E.code});
    if(E.px)await upIcon(j.id,'user',E.px);  // 'user': never replaced by a library update
    cedClose();toast('Saved to the pad ✓');await load();
  }catch(x){toast('Not saved: '+x.message)}
  $('cedSave').disabled=false;
};
$('cedDel').onclick=async()=>{
  if(!confirm('Delete "'+E.name+'"? It is also removed from your builds.'))return;
  try{await post('/api/custom/delete',{id:E.id});cedClose();toast('Deleted');await load()}catch(x){toast(x.message)}
};
// coming back to the tab: refresh if nothing is being edited
document.addEventListener('visibilitychange',()=>{if(!document.hidden){wifiLoad();if(!dirty)load()}});

window.onbeforeunload=e=>{if(dirty){e.preventDefault();return ''}};
load();
</script>
</body></html>)rawliteral";
