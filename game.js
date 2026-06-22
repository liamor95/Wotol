// ============================================================
// WOTOL – War of the Ocean's Legacy | Tactical Demo
// ============================================================

const canvas = document.getElementById('c');
const ctx = canvas.getContext('2d');
const W = 900, H = 620;

// ── Zone definitions (3 vertical combat levels) ──────────────
// y starts at 36 (below top HUD bar); bottom ends at H-22 (above bottom bar)
const TOP_BAR = 36, BOT_BAR = 22;
const ZONE_TOP = TOP_BAR, ZONE_BOT = H - BOT_BAR; // 36 to 598 = 562 px
const ZONE_H = Math.floor((ZONE_BOT - ZONE_TOP) / 3);

const ZONES = [
  { level: 2, label: 'EAU OUVERTE  •  15m', y: ZONE_TOP,              h: ZONE_H,     dark: '#0e2545', light: '#12305a' },
  { level: 1, label: 'EAU PROFONDE  •  5m', y: ZONE_TOP + ZONE_H,     h: ZONE_H,     dark: '#091a35', light: '#0e2545' },
  { level: 0, label: 'FOND MARIN  •  0m',   y: ZONE_TOP + ZONE_H * 2, h: ZONE_BOT - (ZONE_TOP + ZONE_H * 2), dark: '#050e1e', light: '#091828' },
];

// ── Faction colors ────────────────────────────────────────────
const FAC = {
  player: { color: '#4488ff', glow: '#2255bb', accent: '#aaccff', name: 'Aquiloris' },
  enemy:  { color: '#33cc77', glow: '#116633', accent: '#88ffbb', name: 'Thalassidras' },
};

// ── Unit templates ────────────────────────────────────────────
const TEMPLATES = {
  hero:     { hp: 250, dmg: 35, range: 55, speed: 2.0, size: 20, label: 'Héros',      atkRate: 55  },
  infantry: { hp: 130, dmg: 18, range: 42, speed: 1.4, size: 14, label: 'Infanterie', atkRate: 65  },
  ranged:   { hp:  70, dmg: 28, range: 160, speed: 1.1, size: 12, label: 'Distance',  atkRate: 80  },
  mounted:  { hp: 100, dmg: 22, range: 46, speed: 2.6, size: 16, label: 'Monté',      atkRate: 70  },
};

// ── State ─────────────────────────────────────────────────────
let units = [], particles = [], projectiles = [], bgDots = [];
let selected = [], selBox = null, dragStart = null, isDragging = false;
let frameCount = 0, score = 0, running = false;
let mouseX = 0, mouseY = 0;

// ─────────────────────────────────────────────────────────────
// UNIT CLASS
// ─────────────────────────────────────────────────────────────
class Unit {
  constructor(faction, type, x, y, level) {
    const t = TEMPLATES[type];
    Object.assign(this, {
      faction, type, x, y, level,
      hp: t.hp, maxHp: t.hp, dmg: t.dmg,
      range: t.range, speed: t.speed, size: t.size,
      atkRate: t.atkRate, atkTimer: 0,
      target: null, dest: null,
      dead: false, flash: 0,
      id: Math.random(),
    });
  }

  get fac() { return FAC[this.faction]; }
  get zone() { return ZONES.find(z => z.level === this.level) || ZONES[2]; }

  update() {
    if (this.dead) return;
    if (this.atkTimer > 0) this.atkTimer--;
    if (this.flash > 0) this.flash--;

    const foes = units.filter(u => !u.dead && u.faction !== this.faction);
    if (!this.target || this.target.dead) this.target = this.nearest(foes);

    if (this.target) {
      const dx = this.target.x - this.x, dy = this.target.y - this.y;
      const dist = Math.hypot(dx, dy);
      const lvDiff = Math.abs(this.target.level - this.level);
      const canHit = lvDiff === 0 || (this.type === 'ranged' && lvDiff === 1);
      const effRange = canHit ? this.range : 0;

      if (canHit && dist < effRange) {
        if (this.atkTimer === 0) {
          this.atkTimer = this.atkRate;
          this.attack(this.target);
        }
      } else if (!this.dest) {
        // Enemies auto-chase; player units hold position until ordered
        if (this.faction === 'enemy' && dist > 6) {
          this.step(dx / dist * this.speed, dy / dist * this.speed);
        }
      }
    }

    if (this.dest) {
      const dx = this.dest.x - this.x, dy = this.dest.y - this.y;
      const dist = Math.hypot(dx, dy);
      if (dist < 4) { this.x = this.dest.x; this.y = this.dest.y; this.dest = null; }
      else this.step(dx / dist * this.speed, dy / dist * this.speed);
    }
  }

  step(dx, dy) {
    const z = this.zone;
    this.x = clamp(this.x + dx, this.size + 1, W - this.size - 1);
    this.y = clamp(this.y + dy, z.y + this.size + 22, z.y + z.h - this.size - 4);
  }

  nearest(list) {
    let best = null, bd = Infinity;
    for (const u of list) {
      const d = Math.hypot(u.x - this.x, u.y - this.y);
      if (d < bd) { bd = d; best = u; }
    }
    return best;
  }

  attack(target) {
    if (this.type === 'ranged') {
      projectiles.push(new Projectile(this, target));
    } else {
      target.hit(this.dmg);
      burst(target.x, target.y, this.fac.color, 5, 2.5);
    }
  }

  hit(dmg) {
    this.hp -= dmg;
    this.flash = 10;
    if (this.hp <= 0) this.die();
  }

  die() {
    this.dead = true;
    burst(this.x, this.y, this.fac.color, 22, 4.5);
    if (this.faction === 'enemy') score += (this.type === 'hero' ? 300 : 100);
  }

  draw() {
    if (this.dead) return;
    const { color, glow, accent } = this.fac;
    const sel = selected.includes(this);
    const s = this.size;

    ctx.save();
    ctx.shadowBlur = sel ? 24 : this.flash > 0 ? 18 : 10;
    ctx.shadowColor = this.flash > 0 ? '#ff4422' : (sel ? '#ffffff' : glow);

    ctx.strokeStyle = accent;
    ctx.lineWidth = sel ? 2 : 1.5;

    // Unit shape per type
    if (this.type === 'hero') {
      drawStar(this.x, this.y, 5, s, s * 0.45, color);
    } else if (this.type === 'infantry') {
      drawPoly(this.x, this.y, 6, s, color);
    } else if (this.type === 'ranged') {
      drawDiamond(this.x, this.y, s, color);
    } else {
      ctx.fillStyle = color;
      ctx.beginPath(); ctx.arc(this.x, this.y, s, 0, Math.PI * 2);
      ctx.fill(); ctx.stroke();
    }

    ctx.restore();

    // Level badge
    ctx.fillStyle = '#aabbcc';
    ctx.font = 'bold 8px Courier New';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText('L' + this.level, this.x, this.y + s + 10);

    // HP bar
    const bw = s * 2.8, bh = 4;
    const bx = this.x - bw / 2, by = this.y - s - 12;
    ctx.fillStyle = '#0a0a15';
    ctx.fillRect(bx, by, bw, bh);
    const pct = this.hp / this.maxHp;
    ctx.fillStyle = pct > 0.55 ? '#33ee66' : pct > 0.28 ? '#ffcc22' : '#ff3311';
    ctx.fillRect(bx, by, bw * pct, bh);

    // Selection ring
    if (sel) {
      ctx.save();
      ctx.strokeStyle = 'rgba(200,220,255,0.85)';
      ctx.lineWidth = 1.5;
      ctx.setLineDash([5, 4]);
      ctx.lineDashOffset = -(frameCount * 0.12);
      ctx.beginPath(); ctx.arc(this.x, this.y, s + 7, 0, Math.PI * 2);
      ctx.stroke();
      ctx.restore();
    }

    // Hero crown indicator
    if (this.type === 'hero') {
      ctx.fillStyle = this.fac.color;
      ctx.font = 'bold 10px Courier New';
      ctx.textAlign = 'center';
      ctx.fillText('♛', this.x, this.y - s - 18);
    }
  }
}

// ─────────────────────────────────────────────────────────────
// PROJECTILE
// ─────────────────────────────────────────────────────────────
class Projectile {
  constructor(from, target) {
    this.x = from.x; this.y = from.y;
    this.target = target; this.dmg = from.dmg;
    this.color = from.fac.color; this.dead = false;
    const dx = target.x - from.x, dy = target.y - from.y;
    const d = Math.hypot(dx, dy);
    const spd = 6;
    this.vx = dx / d * spd; this.vy = dy / d * spd;
    this.trail = [];
  }

  update() {
    if (this.dead) return;
    this.trail.push({ x: this.x, y: this.y });
    if (this.trail.length > 6) this.trail.shift();
    this.x += this.vx; this.y += this.vy;
    if (Math.hypot(this.target.x - this.x, this.target.y - this.y) < 12) {
      this.target.hit(this.dmg);
      burst(this.x, this.y, this.color, 8, 3);
      this.dead = true;
    }
    if (this.x < 0 || this.x > W || this.y < 0 || this.y > H) this.dead = true;
  }

  draw() {
    if (this.dead) return;
    // Trail
    ctx.save();
    for (let i = 0; i < this.trail.length; i++) {
      const a = (i / this.trail.length) * 0.5;
      ctx.globalAlpha = a;
      ctx.fillStyle = this.color;
      ctx.beginPath(); ctx.arc(this.trail[i].x, this.trail[i].y, 2, 0, Math.PI * 2); ctx.fill();
    }
    ctx.globalAlpha = 1;
    ctx.shadowBlur = 12; ctx.shadowColor = this.color;
    ctx.fillStyle = '#ffffff';
    ctx.beginPath(); ctx.arc(this.x, this.y, 4, 0, Math.PI * 2); ctx.fill();
    ctx.restore();
  }
}

// ─────────────────────────────────────────────────────────────
// PARTICLES
// ─────────────────────────────────────────────────────────────
function burst(x, y, color, n, spd) {
  for (let i = 0; i < n; i++) {
    const a = Math.random() * Math.PI * 2;
    const s = Math.random() * spd + 0.8;
    particles.push({ x, y, vx: Math.cos(a) * s, vy: Math.sin(a) * s, life: 30 + Math.random() * 20, max: 50, color, r: Math.random() * 3 + 1 });
  }
}

function updateParticles() {
  for (const p of particles) {
    p.x += p.vx; p.y += p.vy;
    p.vx *= 0.9; p.vy *= 0.9; p.life--;
  }
  particles = particles.filter(p => p.life > 0);
}

function drawParticles() {
  ctx.save();
  for (const p of particles) {
    ctx.globalAlpha = (p.life / p.max) * 0.85;
    ctx.shadowBlur = 6; ctx.shadowColor = p.color;
    ctx.fillStyle = p.color;
    ctx.beginPath(); ctx.arc(p.x, p.y, p.r * (p.life / p.max), 0, Math.PI * 2); ctx.fill();
  }
  ctx.restore();
}

// ─────────────────────────────────────────────────────────────
// BACKGROUND
// ─────────────────────────────────────────────────────────────
function initBgDots() {
  bgDots = [];
  for (let i = 0; i < 100; i++) {
    bgDots.push({
      x: Math.random() * W, y: Math.random() * H,
      r: Math.random() * 2.5 + 0.5,
      vy: -(Math.random() * 0.35 + 0.1),
      vx: (Math.random() - 0.5) * 0.15,
      a: Math.random() * 0.35 + 0.05,
    });
  }
}

function drawBackground() {
  // Zones
  for (const z of ZONES) {
    const g = ctx.createLinearGradient(0, z.y, 0, z.y + z.h);
    g.addColorStop(0, z.light); g.addColorStop(1, z.dark);
    ctx.fillStyle = g; ctx.fillRect(0, z.y, W, z.h);

    // Zone separator line
    ctx.strokeStyle = '#0e1e38'; ctx.lineWidth = 1;
    ctx.beginPath(); ctx.moveTo(0, z.y + z.h); ctx.lineTo(W, z.y + z.h); ctx.stroke();

    // Level label (left side)
    ctx.fillStyle = '#1a3060';
    ctx.font = '10px Courier New';
    ctx.textAlign = 'left'; ctx.textBaseline = 'top';
    ctx.fillText(z.label, 8, z.y + 6);

    // Right-side level indicator
    ctx.textAlign = 'right';
    ctx.fillText('NIVEAU ' + z.level, W - 8, z.y + 6);
  }

  // Light rays from surface
  ctx.save();
  for (let i = 0; i < 6; i++) {
    const cx = (W / 6) * i + W / 12;
    const wRay = 20 + Math.sin(frameCount * 0.008 + i * 1.1) * 12;
    const g = ctx.createLinearGradient(0, 0, 0, H);
    g.addColorStop(0, 'rgba(80,130,220,0.06)');
    g.addColorStop(0.5, 'rgba(60,100,180,0.03)');
    g.addColorStop(1, 'rgba(40,70,140,0)');
    ctx.fillStyle = g;
    ctx.beginPath();
    ctx.moveTo(cx - wRay, 0); ctx.lineTo(cx + wRay, 0);
    ctx.lineTo(cx + wRay * 2.5, H); ctx.lineTo(cx - wRay * 2.5, H);
    ctx.closePath(); ctx.fill();
  }
  ctx.restore();

  // Bubbles
  ctx.save();
  for (const d of bgDots) {
    d.x += d.vx; d.y += d.vy;
    if (d.y < -10) { d.y = H + 10; d.x = Math.random() * W; }
    ctx.globalAlpha = d.a;
    ctx.strokeStyle = '#4499cc'; ctx.lineWidth = 0.8;
    ctx.beginPath(); ctx.arc(d.x, d.y, d.r, 0, Math.PI * 2); ctx.stroke();
  }
  ctx.restore();
}

// ─────────────────────────────────────────────────────────────
// HUD
// ─────────────────────────────────────────────────────────────
function drawHUD() {
  const pAlive = units.filter(u => !u.dead && u.faction === 'player').length;
  const eAlive = units.filter(u => !u.dead && u.faction === 'enemy').length;

  // Top bar
  ctx.fillStyle = 'rgba(2,5,15,0.8)';
  ctx.fillRect(0, 0, W, 36);

  ctx.font = 'bold 12px Courier New';
  ctx.textBaseline = 'middle';

  ctx.fillStyle = FAC.player.color;
  ctx.textAlign = 'left';
  ctx.fillText(`♛ AQUILORIS  ${pAlive} UNITÉS`, 14, 18);

  ctx.fillStyle = '#ffffff';
  ctx.textAlign = 'center';
  ctx.fillText(`SCORE  ${score}`, W / 2, 18);

  ctx.fillStyle = FAC.enemy.color;
  ctx.textAlign = 'right';
  ctx.fillText(`THALASSIDRAS  ${eAlive} UNITÉS ♛`, W - 14, 18);

  // Selected unit info panel (bottom-left)
  if (selected.length === 1) {
    const u = selected[0];
    const px = 10, py = H - 90, pw = 210, ph = 78;
    ctx.fillStyle = 'rgba(2,6,20,0.85)';
    ctx.fillRect(px, py, pw, ph);
    ctx.strokeStyle = FAC.player.color + '44';
    ctx.lineWidth = 1; ctx.strokeRect(px, py, pw, ph);

    ctx.fillStyle = FAC.player.color;
    ctx.font = 'bold 11px Courier New';
    ctx.textAlign = 'left'; ctx.textBaseline = 'top';
    ctx.fillText(`${TEMPLATES[u.type].label.toUpperCase()} – NIVEAU ${u.level}`, px + 10, py + 10);

    ctx.fillStyle = '#778';
    ctx.font = '10px Courier New';
    ctx.fillText(`PV: ${Math.max(0, Math.ceil(u.hp))} / ${u.maxHp}`, px + 10, py + 28);
    ctx.fillText(`ATQ: ${u.dmg}  PORTée: ${u.range}  VIT: ${u.speed.toFixed(1)}`, px + 10, py + 44);
    ctx.fillText(`Faction: ${u.fac.name}`, px + 10, py + 60);
  } else if (selected.length > 1) {
    const px = 10, py = H - 58, pw = 160, ph = 46;
    ctx.fillStyle = 'rgba(2,6,20,0.85)'; ctx.fillRect(px, py, pw, ph);
    ctx.strokeStyle = FAC.player.color + '44'; ctx.lineWidth = 1; ctx.strokeRect(px, py, pw, ph);
    ctx.fillStyle = FAC.player.color;
    ctx.font = 'bold 11px Courier New'; ctx.textAlign = 'left'; ctx.textBaseline = 'top';
    ctx.fillText(`${selected.length} UNITÉS SÉLECTIONNÉES`, px + 10, py + 10);
    ctx.fillStyle = '#556';
    ctx.font = '10px Courier New';
    ctx.fillText('CLIC DROIT pour déplacer', px + 10, py + 30);
  }

  // Bottom bar
  ctx.fillStyle = 'rgba(2,5,15,0.7)';
  ctx.fillRect(0, H - 22, W, 22);
  ctx.fillStyle = '#334';
  ctx.font = '10px Courier New';
  ctx.textAlign = 'center'; ctx.textBaseline = 'middle';
  ctx.fillText('G.CLIC Sélectionner  |  D.CLIC Déplacer/Attaquer  |  DRAG Groupe  |  Changer de zone avec D.CLIC dans un autre niveau', W / 2, H - 11);

  // Selection box draw
  if (selBox) {
    ctx.save();
    ctx.strokeStyle = 'rgba(160,200,255,0.7)'; ctx.lineWidth = 1;
    ctx.setLineDash([4, 3]);
    ctx.strokeRect(selBox.x, selBox.y, selBox.w, selBox.h);
    ctx.fillStyle = 'rgba(80,140,255,0.06)';
    ctx.fillRect(selBox.x, selBox.y, selBox.w, selBox.h);
    ctx.restore();
  }
}

// ─────────────────────────────────────────────────────────────
// ENEMY AI (simple wave logic)
// ─────────────────────────────────────────────────────────────
let aiTimer = 0;
function runEnemyAI() {
  aiTimer++;
  if (aiTimer % 180 !== 0) return; // every 3 seconds

  const enemies = units.filter(u => !u.dead && u.faction === 'enemy');
  const players = units.filter(u => !u.dead && u.faction === 'player');
  if (!players.length) return;

  for (const e of enemies) {
    // Pick a random player target or charge nearest
    if (Math.random() < 0.4) {
      const t = players[Math.floor(Math.random() * players.length)];
      e.target = t;
    }
    // Occasionally switch level to create vertical pressure
    if (Math.random() < 0.25) {
      const newLevel = Math.floor(Math.random() * 3);
      const newZone = ZONES.find(z => z.level === newLevel);
      const cy = newZone.y + newZone.h * 0.4 + Math.random() * newZone.h * 0.3;
      e.level = newLevel;
      e.dest = { x: e.x, y: clamp(cy, newZone.y + e.size + 22, newZone.y + newZone.h - e.size - 4) };
    }
  }
}

// ─────────────────────────────────────────────────────────────
// GAME INIT
// ─────────────────────────────────────────────────────────────
function initGame() {
  units = []; particles = []; projectiles = [];
  selected = []; selBox = null; dragStart = null; isDragging = false;
  score = 0; frameCount = 0; aiTimer = 0;

  // Aquiloris (Player) – left side
  const pz0 = ZONES.find(z => z.level === 0);
  const pz1 = ZONES.find(z => z.level === 1);
  const pz2 = ZONES.find(z => z.level === 2);

  spawnUnit('player', 'hero',     90,  pz1.y + pz1.h / 2,       1);
  spawnUnit('player', 'infantry', 60,  pz0.y + pz0.h * 0.4,     0);
  spawnUnit('player', 'infantry', 100, pz0.y + pz0.h * 0.65,    0);
  spawnUnit('player', 'ranged',   50,  pz0.y + pz0.h * 0.25,    0);
  spawnUnit('player', 'infantry', 60,  pz1.y + pz1.h * 0.35,    1);
  spawnUnit('player', 'ranged',   45,  pz1.y + pz1.h * 0.65,    1);
  spawnUnit('player', 'mounted',  110, pz2.y + pz2.h * 0.4,     2);
  spawnUnit('player', 'infantry', 75,  pz2.y + pz2.h * 0.65,    2);

  // Thalassidras (Enemy) – right side
  spawnUnit('enemy',  'hero',     W - 90,  pz1.y + pz1.h / 2,    1);
  spawnUnit('enemy',  'infantry', W - 60,  pz0.y + pz0.h * 0.4,  0);
  spawnUnit('enemy',  'infantry', W - 100, pz0.y + pz0.h * 0.65, 0);
  spawnUnit('enemy',  'ranged',   W - 50,  pz0.y + pz0.h * 0.25, 0);
  spawnUnit('enemy',  'infantry', W - 60,  pz1.y + pz1.h * 0.35, 1);
  spawnUnit('enemy',  'ranged',   W - 45,  pz1.y + pz1.h * 0.65, 1);
  spawnUnit('enemy',  'mounted',  W - 110, pz2.y + pz2.h * 0.4,  2);
  spawnUnit('enemy',  'infantry', W - 75,  pz2.y + pz2.h * 0.65, 2);
}

function spawnUnit(faction, type, x, y, level) {
  units.push(new Unit(faction, type, x, y, level));
}

// ─────────────────────────────────────────────────────────────
// END GAME CHECK
// ─────────────────────────────────────────────────────────────
function checkEnd() {
  if (!running) return;
  const pAlive = units.filter(u => !u.dead && u.faction === 'player').length;
  const eAlive = units.filter(u => !u.dead && u.faction === 'enemy').length;

  if (pAlive === 0) { endGame(false); return; }
  if (eAlive === 0) { endGame(true); return; }
}

function endGame(victory) {
  running = false;
  const el = document.getElementById('endScreen');
  const title = document.getElementById('endTitle');
  const sub = document.getElementById('endSub');
  const sc = document.getElementById('endScore');

  title.textContent = victory ? 'VICTOIRE' : 'DÉFAITE';
  title.style.color = victory ? '#55ff88' : '#ff4422';
  sub.textContent = victory
    ? 'Aquiloris écrase les Thalassidras !'
    : 'Les forces d'Aquiloris sont vaincues...';
  sc.textContent = `Score final : ${score}`;

  el.style.display = 'flex';
}

// ─────────────────────────────────────────────────────────────
// MAIN LOOP
// ─────────────────────────────────────────────────────────────
function loop() {
  requestAnimationFrame(loop);
  if (!running) return;
  frameCount++;

  ctx.clearRect(0, 0, W, H);
  drawBackground();

  // Updates
  runEnemyAI();
  for (const u of units) u.update();
  for (const p of projectiles) p.update();
  updateParticles();
  projectiles = projectiles.filter(p => !p.dead);
  units = units.filter(u => { if (u.dead && !u._deathLogged) { u._deathLogged = true; } return true; });

  // Draw
  drawParticles();
  for (const p of projectiles) p.draw();
  for (const u of units) if (!u.dead) u.draw();

  drawHUD();
  checkEnd();
}

// ─────────────────────────────────────────────────────────────
// INPUT
// ─────────────────────────────────────────────────────────────
canvas.addEventListener('mousedown', e => {
  if (!running) return;
  const { mx, my } = canvasMouse(e);
  if (e.button === 0) {
    isDragging = false;
    dragStart = { x: mx, y: my };
    selBox = null;
  }
  if (e.button === 2) {
    if (!selected.length) return;
    const hit = unitAt(mx, my);
    if (hit && hit.faction === 'enemy') {
      for (const u of selected) { u.target = hit; u.dest = null; }
    } else {
      const zone = zoneAt(my);
      if (!zone) return;
      const cy = clamp(my, zone.y + 28, zone.y + zone.h - 8);
      selected.forEach((u, i) => {
        const col = (i % 3) - 1;
        const row = Math.floor(i / 3);
        const tx = clamp(mx + col * 36, u.size + 2, W - u.size - 2);
        const ty = clamp(cy + row * 34, zone.y + u.size + 22, zone.y + zone.h - u.size - 4);
        u.level = zone.level;
        u.dest = { x: tx, y: ty };
      });
    }
  }
});

canvas.addEventListener('mousemove', e => {
  const { mx, my } = canvasMouse(e);
  mouseX = mx; mouseY = my;
  if (dragStart && e.buttons === 1) {
    isDragging = true;
    const x = Math.min(dragStart.x, mx), y = Math.min(dragStart.y, my);
    const w = Math.abs(mx - dragStart.x), h = Math.abs(my - dragStart.y);
    if (w > 6 || h > 6) selBox = { x, y, w, h };
  }
});

canvas.addEventListener('mouseup', e => {
  if (!running) return;
  const { mx, my } = canvasMouse(e);
  if (e.button === 0) {
    if (isDragging && selBox && (selBox.w > 8 || selBox.h > 8)) {
      selected = units.filter(u => !u.dead && u.faction === 'player' &&
        u.x >= selBox.x && u.x <= selBox.x + selBox.w &&
        u.y >= selBox.y && u.y <= selBox.y + selBox.h);
    } else {
      const hit = unitAt(mx, my);
      selected = (hit && hit.faction === 'player') ? [hit] : [];
    }
    selBox = null; isDragging = false; dragStart = null;
  }
});

canvas.addEventListener('contextmenu', e => e.preventDefault());

function canvasMouse(e) {
  const r = canvas.getBoundingClientRect();
  return {
    mx: (e.clientX - r.left) * (W / r.width),
    my: (e.clientY - r.top)  * (H / r.height),
  };
}

function unitAt(mx, my) {
  for (const u of units) {
    if (u.dead) continue;
    if (Math.hypot(u.x - mx, u.y - my) < u.size + 6) return u;
  }
  return null;
}

function zoneAt(my) {
  return ZONES.find(z => my >= z.y && my < z.y + z.h) || null;
}

// ─────────────────────────────────────────────────────────────
// DRAW HELPERS
// ─────────────────────────────────────────────────────────────
function drawPoly(x, y, sides, r, color) {
  ctx.fillStyle = color;
  ctx.beginPath();
  for (let i = 0; i < sides; i++) {
    const a = (Math.PI * 2 / sides) * i - Math.PI / 2;
    i === 0 ? ctx.moveTo(x + r * Math.cos(a), y + r * Math.sin(a))
            : ctx.lineTo(x + r * Math.cos(a), y + r * Math.sin(a));
  }
  ctx.closePath(); ctx.fill(); ctx.stroke();
}

function drawDiamond(x, y, r, color) {
  ctx.fillStyle = color;
  ctx.beginPath();
  ctx.moveTo(x, y - r); ctx.lineTo(x + r, y);
  ctx.lineTo(x, y + r); ctx.lineTo(x - r, y);
  ctx.closePath(); ctx.fill(); ctx.stroke();
}

function drawStar(x, y, pts, ro, ri, color) {
  ctx.fillStyle = color;
  ctx.beginPath();
  for (let i = 0; i < pts * 2; i++) {
    const a = (Math.PI / pts) * i - Math.PI / 2;
    const r = i % 2 === 0 ? ro : ri;
    i === 0 ? ctx.moveTo(x + r * Math.cos(a), y + r * Math.sin(a))
            : ctx.lineTo(x + r * Math.cos(a), y + r * Math.sin(a));
  }
  ctx.closePath(); ctx.fill(); ctx.stroke();
}

function clamp(v, min, max) { return Math.max(min, Math.min(max, v)); }

// ─────────────────────────────────────────────────────────────
// BOOT
// ─────────────────────────────────────────────────────────────
initBgDots();
loop(); // start rendering (no gameplay until button pressed)

document.getElementById('playBtn').addEventListener('click', () => {
  document.getElementById('menu').style.display = 'none';
  initGame();
  initBgDots();
  running = true;
});

document.getElementById('replayBtn').addEventListener('click', () => {
  document.getElementById('endScreen').style.display = 'none';
  initGame();
  initBgDots();
  running = true;
});
