/*
  ==============================================================================

  Eq4Band
  
  ==============================================================================
*/

#include "Eq4Band.h"

Eq4Band::Eq4Band()
{
    setParameters(0.0, 0.0, 0.0, 0.0);
}

void Eq4Band::process (const float* inData, float* outData,
                                    MidiBuffer& midiMessages, 
                                    const int numSamples, 
                                    const int numInputChannels,
                                    const int sampleRate)
{
    // Reset params if new sampleRate detected
    if (srate != sampleRate) {
        srate = sampleRate;
        resetSampleRate();
    }
    for (int sample = 0; sample < numSamples; ++sample) {
        spl0 = inData[sample];
        s0 = spl0;
        low0 = (tmplMID = a0MID * s0 - b1MID * tmplMID + cDenorm);
        spl0 = (tmplLOW = a0LOW * low0 - b1LOW * tmplLOW + cDenorm);
        lowS0 = low0 - spl0;
        hi0 = s0 - low0;
        midS0 = (tmplHI = a0HI * hi0 - b1HI * tmplHI + cDenorm);
        highS0 = hi0 - midS0;
        spl0 = (spl0 * lVol + lowS0 * lmVol + midS0 * hmVol + highS0 * hVol);// * outVol;
   
        outData[sample] = spl0;
    }
}

void Eq4Band::setParameters(float bass_slider, float mid_slider, float treble_slider, float presence_slider)
{
    setBass(bass_slider);
    setMid(mid_slider);
    setTreble(treble_slider);
    setPresence(presence_slider);
    outVol = exp(0.0 / cAmpDB);

    xHI = exp(-2.0 * pi * treble_frequency / srate);
    a0HI = 1.0 - xHI;
    b1HI = -xHI;

    xMID = exp(-2.0 * pi * mid_frequency / srate);
    a0MID = 1.0 - xMID;
    b1MID = -xMID;

    xLOW = exp(-2.0 * pi * bass_frequency / srate);
    a0LOW = 1.0 - xLOW;
    b1LOW = -xLOW;
}

void Eq4Band::setBass(float bass_slider)
{
    lVol = exp(bass_slider / cAmpDB);
}

void Eq4Band::setMid(float mid_slider)
{
    lmVol = exp(mid_slider / cAmpDB);
}

void Eq4Band::setTreble(float treble_slider)
{
    hmVol = exp(treble_slider / cAmpDB);
}

void Eq4Band::setPresence(float presence_slider)
{
    hVol = exp(presence_slider / cAmpDB);
}

void Eq4Band::resetSampleRate()
{
    xHI = exp(-2.0 * pi * treble_frequency / srate);
    a0HI = 1.0 - xHI;
    b1HI = -xHI;

    xMID = exp(-2.0 * pi * mid_frequency / srate);
    a0MID = 1.0 - xMID;
    b1MID = -xMID;

    xLOW = exp(-2.0 * pi * bass_frequency / srate);
    a0LOW = 1.0 - xLOW;
    b1LOW = -xLOW;
}