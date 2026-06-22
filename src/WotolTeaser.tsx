import React from 'react';
import {
  AbsoluteFill,
  Easing,
  Sequence,
  interpolate,
  useCurrentFrame,
  useVideoConfig,
} from 'remotion';

const GOLD = '#c8a96e';
const RED = '#8b1a1a';
const BG = '#08080d';

const letters = ['W', 'O', 'T', 'O', 'L'];

const Letter: React.FC<{char: string; index: number; startFrame: number}> = ({
  char,
  index,
  startFrame,
}) => {
  const frame = useCurrentFrame();
  const delay = startFrame + index * 6;

  const y = interpolate(frame, [delay, delay + 20], [60, 0], {
    extrapolateLeft: 'clamp',
    extrapolateRight: 'clamp',
    easing: Easing.bezier(0.16, 1, 0.3, 1),
  });

  const opacity = interpolate(frame, [delay, delay + 14], [0, 1], {
    extrapolateLeft: 'clamp',
    extrapolateRight: 'clamp',
  });

  return (
    <span
      style={{
        opacity,
        translate: `0px ${y}px`,
        display: 'inline-block',
        color: GOLD,
        textShadow: `0 0 40px ${GOLD}88, 0 0 80px ${GOLD}44`,
        fontFamily: "'Georgia', 'Times New Roman', serif",
        fontSize: 160,
        fontWeight: 700,
        letterSpacing: '0.15em',
        lineHeight: 1,
      }}
    >
      {char}
    </span>
  );
};

const CenterGlow: React.FC = () => {
  const frame = useCurrentFrame();

  const opacity = interpolate(frame, [20, 60], [0, 1], {
    extrapolateLeft: 'clamp',
    extrapolateRight: 'clamp',
    easing: Easing.out(Easing.quad),
  });

  const scale = String(
    interpolate(frame, [20, 70], [0.4, 1], {
      extrapolateLeft: 'clamp',
      extrapolateRight: 'clamp',
      easing: Easing.bezier(0.16, 1, 0.3, 1),
    })
  );

  return (
    <div
      style={{
        position: 'absolute',
        width: 800,
        height: 800,
        borderRadius: '50%',
        background: `radial-gradient(circle, ${RED}22 0%, transparent 70%)`,
        opacity,
        scale,
        top: '50%',
        left: '50%',
        translate: '-50% -50%',
        pointerEvents: 'none',
      }}
    />
  );
};

const Divider: React.FC<{startFrame: number}> = ({startFrame}) => {
  const frame = useCurrentFrame();

  const sx = interpolate(frame, [startFrame, startFrame + 30], [0, 1], {
    extrapolateLeft: 'clamp',
    extrapolateRight: 'clamp',
    easing: Easing.bezier(0.16, 1, 0.3, 1),
  });

  const opacity = interpolate(frame, [startFrame, startFrame + 10], [0, 1], {
    extrapolateLeft: 'clamp',
    extrapolateRight: 'clamp',
  });

  return (
    <div
      style={{
        width: 600,
        height: 1,
        background: `linear-gradient(90deg, transparent, ${GOLD}, transparent)`,
        scale: `${sx} 1`,
        opacity,
      }}
    />
  );
};

const Subtitle: React.FC<{startFrame: number}> = ({startFrame}) => {
  const frame = useCurrentFrame();

  const opacity = interpolate(frame, [startFrame, startFrame + 25], [0, 1], {
    extrapolateLeft: 'clamp',
    extrapolateRight: 'clamp',
    easing: Easing.out(Easing.quad),
  });

  const letterSpacing = interpolate(
    frame,
    [startFrame, startFrame + 40],
    ['0.6em', '0.4em'],
    {extrapolateLeft: 'clamp', extrapolateRight: 'clamp'}
  );

  return (
    <p
      style={{
        opacity,
        color: '#aaaaaa',
        fontFamily: "'Georgia', serif",
        fontSize: 28,
        fontWeight: 300,
        letterSpacing,
        margin: 0,
        textTransform: 'uppercase',
      }}
    >
      World of Legends
    </p>
  );
};

const FadeOut: React.FC<{startFrame: number}> = ({startFrame}) => {
  const frame = useCurrentFrame();
  const {durationInFrames} = useVideoConfig();

  const opacity = interpolate(
    frame,
    [startFrame, durationInFrames - 5],
    [0, 1],
    {extrapolateLeft: 'clamp', extrapolateRight: 'clamp'}
  );

  return (
    <div
      style={{
        position: 'absolute',
        inset: 0,
        background: BG,
        opacity,
        pointerEvents: 'none',
      }}
    />
  );
};

export const WotolTeaser: React.FC = () => {
  const {fps} = useVideoConfig();

  const titleStart = Math.round(fps * 1);
  const dividerStart = Math.round(fps * 2.6);
  const subtitleStart = Math.round(fps * 3.2);
  const fadeOutStart = Math.round(fps * 4.8);

  return (
    <AbsoluteFill style={{background: BG}}>
      <CenterGlow />

      <AbsoluteFill
        style={{
          display: 'flex',
          flexDirection: 'column',
          alignItems: 'center',
          justifyContent: 'center',
          gap: 24,
        }}
      >
        <div style={{display: 'flex', gap: 4}}>
          {letters.map((char, i) => (
            <Letter key={i} char={char} index={i} startFrame={titleStart} />
          ))}
        </div>

        <Sequence from={dividerStart} layout="none">
          <Divider startFrame={0} />
        </Sequence>

        <Sequence from={subtitleStart} layout="none">
          <Subtitle startFrame={0} />
        </Sequence>
      </AbsoluteFill>

      <FadeOut startFrame={fadeOutStart} />
    </AbsoluteFill>
  );
};
