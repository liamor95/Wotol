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

// ─── Palette ──────────────────────────────────────────────────────────────────
const BG_DEEP  = '#020810';
const CYAN     = '#4fc3f7';
const CYAN_DIM = '#1a6e8a';
const GOLD     = '#c8a020';
const WHITE    = '#e8f4f8';

const FACTIONS = [
  {letter: 'A', name: 'Thalassidras', color: '#4fc3f7', glow: '#1a9ed4'},
  {letter: 'N', name: 'Noxéens',      color: '#4caf50', glow: '#2d8a3e'},
  {letter: 'T', name: 'Torannides',   color: '#ffb300', glow: '#c8a020'},
  {letter: 'M', name: 'Muréniens',    color: '#9c27b0', glow: '#6a1b9a'},
  {letter: 'P', name: 'Pirates',      color: '#ef5350', glow: '#b71c1c'},
];

// ─── Slideshow background ──────────────────────────────────────────────────────
const SLIDES = [
  {src: staticFile('splash.png'),          start: 0,   end: 130},
  {src: staticFile('gameplay-battle.png'), start: 100, end: 240},
];

const BackgroundSlideshow: React.FC = () => {
  const frame = useCurrentFrame();

  return (
    <>
      {SLIDES.map((slide, i) => {
        const fadeIn  = interpolate(frame, [slide.start, slide.start + 25], [0, 1], {
          extrapolateLeft: 'clamp', extrapolateRight: 'clamp',
          easing: Easing.out(Easing.quad),
        });
        const fadeOut = i === SLIDES.length - 1 ? 1 : interpolate(
          frame,
          [SLIDES[i + 1].start - 5, SLIDES[i + 1].start + 25],
          [1, 0],
          {extrapolateLeft: 'clamp', extrapolateRight: 'clamp'},
        );
        // Ken Burns lent : scale 1.0 → 1.06
        const ks = String(interpolate(frame, [slide.start, slide.end], [1, 1.06], {
          extrapolateLeft: 'clamp', extrapolateRight: 'clamp',
        }));

        return (
          <div key={i} style={{position: 'absolute', inset: 0, opacity: fadeIn * fadeOut}}>
            <Img
              src={slide.src}
              style={{width: '100%', height: '100%', objectFit: 'cover', scale: ks}}
            />
          </div>
        );
      })}
      {/* Assombrissement fort pour lisibilité texte */}
      <div style={{
        position: 'absolute', inset: 0,
        background: `linear-gradient(180deg, ${BG_DEEP}99 0%, ${BG_DEEP}bb 50%, ${BG_DEEP}ee 100%)`,
      }} />
    </>
  );
};

// ─── Rayons lumineux ───────────────────────────────────────────────────────────
const LightRay: React.FC<{x: number; angle: number; delay: number}> = ({x, angle, delay}) => {
  const frame = useCurrentFrame();
  const opacity = interpolate(
    frame,
    [delay, delay + 40, delay + 120, delay + 160],
    [0, 0.07, 0.04, 0],
    {extrapolateLeft: 'clamp', extrapolateRight: 'clamp'},
  );
  return (
    <div style={{
      position: 'absolute', top: -200, left: x, width: 80, height: 1400,
      background: `linear-gradient(180deg, ${CYAN}cc 0%, ${CYAN}44 30%, transparent 80%)`,
      opacity, rotate: `${angle}deg`, transformOrigin: 'top center',
      pointerEvents: 'none', filter: 'blur(8px)',
    }} />
  );
};

// ─── Bulle ─────────────────────────────────────────────────────────────────────
const Bubble: React.FC<{x: number; size: number; startFrame: number; speed: number}> = ({
  x, size, startFrame, speed,
}) => {
  const frame = useCurrentFrame();
  const {height} = useVideoConfig();
  const e = Math.max(0, frame - startFrame);
  const y       = interpolate(e, [0, speed], [height + 20, -20], {extrapolateRight: 'clamp'});
  const wobble  = interpolate(Math.sin((e / speed) * Math.PI * 4), [-1, 1], [-8, 8]);
  const opacity = interpolate(e, [0, 10, speed - 20, speed], [0, 0.45, 0.25, 0], {extrapolateRight: 'clamp'});
  return (
    <div style={{
      position: 'absolute', left: x + wobble, top: y, width: size, height: size,
      borderRadius: '50%', border: `1px solid ${CYAN}88`,
      background: `radial-gradient(circle at 30% 30%, ${CYAN}22, transparent)`,
      opacity, pointerEvents: 'none',
    }} />
  );
};

// ─── Lettre logo ───────────────────────────────────────────────────────────────
const LogoLetter: React.FC<{char: string; index: number; startFrame: number}> = ({char, index, startFrame}) => {
  const frame = useCurrentFrame();
  const delay = startFrame + index * 8;
  const y       = interpolate(frame, [delay, delay + 22], [50, 0], {extrapolateLeft: 'clamp', extrapolateRight: 'clamp', easing: Easing.bezier(0.16, 1, 0.3, 1)});
  const opacity = interpolate(frame, [delay, delay + 16], [0, 1], {extrapolateLeft: 'clamp', extrapolateRight: 'clamp'});
  const blur    = interpolate(frame, [delay, delay + 16], [20, 0], {extrapolateLeft: 'clamp', extrapolateRight: 'clamp'});
  return (
    <span style={{
      opacity, translate: `0px ${y}px`, display: 'inline-block',
      color: CYAN,
      fontFamily: "'Palatino Linotype', 'Book Antiqua', Georgia, serif",
      fontSize: 180, fontWeight: 700, letterSpacing: '0.08em', lineHeight: 1,
      filter: `blur(${blur}px) drop-shadow(0 0 30px ${CYAN}) drop-shadow(0 0 60px ${CYAN_DIM})`,
    }}>
      {char}
    </span>
  );
};

// ─── Carte faction ─────────────────────────────────────────────────────────────
const FactionCard: React.FC<{faction: typeof FACTIONS[0]; index: number}> = ({faction, index}) => {
  const frame = useCurrentFrame();
  const delay = index * 12;
  const y       = interpolate(frame, [delay, delay + 25], [60, 0], {extrapolateLeft: 'clamp', extrapolateRight: 'clamp', easing: Easing.bezier(0.16, 1, 0.3, 1)});
  const opacity = interpolate(frame, [delay, delay + 18], [0, 1], {extrapolateLeft: 'clamp', extrapolateRight: 'clamp'});
  return (
    <div style={{opacity, translate: `0px ${y}px`, display: 'flex', flexDirection: 'column', alignItems: 'center', gap: 12}}>
      <div style={{
        width: 100, height: 120,
        background: `linear-gradient(180deg, ${faction.color}22, ${faction.color}11)`,
        border: `2px solid ${faction.color}88`,
        borderRadius: '4px 4px 50% 50% / 4px 4px 30% 30%',
        display: 'flex', alignItems: 'center', justifyContent: 'center',
        boxShadow: `0 0 20px ${faction.glow}66, inset 0 0 20px ${faction.color}11`,
      }}>
        <span style={{
          color: faction.color, fontSize: 56,
          fontFamily: "'Palatino Linotype', Georgia, serif", fontWeight: 700,
          textShadow: `0 0 20px ${faction.glow}, 0 0 40px ${faction.glow}88`,
        }}>
          {faction.letter}
        </span>
      </div>
      <span style={{
        color: `${faction.color}cc`, fontSize: 18,
        fontFamily: 'Georgia, serif', letterSpacing: '0.12em', textTransform: 'uppercase',
      }}>
        {faction.name}
      </span>
    </div>
  );
};

// ─── Composition principale ────────────────────────────────────────────────────
export const WotolTeaser: React.FC = () => {
  const frame = useCurrentFrame();
  const {fps} = useVideoConfig();

  const logoStart     = Math.round(fps * 1.0);
  const subtitleStart = Math.round(fps * 2.8);
  const dividerStart  = Math.round(fps * 3.2);
  const taglineStart  = Math.round(fps * 3.6);
  const factionsStart = Math.round(fps * 4.2);
  const fadeOutStart  = Math.round(fps * 7.5);

  const haloOpacity = interpolate(frame, [logoStart - 20, logoStart + 20], [0, 1], {
    extrapolateLeft: 'clamp', extrapolateRight: 'clamp',
  });
  const subtitleOpacity = interpolate(frame, [subtitleStart, subtitleStart + 25], [0, 1], {
    extrapolateLeft: 'clamp', extrapolateRight: 'clamp',
  });
  const dividerScale = String(interpolate(frame, [dividerStart, dividerStart + 35], [0, 1], {
    extrapolateLeft: 'clamp', extrapolateRight: 'clamp',
    easing: Easing.bezier(0.16, 1, 0.3, 1),
  }));
  const taglineOpacity = interpolate(frame, [taglineStart, taglineStart + 30], [0, 1], {
    extrapolateLeft: 'clamp', extrapolateRight: 'clamp',
  });
  const fadeOut = interpolate(frame, [fadeOutStart, fadeOutStart + 40], [0, 1], {
    extrapolateLeft: 'clamp', extrapolateRight: 'clamp',
  });

  const bubbles = [
    {x: 120, size: 6, start: 0, speed: 200}, {x: 280, size: 10, start: 15, speed: 260},
    {x: 450, size: 5, start: 30, speed: 180}, {x: 620, size: 8, start: 5, speed: 230},
    {x: 800, size: 12, start: 50, speed: 290}, {x: 960, size: 6, start: 10, speed: 210},
    {x: 1100, size: 9, start: 40, speed: 250}, {x: 1300, size: 5, start: 20, speed: 185},
    {x: 1480, size: 11, start: 35, speed: 270}, {x: 1650, size: 7, start: 0, speed: 195},
    {x: 1800, size: 8, start: 25, speed: 240}, {x: 380, size: 5, start: 80, speed: 200},
    {x: 1550, size: 6, start: 70, speed: 215},
  ];

  return (
    <AbsoluteFill style={{background: BG_DEEP, overflow: 'hidden'}}>

      <BackgroundSlideshow />

      <LightRay x={700} angle={-8} delay={10} />
      <LightRay x={900} angle={0}  delay={0}  />
      <LightRay x={1100} angle={6} delay={20} />
      <LightRay x={500} angle={-14} delay={35} />
      <LightRay x={1300} angle={10} delay={25} />

      {bubbles.map((b, i) => <Bubble key={i} {...b} startFrame={b.start} />)}

      {/* Halo central */}
      <div style={{
        position: 'absolute', width: 700, height: 700, borderRadius: '50%',
        background: `radial-gradient(circle, ${CYAN}18 0%, ${CYAN_DIM}0a 40%, transparent 70%)`,
        opacity: haloOpacity, top: '50%', left: '50%',
        translate: '-50% -50%', filter: 'blur(40px)', pointerEvents: 'none',
      }} />

      <AbsoluteFill style={{
        display: 'flex', flexDirection: 'column',
        alignItems: 'center', justifyContent: 'center', gap: 16,
      }}>
        {/* Logo */}
        <div style={{display: 'flex'}}>
          {'WOTOL'.split('').map((c, i) => <LogoLetter key={i} char={c} index={i} startFrame={logoStart} />)}
        </div>

        {/* Divider */}
        <div style={{
          width: 700, height: 1,
          background: `linear-gradient(90deg, transparent, ${CYAN}88, ${GOLD}88, ${CYAN}88, transparent)`,
          scale: `${dividerScale} 1`, opacity: subtitleOpacity,
        }} />

        {/* Sous-titre */}
        <p style={{
          opacity: subtitleOpacity, color: WHITE, margin: 0,
          fontFamily: 'Georgia, serif', fontSize: 26, fontWeight: 300,
          letterSpacing: '0.45em', textTransform: 'uppercase',
          textShadow: `0 0 20px ${CYAN}88`,
        }}>
          War of the Ocean's Legacy
        </p>

        {/* Tagline */}
        <Sequence from={taglineStart} layout="none">
          <p style={{
            opacity: taglineOpacity, color: `${CYAN}99`, margin: '8px 0 0',
            fontFamily: 'Georgia, serif', fontSize: 20,
            letterSpacing: '0.25em', textTransform: 'uppercase',
          }}>
            Choisissez votre faction
          </p>
        </Sequence>

        {/* Factions */}
        <Sequence from={factionsStart} layout="none">
          <div style={{display: 'flex', gap: 40, marginTop: 12}}>
            {FACTIONS.map((f, i) => <FactionCard key={f.letter} faction={f} index={i} />)}
          </div>
        </Sequence>
      </AbsoluteFill>

      {/* Vignette */}
      <div style={{
        position: 'absolute', inset: 0, pointerEvents: 'none',
        background: `radial-gradient(ellipse 100% 100% at 50% 50%, transparent 50%, ${BG_DEEP}bb 100%)`,
      }} />

      {/* Fade out */}
      <div style={{position: 'absolute', inset: 0, background: BG_DEEP, opacity: fadeOut, pointerEvents: 'none'}} />

    </AbsoluteFill>
  );
};
