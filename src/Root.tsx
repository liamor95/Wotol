import React from 'react';
import {Composition} from 'remotion';
import {FactionReveal} from './FactionReveal';
import {WotolIntro} from './WotolIntro';
import {WotolTeaser} from './WotolTeaser';
import './style.css';

export const Root: React.FC = () => {
  return (
    <>
      <Composition
        id="WotolTeaser"
        component={WotolTeaser}
        durationInFrames={240}
        fps={30}
        width={1920}
        height={1080}
        defaultProps={{}}
      />
      <Composition
        id="FactionReveal_Thalassidras"
        component={FactionReveal}
        durationInFrames={330}
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
