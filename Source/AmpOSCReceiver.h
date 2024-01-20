#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#include <functional>
#include <vector>

class AmpOSCReceiver :
        private juce::OSCReceiver,
        private juce::OSCReceiver::ListenerWithOSCAddress<juce::OSCReceiver::RealtimeCallback>
{

public:
    AmpOSCReceiver()
    {
        if(!connect(9001))
        {
            DBG("Error: could not connect to UDP port 9001.");
        }
        
        gainAddressPattern = "/parameter/NeuralPi/Gain";
        masterAddressPattern = "/parameter/NeuralPi/Master";
        bassAddressPattern = "/parameter/NeuralPi/Bass";
        midAddressPattern = "/parameter/NeuralPi/Mid";
        trebleAddressPattern = "/parameter/NeuralPi/Treble";
        presenceAddressPattern = "/parameter/NeuralPi/Presence";

        reverbAddressPattern = "/parameter/NeuralPi/Reverb";
        delayAddressPattern = "/parameter/NeuralPi/Delay";
        chorusAddressPattern = "/parameter/NeuralPi/Chorus";
        flangerAddressPattern = "/parameter/NeuralPi/Flanger";
        
        modelAddressPattern = "/parameter/NeuralPi/Model";
        irAddressPattern = "/parameter/NeuralPi/Ir";

        addListener(this, gainAddressPattern);
        addListener(this, masterAddressPattern);
        addListener(this, bassAddressPattern);
        addListener(this, midAddressPattern);
        addListener(this, trebleAddressPattern);
        addListener(this, presenceAddressPattern);
        addListener(this, reverbAddressPattern);
        addListener(this, delayAddressPattern);
        addListener(this, chorusAddressPattern);
        addListener(this, flangerAddressPattern);
        addListener(this, modelAddressPattern);
        addListener(this, irAddressPattern);
    }

    void oscMessageReceived (const juce::OSCMessage& message) override
    {
        if (message.size() == 1 && message[0].isString())
        {
            if (message.getAddressPattern().matches(modelAddressPattern))
            {
                modelCallback(message[0].getString());
            }
            if (message.getAddressPattern().matches(irAddressPattern))
            {
                irCallback(message[0].getString());
            }
        }

        if (message.size() == 1 && message[0].isFloat32())
        {
            if (message.getAddressPattern().matches(gainAddressPattern))
            {
                gainCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            }
            if (message.getAddressPattern().matches(masterAddressPattern))
            {
                masterCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            }
            if (message.getAddressPattern().matches(bassAddressPattern))
            {
                bassCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            }
            if (message.getAddressPattern().matches(midAddressPattern))
            {
                midCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            }
            if (message.getAddressPattern().matches(trebleAddressPattern))
            {
                trebleCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            }
            if (message.getAddressPattern().matches(presenceAddressPattern))
            {
                presenceCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            }
            if (message.getAddressPattern().matches(reverbAddressPattern))
            {
                reverbCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            }
            if (message.getAddressPattern().matches(delayAddressPattern))
            {
                delayCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            }
            if (message.getAddressPattern().matches(chorusAddressPattern))
            {
                chorusCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            }
            if (message.getAddressPattern().matches(flangerAddressPattern))
            {
                flangerCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            }
        }
    }

public:
    std::function<void(float)> gainCallback;
    std::function<void(float)> masterCallback;
    std::function<void(float)> bassCallback;
    std::function<void(float)> midCallback;
    std::function<void(float)> trebleCallback;
    std::function<void(float)> presenceCallback;
    std::function<void(float)> reverbCallback;
    std::function<void(float)> delayCallback;
    std::function<void(float)> chorusCallback;
    std::function<void(float)> flangerCallback;
    std::function<void(juce::String)> modelCallback;
    std::function<void(juce::String)> irCallback;

private:
    String gainAddressPattern;
    String masterAddressPattern;
    String bassAddressPattern;
    String midAddressPattern;
    String trebleAddressPattern;
    String presenceAddressPattern;
    String reverbAddressPattern;
    String delayAddressPattern;
    String chorusAddressPattern;
    String flangerAddressPattern;
    String modelAddressPattern;
    String irAddressPattern;

}; //end class AmpOSCReceiver