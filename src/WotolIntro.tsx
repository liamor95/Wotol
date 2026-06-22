import React from 'react';
import {AbsoluteFill, spring, useCurrentFrame, useVideoConfig} from 'remotion';

export const WotolIntro: React.FC = () => {
  const frame = useCurrentFrame();
  const {fps} = useVideoConfig();

  const scale = spring({
    fps,
    frame,
    config: {damping: 12},
  });

  const opacity = Math.min(1, frame / 20);

  return (
    <AbsoluteFill className="bg-black flex items-center justify-center">
      <div
        style={{transform: `scale(${scale})`, opacity}}
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
