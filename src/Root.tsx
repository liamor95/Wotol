import React from 'react';
import {Composition} from 'remotion';
import {WotolIntro} from './WotolIntro';
import './style.css';

export const Root: React.FC = () => {
  return (
    <>
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
