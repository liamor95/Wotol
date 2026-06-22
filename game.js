// WOTOL – War of the Ocean's Legacy | Isometric Tactical Demo
const C = document.getElementById('c');
const ctx = C.getContext('2d');
const W = 900, H = 600;

const TW = 64, TH = 32, ZS = 38;
const GW = 12, GH = 8;
let camX = 355, camY = 155;

const KEYS = {};
window.addEventListener('keydown', e => { KEYS[e.code] = true; e.preventDefault && ['Space','ArrowUp','ArrowDown','ArrowLeft','ArrowRight'].includes(e.code) && e.preventDefault(); });
window.addEventListener('keyup',  e => { KEYS[e.code] = false; });

function iso(gx, gy, gz = 0) {
  return { x: camX + (gx - gy) * TW / 2, y: camY + (gx + gy) * TH / 2 - gz * ZS };
}
function zoneOf(gy) {
  if (gy <= 2) return 2;
  if (gy <= 5) return 1;
  return 0;
}

// ── All 5 factions ────────────────────────────────────────────
const ALL_FACS = [
  { id:'aquiloris',     name:'Aquiloris',    color:'#4aa8ff', glow:'#1155cc', accent:'#aaddff', locked:false },
  { id:'noxeens',      name:"Noxeens",      color:'#00ee44', glow:'#004422', accent:'#88ffaa', locked:false, isEnemy:true },
  { id:'thalassidras', name:'Thalassidras', color:'#22ddbb', glow:'#0a5550', accent:'#88ffee', locked:true },
  { id:'mureens',      name:"Mureens",      color:'#dd8833', glow:'#663311', accent:'#ffcc88', locked:true },
  { id:'pirates',      name:'Pirates',      color:'#dd2244', glow:'#661122', accent:'#ff8899', locked:true },
];

const FAC = {
  aquiloris: { name:'Aquiloris', color:'#4aa8ff', glow:'#1155cc', accent:'#aaddff', darkBg:'#040c22',
    desc1:'Technologie cristalline', desc2:'Discipline militaire', res:'Cristaux' },
  noxeens:   { name:"Noxeens",  color:'#00ee44', glow:'#004422', accent:'#88ffaa', darkBg:'#020d04',
    desc1:"Creatures abyssales", desc2:'Bioluminescence mortelle', res:'Biolumens' },
};

const UDEFS = {
  hero:     { hp:280, dmg:38, range:2.2, spd:0.032, sz:22, lbl:"Leviaphenix", sublbl:"Unite mythique"   },
  infantry: { hp:140, dmg:20, range:1.4, spd:0.024, sz:16, lbl:'Aquiloryon',  sublbl:'Infanterie lourde' },
  ranged:   { hp: 75, dmg:30, range:5.5, spd:0.018, sz:14, lbl:'Aquistance',  sublbl:"Tireur a distance" },
  mounted:  { hp:115, dmg:24, range:1.7, spd:0.044, sz:18, lbl:'Aquilance',   sublbl:'Cavalier des mers' },
};

let state='MENU', playerFac='aquiloris', enemyFac='noxeens';
let units=[], particles=[], projs=[];
let selected=null, hovCell=null;
let placed=[], placingType=null;
let score=0, victory=false, frame=0;
let bgDots=[], aiTick=0;
let loadTimer=0, battleKills=0, playerLosses=0;
let dragStart=null, dragCur=null;
const LOAD_DUR = 160;
let BTN = {};

// ── Unit ──────────────────────────────────────────────────────
class Unit {
  constructor(fac, type, gx, gy) {
    const d = UDEFS[type];
    Object.assign(this, { fac, type, gx:+gx, gy:+gy, gz:zoneOf(gy),
      hp:d.hp, maxHp:d.hp, dmg:d.dmg, range:d.range, spd:d.spd, sz:d.sz,
      target:null, dest:null, cd:0, flash:0, dead:false });
  }
  get f()  { return FAC[this.fac]; }
  get sp() {
    return { x: camX + (this.gx - this.gy)*TW/2,
             y: camY + (this.gx + this.gy)*TH/2 - (this.gz + 0.55)*ZS };
  }
  dist(o) { return Math.hypot(o.gx - this.gx, o.gy - this.gy); }
  nearest(list) {
    let b=null, bd=Infinity;
    for (const u of list) { const d=this.dist(u); if (d<bd) { bd=d; b=u; } }
    return b;
  }
  update() {
    if (this.dead) return;
    if (this.cd>0) this.cd--;
    if (this.flash>0) this.flash--;
    const foes = units.filter(u => !u.dead && u.fac !== this.fac);
    if (!this.target || this.target.dead) this.target = this.nearest(foes);
    if (this.target) {
      const d = this.dist(this.target);
      if (d <= this.range) { if (this.cd===0) { this.cd=85; this.fire(this.target); } }
      else if (!this.dest && this.fac===enemyFac) this.dest = {gx:this.target.gx, gy:this.target.gy};
    }
    if (this.dest) {
      const dx=this.dest.gx-this.gx, dy=this.dest.gy-this.gy, d=Math.hypot(dx,dy);
      if (d<0.08) { this.gx=this.dest.gx; this.gy=this.dest.gy; this.gz=zoneOf(Math.round(this.gy)); this.dest=null; }
      else { this.gx+=dx/d*this.spd; this.gy+=dy/d*this.spd; this.gz=zoneOf(Math.round(this.gy)); }
    }
  }
  fire(t) {
    if (this.type==='ranged') {
      const sp=this.sp, tp=t.sp;
      projs.push({x:sp.x,y:sp.y,tx:tp.x,ty:tp.y,t,dmg:this.dmg,col:this.f.color,dead:false});
    } else { t.hurt(this.dmg); burst(t.sp.x,t.sp.y,this.f.color,7); }
  }
  hurt(dmg) {
    this.hp-=dmg; this.flash=12;
    if (this.hp<=0) {
      this.dead=true; burst(this.sp.x,this.sp.y,this.f.color,22);
      if (this.fac===enemyFac) { score+=this.type==='hero'?300:100; battleKills++; }
      if (this.fac===playerFac) playerLosses++;
    }
  }
  draw() {
    if (this.dead) return;
    const {x,y} = this.sp;
    const bob = Math.sin(frame*0.04 + this.gx*1.3 + this.gy*0.9) * 1.8;
    const sy = y + bob;
    const f = this.f, s = this.sz;
    const sel = selected===this;
    ctx.save();
    ctx.shadowBlur = this.flash>0 ? 22 : (sel ? 28 : 10);
    ctx.shadowColor = this.flash>0 ? '#ff4400' : (sel ? '#ffffff' : f.glow);

    // Ground shadow
    ctx.globalAlpha = 0.22;
    ctx.fillStyle = '#000020';
    ctx.beginPath(); ctx.ellipse(x, y+s*0.6, s*0.85, s*0.28, 0, 0, Math.PI*2); ctx.fill();
    ctx.globalAlpha = 1;

    if (this.fac === 'aquiloris') drawAquSprite(x, sy, s, this.type, f.color, f.accent);
    else drawNoxSprite(x, sy, s, this.type, f.color, f.accent);

    ctx.restore();

    // HP bar
    const bw=s*3, bh=4, bx=x-bw/2, by=sy-s-18;
    ctx.fillStyle='#06081a'; ctx.fillRect(bx,by,bw,bh);
    const pct=Math.max(0,this.hp/this.maxHp);
    ctx.fillStyle=pct>0.5?'#22ee55':pct>0.25?'#ffcc00':'#ff3300';
    ctx.fillRect(bx,by,bw*pct,bh);

    if (sel) {
      ctx.save();
      ctx.strokeStyle='rgba(255,255,255,0.85)'; ctx.lineWidth=1.5;
      ctx.setLineDash([4,3]); ctx.lineDashOffset=-frame*0.1;
      ctx.shadowBlur=0;
      ctx.beginPath(); ctx.arc(x,sy,s+10,0,Math.PI*2); ctx.stroke();
      ctx.restore();
    }
  }
}

// ── Aquiloris sprites ─────────────────────────────────────────
function drawAquSprite(x, y, s, type, col, acc) {
  ctx.strokeStyle = acc; ctx.lineWidth = 1.5;
  if (type === 'hero') {
    // Crystal wings
    ctx.fillStyle = col+'44';
    ctx.beginPath();
    ctx.moveTo(x,y-s*0.7); ctx.lineTo(x-s*1.5,y+s*0.1); ctx.lineTo(x-s*0.6,y+s*0.3); ctx.closePath(); ctx.fill();
    ctx.beginPath();
    ctx.moveTo(x,y-s*0.7); ctx.lineTo(x+s*1.5,y+s*0.1); ctx.lineTo(x+s*0.6,y+s*0.3); ctx.closePath(); ctx.fill();
    ctx.strokeStyle=acc+'88'; ctx.lineWidth=1;
    ctx.beginPath(); ctx.moveTo(x,y-s*0.7); ctx.lineTo(x-s*1.5,y+s*0.1); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(x,y-s*0.7); ctx.lineTo(x+s*1.5,y+s*0.1); ctx.stroke();
    // Body
    ctx.fillStyle=col; ctx.strokeStyle=acc; ctx.lineWidth=1.5;
    ctx.beginPath(); ctx.ellipse(x,y-s*0.35,s*0.42,s*0.62,0,0,Math.PI*2); ctx.fill(); ctx.stroke();
    // Head
    ctx.fillStyle=col; ctx.beginPath(); ctx.arc(x,y-s*1.1,s*0.3,0,Math.PI*2); ctx.fill(); ctx.stroke();
    // Crystal crown spikes
    ctx.strokeStyle=acc; ctx.lineWidth=2;
    for (let i=-2;i<=2;i++) {
      ctx.shadowBlur=8; ctx.shadowColor=acc;
      ctx.beginPath();
      ctx.moveTo(x+i*s*0.1,y-s*1.38);
      ctx.lineTo(x+i*s*0.09,y-s*1.38-s*(0.22-Math.abs(i)*0.05));
      ctx.stroke();
    }
    ctx.shadowBlur=12; ctx.shadowColor=acc;
    // Central orb
    ctx.fillStyle='#eef'; ctx.beginPath(); ctx.arc(x,y-s*0.32,s*0.16,0,Math.PI*2); ctx.fill();
    ctx.fillStyle=acc; ctx.beginPath(); ctx.arc(x,y-s*0.32,s*0.09,0,Math.PI*2); ctx.fill();

  } else if (type === 'infantry') {
    // Legs
    ctx.fillStyle=col+'cc';
    ctx.fillRect(x-s*0.28,y+s*0.05,s*0.22,s*0.42); ctx.strokeRect(x-s*0.28,y+s*0.05,s*0.22,s*0.42);
    ctx.fillRect(x+s*0.06,y+s*0.05,s*0.22,s*0.42); ctx.strokeRect(x+s*0.06,y+s*0.05,s*0.22,s*0.42);
    // Armored torso
    ctx.fillStyle=col;
    ctx.beginPath();
    ctx.moveTo(x-s*0.42,y+s*0.05); ctx.lineTo(x+s*0.42,y+s*0.05);
    ctx.lineTo(x+s*0.36,y-s*0.8); ctx.lineTo(x-s*0.36,y-s*0.8); ctx.closePath();
    ctx.fill(); ctx.stroke();
    // Chest plate highlight
    ctx.fillStyle=acc+'55';
    ctx.beginPath();
    ctx.moveTo(x-s*0.22,y+s*0.02); ctx.lineTo(x+s*0.22,y+s*0.02);
    ctx.lineTo(x+s*0.18,y-s*0.55); ctx.lineTo(x-s*0.18,y-s*0.55); ctx.closePath(); ctx.fill();
    // Head + helmet
    ctx.fillStyle=col; ctx.beginPath(); ctx.arc(x,y-s*0.98,s*0.29,0,Math.PI*2); ctx.fill(); ctx.stroke();
    // Helmet crest (vertical plume)
    ctx.fillStyle=acc+'cc';
    ctx.beginPath();
    ctx.moveTo(x-s*0.08,y-s*1.22); ctx.quadraticCurveTo(x,y-s*1.52,x+s*0.08,y-s*1.22);
    ctx.quadraticCurveTo(x,y-s*1.1,x-s*0.08,y-s*1.22); ctx.closePath(); ctx.fill();
    // Shield
    ctx.fillStyle=col+'dd'; ctx.strokeStyle=acc; ctx.lineWidth=1.5;
    ctx.beginPath(); ctx.ellipse(x-s*0.6,y-s*0.38,s*0.19,s*0.34,-0.25,0,Math.PI*2); ctx.fill(); ctx.stroke();
    ctx.strokeStyle=acc+'99'; ctx.lineWidth=0.8;
    ctx.beginPath(); ctx.moveTo(x-s*0.6,y-s*0.67); ctx.lineTo(x-s*0.6,y-s*0.1); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(x-s*0.78,y-s*0.38); ctx.lineTo(x-s*0.42,y-s*0.38); ctx.stroke();
    // Spear
    ctx.strokeStyle=acc; ctx.lineWidth=2; ctx.shadowBlur=6; ctx.shadowColor=acc;
    ctx.beginPath(); ctx.moveTo(x+s*0.36,y+s*0.35); ctx.lineTo(x+s*0.52,y-s*1.55); ctx.stroke();
    ctx.fillStyle='#eef'; ctx.strokeStyle=acc; ctx.lineWidth=1;
    ctx.beginPath(); ctx.moveTo(x+s*0.52,y-s*1.55); ctx.lineTo(x+s*0.39,y-s*1.36); ctx.lineTo(x+s*0.65,y-s*1.36); ctx.closePath(); ctx.fill(); ctx.stroke();

  } else if (type === 'ranged') {
    // Light armor
    ctx.fillStyle=col+'99';
    ctx.fillRect(x-s*0.2,y+s*0.05,s*0.16,s*0.38); ctx.fillRect(x+s*0.04,y+s*0.05,s*0.16,s*0.38);
    ctx.fillStyle=col+'bb';
    ctx.beginPath();
    ctx.moveTo(x-s*0.3,y+s*0.05); ctx.lineTo(x+s*0.3,y+s*0.05);
    ctx.lineTo(x+s*0.25,y-s*0.76); ctx.lineTo(x-s*0.25,y-s*0.76); ctx.closePath();
    ctx.fill(); ctx.stroke();
    // Hood/head
    ctx.fillStyle=col; ctx.beginPath(); ctx.arc(x,y-s*0.92,s*0.26,0,Math.PI*2); ctx.fill(); ctx.stroke();
    ctx.fillStyle=col+'88';
    ctx.beginPath(); ctx.arc(x,y-s*0.92,s*0.26,Math.PI,Math.PI*2); ctx.fill();
    // Crystal bow
    ctx.strokeStyle=acc; ctx.lineWidth=2.8; ctx.shadowBlur=10; ctx.shadowColor=acc;
    ctx.beginPath(); ctx.arc(x+s*0.52,y-s*0.48,s*0.52,-Math.PI*0.72,Math.PI*0.72); ctx.stroke();
    ctx.strokeStyle=col+'66'; ctx.lineWidth=1; ctx.shadowBlur=0;
    const bsy = y-s*0.48-s*0.52*Math.sin(Math.PI*0.72);
    const bey = y-s*0.48+s*0.52*Math.sin(Math.PI*0.72);
    ctx.beginPath(); ctx.moveTo(x+s*0.52,bsy); ctx.lineTo(x+s*0.52,bey); ctx.stroke();
    // Arrow
    ctx.strokeStyle=acc; ctx.lineWidth=1.2;
    ctx.beginPath(); ctx.moveTo(x+s*0.52,y-s*0.48); ctx.lineTo(x-s*0.1,y-s*0.5); ctx.stroke();
    ctx.fillStyle='#eef'; ctx.beginPath();
    ctx.moveTo(x-s*0.1,y-s*0.5); ctx.lineTo(x+s*0.04,y-s*0.55); ctx.lineTo(x+s*0.04,y-s*0.45); ctx.closePath(); ctx.fill();

  } else {
    // Mounted – manta ray mount
    ctx.fillStyle=col+'55'; ctx.strokeStyle=acc+'66'; ctx.lineWidth=1;
    ctx.beginPath();
    ctx.moveTo(x-s*1.2,y+s*0.2); ctx.quadraticCurveTo(x,y-s*0.3,x+s*1.2,y+s*0.2);
    ctx.quadraticCurveTo(x,y+s*0.5,x-s*1.2,y+s*0.2); ctx.closePath(); ctx.fill(); ctx.stroke();
    ctx.fillStyle=col+'88';
    ctx.beginPath(); ctx.ellipse(x,y+s*0.15,s*0.55,s*0.2,0,0,Math.PI*2); ctx.fill();
    // Rider
    ctx.fillStyle=col; ctx.strokeStyle=acc; ctx.lineWidth=1.5;
    ctx.beginPath(); ctx.ellipse(x,y-s*0.52,s*0.3,s*0.42,0,0,Math.PI*2); ctx.fill(); ctx.stroke();
    ctx.beginPath(); ctx.arc(x,y-s*1.0,s*0.26,0,Math.PI*2); ctx.fill(); ctx.stroke();
    // Visor glint
    ctx.fillStyle=acc; ctx.beginPath(); ctx.ellipse(x,y-s*1.03,s*0.12,s*0.07,0,0,Math.PI*2); ctx.fill();
    // Lance
    ctx.strokeStyle=acc; ctx.lineWidth=2.5; ctx.shadowBlur=8; ctx.shadowColor=acc;
    ctx.beginPath(); ctx.moveTo(x-s*0.2,y-s*0.65); ctx.lineTo(x+s*1.35,y-s*0.1); ctx.stroke();
    ctx.fillStyle='#eef'; ctx.strokeStyle=acc; ctx.lineWidth=1;
    ctx.beginPath(); ctx.moveTo(x+s*1.35,y-s*0.1); ctx.lineTo(x+s*1.18,y-s*0.22); ctx.lineTo(x+s*1.18,y+s*0.02); ctx.closePath(); ctx.fill(); ctx.stroke();
  }
}

// ── Noxeen sprites ────────────────────────────────────────────
function drawNoxSprite(x, y, s, type, col, acc) {
  const dark = '#0c1010';
  ctx.strokeStyle = col; ctx.lineWidth = 1.2;
  if (type === 'hero') {
    // Tentacles
    for (let i=0;i<6;i++) {
      const a = (Math.PI*2/6)*i + frame*0.015;
      const len = s*(1.1+0.15*Math.sin(frame*0.03+i));
      const tx=x+Math.cos(a)*len, ty=y+Math.sin(a)*len*0.55;
      ctx.strokeStyle=dark; ctx.lineWidth=s*0.2;
      ctx.beginPath(); ctx.moveTo(x,y-s*0.15); ctx.lineTo(tx,ty); ctx.stroke();
      ctx.strokeStyle=col+'55'; ctx.lineWidth=s*0.08;
      ctx.beginPath(); ctx.moveTo(x,y-s*0.15); ctx.lineTo(tx,ty); ctx.stroke();
      ctx.shadowBlur=8; ctx.shadowColor=col;
      ctx.fillStyle=col; ctx.beginPath(); ctx.arc(tx,ty,s*0.1,0,Math.PI*2); ctx.fill();
    }
    // Body
    ctx.shadowBlur=16; ctx.shadowColor=col;
    ctx.fillStyle=dark; ctx.strokeStyle=col; ctx.lineWidth=1.5;
    ctx.beginPath(); ctx.arc(x,y-s*0.3,s*0.58,0,Math.PI*2); ctx.fill(); ctx.stroke();
    // Glow ring
    ctx.strokeStyle=col+'33'; ctx.lineWidth=3;
    ctx.beginPath(); ctx.arc(x,y-s*0.3,s*0.65,0,Math.PI*2); ctx.stroke();
    // Eyes
    ctx.fillStyle=col; ctx.shadowBlur=14; ctx.shadowColor=col;
    ctx.beginPath(); ctx.ellipse(x-s*0.2,y-s*0.38,s*0.13,s*0.09,0,0,Math.PI*2); ctx.fill();
    ctx.beginPath(); ctx.ellipse(x+s*0.2,y-s*0.38,s*0.13,s*0.09,0,0,Math.PI*2); ctx.fill();
    ctx.fillStyle='#fff'; ctx.shadowBlur=0;
    ctx.beginPath(); ctx.arc(x-s*0.18,y-s*0.38,s*0.04,0,Math.PI*2); ctx.fill();
    ctx.beginPath(); ctx.arc(x+s*0.22,y-s*0.38,s*0.04,0,Math.PI*2); ctx.fill();

  } else if (type === 'infantry') {
    // Legs
    ctx.fillStyle=dark; ctx.strokeStyle=col+'55'; ctx.lineWidth=1;
    ctx.fillRect(x-s*0.28,y+s*0.05,s*0.22,s*0.42); ctx.strokeRect(x-s*0.28,y+s*0.05,s*0.22,s*0.42);
    ctx.fillRect(x+s*0.06,y+s*0.05,s*0.22,s*0.42); ctx.strokeRect(x+s*0.06,y+s*0.05,s*0.22,s*0.42);
    // Biolum leg stripes
    ctx.strokeStyle=col+'66'; ctx.lineWidth=1;
    ctx.beginPath(); ctx.moveTo(x-s*0.17,y+s*0.05); ctx.lineTo(x-s*0.17,y+s*0.44); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(x+s*0.17,y+s*0.05); ctx.lineTo(x+s*0.17,y+s*0.44); ctx.stroke();
    // Torso
    ctx.shadowBlur=8; ctx.shadowColor=col;
    ctx.fillStyle=dark; ctx.strokeStyle=col+'66'; ctx.lineWidth=1.5;
    ctx.beginPath();
    ctx.moveTo(x-s*0.42,y+s*0.05); ctx.lineTo(x+s*0.42,y+s*0.05);
    ctx.lineTo(x+s*0.36,y-s*0.82); ctx.lineTo(x-s*0.36,y-s*0.82); ctx.closePath(); ctx.fill(); ctx.stroke();
    // Chest biolum spots
    ctx.fillStyle=col; ctx.shadowBlur=10; ctx.shadowColor=col;
    for (let i=0;i<4;i++) {
      const bx=x+(i%2===0?-1:1)*s*0.14, by=y-s*(0.2+Math.floor(i/2)*0.32);
      ctx.beginPath(); ctx.arc(bx,by,s*0.07,0,Math.PI*2); ctx.fill();
    }
    // Head
    ctx.fillStyle=dark; ctx.strokeStyle=col+'77'; ctx.lineWidth=1.5; ctx.shadowBlur=6;
    ctx.beginPath(); ctx.arc(x,y-s*0.98,s*0.3,0,Math.PI*2); ctx.fill(); ctx.stroke();
    // Eyes
    ctx.fillStyle=col; ctx.shadowBlur=12; ctx.shadowColor=col;
    ctx.beginPath(); ctx.ellipse(x-s*0.12,y-s*1.02,s*0.09,s*0.06,0,0,Math.PI*2); ctx.fill();
    ctx.beginPath(); ctx.ellipse(x+s*0.12,y-s*1.02,s*0.09,s*0.06,0,0,Math.PI*2); ctx.fill();
    // Dark blade (green edge)
    ctx.fillStyle=dark; ctx.strokeStyle=col+'aa'; ctx.lineWidth=1.5; ctx.shadowBlur=8;
    ctx.beginPath();
    ctx.moveTo(x+s*0.38,y+s*0.35); ctx.lineTo(x+s*0.58,y-s*1.28); ctx.lineTo(x+s*0.44,y-s*1.18); ctx.closePath(); ctx.fill(); ctx.stroke();

  } else if (type === 'ranged') {
    // Floaty squid body
    ctx.shadowBlur=10; ctx.shadowColor=col;
    ctx.fillStyle=dark; ctx.strokeStyle=col+'55'; ctx.lineWidth=1.5;
    ctx.beginPath(); ctx.ellipse(x,y-s*0.35,s*0.3,s*0.56,0,0,Math.PI*2); ctx.fill(); ctx.stroke();
    // Biolum stripes
    ctx.strokeStyle=col+'44'; ctx.lineWidth=1;
    for (let i=0;i<3;i++) {
      ctx.beginPath(); ctx.ellipse(x,y-s*0.35,s*(0.16+i*0.07),s*(0.28+i*0.09),0,0,Math.PI*2); ctx.stroke();
    }
    // Small tentacles
    ctx.strokeStyle=dark; ctx.lineWidth=s*0.12;
    for (let i=-2;i<=2;i++) {
      ctx.beginPath(); ctx.moveTo(x+i*s*0.12,y+s*0.22);
      ctx.quadraticCurveTo(x+i*s*0.16+s*0.04*Math.sin(frame*0.06+i),y+s*0.48,x+i*s*0.1,y+s*0.58);
      ctx.stroke();
    }
    // Main eye
    ctx.fillStyle=col; ctx.shadowBlur=14; ctx.shadowColor=col;
    ctx.beginPath(); ctx.arc(x,y-s*0.55,s*0.14,0,Math.PI*2); ctx.fill();
    ctx.fillStyle='#fff'; ctx.shadowBlur=0;
    ctx.beginPath(); ctx.arc(x+s*0.05,y-s*0.57,s*0.05,0,Math.PI*2); ctx.fill();
    // Charge orb
    const cr=s*0.12+s*0.04*Math.sin(frame*0.09);
    ctx.fillStyle=col; ctx.shadowBlur=18; ctx.shadowColor=col;
    ctx.beginPath(); ctx.arc(x+s*0.42,y-s*0.35,cr,0,Math.PI*2); ctx.fill();
    ctx.fillStyle='#fff'; ctx.shadowBlur=0;
    ctx.beginPath(); ctx.arc(x+s*0.44,y-s*0.37,cr*0.38,0,Math.PI*2); ctx.fill();

  } else {
    // Mounted on abyss-beast
    ctx.shadowBlur=12; ctx.shadowColor=col;
    // Mount body (anglerfish-like)
    ctx.fillStyle=dark; ctx.strokeStyle=col+'44'; ctx.lineWidth=1.5;
    ctx.beginPath(); ctx.ellipse(x,y+s*0.12,s*0.78,s*0.36,0,0,Math.PI*2); ctx.fill(); ctx.stroke();
    // Mount stripes
    ctx.strokeStyle=col+'2a'; ctx.lineWidth=1;
    for (let i=0;i<3;i++) {
      ctx.beginPath(); ctx.ellipse(x,y+s*0.12,s*(0.38+i*0.16),s*(0.16+i*0.08),0,0,Math.PI*2); ctx.stroke();
    }
    // Mount eye
    ctx.fillStyle=col; ctx.shadowBlur=10; ctx.shadowColor=col;
    ctx.beginPath(); ctx.arc(x-s*0.6,y+s*0.05,s*0.09,0,Math.PI*2); ctx.fill();
    // Lure
    const lsz=s*0.11+s*0.05*Math.sin(frame*0.07);
    ctx.beginPath(); ctx.arc(x-s*0.82,y-s*0.28,lsz,0,Math.PI*2); ctx.fill();
    // Rider
    ctx.fillStyle=dark; ctx.strokeStyle=col+'66'; ctx.lineWidth=1.5;
    ctx.beginPath(); ctx.ellipse(x+s*0.12,y-s*0.58,s*0.26,s*0.4,0,0,Math.PI*2); ctx.fill(); ctx.stroke();
    ctx.beginPath(); ctx.arc(x+s*0.12,y-s*1.04,s*0.23,0,Math.PI*2); ctx.fill(); ctx.stroke();
    ctx.fillStyle=col; ctx.shadowBlur=10;
    ctx.beginPath(); ctx.arc(x+s*0.12,y-s*1.07,s*0.07,0,Math.PI*2); ctx.fill();
    // Trident
    ctx.strokeStyle=col; ctx.lineWidth=2; ctx.shadowBlur=8;
    ctx.beginPath(); ctx.moveTo(x-s*0.1,y-s*0.7); ctx.lineTo(x+s*0.9,y-s*0.1); ctx.stroke();
    ctx.lineWidth=1.2;
    for (let i=-1;i<=1;i++) {
      ctx.beginPath();
      ctx.moveTo(x+s*0.9+i*s*0.08,y-s*0.1);
      ctx.lineTo(x+s*0.9+i*s*0.12,y-s*0.26);
      ctx.stroke();
    }
  }
}

// ── Particles ─────────────────────────────────────────────────
function burst(x,y,col,n) {
  for (let i=0;i<n;i++) {
    const a=Math.random()*Math.PI*2, sp=Math.random()*4.5+1;
    particles.push({x,y,vx:Math.cos(a)*sp,vy:Math.sin(a)*sp,life:38,max:38,col,r:Math.random()*3+1});
  }
}
function tickParticles() {
  for (const p of particles) { p.x+=p.vx; p.y+=p.vy; p.vx*=0.88; p.vy*=0.88; p.life--; }
  particles=particles.filter(p=>p.life>0);
}
function drawParticles() {
  ctx.save();
  for (const p of particles) {
    ctx.globalAlpha=(p.life/p.max)*0.92; ctx.shadowBlur=7; ctx.shadowColor=p.col;
    ctx.fillStyle=p.col; ctx.beginPath(); ctx.arc(p.x,p.y,p.r*(p.life/p.max),0,Math.PI*2); ctx.fill();
  }
  ctx.restore();
}
function tickProjs() {
  for (const p of projs) {
    if (p.dead) continue;
    const dx=p.tx-p.x, dy=p.ty-p.y, d=Math.hypot(dx,dy);
    if (d<8) { p.t.hurt(p.dmg); burst(p.x,p.y,p.col,7); p.dead=true; }
    else { p.x+=dx/d*7; p.y+=dy/d*7; }
  }
  projs=projs.filter(p=>!p.dead);
}
function drawProjs() {
  ctx.save();
  for (const p of projs) {
    ctx.shadowBlur=14; ctx.shadowColor=p.col;
    ctx.fillStyle=p.col; ctx.beginPath(); ctx.arc(p.x,p.y,4,0,Math.PI*2); ctx.fill();
    ctx.fillStyle='#fff'; ctx.shadowBlur=0;
    ctx.beginPath(); ctx.arc(p.x,p.y,1.8,0,Math.PI*2); ctx.fill();
  }
  ctx.restore();
}

// ── Background ────────────────────────────────────────────────
function initDots() {
  bgDots=[];
  for (let i=0;i<60;i++) bgDots.push({x:Math.random()*W,y:Math.random()*H,r:Math.random()*2.2+0.4,vy:-(Math.random()*0.28+0.1),vx:(Math.random()-0.5)*0.1,a:Math.random()*0.28+0.06});
}
function drawBg(top) {
  const g=ctx.createLinearGradient(0,0,0,H);
  g.addColorStop(0,top||'#06101e'); g.addColorStop(1,'#020810');
  ctx.fillStyle=g; ctx.fillRect(0,0,W,H);
  ctx.save();
  for (let i=0;i<5;i++) {
    const cx=W/5*i+W/10, ww=14+Math.sin(frame*0.007+i)*9;
    const g2=ctx.createLinearGradient(0,0,0,H);
    g2.addColorStop(0,'rgba(50,90,200,0.055)'); g2.addColorStop(1,'rgba(30,60,140,0)');
    ctx.fillStyle=g2;
    ctx.beginPath(); ctx.moveTo(cx-ww,0); ctx.lineTo(cx+ww,0); ctx.lineTo(cx+ww*3,H); ctx.lineTo(cx-ww*3,H); ctx.closePath(); ctx.fill();
  }
  for (const d of bgDots) {
    d.x+=d.vx; d.y+=d.vy; if (d.y<-4){d.y=H+4;d.x=Math.random()*W;}
    ctx.globalAlpha=d.a; ctx.strokeStyle='#3377aa'; ctx.lineWidth=0.7;
    ctx.beginPath(); ctx.arc(d.x,d.y,d.r,0,Math.PI*2); ctx.stroke();
  }
  ctx.restore();
}

// ── Map ───────────────────────────────────────────────────────
const TC=[
  {top:'#050c1a',sL:'#030710',sR:'#040912'},
  {top:'#08162e',sL:'#04090e',sR:'#060e1e'},
  {top:'#0d2244',sL:'#071228',sR:'#0a1a38'},
];
function drawMap(hl) {
  for (let gy=0;gy<GH;gy++) for (let gx=0;gx<GW;gx++) drawTile(gx,gy,zoneOf(gy),hl);
  drawZoneLabels();
}
function drawTile(gx,gy,gz,hl) {
  const {x,y}=iso(gx,gy,gz); const hw=TW/2,hh=TH/2; const tc=TC[gz];
  const hov=hovCell&&hovCell.gx===gx&&hovCell.gy===gy; const isL=gx<6;
  if (gz>0) {
    ctx.fillStyle=tc.sL;
    ctx.beginPath(); ctx.moveTo(x-hw,y+hh); ctx.lineTo(x-hw,y+hh+gz*ZS); ctx.lineTo(x,y+TH+gz*ZS); ctx.lineTo(x,y+TH); ctx.closePath(); ctx.fill();
    ctx.fillStyle=tc.sR;
    ctx.beginPath(); ctx.moveTo(x+hw,y+hh); ctx.lineTo(x+hw,y+hh+gz*ZS); ctx.lineTo(x,y+TH+gz*ZS); ctx.lineTo(x,y+TH); ctx.closePath(); ctx.fill();
  }
  let top=tc.top;
  if (hov) top='#1a3c70';
  else if (hl&&isL) top=gz===2?'#0e2a55':gz===1?'#0a1e42':'#080e28';
  ctx.fillStyle=top;
  ctx.beginPath(); ctx.moveTo(x,y); ctx.lineTo(x+hw,y+hh); ctx.lineTo(x,y+TH); ctx.lineTo(x-hw,y+hh); ctx.closePath(); ctx.fill();
  ctx.strokeStyle='rgba(20,50,110,0.22)'; ctx.lineWidth=0.5;
  ctx.beginPath(); ctx.moveTo(x,y); ctx.lineTo(x+hw,y+hh); ctx.lineTo(x,y+TH); ctx.lineTo(x-hw,y+hh); ctx.closePath(); ctx.stroke();
  if (hl&&isL) {
    ctx.strokeStyle='rgba(60,120,255,0.32)'; ctx.lineWidth=1;
    ctx.beginPath(); ctx.moveTo(x,y); ctx.lineTo(x+hw,y+hh); ctx.lineTo(x,y+TH); ctx.lineTo(x-hw,y+hh); ctx.closePath(); ctx.stroke();
  }
  if (gz===0&&Math.sin(gx*1.9+gy*2.7+frame*0.018)>0.82) {
    ctx.fillStyle=`rgba(0,200,70,${0.22+0.12*Math.sin(frame*0.04)})`;
    ctx.beginPath(); ctx.arc(x+(gx%3-1)*7,y+hh,1.8,0,Math.PI*2); ctx.fill();
  }
  if (gz===2&&Math.sin(gx*2.3+gy*1.6+frame*0.013)>0.88) {
    ctx.fillStyle=`rgba(80,160,255,${0.36+0.2*Math.sin(frame*0.035)})`;
    ctx.beginPath(); ctx.arc(x-4+(gy%3)*4,y+6,1.5,0,Math.PI*2); ctx.fill();
  }
}
function drawZoneLabels() {
  const zl=[{gz:2,label:'SURFACE  -  15m',gy:1},{gz:1,label:'MI-FOND  -  5m',gy:4},{gz:0,label:'SOL  -  0m',gy:6.5}];
  ctx.font='9px Courier New'; ctx.textAlign='right'; ctx.textBaseline='middle';
  for (const z of zl) {
    const {x,y}=iso(-0.4,z.gy,z.gz+0.9);
    ctx.fillStyle='rgba(40,100,180,0.5)'; ctx.fillText(z.label,x-2,y);
  }
}
function drawEnemyPreview() {
  const types=['hero','infantry','infantry','ranged','mounted','infantry'];
  const pos=[[10,1],[11,2],[10,4],[11,5],[9,3],[10,6]];
  const f=FAC[enemyFac];
  ctx.save(); ctx.globalAlpha=0.42;
  pos.forEach(([gx,gy],i)=>{
    if (i>=types.length) return;
    const gz=zoneOf(gy); const {x,y}=iso(gx,gy,gz+0.55);
    ctx.shadowBlur=6; ctx.shadowColor=f.glow;
    drawNoxSprite(x,y,UDEFS[types[i]].sz,types[i],f.color,f.accent);
  });
  ctx.restore();
}

// ── AI ────────────────────────────────────────────────────────
function runAI() {
  aiTick++;
  if (aiTick%140!==0) return;
  const enemies=units.filter(u=>!u.dead&&u.fac===enemyFac);
  const players=units.filter(u=>!u.dead&&u.fac===playerFac);
  if (!players.length) return;
  for (const e of enemies) {
    const t=players[Math.floor(Math.random()*players.length)];
    e.target=t;
    if (Math.random()<0.35) e.dest={gx:Math.max(0,t.gx-0.5+Math.random()),gy:Math.max(0,Math.min(GH-1,t.gy+(Math.random()-0.5)*2))};
  }
}

// ── Camera ────────────────────────────────────────────────────
function updateCamera() {
  if (state!=='BATTLE'&&state!=='PLACEMENT') return;
  const sp=4;
  if (KEYS['KeyW']||KEYS['ArrowUp'])    camY+=sp;
  if (KEYS['KeyS']||KEYS['ArrowDown'])  camY-=sp;
  if (KEYS['KeyA']||KEYS['ArrowLeft'])  camX+=sp;
  if (KEYS['KeyD']||KEYS['ArrowRight']) camX-=sp;
  camX=Math.max(150,Math.min(550,camX));
  camY=Math.max(50, Math.min(300,camY));
}

// ── Game start ────────────────────────────────────────────────
function startBattle() {
  units=[]; particles=[]; projs=[];
  selected=null; score=0; frame=0; aiTick=0;
  battleKills=0; playerLosses=0;
  camX=355; camY=155;
  for (const p of placed) units.push(new Unit(playerFac,p.type,p.gx,p.gy));
  const eT=['hero','infantry','infantry','ranged','mounted','infantry'];
  const eP=[[10,1],[11,2],[10,4],[11,5],[9,3],[10,6]];
  eT.forEach((t,i)=>{ const [gx,gy]=eP[i]||[10,4]; units.push(new Unit(enemyFac,t,gx,Math.min(gy,GH-1))); });
  state='BATTLE';
}
function checkEnd() {
  const pA=units.filter(u=>!u.dead&&u.fac===playerFac).length;
  const eA=units.filter(u=>!u.dead&&u.fac===enemyFac).length;
  if (pA===0){victory=false;state='END';}
  if (eA===0){victory=true; state='END';}
}

// ── HUD helpers ───────────────────────────────────────────────
function drawBtn(label,x,y,w,h,col,fill) {
  ctx.fillStyle=fill||'rgba(0,0,0,0)'; ctx.fillRect(x,y,w,h);
  ctx.strokeStyle=col; ctx.lineWidth=1; ctx.strokeRect(x,y,w,h);
  ctx.fillStyle=col; ctx.font='bold 13px Courier New';
  ctx.textAlign='center'; ctx.textBaseline='middle';
  ctx.fillText(label,x+w/2,y+h/2);
  return {x,y,w,h};
}
function hit(mx,my,b){return b&&mx>=b.x&&mx<=b.x+b.w&&my>=b.y&&my<=b.y+b.h;}
function fmtTime(frames){const s=Math.floor(frames/60),m=Math.floor(s/60),ss=s%60;return `${m}m ${ss<10?'0':''}${ss}s`;}

function drawBattleHUD() {
  const pf=FAC[playerFac],ef=FAC[enemyFac];
  const pA=units.filter(u=>!u.dead&&u.fac===playerFac).length;
  const eA=units.filter(u=>!u.dead&&u.fac===enemyFac).length;

  // Top bar
  ctx.fillStyle='rgba(2,4,14,0.92)'; ctx.fillRect(0,0,W,34);
  ctx.font='bold 12px Courier New'; ctx.textBaseline='middle';
  ctx.shadowBlur=8; ctx.shadowColor=pf.glow;
  ctx.fillStyle=pf.color; ctx.textAlign='left';
  ctx.fillText(`> ${pf.name.toUpperCase()}  [${pA}]`,12,17);
  ctx.shadowBlur=0; ctx.fillStyle='#667'; ctx.textAlign='center';
  ctx.fillText(`SCORE  ${score}`,W/2,17);
  ctx.shadowBlur=6; ctx.shadowColor=ef.glow;
  ctx.fillStyle=ef.color; ctx.textAlign='right';
  ctx.fillText(`[${eA}]  ${ef.name.toUpperCase()}  <`,W-12,17);
  ctx.shadowBlur=0;

  // Bottom bar (controls)
  ctx.fillStyle='rgba(2,4,14,0.82)'; ctx.fillRect(0,H-24,W,24);
  ctx.fillStyle='#233048'; ctx.font='10px Courier New'; ctx.textAlign='center'; ctx.textBaseline='middle';
  ctx.fillText('Z/Q/S/D ou FLECHES : Camera  |  CLIC GAUCHE : Selectionner  |  CLIC DROIT : Deplacer / Attaquer',W/2,H-12);

  // SURFACE / MID / SOL panel
  const selZ=selected?selected.gz:-1;
  const lvl=[{gz:2,lbl:'SURFACE',y:90},{gz:1,lbl:'MID',y:150},{gz:0,lbl:'SOL',y:210}];
  ctx.fillStyle='rgba(2,4,14,0.88)'; ctx.fillRect(6,78,66,150);
  ctx.strokeStyle='#1a2840'; ctx.lineWidth=1; ctx.strokeRect(6,78,66,150);
  for (const lv of lvl) {
    const act=lv.gz===selZ;
    ctx.fillStyle=act?pf.color+'33':'transparent'; ctx.fillRect(8,lv.y-14,62,28);
    if (act){ctx.strokeStyle=pf.color;ctx.lineWidth=1.5;ctx.strokeRect(8,lv.y-14,62,28);ctx.fillStyle='#fff';ctx.shadowBlur=8;ctx.shadowColor=pf.color;}
    else{ctx.fillStyle='#2a3a55';ctx.shadowBlur=0;}
    ctx.font=act?'bold 10px Courier New':'9px Courier New'; ctx.textAlign='center'; ctx.textBaseline='middle';
    ctx.fillText(lv.lbl,39,lv.y); ctx.shadowBlur=0;
    if (lv.gz>0){ctx.strokeStyle='#1a2840';ctx.lineWidth=1;ctx.beginPath();ctx.moveTo(39,lv.y+14);ctx.lineTo(39,lv.y+28);ctx.stroke();}
  }

  // Unit info
  if (selected) {
    const px=10,py=H-116,pw=210,ph=88;
    ctx.fillStyle='rgba(2,6,20,0.94)'; ctx.fillRect(px,py,pw,ph);
    ctx.strokeStyle=pf.color+'44'; ctx.lineWidth=1; ctx.strokeRect(px,py,pw,ph);
    ctx.fillStyle=pf.color; ctx.font='bold 11px Courier New'; ctx.textAlign='left'; ctx.textBaseline='top';
    ctx.fillText(`${UDEFS[selected.type].lbl.toUpperCase()}  -  Z${selected.gz}`,px+8,py+8);
    ctx.fillStyle='#5a6880'; ctx.font='10px Courier New';
    ctx.fillText(`PV: ${Math.max(0,Math.ceil(selected.hp))} / ${selected.maxHp}`,px+8,py+26);
    ctx.fillText(`ATQ: ${selected.dmg}   Portee: ${selected.range}   Vit: ${(selected.spd*100).toFixed(0)}`,px+8,py+42);
    const zn=selected.gz===2?'Surface':selected.gz===1?'Mi-fond':'Sol';
    ctx.fillText(`Zone: ${zn}   Faction: ${pf.name}`,px+8,py+58);
    ctx.fillStyle=pf.color+'88'; ctx.font='9px Courier New';
    ctx.fillText('CLIC DROIT sur cible ou case pour agir',px+8,py+74);
  }
}

// ── Screens ───────────────────────────────────────────────────
function drawMenu() {
  drawBg();
  ctx.textAlign='center';
  const shimmer=0.05+0.035*Math.sin(frame*0.02);
  ctx.fillStyle=`rgba(20,60,180,${shimmer})`; ctx.fillRect(0,0,W,H*0.38);

  ctx.fillStyle='#122244'; ctx.font='11px Courier New';
  ctx.fillText('WAR  OF  THE  OCEAN\'S  LEGACY',W/2,112);
  ctx.strokeStyle='#0d1e42'; ctx.lineWidth=0.5;
  ctx.beginPath(); ctx.moveTo(W/2-190,126); ctx.lineTo(W/2-26,126); ctx.stroke();
  ctx.beginPath(); ctx.moveTo(W/2+26,126); ctx.lineTo(W/2+190,126); ctx.stroke();

  ctx.save();
  const pulse=26+10*Math.sin(frame*0.025);
  ctx.shadowBlur=pulse; ctx.shadowColor='#1155cc';
  ctx.fillStyle='#4aa8ff'; ctx.font='bold 92px Courier New';
  ctx.fillText('WOTOL',W/2,228);
  ctx.shadowBlur=pulse*0.4; ctx.shadowColor='#aaddff'; ctx.globalAlpha=0.22;
  ctx.fillText('WOTOL',W/2,228);
  ctx.restore();

  ctx.fillStyle='#0d1e42'; ctx.font='10px Courier New';
  ctx.fillText('DEMO TACTIQUE  -  PRE-ALPHA',W/2,255);
  ctx.strokeStyle='#0a1830'; ctx.lineWidth=0.5;
  ctx.beginPath(); ctx.moveTo(W/2-155,272); ctx.lineTo(W/2+155,272); ctx.stroke();

  ctx.fillStyle='#1a2e4a'; ctx.font='12px Courier New';
  ctx.fillText('Les peuples des oceans vivent dans une paix fragile.',W/2,308);
  ctx.fillText("Un ancien artefact refait surface. Une nouvelle guerre eclate.",W/2,330);
  ctx.fillStyle='#0f1e36';
  ctx.fillText("Derriere cette guerre se cache une verite bien plus ancienne.",W/2,352);

  BTN.play=drawBtn('NOUVELLE BATAILLE',W/2-130,404,260,50,'#4aa8ff','rgba(0,20,60,0.55)');

  ctx.fillStyle='#080e1c'; ctx.font='9px Courier New';
  ctx.fillText('c WOTOL PROJECT  -  DEMO v0.2  -  5 FACTIONS  -  TACTICAL RTS',W/2,H-14);
}

function drawFaction() {
  drawBg();
  ctx.textAlign='center';
  ctx.fillStyle='#4aa8ff'; ctx.font='bold 22px Courier New'; ctx.fillText('CHOISIR VOTRE FACTION',W/2,46);
  ctx.fillStyle='#122244'; ctx.font='10px Courier New'; ctx.fillText('DEMO : Aquiloris contre les Noxeens  |  3 factions a venir',W/2,66);

  const cW=156,cH=236,sX=38,sY=82,gap=7;
  BTN.pick=null; BTN.back=null;
  ALL_FACS.forEach((fac,i)=>{
    const cx=sX+i*(cW+gap);
    const isL=fac.locked,isE=fac.isEnemy,isP=!isL&&!isE;
    ctx.globalAlpha=isL?0.32:1;
    ctx.fillStyle=isP?'#060e24':isE?'#040e06':'#050810'; ctx.fillRect(cx,sY,cW,cH);
    ctx.strokeStyle=isP?fac.color+'88':isE?fac.color+'33':'#080e18'; ctx.lineWidth=isP?1.5:1; ctx.strokeRect(cx,sY,cW,cH);
    ctx.globalAlpha=1;
    ctx.fillStyle=isL?'#1a2030':fac.color; ctx.font='bold 10px Courier New'; ctx.fillText(fac.name.toUpperCase(),cx+cW/2,sY+20);
    // Emblem
    ctx.save(); ctx.globalAlpha=isL?0.16:isE?0.5:1; ctx.shadowBlur=isL?0:18; ctx.shadowColor=fac.glow; ctx.strokeStyle=fac.color; ctx.lineWidth=1.5;
    const ex=cx+cW/2,ey=sY+116;
    if (i===0) { // Aquiloris – mini star warrior
      ctx.fillStyle=fac.color; ctx.beginPath();
      ctx.moveTo(ex,ey-34); ctx.lineTo(ex-10,ey-18); ctx.lineTo(ex-36,ey-12); ctx.lineTo(ex-16,ey+4);
      ctx.lineTo(ex-22,ey+30); ctx.lineTo(ex,ey+18); ctx.lineTo(ex+22,ey+30); ctx.lineTo(ex+16,ey+4);
      ctx.lineTo(ex+36,ey-12); ctx.lineTo(ex+10,ey-18); ctx.closePath(); ctx.fill(); ctx.stroke();
    } else if (i===1) { // Noxeen – tentacled circle
      ctx.fillStyle='#0c1010'; ctx.beginPath(); ctx.arc(ex,ey,26,0,Math.PI*2); ctx.fill();
      ctx.strokeStyle=fac.color; ctx.lineWidth=1.5; ctx.beginPath(); ctx.arc(ex,ey,26,0,Math.PI*2); ctx.stroke();
      ctx.fillStyle=fac.color; ctx.shadowBlur=12;
      ctx.beginPath(); ctx.ellipse(ex-9,ey-2,6,4,0,0,Math.PI*2); ctx.fill();
      ctx.beginPath(); ctx.ellipse(ex+9,ey-2,6,4,0,0,Math.PI*2); ctx.fill();
      for (let t=0;t<5;t++){const a=Math.PI/5*t*2;ctx.beginPath();ctx.moveTo(ex,ey+16);ctx.lineTo(ex+Math.cos(a)*30,ey+16+Math.sin(a)*14);ctx.stroke();}
    } else if (i===2) {
      ctx.fillStyle=fac.color; ctx.beginPath(); ctx.moveTo(ex,ey-32); ctx.lineTo(ex+28,ey); ctx.lineTo(ex,ey+32); ctx.lineTo(ex-28,ey); ctx.closePath(); ctx.fill(); ctx.stroke();
    } else if (i===3) {
      ctx.fillStyle=fac.color; ctx.beginPath(); ctx.arc(ex,ey,28,0,Math.PI*2); ctx.fill(); ctx.stroke();
      ctx.strokeStyle=fac.color+'88'; ctx.lineWidth=5; ctx.beginPath(); ctx.arc(ex,ey,18,0,Math.PI*2); ctx.stroke();
    } else {
      ctx.fillStyle=fac.color;
      ctx.beginPath(); ctx.moveTo(ex,ey-32); ctx.lineTo(ex+28,ey+20); ctx.lineTo(ex-28,ey+20); ctx.closePath(); ctx.fill(); ctx.stroke();
    }
    ctx.restore();
    // Badge
    const bx=cx+8,by=sY+188,bw=cW-16,bh=26;
    if (isL) {
      ctx.fillStyle='#08101c'; ctx.fillRect(bx,by,bw,bh);
      ctx.strokeStyle='#0a1428'; ctx.lineWidth=1; ctx.strokeRect(bx,by,bw,bh);
      ctx.fillStyle='#1e2a3a'; ctx.font='bold 9px Courier New'; ctx.fillText('[  BIENTOT  ]',cx+cW/2,by+13);
    } else if (isE) {
      ctx.fillStyle='#02080a'; ctx.fillRect(bx,by,bw,bh);
      ctx.strokeStyle=fac.color+'22'; ctx.lineWidth=1; ctx.strokeRect(bx,by,bw,bh);
      ctx.fillStyle=fac.color+'88'; ctx.font='bold 9px Courier New'; ctx.fillText('ENNEMI  (IA)',cx+cW/2,by+13);
    } else {
      ctx.fillStyle=fac.color+'18'; ctx.fillRect(bx,by,bw,bh);
      ctx.strokeStyle=fac.color; ctx.lineWidth=1; ctx.strokeRect(bx,by,bw,bh);
      ctx.fillStyle=fac.color; ctx.font='bold 9px Courier New'; ctx.fillText('> CHOISIR',cx+cW/2,by+13);
      BTN.pick={x:bx,y:by,w:bw,h:bh};
    }
  });
  // Info
  const pf=FAC.aquiloris,ef=FAC.noxeens;
  const ip=330;
  ctx.fillStyle='rgba(4,8,24,0.94)'; ctx.fillRect(38,ip,824,164); ctx.strokeStyle=pf.color+'22'; ctx.lineWidth=1; ctx.strokeRect(38,ip,824,164);
  ctx.save(); ctx.shadowBlur=12; ctx.shadowColor=pf.glow;
  ctx.fillStyle=pf.color; ctx.font='bold 15px Courier New'; ctx.textAlign='left'; ctx.fillText('AQUILORIS',60,ip+28); ctx.restore();
  ctx.fillStyle='#3a5070'; ctx.font='10px Courier New'; ctx.textAlign='left';
  ctx.fillText("Maitres des cristaux des profondeurs. Empire de lumiere et d'ordre militaire.",60,ip+50);
  ctx.fillStyle='#2a3a55'; ctx.fillText(`Specialite: ${pf.desc1}  -  ${pf.desc2}`,60,ip+70);
  ctx.fillText(`Ressource: ${pf.res}`,60,ip+88);
  ctx.fillStyle='#cc2244'; ctx.fillRect(W/2-1,ip+14,2,132);
  ctx.fillStyle='#cc3322'; ctx.font='bold 14px Courier New'; ctx.textAlign='center'; ctx.fillText('VS',W/2,ip+88);
  ctx.save(); ctx.shadowBlur=10; ctx.shadowColor=ef.glow;
  ctx.fillStyle=ef.color; ctx.font='bold 15px Courier New'; ctx.textAlign='right'; ctx.fillText('NOXEENS  (IA)',W-60,ip+28); ctx.restore();
  ctx.fillStyle='#1a3020'; ctx.font='10px Courier New'; ctx.textAlign='right';
  ctx.fillText("Surgis des abysses. Creatures de tenebres a bioluminescence mortelle.",W-60,ip+50);
  ctx.fillStyle='#1a2a1a'; ctx.fillText(`${ef.desc1}  -  ${ef.desc2}`,W-60,ip+70); ctx.fillText(`Ressource: ${ef.res}`,W-60,ip+88);
  BTN.back=drawBtn('< RETOUR',38,ip+132,110,24,'#446688','rgba(4,8,20,0.8)');
}

function drawLoading() {
  drawBg();
  const pf=FAC[playerFac],ef=FAC[enemyFac];
  const prog=Math.min(1,loadTimer/LOAD_DUR);
  ctx.textAlign='center'; ctx.textBaseline='middle';
  ctx.fillStyle='#1a2840'; ctx.font='11px Courier New'; ctx.fillText('PREPARATION AU COMBAT',W/2,72);

  // Player side
  const lx=W/2-170;
  ctx.save(); ctx.shadowBlur=24+10*Math.sin(frame*0.04); ctx.shadowColor=pf.glow; ctx.strokeStyle=pf.accent; ctx.lineWidth=2;
  drawAquSprite(lx,H/2-10,28,'hero',pf.color,pf.accent);
  ctx.restore();
  ctx.save(); ctx.shadowBlur=12; ctx.shadowColor=pf.glow;
  ctx.fillStyle=pf.color; ctx.font='bold 18px Courier New'; ctx.fillText(pf.name.toUpperCase(),lx,H/2+68); ctx.restore();
  ctx.fillStyle='#2a3a55'; ctx.font='9px Courier New'; ctx.fillText('JOUEUR',lx,H/2+86);

  // VS
  ctx.save(); ctx.shadowBlur=22; ctx.shadowColor='#cc2200';
  ctx.fillStyle='#ff2200'; ctx.font='bold 40px Courier New'; ctx.fillText('VS',W/2,H/2-10); ctx.restore();

  // Enemy side
  const rx=W/2+170;
  ctx.save(); ctx.globalAlpha=0.65; ctx.shadowBlur=18; ctx.shadowColor=ef.glow; ctx.strokeStyle=ef.accent; ctx.lineWidth=2;
  drawNoxSprite(rx,H/2-10,28,'hero',ef.color,ef.accent);
  ctx.restore();
  ctx.fillStyle=ef.color+'bb'; ctx.font='bold 18px Courier New'; ctx.fillText(ef.name.toUpperCase(),rx,H/2+68);
  ctx.fillStyle='#1a2a1a'; ctx.font='9px Courier New'; ctx.fillText('INTELLIGENCE ARTIFICIELLE',rx,H/2+86);

  // Progress
  const bx=180,by=H-90,bw=540,bh=7;
  ctx.fillStyle='#080e1c'; ctx.fillRect(bx,by,bw,bh);
  const bg=ctx.createLinearGradient(bx,0,bx+bw,0);
  bg.addColorStop(0,pf.glow); bg.addColorStop(1,pf.color);
  ctx.fillStyle=bg; ctx.fillRect(bx,by,bw*prog,bh);
  ctx.strokeStyle=pf.color+'33'; ctx.lineWidth=1; ctx.strokeRect(bx,by,bw,bh);
  const dots='.'.repeat(Math.floor(frame/20)%4);
  ctx.fillStyle='#2a3a55'; ctx.font='9px Courier New';
  ctx.fillText(`CHARGEMENT${dots}  ${Math.floor(prog*100)}%`,W/2,H-66);
}

const ROSTER_LIST=[
  {type:'hero',    lbl:'LEVIAPHENIX', role:'Unite mythique'},
  {type:'infantry',lbl:'AQUILORYON',  role:'Infanterie lourde'},
  {type:'ranged',  lbl:'AQUISTANCE',  role:'Tireur a distance'},
  {type:'mounted', lbl:'AQUILANCE',   role:'Cavalier des mers'},
];
function drawPlacement() {
  drawBg(); drawMap(true); drawEnemyPreview();
  const pf=FAC[playerFac];
  ctx.save(); ctx.shadowBlur=12; ctx.shadowColor=pf.glow; ctx.strokeStyle=pf.accent; ctx.lineWidth=1.5;
  for (const p of placed) {
    const gz=zoneOf(p.gy); const {x,y}=iso(p.gx,p.gy,gz+0.55);
    drawAquSprite(x,y,UDEFS[p.type].sz,p.type,pf.color,pf.accent);
  }
  ctx.restore();
  if (hovCell&&hovCell.gx<6) {
    const {x,y}=iso(hovCell.gx,hovCell.gy,zoneOf(hovCell.gy)); const hw=TW/2;
    ctx.strokeStyle='rgba(80,160,255,0.7)'; ctx.lineWidth=1.5;
    ctx.beginPath(); ctx.moveTo(x,y); ctx.lineTo(x+hw,y+TH/2); ctx.lineTo(x,y+TH); ctx.lineTo(x-hw,y+TH/2); ctx.closePath(); ctx.stroke();
  }
  // Right panel
  const px=716,py=36;
  ctx.fillStyle='rgba(2,5,16,0.95)'; ctx.fillRect(px,py,176,428);
  ctx.strokeStyle=pf.color+'33'; ctx.lineWidth=1; ctx.strokeRect(px,py,176,428);
  ctx.fillStyle=pf.color; ctx.font='bold 11px Courier New'; ctx.textAlign='center'; ctx.textBaseline='top';
  ctx.fillText('ROSTER',px+88,py+10);
  ctx.fillStyle='#2a3a55'; ctx.font='9px Courier New'; ctx.fillText(`Places : ${placed.length} / 6`,px+88,py+26);
  BTN.roster=[];
  ROSTER_LIST.forEach((item,i)=>{
    const iy=py+46+i*80; const isSel=placingType===item.type; const d=UDEFS[item.type];
    ctx.fillStyle=isSel?pf.color+'20':'rgba(4,8,22,0.9)'; ctx.fillRect(px+8,iy,160,70);
    ctx.strokeStyle=isSel?pf.color:'#12223a'; ctx.lineWidth=isSel?1.5:1; ctx.strokeRect(px+8,iy,160,70);
    // Mini sprite preview
    ctx.save(); ctx.shadowBlur=isSel?10:4; ctx.shadowColor=pf.glow; ctx.strokeStyle=pf.accent; ctx.lineWidth=1;
    drawAquSprite(px+32,iy+36,11,item.type,pf.color,pf.accent);
    ctx.restore();
    ctx.fillStyle=isSel?pf.color:'#3a5070'; ctx.font='bold 10px Courier New'; ctx.textAlign='left'; ctx.textBaseline='top';
    ctx.fillText(item.lbl,px+52,iy+8);
    ctx.fillStyle='#2a3a50'; ctx.font='8px Courier New';
    ctx.fillText(item.role,px+52,iy+24);
    ctx.fillText(`PV:${d.hp}  ATQ:${d.dmg}  PTee:${d.range}`,px+52,iy+38);
    ctx.fillStyle=isSel?pf.color+'88':'#1a2840'; ctx.font='8px Courier New';
    ctx.fillText('< clic pour selectionner',px+52,iy+52);
    BTN.roster.push({type:item.type,x:px+8,y:iy,w:160,h:70});
  });
  ctx.fillStyle='#1a2e50'; ctx.font='9px Courier New'; ctx.textAlign='center'; ctx.textBaseline='top';
  ctx.fillText('Sel. type puis clic case bleue',px+88,py+368);
  ctx.fillText('(zone gauche de la carte)',px+88,py+382);
  const ok=placed.length>0;
  BTN.launch=drawBtn('LANCER BATAILLE',px+8,py+400,160,40,ok?pf.color:'#1a2840',ok?'rgba(0,20,60,0.5)':'rgba(4,8,18,0.8)');
  // Title bar
  ctx.fillStyle='rgba(2,4,14,0.88)'; ctx.fillRect(0,0,W,30);
  ctx.fillStyle=pf.color; ctx.font='bold 11px Courier New'; ctx.textAlign='center'; ctx.textBaseline='middle';
  ctx.fillText('DEPLOIEMENT  -  Placez vos unites sur la zone bleue (gauche)',W/2-88,15);
}

function drawBattle() {
  drawBg(); drawMap(false);
  const sorted=units.filter(u=>!u.dead).sort((a,b)=>(a.gy+a.gz*0.1)-(b.gy+b.gz*0.1));
  drawParticles(); drawProjs();
  for (const u of sorted) u.draw();
  drawBattleHUD();
}

function drawEnd() {
  drawBg(); drawMap(false);
  units.filter(u=>!u.dead).forEach(u=>u.draw());
  drawParticles();
  ctx.fillStyle='rgba(0,2,10,0.9)'; ctx.fillRect(0,0,W,H);
  ctx.textAlign='center'; ctx.textBaseline='middle';
  ctx.fillStyle='#1a2840'; ctx.font='11px Courier New'; ctx.fillText('RESUME DE LA PARTIE',W/2,52);
  ctx.strokeStyle='#0d1828'; ctx.lineWidth=0.5;
  ctx.beginPath(); ctx.moveTo(W/2-180,64); ctx.lineTo(W/2+180,64); ctx.stroke();
  const col=victory?'#33ff66':'#ff3311';
  ctx.save(); ctx.shadowBlur=36; ctx.shadowColor=col;
  ctx.fillStyle=col; ctx.font='bold 54px Courier New'; ctx.fillText(victory?'VICTOIRE':'DEFAITE',W/2,134); ctx.restore();
  ctx.fillStyle=victory?'#1a4a28':'#3a1a0a'; ctx.font='13px Courier New';
  ctx.fillText(victory?"Aquiloris ecrase les Noxeens !":`Les forces d'Aquiloris capitulent...`,W/2,174);
  // Stats panel
  const px=248,py=196,pw=404,ph=220;
  ctx.fillStyle='rgba(4,8,24,0.96)'; ctx.fillRect(px,py,pw,ph);
  ctx.strokeStyle=col+'44'; ctx.lineWidth=1; ctx.strokeRect(px,py,pw,ph);
  ctx.strokeStyle=col+'18'; ctx.lineWidth=0.5;
  ctx.beginPath(); ctx.moveTo(px+20,py+26); ctx.lineTo(px+pw-20,py+26); ctx.stroke();
  ctx.fillStyle=col+'cc'; ctx.font='bold 10px Courier New'; ctx.fillText('STATISTIQUES DE COMBAT',W/2,py+16);
  const stats=[
    {l:'Faction joueur',     v:FAC[playerFac].name, c:FAC[playerFac].color},
    {l:'Faction ennemie',    v:FAC[enemyFac].name+' (IA)', c:FAC[enemyFac].color},
    {l:'Ennemis elimines',   v:String(battleKills), c:'#33ff66'},
    {l:'Unites perdues',     v:String(playerLosses), c:'#ff6633'},
    {l:'Score total',        v:String(score), c:col},
    {l:'Duree du combat',    v:fmtTime(frame), c:'#99aacc'},
  ];
  ctx.textAlign='left';
  stats.forEach((s,i)=>{
    const sy=py+46+i*30;
    ctx.fillStyle='#2a3a55'; ctx.font='10px Courier New'; ctx.fillText(s.l,px+24,sy);
    ctx.fillStyle=s.c; ctx.textAlign='right'; ctx.font=i===4?'bold 10px Courier New':'10px Courier New';
    ctx.fillText(s.v,px+pw-24,sy); ctx.textAlign='left';
    if (i<stats.length-1){ctx.strokeStyle='#0a1428';ctx.lineWidth=0.5;ctx.beginPath();ctx.moveTo(px+16,sy+18);ctx.lineTo(px+pw-16,sy+18);ctx.stroke();}
  });
  BTN.replay=drawBtn('REJOUER',W/2-240,H-82,210,44,'#4aa8ff','rgba(0,15,50,0.7)');
  BTN.menu  =drawBtn('MENU PRINCIPAL',W/2+30,H-82,210,44,'#446688','rgba(4,8,20,0.7)');
}

// ── Grid picking ──────────────────────────────────────────────
function pick(sx,sy) {
  for (let gz=2;gz>=0;gz--) {
    const dx=sx-camX, dy=sy-camY+gz*ZS;
    const gx=Math.floor((dx/( TW/2)+dy/(TH/2))/2);
    const gy=Math.floor((dy/(TH/2)-dx/(TW/2))/2);
    if (gx>=0&&gx<GW&&gy>=0&&gy<GH&&zoneOf(gy)===gz) return {gx,gy,gz};
  }
  return null;
}
function unitAt(sx,sy) {
  for (const u of units) {
    if (u.dead) continue;
    const p=u.sp;
    if (Math.hypot(sx-p.x,sy-p.y)<u.sz+10) return u;
  }
  return null;
}
function getMouse(e) {
  const r=C.getBoundingClientRect();
  return {mx:(e.clientX-r.left)*(W/r.width),my:(e.clientY-r.top)*(H/r.height)};
}

// ── Input ─────────────────────────────────────────────────────
C.addEventListener('click',e=>{
  const {mx,my}=getMouse(e);
  if (state==='MENU') { if (hit(mx,my,BTN.play)) state='FACTION'; }
  else if (state==='FACTION') {
    if (BTN.pick&&hit(mx,my,BTN.pick)) { playerFac='aquiloris';enemyFac='noxeens';placed=[];placingType=null;loadTimer=0;state='LOADING'; }
    if (BTN.back&&hit(mx,my,BTN.back)) state='MENU';
  }
  else if (state==='PLACEMENT') {
    if (BTN.roster) for (const rb of BTN.roster) { if (hit(mx,my,rb)){placingType=rb.type;return;} }
    if (hit(mx,my,BTN.launch)&&placed.length>0) { startBattle(); return; }
    if (placingType&&placed.length<6) {
      const cell=pick(mx,my);
      if (cell&&cell.gx<6&&!placed.find(p=>p.gx===cell.gx&&p.gy===cell.gy)) placed.push({type:placingType,gx:cell.gx,gy:cell.gy});
    }
  }
  else if (state==='BATTLE') { const u=unitAt(mx,my); selected=(u&&u.fac===playerFac)?u:null; }
  else if (state==='END') {
    if (hit(mx,my,BTN.replay)){placed=[];placingType=null;state='PLACEMENT';}
    if (hit(mx,my,BTN.menu)){placed=[];placingType=null;selected=null;state='MENU';}
  }
});
C.addEventListener('contextmenu',e=>{
  e.preventDefault();
  if (state!=='BATTLE'||!selected) return;
  const {mx,my}=getMouse(e);
  const foe=unitAt(mx,my);
  if (foe&&foe.fac===enemyFac){selected.target=foe;selected.dest=null;}
  else { const cell=pick(mx,my); if (cell) selected.dest={gx:cell.gx,gy:cell.gy}; }
});
C.addEventListener('mousemove',e=>{
  if (state!=='PLACEMENT'&&state!=='BATTLE') return;
  const {mx,my}=getMouse(e); hovCell=pick(mx,my);
});

// ── Loop ──────────────────────────────────────────────────────
function loop() {
  requestAnimationFrame(loop);
  frame++; ctx.clearRect(0,0,W,H);
  updateCamera();
  if      (state==='MENU')      drawMenu();
  else if (state==='FACTION')   drawFaction();
  else if (state==='LOADING')   { loadTimer++; drawLoading(); if (loadTimer>=LOAD_DUR) state='PLACEMENT'; }
  else if (state==='PLACEMENT') drawPlacement();
  else if (state==='BATTLE')    { runAI(); for (const u of units) u.update(); tickParticles(); tickProjs(); drawBattle(); checkEnd(); }
  else if (state==='END')       { tickParticles(); drawEnd(); }
}
initDots(); loop();
