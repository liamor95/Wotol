import React from 'react';
import {Composition} from 'remotion';
import {WotolIntro} from './WotolIntro';
import {WotolTeaser} from './WotolTeaser';
import './style.css';

export const Root: React.FC = () => {
  return (
    <>
      <Composition
        id="WotolTeaser"
        component={WotolTeaser}
        durationInFrames={180}
        fps={30}
        width={1920}
        height={1080}
        defaultProps={{}}
      />
      <Composition
        id="WotolIntro"
        component={WotolIntro}
        durationInFrames={150}
        fps={30}
        width={1920}
        height={1080}
        defaultProps={{}}
      />
    </>
  );
};
