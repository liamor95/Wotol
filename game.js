// ============================================================
// WOTOL – War of the Ocean's Legacy | Isometric Tactical Demo
// States: MENU → FACTION → LOADING → PLACEMENT → BATTLE → END
// ============================================================

const C = document.getElementById('c');
const ctx = C.getContext('2d');
const W = 900, H = 600;

// ── Isometric config ─────────────────────────────────────────
const TW = 64, TH = 32, ZS = 38;
const GW = 12, GH = 8;
const OX = 355, OY = 155;

function iso(gx, gy, gz = 0) {
  return {
    x: OX + (gx - gy) * TW / 2,
    y: OY + (gx + gy) * TH / 2 - gz * ZS,
  };
}

function zoneOf(gy) {
  if (gy <= 2) return 2; // SURFACE
  if (gy <= 5) return 1; // MID
  return 0;              // SOL
}

// ── All 5 factions (for selection screen) ────────────────────
const ALL_FACS = [
  { id: 'aquiloris',     name: 'Aquiloris',   color: '#3399ff', glow: '#1155cc', accent: '#99ccff', locked: false },
  { id: 'noxeens',      name: 'Noxeens',     color: '#00ee44', glow: '#005522', accent: '#88ffaa', locked: false, isEnemy: true },
  { id: 'thalassidras', name: 'Thalassidras', color: '#22ccbb', glow: '#0a5550', accent: '#88ffee', locked: true },
  { id: 'mureens',      name: 'Muréniens',   color: '#cc7722', glow: '#663311', accent: '#ffcc88', locked: true },
  { id: 'pirates',      name: 'Pirates',     color: '#cc2244', glow: '#661122', accent: '#ff8899', locked: true },
];

// ── Active factions ───────────────────────────────────────────
const FAC = {
  aquiloris: {
    name: 'Aquiloris', color: '#3399ff', glow: '#1155cc', accent: '#99ccff', darkBg: '#040c22',
    desc1: 'Technologie cristalline', desc2: 'Discipline militaire', res: 'Cristaux',
    lore: "Maitres des cristaux des profondeurs, les Aquiloris ont bati un empire de lumiere et d'ordre.",
  },
  noxeens: {
    name: 'Noxeens', color: '#00ee44', glow: '#005522', accent: '#88ffaa', darkBg: '#020d04',
    desc1: 'Creatures abyssales', desc2: 'Bioluminescence mortelle', res: 'Biolumens',
    lore: "Surgis des abysses, les Noxeens envahissent en vagues de tenebres bioluminescentes.",
  },
};

// ── Unit definitions ──────────────────────────────────────────
const UDEFS = {
  hero:     { hp: 280, dmg: 38, range: 2.2, spd: 0.032, sz: 20, lbl: 'Leviaphenix', sublbl: 'Unite mythique'    },
  infantry: { hp: 140, dmg: 20, range: 1.4, spd: 0.024, sz: 15, lbl: 'Aquiloryon',  sublbl: 'Infanterie lourde' },
  ranged:   { hp:  75, dmg: 30, range: 5.5, spd: 0.018, sz: 13, lbl: 'Aquistance',  sublbl: 'Tireur a distance' },
  mounted:  { hp: 115, dmg: 24, range: 1.7, spd: 0.044, sz: 17, lbl: 'Aquilance',   sublbl: 'Cavalier des mers' },
};

// ── State ─────────────────────────────────────────────────────
let state        = 'MENU';
let playerFac    = 'aquiloris';
let enemyFac     = 'noxeens';
let units        = [];
let particles    = [];
let projs        = [];
let selected     = null;
let hovCell      = null;
let placed       = [];
let placingType  = null;
let score        = 0;
let victory      = false;
let frame        = 0;
let bgDots       = [];
let aiTick       = 0;
let loadTimer    = 0;
let battleKills  = 0;
let playerLosses = 0;

const LOAD_DURATION = 180;

let BTN = {};

// ── Unit ─────────────────────────────────────────────────────
class Unit {
  constructor(fac, type, gx, gy) {
    const d = UDEFS[type];
    Object.assign(this, {
      fac, type,
      gx: +gx, gy: +gy, gz: zoneOf(gy),
      hp: d.hp, maxHp: d.hp,
      dmg: d.dmg, range: d.range, spd: d.spd, sz: d.sz,
      target: null, dest: null,
      cd: 0, flash: 0, dead: false,
    });
  }

  get f()  { return FAC[this.fac]; }
  get sp() {
    return {
      x: OX + (this.gx - this.gy) * TW / 2,
      y: OY + (this.gx + this.gy) * TH / 2 - (this.gz + 0.55) * ZS,
    };
  }

  dist(o) { return Math.hypot(o.gx - this.gx, o.gy - this.gy); }

  nearest(list) {
    let b = null, bd = Infinity;
    for (const u of list) { const d = this.dist(u); if (d < bd) { bd = d; b = u; } }
    return b;
  }

  update() {
    if (this.dead) return;
    if (this.cd > 0) this.cd--;
    if (this.flash > 0) this.flash--;

    const foes = units.filter(u => !u.dead && u.fac !== this.fac);
    if (!this.target || this.target.dead) this.target = this.nearest(foes);

    if (this.target) {
      const d = this.dist(this.target);
      if (d <= this.range) {
        if (this.cd === 0) { this.cd = 85; this.fire(this.target); }
      } else if (!this.dest && this.fac === enemyFac) {
        this.dest = { gx: this.target.gx, gy: this.target.gy };
      }
    }

    if (this.dest) {
      const dx = this.dest.gx - this.gx, dy = this.dest.gy - this.gy;
      const d = Math.hypot(dx, dy);
      if (d < 0.08) {
        this.gx = this.dest.gx; this.gy = this.dest.gy;
        this.gz = zoneOf(Math.round(this.gy));
        this.dest = null;
      } else {
        this.gx += dx / d * this.spd;
        this.gy += dy / d * this.spd;
        this.gz = zoneOf(Math.round(this.gy));
      }
    }
  }

  fire(t) {
    if (this.type === 'ranged') {
      const sp = this.sp, tp = t.sp;
      projs.push({ x: sp.x, y: sp.y, tx: tp.x, ty: tp.y, t, dmg: this.dmg, col: this.f.color, dead: false });
    } else {
      t.hurt(this.dmg);
      burst(t.sp.x, t.sp.y, this.f.color, 7);
    }
  }

  hurt(dmg) {
    this.hp -= dmg; this.flash = 12;
    if (this.hp <= 0) {
      this.dead = true;
      burst(this.sp.x, this.sp.y, this.f.color, 22);
      if (this.fac === enemyFac) { score += this.type === 'hero' ? 300 : 100; battleKills++; }
      if (this.fac === playerFac) playerLosses++;
    }
  }

  draw() {
    if (this.dead) return;
    const { x, y } = this.sp;
    const f = this.f;
    const sel = selected === this;
    const s = this.sz;

    ctx.save();
    ctx.shadowBlur = sel ? 28 : (this.flash > 0 ? 18 : 10);
    ctx.shadowColor = this.flash > 0 ? '#ff3300' : (sel ? '#fff' : f.glow);
    ctx.strokeStyle = f.accent;
    ctx.lineWidth = sel ? 2.5 : 1.5;

    ctx.globalAlpha = 0.25;
    ctx.fillStyle = '#000020';
    ctx.beginPath(); ctx.ellipse(x, y + s * 0.7, s * 0.8, s * 0.28, 0, 0, Math.PI * 2); ctx.fill();
    ctx.globalAlpha = 1;

    if      (this.type === 'hero')     drawStar(x, y, 5, s, s * 0.38, f.color);
    else if (this.type === 'infantry') drawPoly(x, y, 6, s, f.color);
    else if (this.type === 'ranged')   drawDiamond(x, y, s, f.color);
    else { ctx.fillStyle = f.color; ctx.beginPath(); ctx.arc(x, y, s, 0, Math.PI * 2); ctx.fill(); ctx.stroke(); }

    ctx.restore();

    const bw = s * 3, bh = 4, bx = x - bw / 2, by = y - s - 14;
    ctx.fillStyle = '#08081a'; ctx.fillRect(bx, by, bw, bh);
    const pct = Math.max(0, this.hp / this.maxHp);
    ctx.fillStyle = pct > 0.5 ? '#22ee55' : pct > 0.25 ? '#ffcc00' : '#ff3300';
    ctx.fillRect(bx, by, bw * pct, bh);

    if (sel) {
      ctx.save();
      ctx.strokeStyle = 'rgba(255,255,255,0.9)';
      ctx.lineWidth = 1.5;
      ctx.setLineDash([4, 3]);
      ctx.lineDashOffset = -frame * 0.1;
      ctx.beginPath(); ctx.arc(x, y, s + 9, 0, Math.PI * 2); ctx.stroke();
      ctx.restore();
    }

    ctx.fillStyle = '#778899';
    ctx.font = '8px Courier New';
    ctx.textAlign = 'center'; ctx.textBaseline = 'top';
    ctx.fillText('z' + this.gz, x, y + s + 3);
  }
}

// ── Particles ─────────────────────────────────────────────────
function burst(x, y, col, n) {
  for (let i = 0; i < n; i++) {
    const a = Math.random() * Math.PI * 2, sp = Math.random() * 4.5 + 1;
    particles.push({ x, y, vx: Math.cos(a) * sp, vy: Math.sin(a) * sp, life: 38, max: 38, col, r: Math.random() * 3 + 1 });
  }
}

function tickParticles() {
  for (const p of particles) {
    p.x += p.vx; p.y += p.vy; p.vx *= 0.88; p.vy *= 0.88; p.life--;
  }
  particles = particles.filter(p => p.life > 0);
}

function drawParticles() {
  ctx.save();
  for (const p of particles) {
    ctx.globalAlpha = (p.life / p.max) * 0.92;
    ctx.shadowBlur = 7; ctx.shadowColor = p.col;
    ctx.fillStyle = p.col;
    ctx.beginPath(); ctx.arc(p.x, p.y, p.r * (p.life / p.max), 0, Math.PI * 2); ctx.fill();
  }
  ctx.restore();
}

// ── Projectiles ───────────────────────────────────────────────
function tickProjs() {
  for (const p of projs) {
    if (p.dead) continue;
    const dx = p.tx - p.x, dy = p.ty - p.y, d = Math.hypot(dx, dy);
    if (d < 8) { p.t.hurt(p.dmg); burst(p.x, p.y, p.col, 7); p.dead = true; }
    else { p.x += dx / d * 7; p.y += dy / d * 7; }
  }
  projs = projs.filter(p => !p.dead);
}

function drawProjs() {
  ctx.save();
  for (const p of projs) {
    ctx.shadowBlur = 12; ctx.shadowColor = p.col;
    ctx.fillStyle = '#ffffff';
    ctx.beginPath(); ctx.arc(p.x, p.y, 3.5, 0, Math.PI * 2); ctx.fill();
  }
  ctx.restore();
}

// ── Background ────────────────────────────────────────────────
function initDots() {
  bgDots = [];
  for (let i = 0; i < 55; i++) {
    bgDots.push({ x: Math.random() * W, y: Math.random() * H, r: Math.random() * 2.2 + 0.4, vy: -(Math.random() * 0.28 + 0.1), vx: (Math.random() - 0.5) * 0.1, a: Math.random() * 0.28 + 0.06 });
  }
}

function drawBg(topCol) {
  const g = ctx.createLinearGradient(0, 0, 0, H);
  g.addColorStop(0, topCol || '#06101e');
  g.addColorStop(1, '#020810');
  ctx.fillStyle = g; ctx.fillRect(0, 0, W, H);

  ctx.save();
  for (let i = 0; i < 5; i++) {
    const cx = W / 5 * i + W / 10;
    const ww = 14 + Math.sin(frame * 0.007 + i) * 9;
    const g2 = ctx.createLinearGradient(0, 0, 0, H);
    g2.addColorStop(0, 'rgba(50,90,200,0.055)');
    g2.addColorStop(1, 'rgba(30,60,140,0)');
    ctx.fillStyle = g2;
    ctx.beginPath();
    ctx.moveTo(cx - ww, 0); ctx.lineTo(cx + ww, 0);
    ctx.lineTo(cx + ww * 3, H); ctx.lineTo(cx - ww * 3, H);
    ctx.closePath(); ctx.fill();
  }
  ctx.restore();

  ctx.save();
  for (const d of bgDots) {
    d.x += d.vx; d.y += d.vy;
    if (d.y < -4) { d.y = H + 4; d.x = Math.random() * W; }
    ctx.globalAlpha = d.a;
    ctx.strokeStyle = '#3377aa'; ctx.lineWidth = 0.7;
    ctx.beginPath(); ctx.arc(d.x, d.y, d.r, 0, Math.PI * 2); ctx.stroke();
  }
  ctx.restore();
}

// ── Isometric map ─────────────────────────────────────────────
function drawMap(highlightLeft) {
  for (let gy = 0; gy < GH; gy++) {
    for (let gx = 0; gx < GW; gx++) {
      const gz = zoneOf(gy);
      drawTile(gx, gy, gz, highlightLeft);
    }
  }
  drawZoneLabels();
}

const TILE_COLORS = [
  { top: '#050c1a', sL: '#030710', sR: '#040912' },
  { top: '#08162e', sL: '#04090e', sR: '#060e1e' },
  { top: '#0d2244', sL: '#071228', sR: '#0a1a38' },
];

function drawTile(gx, gy, gz, highlightLeft) {
  const { x, y } = iso(gx, gy, gz);
  const hw = TW / 2, hh = TH / 2;
  const tc = TILE_COLORS[gz];
  const hov = hovCell && hovCell.gx === gx && hovCell.gy === gy;
  const isLeft = gx < 6;

  if (gz > 0) {
    ctx.fillStyle = tc.sL;
    ctx.beginPath();
    ctx.moveTo(x - hw, y + hh); ctx.lineTo(x - hw, y + hh + gz * ZS);
    ctx.lineTo(x, y + TH + gz * ZS); ctx.lineTo(x, y + TH);
    ctx.closePath(); ctx.fill();

    ctx.fillStyle = tc.sR;
    ctx.beginPath();
    ctx.moveTo(x + hw, y + hh); ctx.lineTo(x + hw, y + hh + gz * ZS);
    ctx.lineTo(x, y + TH + gz * ZS); ctx.lineTo(x, y + TH);
    ctx.closePath(); ctx.fill();
  }

  let topCol = tc.top;
  if (hov) topCol = '#1a3c70';
  else if (highlightLeft && isLeft) topCol = gz === 2 ? '#0e2a55' : gz === 1 ? '#0a1e42' : '#080e28';

  ctx.fillStyle = topCol;
  ctx.beginPath();
  ctx.moveTo(x, y); ctx.lineTo(x + hw, y + hh);
  ctx.lineTo(x, y + TH); ctx.lineTo(x - hw, y + hh);
  ctx.closePath(); ctx.fill();

  ctx.strokeStyle = 'rgba(20,50,110,0.25)';
  ctx.lineWidth = 0.5;
  ctx.beginPath();
  ctx.moveTo(x, y); ctx.lineTo(x + hw, y + hh);
  ctx.lineTo(x, y + TH); ctx.lineTo(x - hw, y + hh);
  ctx.closePath(); ctx.stroke();

  if (highlightLeft && isLeft) {
    ctx.strokeStyle = 'rgba(60,120,255,0.35)';
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(x, y); ctx.lineTo(x + hw, y + hh);
    ctx.lineTo(x, y + TH); ctx.lineTo(x - hw, y + hh);
    ctx.closePath(); ctx.stroke();
  }

  if (gz === 0 && Math.sin(gx * 1.9 + gy * 2.7 + frame * 0.018) > 0.82) {
    ctx.fillStyle = `rgba(0,180,60,${0.22 + 0.12 * Math.sin(frame * 0.04)})`;
    ctx.beginPath(); ctx.arc(x + (gx % 3 - 1) * 7, y + hh, 1.8, 0, Math.PI * 2); ctx.fill();
  }

  if (gz === 2 && Math.sin(gx * 2.3 + gy * 1.6 + frame * 0.013) > 0.88) {
    ctx.fillStyle = `rgba(80,160,255,${0.38 + 0.22 * Math.sin(frame * 0.035)})`;
    ctx.beginPath(); ctx.arc(x - 4 + (gy % 3) * 4, y + 6, 1.5, 0, Math.PI * 2); ctx.fill();
  }
}

function drawZoneLabels() {
  const zones = [
    { gz: 2, label: 'EAU OUVERTE  -  15m', gy: 1 },
    { gz: 1, label: 'EAU PROFONDE  -  5m', gy: 4 },
    { gz: 0, label: 'FOND MARIN  -  0m',   gy: 6.5 },
  ];
  ctx.font = '9px Courier New'; ctx.textAlign = 'right'; ctx.textBaseline = 'middle';
  for (const z of zones) {
    const { x, y } = iso(-0.4, z.gy, z.gz + 0.9);
    ctx.fillStyle = 'rgba(40,100,180,0.5)';
    ctx.fillText(z.label, x - 2, y);
  }
}

// ── Enemy preview during placement ───────────────────────────
function drawEnemyPreview() {
  const types = ['hero', 'infantry', 'infantry', 'ranged', 'mounted', 'infantry'];
  const positions = [[10,1],[11,2],[10,4],[11,5],[9,3],[10,6]];
  const f = FAC[enemyFac];
  ctx.save();
  ctx.globalAlpha = 0.4;
  positions.forEach(([gx, gy], i) => {
    if (i >= types.length) return;
    const gz = zoneOf(gy);
    const { x, y } = iso(gx, gy, gz + 0.55);
    const s = UDEFS[types[i]].sz;
    ctx.shadowBlur = 8; ctx.shadowColor = f.glow;
    ctx.strokeStyle = f.accent; ctx.lineWidth = 1.5;
    if (types[i] === 'hero') drawStar(x, y, 5, s, s * 0.38, f.color);
    else if (types[i] === 'infantry') drawPoly(x, y, 6, s, f.color);
    else if (types[i] === 'ranged') drawDiamond(x, y, s, f.color);
    else { ctx.fillStyle = f.color; ctx.beginPath(); ctx.arc(x, y, s, 0, Math.PI * 2); ctx.fill(); ctx.stroke(); }
  });
  ctx.restore();
}

// ── AI ────────────────────────────────────────────────────────
function runAI() {
  aiTick++;
  if (aiTick % 140 !== 0) return;
  const enemies = units.filter(u => !u.dead && u.fac === enemyFac);
  const players = units.filter(u => !u.dead && u.fac === playerFac);
  if (!players.length) return;
  for (const e of enemies) {
    const t = players[Math.floor(Math.random() * players.length)];
    e.target = t;
    if (Math.random() < 0.35) {
      e.dest = { gx: Math.max(0, t.gx - 0.5 + Math.random()), gy: Math.max(0, Math.min(GH - 1, t.gy + (Math.random() - 0.5) * 2)) };
    }
  }
}

// ── Game start ────────────────────────────────────────────────
function startBattle() {
  units = []; particles = []; projs = [];
  selected = null; score = 0; frame = 0; aiTick = 0;
  battleKills = 0; playerLosses = 0;

  for (const p of placed) units.push(new Unit(playerFac, p.type, p.gx, p.gy));

  const eT = ['hero', 'infantry', 'infantry', 'ranged', 'mounted', 'infantry'];
  const eP = [[10,1],[11,2],[10,4],[11,5],[9,3],[10,6]];
  eT.forEach((t, i) => {
    const [gx, gy] = eP[i] || [10, 4];
    units.push(new Unit(enemyFac, t, gx, Math.min(gy, GH - 1)));
  });

  state = 'BATTLE';
}

function checkEnd() {
  const pA = units.filter(u => !u.dead && u.fac === playerFac).length;
  const eA = units.filter(u => !u.dead && u.fac === enemyFac).length;
  if (pA === 0) { victory = false; state = 'END'; }
  if (eA === 0) { victory = true;  state = 'END'; }
}

// ── Battle HUD ────────────────────────────────────────────────
function drawBattleHUD() {
  const pf = FAC[playerFac], ef = FAC[enemyFac];
  const pA = units.filter(u => !u.dead && u.fac === playerFac).length;
  const eA = units.filter(u => !u.dead && u.fac === enemyFac).length;

  ctx.fillStyle = 'rgba(2,4,14,0.88)'; ctx.fillRect(0, 0, W, 32);
  ctx.font = 'bold 12px Courier New'; ctx.textBaseline = 'middle';
  ctx.fillStyle = pf.color; ctx.textAlign = 'left';
  ctx.fillText(`> ${pf.name}  ${pA} unites`, 12, 16);
  ctx.fillStyle = '#fff'; ctx.textAlign = 'center';
  ctx.fillText(`SCORE  ${score}`, W / 2, 16);
  ctx.fillStyle = ef.color; ctx.textAlign = 'right';
  ctx.fillText(`${ef.name}  ${eA} unites <`, W - 12, 16);

  ctx.fillStyle = 'rgba(2,4,14,0.75)'; ctx.fillRect(0, H - 22, W, 22);
  ctx.fillStyle = '#2a3550'; ctx.font = '10px Courier New';
  ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
  ctx.fillText('CLIC G. : Selectionner    |    CLIC D. : Deplacer / Attaquer', W / 2, H - 11);

  // SURFACE / MID / SOL vertical indicator
  const selZ = selected ? selected.gz : -1;
  const levelPanel = [
    { gz: 2, lbl: 'SURFACE', y: 90  },
    { gz: 1, lbl: 'MID',     y: 150 },
    { gz: 0, lbl: 'SOL',     y: 210 },
  ];
  ctx.fillStyle = 'rgba(2,4,14,0.85)'; ctx.fillRect(6, 80, 64, 148);
  ctx.strokeStyle = '#1a2840'; ctx.lineWidth = 1; ctx.strokeRect(6, 80, 64, 148);
  for (const lv of levelPanel) {
    const active = lv.gz === selZ;
    ctx.fillStyle = active ? pf.color + '33' : 'rgba(0,0,0,0)';
    ctx.fillRect(8, lv.y - 14, 60, 28);
    if (active) {
      ctx.strokeStyle = pf.color; ctx.lineWidth = 1.5;
      ctx.strokeRect(8, lv.y - 14, 60, 28);
      ctx.fillStyle = '#ffffff'; ctx.shadowBlur = 8; ctx.shadowColor = pf.color;
    } else {
      ctx.fillStyle = '#2a3a55'; ctx.shadowBlur = 0;
    }
    ctx.font = active ? 'bold 10px Courier New' : '9px Courier New';
    ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
    ctx.fillText(lv.lbl, 38, lv.y);
    ctx.shadowBlur = 0;
    if (lv.gz > 0) {
      ctx.strokeStyle = '#1a2840'; ctx.lineWidth = 1;
      ctx.beginPath(); ctx.moveTo(38, lv.y + 14); ctx.lineTo(38, lv.y + 28); ctx.stroke();
    }
  }

  if (selected) {
    const px = 10, py = H - 112, pw = 205, ph = 86;
    ctx.fillStyle = 'rgba(2,6,20,0.92)'; ctx.fillRect(px, py, pw, ph);
    ctx.strokeStyle = pf.color + '44'; ctx.lineWidth = 1; ctx.strokeRect(px, py, pw, ph);
    ctx.fillStyle = pf.color; ctx.font = 'bold 11px Courier New';
    ctx.textAlign = 'left'; ctx.textBaseline = 'top';
    ctx.fillText(`${UDEFS[selected.type].lbl.toUpperCase()}  -  Z${selected.gz}`, px + 8, py + 8);
    ctx.fillStyle = '#5a6880'; ctx.font = '10px Courier New';
    ctx.fillText(`PV: ${Math.max(0, Math.ceil(selected.hp))} / ${selected.maxHp}`, px + 8, py + 26);
    ctx.fillText(`ATQ: ${selected.dmg}   Portee: ${selected.range}   Vit: ${(selected.spd * 100).toFixed(0)}`, px + 8, py + 42);
    const zoneName = selected.gz === 2 ? 'Eau ouverte' : selected.gz === 1 ? 'Eau profonde' : 'Fond marin';
    ctx.fillText(`Niveau: ${zoneName}`, px + 8, py + 58);
    ctx.fillText(`Faction: ${pf.name}`, px + 8, py + 72);
  }
}

// ── Draw helpers ──────────────────────────────────────────────
function drawPoly(x, y, n, r, col) {
  ctx.fillStyle = col; ctx.beginPath();
  for (let i = 0; i < n; i++) {
    const a = Math.PI * 2 / n * i - Math.PI / 2;
    i === 0 ? ctx.moveTo(x + r * Math.cos(a), y + r * Math.sin(a))
            : ctx.lineTo(x + r * Math.cos(a), y + r * Math.sin(a));
  }
  ctx.closePath(); ctx.fill(); ctx.stroke();
}

function drawDiamond(x, y, r, col) {
  ctx.fillStyle = col; ctx.beginPath();
  ctx.moveTo(x, y - r); ctx.lineTo(x + r, y); ctx.lineTo(x, y + r); ctx.lineTo(x - r, y);
  ctx.closePath(); ctx.fill(); ctx.stroke();
}

function drawStar(x, y, pts, ro, ri, col) {
  ctx.fillStyle = col; ctx.beginPath();
  for (let i = 0; i < pts * 2; i++) {
    const a = Math.PI / pts * i - Math.PI / 2;
    const r = i % 2 === 0 ? ro : ri;
    i === 0 ? ctx.moveTo(x + r * Math.cos(a), y + r * Math.sin(a))
            : ctx.lineTo(x + r * Math.cos(a), y + r * Math.sin(a));
  }
  ctx.closePath(); ctx.fill(); ctx.stroke();
}

function drawBtn(label, x, y, w, h, col, fill) {
  ctx.fillStyle = fill || 'rgba(0,0,0,0)'; ctx.fillRect(x, y, w, h);
  ctx.strokeStyle = col; ctx.lineWidth = 1; ctx.strokeRect(x, y, w, h);
  ctx.fillStyle = col; ctx.font = 'bold 13px Courier New';
  ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
  ctx.fillText(label, x + w / 2, y + h / 2);
  return { x, y, w, h };
}

function hit(mx, my, b) { return b && mx >= b.x && mx <= b.x + b.w && my >= b.y && my <= b.y + b.h; }

function fmtTime(frames) {
  const sec = Math.floor(frames / 60);
  const m = Math.floor(sec / 60);
  const s = sec % 60;
  return `${m}m ${s < 10 ? '0' : ''}${s}s`;
}

// ── Screen: MENU ──────────────────────────────────────────────
function drawMenu() {
  drawBg();
  ctx.textAlign = 'center';

  // Animated shimmer at top
  const shimmer = 0.05 + 0.03 * Math.sin(frame * 0.02);
  ctx.fillStyle = `rgba(20,60,180,${shimmer})`;
  ctx.fillRect(0, 0, W, H * 0.35);

  // Eyebrow
  ctx.fillStyle = '#122244';
  ctx.font = '11px Courier New';
  ctx.fillText('WAR  OF  THE  OCEAN\'S  LEGACY', W / 2, 115);

  // Decorative lines around title
  ctx.strokeStyle = '#0d1e42'; ctx.lineWidth = 0.5;
  ctx.beginPath(); ctx.moveTo(W/2 - 200, 128); ctx.lineTo(W/2 - 30, 128); ctx.stroke();
  ctx.beginPath(); ctx.moveTo(W/2 + 30, 128); ctx.lineTo(W/2 + 200, 128); ctx.stroke();

  // Animated logo
  ctx.save();
  const pulse = 28 + 10 * Math.sin(frame * 0.025);
  ctx.shadowBlur = pulse; ctx.shadowColor = '#1155cc';
  ctx.fillStyle = '#3399ff';
  ctx.font = 'bold 92px Courier New';
  ctx.fillText('WOTOL', W / 2, 228);
  ctx.shadowBlur = pulse * 0.4; ctx.shadowColor = '#99ccff';
  ctx.globalAlpha = 0.25;
  ctx.fillText('WOTOL', W / 2, 228);
  ctx.restore();

  // Tagline
  ctx.fillStyle = '#0d1e42';
  ctx.font = '10px Courier New';
  ctx.fillText('DEMO TACTIQUE  -  PRE-ALPHA', W / 2, 256);

  // Separator
  ctx.strokeStyle = '#0a1830'; ctx.lineWidth = 0.5;
  ctx.beginPath(); ctx.moveTo(W/2 - 160, 272); ctx.lineTo(W/2 + 160, 272); ctx.stroke();

  // Lore text
  ctx.fillStyle = '#1a2e4a';
  ctx.font = '12px Courier New';
  ctx.fillText('Les peuples des oceans vivent dans une paix fragile.', W / 2, 308);
  ctx.fillText('Un ancien artefact refait surface. Une nouvelle guerre eclate.', W / 2, 330);
  ctx.fillStyle = '#0f1e36';
  ctx.fillText('Derriere cette guerre se cache une verite bien plus ancienne.', W / 2, 352);

  // Play button
  BTN.play = drawBtn('NOUVELLE BATAILLE', W / 2 - 130, 406, 260, 50, '#3399ff', 'rgba(0,20,60,0.55)');

  // Copyright
  ctx.fillStyle = '#080e1c';
  ctx.font = '9px Courier New';
  ctx.fillText('c WOTOL PROJECT  -  DEMO v0.1  -  5 FACTIONS  -  TACTICAL RTS', W / 2, H - 14);
}

// ── Screen: FACTION (5 factions) ─────────────────────────────
function drawFaction() {
  drawBg();
  ctx.textAlign = 'center';

  ctx.fillStyle = '#3399ff';
  ctx.font = 'bold 22px Courier New';
  ctx.fillText('CHOISIR VOTRE FACTION', W / 2, 46);
  ctx.fillStyle = '#122244';
  ctx.font = '10px Courier New';
  ctx.fillText('DEMO : Aquiloris contre les Noxeens  |  3 factions a venir', W / 2, 66);

  // 5 faction cards
  const cardW = 156, cardH = 234, startX = 38, startY = 84, gap = 7;
  BTN.pick = null;
  BTN.back = null;

  ALL_FACS.forEach((fac, i) => {
    const cx = startX + i * (cardW + gap);
    const isLocked = fac.locked;
    const isEnemy = fac.isEnemy;
    const isPlayer = !isLocked && !isEnemy;

    // Card background
    ctx.globalAlpha = isLocked ? 0.35 : 1;
    ctx.fillStyle = isPlayer ? '#060e24' : (isEnemy ? '#040e06' : '#050810');
    ctx.fillRect(cx, startY, cardW, cardH);
    ctx.strokeStyle = isPlayer ? fac.color + '88' : (isEnemy ? fac.color + '33' : '#080e18');
    ctx.lineWidth = isPlayer ? 1.5 : 1;
    ctx.strokeRect(cx, startY, cardW, cardH);
    ctx.globalAlpha = 1;

    // Faction name
    ctx.fillStyle = isLocked ? '#1a2030' : fac.color;
    ctx.font = `bold 10px Courier New`;
    ctx.fillText(fac.name.toUpperCase(), cx + cardW / 2, startY + 20);

    // Emblem
    ctx.save();
    ctx.globalAlpha = isLocked ? 0.18 : (isEnemy ? 0.55 : 1);
    ctx.shadowBlur = isLocked ? 0 : 18;
    ctx.shadowColor = fac.glow;
    ctx.strokeStyle = fac.color; ctx.lineWidth = 1.5;
    const ex = cx + cardW / 2, ey = startY + 118;
    if (i === 0)      drawStar(ex, ey, 5, 36, 14, fac.color);
    else if (i === 1) drawPoly(ex, ey, 6, 36, fac.color);
    else if (i === 2) drawDiamond(ex, ey, 36, fac.color);
    else if (i === 3) { ctx.fillStyle = fac.color; ctx.beginPath(); ctx.arc(ex, ey, 32, 0, Math.PI * 2); ctx.fill(); ctx.stroke(); }
    else              drawPoly(ex, ey, 3, 36, fac.color);
    ctx.restore();

    // Status badge inside card
    const bx = cx + 8, by = startY + 188, bw = cardW - 16, bh = 26;
    if (isLocked) {
      ctx.fillStyle = '#08101c'; ctx.fillRect(bx, by, bw, bh);
      ctx.strokeStyle = '#0a1428'; ctx.lineWidth = 1; ctx.strokeRect(bx, by, bw, bh);
      ctx.fillStyle = '#1e2a3a'; ctx.font = 'bold 9px Courier New';
      ctx.fillText('[  BIENTOT  ]', cx + cardW / 2, by + 13);
    } else if (isEnemy) {
      ctx.fillStyle = '#02080a'; ctx.fillRect(bx, by, bw, bh);
      ctx.strokeStyle = fac.color + '22'; ctx.lineWidth = 1; ctx.strokeRect(bx, by, bw, bh);
      ctx.fillStyle = fac.color + '77'; ctx.font = 'bold 9px Courier New';
      ctx.fillText('ENNEMI  (IA)', cx + cardW / 2, by + 13);
    } else {
      ctx.fillStyle = fac.color + '18'; ctx.fillRect(bx, by, bw, bh);
      ctx.strokeStyle = fac.color; ctx.lineWidth = 1; ctx.strokeRect(bx, by, bw, bh);
      ctx.fillStyle = fac.color; ctx.font = 'bold 9px Courier New';
      ctx.fillText('> CHOISIR', cx + cardW / 2, by + 13);
      BTN.pick = { x: bx, y: by, w: bw, h: bh };
    }
  });

  // Info panel
  const pf = FAC.aquiloris, ef = FAC.noxeens;
  const infoPy = 332;
  ctx.fillStyle = 'rgba(4,8,24,0.94)'; ctx.fillRect(38, infoPy, 824, 162);
  ctx.strokeStyle = pf.color + '22'; ctx.lineWidth = 1; ctx.strokeRect(38, infoPy, 824, 162);

  ctx.save();
  ctx.shadowBlur = 12; ctx.shadowColor = pf.glow;
  ctx.fillStyle = pf.color; ctx.font = 'bold 15px Courier New'; ctx.textAlign = 'left';
  ctx.fillText('AQUILORIS', 60, infoPy + 26);
  ctx.restore();
  ctx.fillStyle = '#3a5070'; ctx.font = '10px Courier New'; ctx.textAlign = 'left';
  ctx.fillText(pf.lore, 60, infoPy + 48);
  ctx.fillStyle = '#2a3a55';
  ctx.fillText(`Specialite: ${pf.desc1}  -  ${pf.desc2}`, 60, infoPy + 68);
  ctx.fillText(`Ressource strategique: ${pf.res}`, 60, infoPy + 86);

  // VS divider
  ctx.fillStyle = '#ff220033'; ctx.fillRect(W/2 - 1, infoPy + 14, 2, 130);
  ctx.fillStyle = '#cc3322'; ctx.font = 'bold 14px Courier New'; ctx.textAlign = 'center';
  ctx.fillText('VS', W/2, infoPy + 85);

  // Enemy info
  ctx.save();
  ctx.shadowBlur = 10; ctx.shadowColor = ef.glow;
  ctx.fillStyle = ef.color; ctx.font = 'bold 15px Courier New'; ctx.textAlign = 'right';
  ctx.fillText('NOXEENS  (IA)', W - 60, infoPy + 26);
  ctx.restore();
  ctx.fillStyle = '#1a3020'; ctx.font = '10px Courier New'; ctx.textAlign = 'right';
  ctx.fillText(ef.lore, W - 60, infoPy + 48);
  ctx.fillStyle = '#1a2a1a';
  ctx.fillText(`${ef.desc1}  -  ${ef.desc2}`, W - 60, infoPy + 68);
  ctx.fillText(`Ressource: ${ef.res}`, W - 60, infoPy + 86);

  // Back button
  BTN.back = drawBtn('< RETOUR', 38, infoPy + 130, 110, 24, '#446688', 'rgba(4,8,20,0.8)');
}

// ── Screen: LOADING ───────────────────────────────────────────
function drawLoading() {
  drawBg();
  const pf = FAC[playerFac];
  const ef = FAC[enemyFac];
  const progress = Math.min(1, loadTimer / LOAD_DURATION);

  // Title
  ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
  ctx.fillStyle = '#1a2840';
  ctx.font = '11px Courier New';
  ctx.fillText('PREPARATION AU COMBAT', W / 2, 72);

  // Player emblem (left)
  const lx = W / 2 - 170;
  ctx.save();
  const p1 = 24 + 10 * Math.sin(frame * 0.04);
  ctx.shadowBlur = p1; ctx.shadowColor = pf.glow;
  ctx.strokeStyle = pf.accent; ctx.lineWidth = 2;
  drawStar(lx, H / 2 - 20, 5, 52, 20, pf.color);
  ctx.restore();

  ctx.save();
  ctx.shadowBlur = 12; ctx.shadowColor = pf.glow;
  ctx.fillStyle = pf.color; ctx.font = 'bold 17px Courier New';
  ctx.fillText(pf.name.toUpperCase(), lx, H / 2 + 62);
  ctx.restore();
  ctx.fillStyle = '#2a3a55'; ctx.font = '9px Courier New';
  ctx.fillText('JOUEUR', lx, H / 2 + 82);

  // VS
  ctx.save();
  ctx.shadowBlur = 20; ctx.shadowColor = '#cc2200';
  ctx.fillStyle = '#ff2200'; ctx.font = 'bold 38px Courier New';
  ctx.fillText('VS', W / 2, H / 2 - 20);
  ctx.restore();

  // Enemy emblem (right)
  const rx = W / 2 + 170;
  ctx.save();
  ctx.globalAlpha = 0.6;
  ctx.shadowBlur = 16; ctx.shadowColor = ef.glow;
  ctx.strokeStyle = ef.accent; ctx.lineWidth = 2;
  drawPoly(rx, H / 2 - 20, 6, 52, ef.color);
  ctx.restore();

  ctx.fillStyle = ef.color + 'aa'; ctx.font = 'bold 17px Courier New';
  ctx.fillText(ef.name.toUpperCase(), rx, H / 2 + 62);
  ctx.fillStyle = '#1a2a1a'; ctx.font = '9px Courier New';
  ctx.fillText('INTELLIGENCE ARTIFICIELLE', rx, H / 2 + 82);

  // Progress bar
  const bx = 180, by = H - 88, bw = 540, bh = 7;
  ctx.fillStyle = '#080e1c'; ctx.fillRect(bx, by, bw, bh);
  const bfill = bw * progress;
  const bg = ctx.createLinearGradient(bx, 0, bx + bw, 0);
  bg.addColorStop(0, pf.glow);
  bg.addColorStop(1, pf.color);
  ctx.fillStyle = bg; ctx.fillRect(bx, by, bfill, bh);
  ctx.strokeStyle = pf.color + '33'; ctx.lineWidth = 1; ctx.strokeRect(bx, by, bw, bh);

  // Loading text
  const dots = '.'.repeat(Math.floor(frame / 20) % 4);
  ctx.fillStyle = '#2a3a55'; ctx.font = '9px Courier New';
  ctx.fillText(`CHARGEMENT${dots}  ${Math.floor(progress * 100)}%`, W / 2, H - 64);
}

// ── Screen: PLACEMENT ─────────────────────────────────────────
const ROSTER_LIST = [
  { type: 'hero',     icon: '*', label: 'LEVIAPHENIX' },
  { type: 'infantry', icon: '#', label: 'AQUILORYON'  },
  { type: 'ranged',   icon: '<', label: 'AQUISTANCE'  },
  { type: 'mounted',  icon: 'o', label: 'AQUILANCE'   },
];

function drawPlacement() {
  drawBg();
  drawMap(true);
  drawEnemyPreview();

  const pf = FAC[playerFac];
  ctx.save();
  ctx.shadowBlur = 12; ctx.shadowColor = pf.glow;
  ctx.strokeStyle = pf.accent; ctx.lineWidth = 1.5;
  for (const p of placed) {
    const gz = zoneOf(p.gy);
    const { x, y } = iso(p.gx, p.gy, gz + 0.55);
    const s = UDEFS[p.type].sz;
    if (p.type === 'hero') drawStar(x, y, 5, s, s * 0.38, pf.color);
    else if (p.type === 'infantry') drawPoly(x, y, 6, s, pf.color);
    else if (p.type === 'ranged') drawDiamond(x, y, s, pf.color);
    else { ctx.fillStyle = pf.color; ctx.beginPath(); ctx.arc(x, y, s, 0, Math.PI * 2); ctx.fill(); ctx.stroke(); }
  }
  ctx.restore();

  if (hovCell && hovCell.gx < 6) {
    const gz = zoneOf(hovCell.gy);
    const { x, y } = iso(hovCell.gx, hovCell.gy, gz);
    const hw = TW / 2;
    ctx.strokeStyle = 'rgba(80,160,255,0.7)'; ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.moveTo(x, y); ctx.lineTo(x + hw, y + TH / 2); ctx.lineTo(x, y + TH); ctx.lineTo(x - hw, y + TH / 2);
    ctx.closePath(); ctx.stroke();
  }

  // Right panel
  const px = 720, py = 38;
  ctx.fillStyle = 'rgba(2,5,16,0.94)'; ctx.fillRect(px, py, 172, 420);
  ctx.strokeStyle = pf.color + '33'; ctx.lineWidth = 1; ctx.strokeRect(px, py, 172, 420);

  ctx.fillStyle = pf.color; ctx.font = 'bold 11px Courier New';
  ctx.textAlign = 'center'; ctx.textBaseline = 'top';
  ctx.fillText('ROSTER', px + 86, py + 10);
  ctx.fillStyle = '#2a3a55'; ctx.font = '9px Courier New';
  ctx.fillText(`Places: ${placed.length} / 6`, px + 86, py + 26);

  BTN.roster = [];
  ROSTER_LIST.forEach((item, i) => {
    const iy = py + 48 + i * 72;
    const isSel = placingType === item.type;
    const d = UDEFS[item.type];
    ctx.fillStyle = isSel ? pf.color + '22' : 'rgba(4,8,22,0.9)'; ctx.fillRect(px + 8, iy, 156, 62);
    ctx.strokeStyle = isSel ? pf.color : '#12223a'; ctx.lineWidth = isSel ? 1.5 : 1;
    ctx.strokeRect(px + 8, iy, 156, 62);
    ctx.fillStyle = isSel ? pf.color : '#3a5070';
    ctx.font = 'bold 11px Courier New'; ctx.textAlign = 'left'; ctx.textBaseline = 'top';
    ctx.fillText(`${item.icon} ${item.label}`, px + 16, iy + 8);
    ctx.fillStyle = '#2a3a50'; ctx.font = '9px Courier New';
    ctx.fillText(`PV:${d.hp}  ATQ:${d.dmg}  PTee:${d.range}`, px + 16, iy + 26);
    ctx.fillText('< Clic pour selectionner', px + 16, iy + 42);
    BTN.roster.push({ type: item.type, x: px + 8, y: iy, w: 156, h: 62 });
  });

  ctx.fillStyle = '#1a2e50'; ctx.font = '9px Courier New'; ctx.textAlign = 'center'; ctx.textBaseline = 'top';
  ctx.fillText('Selectionnez un type puis', px + 86, py + 342);
  ctx.fillText('cliquez sur les cases bleues', px + 86, py + 356);
  ctx.fillText('(moitie gauche de la carte)', px + 86, py + 370);

  const canLaunch = placed.length > 0;
  BTN.launch = drawBtn('LANCER BATAILLE', px + 8, py + 392, 156, 38, canLaunch ? pf.color : '#1a2840');

  ctx.fillStyle = 'rgba(2,4,14,0.85)'; ctx.fillRect(0, 0, W, 30);
  ctx.fillStyle = pf.color; ctx.font = 'bold 12px Courier New'; ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
  ctx.fillText('DEPLOIEMENT  -  Placez vos unites sur la zone bleue (gauche)', W / 2 - 90, 15);
}

// ── Screen: BATTLE ────────────────────────────────────────────
function drawBattle() {
  drawBg();
  drawMap(false);

  const sorted = units.filter(u => !u.dead).sort((a, b) => (a.gy + a.gz * 0.1) - (b.gy + b.gz * 0.1));
  drawParticles();
  drawProjs();
  for (const u of sorted) u.draw();
  drawBattleHUD();
}

// ── Screen: END (Resume de la partie) ────────────────────────
function drawEnd() {
  drawBg();
  drawMap(false);
  units.filter(u => !u.dead).forEach(u => u.draw());
  drawParticles();

  ctx.fillStyle = 'rgba(0,2,10,0.90)'; ctx.fillRect(0, 0, W, H);
  ctx.textAlign = 'center'; ctx.textBaseline = 'middle';

  // Header
  ctx.fillStyle = '#1a2840';
  ctx.font = '11px Courier New';
  ctx.fillText('RESUME DE LA PARTIE', W / 2, 52);
  ctx.strokeStyle = '#0d1828'; ctx.lineWidth = 0.5;
  ctx.beginPath(); ctx.moveTo(W/2 - 180, 64); ctx.lineTo(W/2 + 180, 64); ctx.stroke();

  // Victory / Defeat title
  const col = victory ? '#33ff66' : '#ff3311';
  ctx.save();
  ctx.shadowBlur = 36; ctx.shadowColor = col;
  ctx.fillStyle = col; ctx.font = 'bold 54px Courier New';
  ctx.fillText(victory ? 'VICTOIRE' : 'DEFAITE', W / 2, 132);
  ctx.restore();

  ctx.fillStyle = victory ? '#1a4a28' : '#3a1a0a';
  ctx.font = '13px Courier New';
  ctx.fillText(
    victory ? 'Aquiloris ecrase les Noxeens !' : 'Les forces d\'Aquiloris capitulent...',
    W / 2, 172
  );

  // Stats panel
  const px = 248, py = 196, pw = 404, ph = 220;
  ctx.fillStyle = 'rgba(4,8,24,0.96)'; ctx.fillRect(px, py, pw, ph);
  ctx.strokeStyle = col + '44'; ctx.lineWidth = 1; ctx.strokeRect(px, py, pw, ph);
  ctx.strokeStyle = col + '18'; ctx.lineWidth = 0.5;
  ctx.beginPath(); ctx.moveTo(px + 20, py + 26); ctx.lineTo(px + pw - 20, py + 26); ctx.stroke();

  ctx.fillStyle = col + 'cc'; ctx.font = 'bold 10px Courier New';
  ctx.fillText('STATISTIQUES DE COMBAT', W / 2, py + 16);

  const stats = [
    { label: 'Faction joueur',      value: FAC[playerFac].name, color: FAC[playerFac].color },
    { label: 'Faction ennemie',     value: FAC[enemyFac].name + ' (IA)', color: FAC[enemyFac].color },
    { label: 'Ennemis elimines',    value: String(battleKills), color: '#33ff66' },
    { label: 'Unites perdues',      value: String(playerLosses), color: '#ff6633' },
    { label: 'Score total',         value: String(score), color: col },
    { label: 'Duree du combat',     value: fmtTime(frame), color: '#99aacc' },
  ];

  ctx.textAlign = 'left';
  stats.forEach((s, i) => {
    const sy = py + 46 + i * 30;
    const isLast = i === stats.length - 1;
    ctx.fillStyle = '#2a3a55'; ctx.font = '10px Courier New';
    ctx.fillText(s.label, px + 24, sy);
    ctx.fillStyle = s.color; ctx.textAlign = 'right';
    ctx.font = i === 4 ? 'bold 10px Courier New' : '10px Courier New';
    ctx.fillText(s.value, px + pw - 24, sy);
    ctx.textAlign = 'left';
    if (!isLast) {
      ctx.strokeStyle = '#0a1428'; ctx.lineWidth = 0.5;
      ctx.beginPath(); ctx.moveTo(px + 16, sy + 18); ctx.lineTo(px + pw - 16, sy + 18); ctx.stroke();
    }
  });

  // Buttons
  BTN.replay = drawBtn('REJOUER', W / 2 - 238, H - 80, 208, 44, '#3399ff', 'rgba(0,15,50,0.7)');
  BTN.menu   = drawBtn('MENU PRINCIPAL', W / 2 + 30, H - 80, 208, 44, '#446688', 'rgba(4,8,20,0.7)');
}

// ── Grid picking ──────────────────────────────────────────────
function pick(sx, sy) {
  for (let gz = 2; gz >= 0; gz--) {
    const dx = sx - OX, dy = sy - OY + gz * ZS;
    const hw = TW / 2, hh = TH / 2;
    const gx = Math.floor((dx / hw + dy / hh) / 2);
    const gy = Math.floor((dy / hh - dx / hw) / 2);
    if (gx >= 0 && gx < GW && gy >= 0 && gy < GH && zoneOf(gy) === gz) return { gx, gy, gz };
  }
  return null;
}

function unitAt(sx, sy) {
  for (const u of units) {
    if (u.dead) continue;
    const p = u.sp;
    if (Math.hypot(sx - p.x, sy - p.y) < u.sz + 8) return u;
  }
  return null;
}

function getMouse(e) {
  const r = C.getBoundingClientRect();
  return {
    mx: (e.clientX - r.left) * (W / r.width),
    my: (e.clientY - r.top) * (H / r.height),
  };
}

// ── Input ─────────────────────────────────────────────────────
C.addEventListener('click', e => {
  const { mx, my } = getMouse(e);

  if (state === 'MENU') {
    if (hit(mx, my, BTN.play)) state = 'FACTION';
  }
  else if (state === 'FACTION') {
    if (BTN.pick && hit(mx, my, BTN.pick)) {
      playerFac = 'aquiloris'; enemyFac = 'noxeens';
      placed = []; placingType = null;
      loadTimer = 0;
      state = 'LOADING';
    }
    if (BTN.back && hit(mx, my, BTN.back)) state = 'MENU';
  }
  else if (state === 'PLACEMENT') {
    if (BTN.roster) {
      for (const rb of BTN.roster) { if (hit(mx, my, rb)) { placingType = rb.type; return; } }
    }
    if (hit(mx, my, BTN.launch) && placed.length > 0) { startBattle(); return; }
    if (placingType && placed.length < 6) {
      const cell = pick(mx, my);
      if (cell && cell.gx < 6) {
        if (!placed.find(p => p.gx === cell.gx && p.gy === cell.gy)) {
          placed.push({ type: placingType, gx: cell.gx, gy: cell.gy });
        }
      }
    }
  }
  else if (state === 'BATTLE') {
    const u = unitAt(mx, my);
    selected = (u && u.fac === playerFac) ? u : null;
  }
  else if (state === 'END') {
    if (hit(mx, my, BTN.replay)) { placed = []; placingType = null; state = 'PLACEMENT'; }
    if (hit(mx, my, BTN.menu))   { placed = []; placingType = null; selected = null; state = 'MENU'; }
  }
});

C.addEventListener('contextmenu', e => {
  e.preventDefault();
  if (state !== 'BATTLE' || !selected) return;
  const { mx, my } = getMouse(e);
  const foe = unitAt(mx, my);
  if (foe && foe.fac === enemyFac) { selected.target = foe; selected.dest = null; }
  else {
    const cell = pick(mx, my);
    if (cell) selected.dest = { gx: cell.gx, gy: cell.gy };
  }
});

C.addEventListener('mousemove', e => {
  if (state !== 'PLACEMENT' && state !== 'BATTLE') return;
  const { mx, my } = getMouse(e);
  hovCell = pick(mx, my);
});

// ── Main loop ─────────────────────────────────────────────────
function loop() {
  requestAnimationFrame(loop);
  frame++;
  ctx.clearRect(0, 0, W, H);

  if      (state === 'MENU')      drawMenu();
  else if (state === 'FACTION')   drawFaction();
  else if (state === 'LOADING') {
    loadTimer++;
    drawLoading();
    if (loadTimer >= LOAD_DURATION) state = 'PLACEMENT';
  }
  else if (state === 'PLACEMENT') drawPlacement();
  else if (state === 'BATTLE') {
    runAI();
    for (const u of units) u.update();
    tickParticles(); tickProjs();
    drawBattle();
    checkEnd();
  }
  else if (state === 'END') { tickParticles(); drawEnd(); }
}

// ── Boot ──────────────────────────────────────────────────────
initDots();
loop();
