/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include <chowdsp_dsp_utils/chowdsp_dsp_utils.h>
#include <nlohmann/json.hpp>

#include "NeuralNetwork.h"
#include "Eq4Band.h"
#include "CabSim.h"
#include "Delay.h"
#include "AmpOSCReceiver.h"

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"

#define MODEL_ID "model"
#define MODEL_NAME "Model"
#define IR_ID "ir"
#define IR_NAME "Ir"
#define IRWETLEVEL_ID "irWetLevel"
#define IRWETLEVEL_NAME "IrWetLevel"

#define GAIN_ID "gain"
#define GAIN_NAME "Gain"
#define MASTER_ID "master"
#define MASTER_NAME "Master"
#define BASS_ID "bass"
#define BASS_NAME "Bass"
#define MID_ID "mid"
#define MID_NAME "Mid"
#define TREBLE_ID "treble"
#define TREBLE_NAME "Treble"
#define PRESENCE_ID "presence"
#define PRESENCE_NAME "Presence"

#define DELAY_ID "delay"
#define DELAY_NAME "Delay"
#define DELAYWETLEVEL_ID "delayWetLevel"
#define DELAYWETLEVEL_NAME "DelayWetLevel"
#define DELAYTIME_ID "delayTime"
#define DELAYTIME_NAME "DelayTime"
#define DELAYFEEDBACK_ID "delayFeedback"
#define DELAYFEEDBACK_NAME "DelayFeedback"

#define CHORUS_ID "chorus"
#define CHORUS_NAME "Chorus"
#define CHORUSMIX_ID "chorusMix"
#define CHORUSMIX_NAME "ChorusMix"
#define CHORUSRATE_ID "chorusRate"
#define CHORUSRATE_NAME "ChorusRate"
#define CHORUSDEPTH_ID "chorusDepth"
#define CHORUSDEPTH_NAME "ChorusDepth"
#define CHORUSCENTREDELAY_ID "chorusCentreDelay"
#define CHORUSCENTREDELAY_NAME "ChorusCentreDelay"
#define CHORUSFEEDBACK_ID "chorusFeedback"
#define CHORUSFEEDBACK_NAME "ChorusFeedback"

#define FLANGER_ID "flanger"
#define FLANGER_NAME "Flanger"
#define FLANGERMIX_ID "flangerMix"
#define FLANGERMIX_NAME "FlangerMix"
#define FLANGERRATE_ID "flangerRate"
#define FLANGERRATE_NAME "FlangerRate"
#define FLANGERDEPTH_ID "flangerDepth"
#define FLANGERDEPTH_NAME "FlangerDepth"
#define FLANGERCENTREDELAY_ID "flangerCentreDelay"
#define FLANGERCENTREDELAY_NAME "FlangerCentreDelay"
#define FLANGERFEEDBACK_ID "flangerFeedback"
#define FLANGERFEEDBACK_NAME "FlangerFeedback"

#define REVERB_ID "reverb"
#define REVERB_NAME "Reverb"
#define REVERBWETLEVEL_ID "reverbWetLevel"
#define REVERBWETLEVEL_NAME "ReverbWetLevel"
#define REVERBDAMPING_ID "reverbDamping"
#define REVERBDAMPING_NAME "ReverbDamping"
#define REVERBROOMSIZE_ID "reverbRoomSize"
#define REVERBROOMSIZE_NAME "ReverbRoomSize"

#define AMPSTATE_ID "ampState"
#define AMPSTATE_NAME "AmpState"
#define LSTMSTATE_ID "lstmState"
#define LSTMSTATE_NAME "LSTMState"
#define IRSTATE_ID "irState"
#define IRSTATE_NAME "IrState"
#define RECORD_ID "record"
#define RECORD_NAME "Record"

//==============================================================================
/**
*/
class NeuralPiAudioProcessor  : public AudioProcessor, public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    NeuralPiAudioProcessor();
    ~NeuralPiAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override
    {
      return nullptr;
    }

    bool hasEditor() const override
    {
      return false;
    }

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    AudioProcessorValueTreeState::ParameterLayout createParameters();
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void changeModel(File configFile);
    void loadConfig(File configFile, NeuralNetwork &out);
    void loadIR(File irFile);
    void setupDataDirectories();
    void installTones();
    void startRecording(File configFile);

    void set_delayParams(float paramValue);
    void set_reverbParams(float paramValue);
    void set_chorusParams(float paramValue);
    void set_flangerParams(float paramValue);

    void resetDirectory(const File& file);
    void resetDirectoryIR(const File& file);

    std::vector<File> configFiles;
    std::vector<File> irFiles;
    File userAppDataDirectory = File::getSpecialLocation(File::userDocumentsDirectory).getChildFile(JucePlugin_Manufacturer).getChildFile(JucePlugin_Name);
    File userAppDataDirectory_tones = userAppDataDirectory.getFullPathName() + "/tones";
    File userAppDataDirectory_irs = userAppDataDirectory.getFullPathName() + "/irs";

    float gain = 0.5f;
    float master = 0.5f;
    float delayValue = 0.0f;
    float chorusValue = 0.0f;
    float flangerValue = 0.0f;
    float reverbValue = 0.0f;

    bool ampState = true;
    bool lstmState = true;
    bool irState = true;

    // Pedal/amp states
    bool model_loaded = false;
    int model_index = 0;

    bool ir_loaded = false;
    int ir_index = 0;

    juce::AudioProcessorValueTreeState apvts;

    float averagedRMS = 0;

private:
    bool recording = false;
    TimeSliceThread backgroundThread { "Audio Recorder Thread" };
    std::unique_ptr<AudioFormatWriter::ThreadedWriter> threadedWriter;
    CriticalSection writerLock;
    std::atomic<AudioFormatWriter::ThreadedWriter*> activeWriter { nullptr };

    chowdsp::ResampledProcess<chowdsp::ResamplingTypes::LanczosResampler<>> resampler;

    std::atomic<int> currentNeuralNetwork = 0;
    NeuralNetwork neuralNetwork1;
    NeuralNetwork neuralNetwork2;

    Eq4Band eq4band; // Amp EQ

    dsp::IIR::Filter<float> dcBlocker;

    // IR processing
    std::atomic<int> currentIR;
    CabSim cabSimIR1;
    CabSim cabSimIR2;

    AmpOSCReceiver oscReceiver;

    Delay<float> delay;
    juce::dsp::Reverb reverb;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::Chorus<float> flanger;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeuralPiAudioProcessor)
};
