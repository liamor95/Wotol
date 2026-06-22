import React from 'react';
import {AbsoluteFill, Easing, interpolate, useCurrentFrame, useVideoConfig} from 'remotion';

export const WotolIntro: React.FC = () => {
  const frame = useCurrentFrame();
  const {fps} = useVideoConfig();

  return (
    <AbsoluteFill className="bg-black flex items-center justify-center">
      <div
        style={{
          scale: String(interpolate(frame, [0, fps * 0.8], [0.6, 1], {
            extrapolateRight: 'clamp',
            easing: Easing.bezier(0.16, 1, 0.3, 1),
          })),
          opacity: interpolate(frame, [0, fps * 0.5], [0, 1], {
            extrapolateRight: 'clamp',
          }),
        }}
        className="text-center"
      >
        <h1 className="text-white text-8xl font-bold tracking-widest">
          WOTOL
        </h1>
        <p className="text-gray-400 text-2xl mt-4 tracking-[0.3em]">
          WORLD OF LEGENDS
        </p>
      </div>
    </AbsoluteFill>
  );
};
