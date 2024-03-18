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

        delayAddressPattern = "/parameter/NeuralPi/Delay";
        delayWetLevelAddressPattern = "/parameter/NeuralPi/DelayWetLevel";
        delayTimeAddressPattern = "/parameter/NeuralPi/DelayTime";
        delayFeedbackAddressPattern = "/parameter/NeuralPi/DelayFeedback";

        chorusAddressPattern = "/parameter/NeuralPi/Chorus";
        chorusMixAddressPattern = "/parameter/NeuralPi/ChorusMix";
        chorusRateAddressPattern = "/parameter/NeuralPi/ChorusRate";
        chorusDepthAddressPattern = "/parameter/NeuralPi/ChorusDepth";
        chorusCentreDelayAddressPattern = "/parameter/NeuralPi/ChorusCentreDelay";
        chorusFeedbackAddressPattern = "/parameter/NeuralPi/ChorusFeedback";

        flangerAddressPattern = "/parameter/NeuralPi/Flanger";
        flangerMixAddressPattern = "/parameter/NeuralPi/FlangerMix";
        flangerRateAddressPattern = "/parameter/NeuralPi/FlangerRate";
        flangerDepthAddressPattern = "/parameter/NeuralPi/FlangerDepth";
        flangerCentreDelayAddressPattern = "/parameter/NeuralPi/FlangerCentreDelay";
        flangerFeedbackAddressPattern = "/parameter/NeuralPi/FlangerFeedback";

        reverbAddressPattern = "/parameter/NeuralPi/Reverb";
        reverbWetLevelAddressPattern = "/parameter/NeuralPi/ReverbWetLevel";
        reverbDampingAddressPattern = "/parameter/NeuralPi/ReverbDamping";
        reverbRoomSizeAddressPattern = "/parameter/NeuralPi/ReverbRoomSize";
        
        modelAddressPattern = "/parameter/NeuralPi/Model";
        irAddressPattern = "/parameter/NeuralPi/Ir";

        ampStateAddressPattern = "/parameter/NeuralPi/AmpState";
        lstmStateAddressPattern = "/parameter/NeuralPi/LSTMState";
        irStateAddressPattern = "/parameter/NeuralPi/IrState";
        recordAddressPattern = "/parameter/NeuralPi/Record";

        addListener(this, gainAddressPattern);
        addListener(this, masterAddressPattern);
        addListener(this, bassAddressPattern);
        addListener(this, midAddressPattern);
        addListener(this, trebleAddressPattern);
        addListener(this, presenceAddressPattern);

        addListener(this, delayAddressPattern);
        addListener(this, delayWetLevelAddressPattern);
        addListener(this, delayTimeAddressPattern);
        addListener(this, delayFeedbackAddressPattern);

        addListener(this, chorusAddressPattern);
        addListener(this, chorusMixAddressPattern);
        addListener(this, chorusRateAddressPattern);
        addListener(this, chorusDepthAddressPattern);
        addListener(this, chorusCentreDelayAddressPattern);
        addListener(this, chorusFeedbackAddressPattern);

        addListener(this, flangerAddressPattern);
        addListener(this, flangerMixAddressPattern);
        addListener(this, flangerRateAddressPattern);
        addListener(this, flangerDepthAddressPattern);
        addListener(this, flangerCentreDelayAddressPattern);
        addListener(this, flangerFeedbackAddressPattern);

        addListener(this, reverbAddressPattern);
        addListener(this, reverbWetLevelAddressPattern);
        addListener(this, reverbDampingAddressPattern);
        addListener(this, reverbRoomSizeAddressPattern);

        addListener(this, modelAddressPattern);
        addListener(this, irAddressPattern);

        addListener(this, ampStateAddressPattern);
        addListener(this, lstmStateAddressPattern);
        addListener(this, irStateAddressPattern);
        addListener(this, recordAddressPattern);
    }

    void oscMessageReceived (const juce::OSCMessage& message) override
    {
        if (message.size() == 1 && message[0].isString())
        {
            if (message.getAddressPattern().matches(modelAddressPattern))
                modelCallback(message[0].getString());
            if (message.getAddressPattern().matches(irAddressPattern))
                irCallback(message[0].getString());
        }

        if (message.size() == 1 && message[0].isFloat32())
        {
            if (message.getAddressPattern().matches(gainAddressPattern))
                gainCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            if (message.getAddressPattern().matches(masterAddressPattern))
                masterCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            if (message.getAddressPattern().matches(bassAddressPattern))
                bassCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            if (message.getAddressPattern().matches(midAddressPattern))
                midCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            if (message.getAddressPattern().matches(trebleAddressPattern))
                trebleCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            if (message.getAddressPattern().matches(presenceAddressPattern))
                presenceCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
                
            if (message.getAddressPattern().matches(delayAddressPattern))
                delayCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            if (message.getAddressPattern().matches(delayWetLevelAddressPattern))
                delayWetLevelCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            if (message.getAddressPattern().matches(delayTimeAddressPattern))
                delayTimeCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            if (message.getAddressPattern().matches(delayFeedbackAddressPattern))
                delayFeedbackCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
                
            if (message.getAddressPattern().matches(chorusAddressPattern))
                chorusCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            if (message.getAddressPattern().matches(chorusMixAddressPattern))
                chorusMixCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            if (message.getAddressPattern().matches(chorusRateAddressPattern))
                chorusRateCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            if (message.getAddressPattern().matches(chorusDepthAddressPattern))
                chorusDepthCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            if (message.getAddressPattern().matches(chorusCentreDelayAddressPattern))
                chorusCentreDelayCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            if (message.getAddressPattern().matches(chorusFeedbackAddressPattern))
                chorusFeedbackCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
                
            if (message.getAddressPattern().matches(flangerAddressPattern))
                flangerCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            if (message.getAddressPattern().matches(flangerMixAddressPattern))
                flangerMixCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            if (message.getAddressPattern().matches(flangerRateAddressPattern))
                flangerRateCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            if (message.getAddressPattern().matches(flangerDepthAddressPattern))
                flangerDepthCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            if (message.getAddressPattern().matches(flangerCentreDelayAddressPattern))
                flangerCentreDelayCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            if (message.getAddressPattern().matches(flangerFeedbackAddressPattern))
                flangerFeedbackCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
                
            if (message.getAddressPattern().matches(reverbAddressPattern))
                reverbCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            if (message.getAddressPattern().matches(reverbWetLevelAddressPattern))
                reverbWetLevelCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            if (message.getAddressPattern().matches(reverbDampingAddressPattern))
                reverbDampingCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));
            if (message.getAddressPattern().matches(reverbRoomSizeAddressPattern))
                reverbRoomSizeCallback(jlimit(0.0f, 1.0f, message[0].getFloat32()));

            if (message.getAddressPattern().matches(ampStateAddressPattern))
                ampStateCallback(message[0].getFloat32() > 0.5f);
            if (message.getAddressPattern().matches(lstmStateAddressPattern))
                lstmStateCallback(message[0].getFloat32() > 0.5f);
            if (message.getAddressPattern().matches(irStateAddressPattern))
                irStateCallback(message[0].getFloat32() > 0.5f);
            if (message.getAddressPattern().matches(recordAddressPattern))
                recordCallback(message[0].getFloat32() > 0.5f);
        }

        if (message.size() == 1 && message[0].isInt32())
        {
            if (message.getAddressPattern().matches(gainAddressPattern))
                gainCallback(jlimit(0, 1, message[0].getInt32()));
            if (message.getAddressPattern().matches(masterAddressPattern))
                masterCallback(jlimit(0, 1, message[0].getInt32()));
            if (message.getAddressPattern().matches(bassAddressPattern))
                bassCallback(jlimit(0, 1, message[0].getInt32()));
            if (message.getAddressPattern().matches(midAddressPattern))
                midCallback(jlimit(0, 1, message[0].getInt32()));
            if (message.getAddressPattern().matches(trebleAddressPattern))
                trebleCallback(jlimit(0, 1, message[0].getInt32()));
            if (message.getAddressPattern().matches(presenceAddressPattern))
                presenceCallback(jlimit(0, 1, message[0].getInt32()));
                
            if (message.getAddressPattern().matches(delayAddressPattern))
                delayCallback(jlimit(0, 1, message[0].getInt32()));
            if (message.getAddressPattern().matches(delayWetLevelAddressPattern))
                delayWetLevelCallback(jlimit(0, 1, message[0].getInt32()));
            if (message.getAddressPattern().matches(delayTimeAddressPattern))
                delayTimeCallback(jlimit(0, 1, message[0].getInt32()));
            if (message.getAddressPattern().matches(delayFeedbackAddressPattern))
                delayFeedbackCallback(jlimit(0, 1, message[0].getInt32()));
                
            if (message.getAddressPattern().matches(chorusAddressPattern))
                chorusCallback(jlimit(0, 1, message[0].getInt32()));
            if (message.getAddressPattern().matches(chorusMixAddressPattern))
                chorusMixCallback(jlimit(0, 1, message[0].getInt32()));
            if (message.getAddressPattern().matches(chorusRateAddressPattern))
                chorusRateCallback(jlimit(0, 1, message[0].getInt32()));
            if (message.getAddressPattern().matches(chorusDepthAddressPattern))
                chorusDepthCallback(jlimit(0, 1, message[0].getInt32()));
            if (message.getAddressPattern().matches(chorusCentreDelayAddressPattern))
                chorusCentreDelayCallback(jlimit(0, 1, message[0].getInt32()));
            if (message.getAddressPattern().matches(chorusFeedbackAddressPattern))
                chorusFeedbackCallback(jlimit(0, 1, message[0].getInt32()));
                
            if (message.getAddressPattern().matches(flangerAddressPattern))
                flangerCallback(jlimit(0, 1, message[0].getInt32()));
            if (message.getAddressPattern().matches(flangerMixAddressPattern))
                flangerMixCallback(jlimit(0, 1, message[0].getInt32()));
            if (message.getAddressPattern().matches(flangerRateAddressPattern))
                flangerRateCallback(jlimit(0, 1, message[0].getInt32()));
            if (message.getAddressPattern().matches(flangerDepthAddressPattern))
                flangerDepthCallback(jlimit(0, 1, message[0].getInt32()));
            if (message.getAddressPattern().matches(flangerCentreDelayAddressPattern))
                flangerCentreDelayCallback(jlimit(0, 1, message[0].getInt32()));
            if (message.getAddressPattern().matches(flangerFeedbackAddressPattern))
                flangerFeedbackCallback(jlimit(0, 1, message[0].getInt32()));
                
            if (message.getAddressPattern().matches(reverbAddressPattern))
                reverbCallback(jlimit(0, 1, message[0].getInt32()));
            if (message.getAddressPattern().matches(reverbWetLevelAddressPattern))
                reverbWetLevelCallback(jlimit(0, 1, message[0].getInt32()));
            if (message.getAddressPattern().matches(reverbDampingAddressPattern))
                reverbDampingCallback(jlimit(0, 1, message[0].getInt32()));
            if (message.getAddressPattern().matches(reverbRoomSizeAddressPattern))
                reverbRoomSizeCallback(jlimit(0, 1, message[0].getInt32()));

            if (message.getAddressPattern().matches(ampStateAddressPattern))
                ampStateCallback(message[0].getInt32() == 1);
            if (message.getAddressPattern().matches(lstmStateAddressPattern))
                lstmStateCallback(message[0].getInt32() == 1);
            if (message.getAddressPattern().matches(irStateAddressPattern))
                irStateCallback(message[0].getInt32() == 1);
            if (message.getAddressPattern().matches(recordAddressPattern))
                recordCallback(message[0].getInt32() == 1);
        }
    }

public:
    std::function<void(float)> gainCallback;
    std::function<void(float)> masterCallback;
    std::function<void(float)> bassCallback;
    std::function<void(float)> midCallback;
    std::function<void(float)> trebleCallback;
    std::function<void(float)> presenceCallback;

    std::function<void(float)> delayCallback;
    std::function<void(float)> delayWetLevelCallback;
    std::function<void(float)> delayTimeCallback;
    std::function<void(float)> delayFeedbackCallback;

    std::function<void(float)> chorusCallback;
    std::function<void(float)> chorusMixCallback;
    std::function<void(float)> chorusRateCallback;
    std::function<void(float)> chorusDepthCallback;
    std::function<void(float)> chorusCentreDelayCallback;
    std::function<void(float)> chorusFeedbackCallback;

    std::function<void(float)> flangerCallback;
    std::function<void(float)> flangerMixCallback;
    std::function<void(float)> flangerRateCallback;
    std::function<void(float)> flangerDepthCallback;
    std::function<void(float)> flangerCentreDelayCallback;
    std::function<void(float)> flangerFeedbackCallback;

    std::function<void(float)> reverbCallback;
    std::function<void(float)> reverbWetLevelCallback;
    std::function<void(float)> reverbDampingCallback;
    std::function<void(float)> reverbRoomSizeCallback;

    std::function<void(juce::String)> modelCallback;
    std::function<void(juce::String)> irCallback;

    std::function<void(bool)> ampStateCallback;
    std::function<void(bool)> lstmStateCallback;
    std::function<void(bool)> irStateCallback;
    std::function<void(bool)> recordCallback;

private:
    String gainAddressPattern;
    String masterAddressPattern;
    String bassAddressPattern;
    String midAddressPattern;
    String trebleAddressPattern;
    String presenceAddressPattern;

    String delayAddressPattern;
    String delayWetLevelAddressPattern;
    String delayTimeAddressPattern;
    String delayFeedbackAddressPattern;

    String chorusAddressPattern;
    String chorusMixAddressPattern;
    String chorusRateAddressPattern;
    String chorusDepthAddressPattern;
    String chorusCentreDelayAddressPattern;
    String chorusFeedbackAddressPattern;

    String flangerAddressPattern;
    String flangerMixAddressPattern;
    String flangerRateAddressPattern;
    String flangerDepthAddressPattern;
    String flangerCentreDelayAddressPattern;
    String flangerFeedbackAddressPattern;

    String reverbAddressPattern;
    String reverbWetLevelAddressPattern;
    String reverbDampingAddressPattern;
    String reverbRoomSizeAddressPattern;

    String modelAddressPattern;
    String irAddressPattern;

    String ampStateAddressPattern;
    String lstmStateAddressPattern;
    String irStateAddressPattern;
    String recordAddressPattern;

}; //end class AmpOSCReceiver