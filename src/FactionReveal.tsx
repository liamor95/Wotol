import React from 'react';
import {
  AbsoluteFill,
  Easing,
  Img,
  Sequence,
  interpolate,
  staticFile,
  useCurrentFrame,
  useVideoConfig,
} from 'remotion';

// ─── Palette Thalassidras ──────────────────────────────────────────────────────
const BG      = '#020810';
const BLUE    = '#4fc3f7';
const BLUE_DK = '#1a6e8a';
const GOLD    = '#c8a96e';
const WHITE   = '#e8f4f8';

const UNITS = [
  {name: 'Aquistance',  type: 'Distance',   icon: '🏹', color: '#4fc3f7'},
  {name: 'Aquiloryon',  type: 'Infanterie',  icon: '⚔️', color: '#81d4fa'},
  {name: 'Aquilombre',  type: 'Magie',       icon: '✨', color: '#ce93d8'},
  {name: 'Aquilance',   type: 'Montée',      icon: '🐋', color: '#4db6ac'},
  {name: 'Léviaphénix', type: 'Mythique',    icon: '👑', color: '#ffd54f'},
];

// ─── Helpers ───────────────────────────────────────────────────────────────────
const fadeIn = (frame: number, from: number, duration = 20) =>
  interpolate(frame, [from, from + duration], [0, 1], {
    extrapolateLeft: 'clamp', extrapolateRight: 'clamp',
    easing: Easing.out(Easing.quad),
  });

const slideUp = (frame: number, from: number, distance = 40, duration = 22) =>
  interpolate(frame, [from, from + duration], [distance, 0], {
    extrapolateLeft: 'clamp', extrapolateRight: 'clamp',
    easing: Easing.bezier(0.16, 1, 0.3, 1),
  });

// ─── Bulle ─────────────────────────────────────────────────────────────────────
const Bubble: React.FC<{x: number; size: number; startFrame: number; speed: number}> = ({
  x, size, startFrame, speed,
}) => {
  const frame = useCurrentFrame();
  const {height} = useVideoConfig();
  const e = Math.max(0, frame - startFrame);
  const y       = interpolate(e, [0, speed], [height + 10, -10], {extrapolateRight: 'clamp'});
  const wobble  = interpolate(Math.sin((e / speed) * Math.PI * 4), [-1, 1], [-6, 6]);
  const opacity = interpolate(e, [0, 10, speed - 20, speed], [0, 0.4, 0.2, 0], {extrapolateRight: 'clamp'});
  return (
    <div style={{
      position: 'absolute', left: x + wobble, top: y, width: size, height: size,
      borderRadius: '50%', border: `1px solid ${BLUE}66`,
      background: `radial-gradient(circle at 30% 30%, ${BLUE}18, transparent)`,
      opacity, pointerEvents: 'none',
    }} />
  );
};

// ─── Ligne titre ───────────────────────────────────────────────────────────────
const TitleLetter: React.FC<{char: string; delay: number}> = ({char, delay}) => {
  const frame = useCurrentFrame();
  return (
    <span style={{
      display: 'inline-block',
      opacity: fadeIn(frame, delay, 12),
      translate: `0px ${slideUp(frame, delay, 30, 18)}px`,
      color: BLUE, fontFamily: "'Palatino Linotype', Georgia, serif",
      fontSize: 110, fontWeight: 700, letterSpacing: '0.12em',
      filter: `drop-shadow(0 0 20px ${BLUE}) drop-shadow(0 0 50px ${BLUE_DK})`,
    }}>
      {char}
    </span>
  );
};

// ─── Carte unité ───────────────────────────────────────────────────────────────
const UnitCard: React.FC<{unit: typeof UNITS[0]; index: number; startFrame: number}> = ({
  unit, index, startFrame,
}) => {
  const frame = useCurrentFrame();
  const delay = startFrame + index * 14;
  const op = fadeIn(frame, delay, 18);
  const y  = slideUp(frame, delay, 50, 22);

  return (
    <div style={{
      opacity: op, translate: `0px ${y}px`,
      display: 'flex', alignItems: 'center', gap: 20,
      padding: '14px 24px',
      background: `linear-gradient(90deg, ${unit.color}18, ${unit.color}08)`,
      border: `1px solid ${unit.color}44`,
      borderRadius: 6,
      boxShadow: `0 0 16px ${unit.color}22`,
      minWidth: 320,
    }}>
      <span style={{fontSize: 32}}>{unit.icon}</span>
      <div>
        <div style={{
          color: unit.color, fontSize: 22, fontFamily: 'Georgia, serif',
          fontWeight: 700, letterSpacing: '0.08em',
          textShadow: `0 0 12px ${unit.color}88`,
        }}>
          {unit.name}
        </div>
        <div style={{color: `${WHITE}88`, fontSize: 16, letterSpacing: '0.15em', textTransform: 'uppercase'}}>
          {unit.type}
        </div>
      </div>
    </div>
  );
};

// ─── Composition ───────────────────────────────────────────────────────────────
export const FactionReveal: React.FC = () => {
  const frame = useCurrentFrame();
  const {fps} = useVideoConfig();

  const heroStart    = 0;
  const badgeStart   = Math.round(fps * 1.2);
  const titleStart   = Math.round(fps * 1.8);
  const loreStart    = Math.round(fps * 3.2);
  const divStart     = Math.round(fps * 3.6);
  const unitsStart   = Math.round(fps * 4.0);
  const fadeOutStart = Math.round(fps * 9.5);

  // Image héros : apparition lente
  const heroOpacity = fadeIn(frame, heroStart, 45);
  const heroScale   = String(interpolate(frame, [heroStart, heroStart + 60], [1.05, 1.0], {
    extrapolateLeft: 'clamp', extrapolateRight: 'clamp',
  }));

  // Badge faction
  const badgeOp = fadeIn(frame, badgeStart, 20);
  const badgeY  = slideUp(frame, badgeStart, 30, 22);

  // Lore
  const loreOp = fadeIn(frame, loreStart, 25);
  const loreY  = slideUp(frame, loreStart, 20, 25);

  // Divider
  const divScale = String(interpolate(frame, [divStart, divStart + 30], [0, 1], {
    extrapolateLeft: 'clamp', extrapolateRight: 'clamp',
    easing: Easing.bezier(0.16, 1, 0.3, 1),
  }));
  const divOp = fadeIn(frame, divStart, 15);

  // Fade out
  const fadeOut = interpolate(frame, [fadeOutStart, fadeOutStart + 30], [0, 1], {
    extrapolateLeft: 'clamp', extrapolateRight: 'clamp',
  });

  const bubbles = [
    {x: 80,  size: 5,  start: 0,  speed: 220}, {x: 200, size: 8,  start: 20, speed: 270},
    {x: 340, size: 5,  start: 40, speed: 190}, {x: 1600, size: 7, start: 10, speed: 240},
    {x: 1750, size: 10, start: 30, speed: 280}, {x: 1850, size: 5, start: 5, speed: 200},
  ];

  return (
    <AbsoluteFill style={{background: BG, overflow: 'hidden'}}>

      {/* ── Image héros côté gauche ── */}
      <div style={{
        position: 'absolute', left: 0, top: 0, width: '55%', height: '100%',
        opacity: heroOpacity,
      }}>
        <Img
          src={staticFile('thalassidras-intro.png')}
          style={{width: '100%', height: '100%', objectFit: 'cover', scale: heroScale, objectPosition: 'left center'}}
        />
        {/* Dégradé de fondu vers la droite */}
        <div style={{
          position: 'absolute', inset: 0,
          background: `linear-gradient(90deg, transparent 30%, ${BG}ff 100%)`,
        }} />
        {/* Assombrissement bas */}
        <div style={{
          position: 'absolute', inset: 0,
          background: `linear-gradient(180deg, ${BG}88 0%, transparent 20%, transparent 70%, ${BG}cc 100%)`,
        }} />
      </div>

      {/* ── Ambiance océan droite ── */}
      <div style={{
        position: 'absolute', right: 0, top: 0, width: '55%', height: '100%',
        background: `radial-gradient(ellipse 80% 100% at 100% 50%, ${BLUE_DK}18, transparent 70%)`,
        pointerEvents: 'none',
      }} />

      {/* ── Bulles ── */}
      {bubbles.map((b, i) => <Bubble key={i} {...b} startFrame={b.start} />)}

      {/* ── Contenu texte (côté droit) ── */}
      <AbsoluteFill style={{
        display: 'flex', flexDirection: 'column',
        justifyContent: 'center', alignItems: 'flex-end',
        paddingRight: 120,
      }}>

        {/* Badge faction */}
        <Sequence from={badgeStart} layout="none">
          <div style={{
            opacity: badgeOp, translate: `0px ${badgeY}px`,
            display: 'flex', alignItems: 'center', gap: 16, marginBottom: 20,
          }}>
            <div style={{
              width: 50, height: 60,
              background: `linear-gradient(180deg, ${BLUE}22, ${BLUE}11)`,
              border: `2px solid ${BLUE}88`,
              borderRadius: '3px 3px 50% 50% / 3px 3px 25% 25%',
              display: 'flex', alignItems: 'center', justifyContent: 'center',
              boxShadow: `0 0 16px ${BLUE}66`,
            }}>
              <span style={{
                color: BLUE, fontSize: 28, fontFamily: 'Georgia, serif', fontWeight: 700,
                textShadow: `0 0 12px ${BLUE}`,
              }}>A</span>
            </div>
            <span style={{
              color: `${BLUE}99`, fontSize: 18, fontFamily: 'Georgia, serif',
              letterSpacing: '0.3em', textTransform: 'uppercase',
            }}>
              Faction
            </span>
          </div>
        </Sequence>

        {/* Titre THALASSIDRAS */}
        <div style={{textAlign: 'right'}}>
          {'THALASSIDRAS'.split('').map((c, i) => (
            <TitleLetter key={i} char={c} delay={titleStart + i * 5} />
          ))}
        </div>

        {/* Divider */}
        <div style={{
          width: 500, height: 1, marginTop: 12,
          background: `linear-gradient(90deg, transparent, ${BLUE}88, ${GOLD}66, transparent)`,
          scale: `${divScale} 1`, opacity: divOp,
          alignSelf: 'flex-end',
        }} />

        {/* Lore */}
        <Sequence from={loreStart} layout="none">
          <p style={{
            opacity: loreOp, translate: `0px ${loreY}px`,
            color: `${WHITE}bb`, fontFamily: 'Georgia, serif',
            fontSize: 22, fontStyle: 'italic', lineHeight: 1.6,
            maxWidth: 560, textAlign: 'right', margin: '16px 0 0',
          }}>
            Peuple secret et ancestral,<br/>
            maîtres des océans et gardiens<br/>
            des pouvoirs abyssaux.
          </p>
        </Sequence>

        {/* Titre section unités */}
        <Sequence from={unitsStart - 10} layout="none">
          <p style={{
            opacity: fadeIn(frame, unitsStart - 10, 20),
            color: `${BLUE}88`, fontFamily: 'Georgia, serif',
            fontSize: 16, letterSpacing: '0.35em',
            textTransform: 'uppercase', margin: '28px 0 12px',
          }}>
            Unités disponibles
          </p>
        </Sequence>

        {/* Unités */}
        <Sequence from={unitsStart} layout="none">
          <div style={{display: 'flex', flexDirection: 'column', gap: 10, alignItems: 'flex-end'}}>
            {UNITS.map((u, i) => <UnitCard key={u.name} unit={u} index={i} startFrame={0} />)}
          </div>
        </Sequence>

      </AbsoluteFill>

      {/* ── Vignette ── */}
      <div style={{
        position: 'absolute', inset: 0, pointerEvents: 'none',
        background: `radial-gradient(ellipse 100% 100% at 50% 50%, transparent 60%, ${BG}aa 100%)`,
      }} />

      {/* ── Fade out ── */}
      <div style={{
        position: 'absolute', inset: 0,
        background: BG, opacity: fadeOut, pointerEvents: 'none',
      }} />

    </AbsoluteFill>
  );
};
