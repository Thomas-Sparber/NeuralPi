/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include <iostream>
#include <fstream>

//==============================================================================
NeuralPiAudioProcessor::NeuralPiAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", AudioChannelSet::stereo(), true)
#endif
    ),
    apvts (*this, nullptr, "Parameters", createParameters())

#endif
{
    setupDataDirectories();
    installTones();
    resetDirectory(userAppDataDirectory_tones);
    // Sort configFiles alphabetically
    std::sort(configFiles.begin(), configFiles.end());
    if (configFiles.size() > 0) {
        loadConfig(configFiles[model_index], neuralNetwork1);
    }

    resetDirectoryIR(userAppDataDirectory_irs);
    // Sort irFiles alphabetically
    std::sort(irFiles.begin(), irFiles.end());
    if (irFiles.size() > 0) {
        loadIR(irFiles[ir_index]);
    }

    oscReceiver.modelCallback = [&] (juce::String value) {
        bool found = false;

        for(size_t i = 0; i < configFiles.size(); i++) {
            if(value == configFiles[i].getFileNameWithoutExtension()) {
                float value = static_cast<float>(i) / configFiles.size();
                apvts.getParameter(MODEL_ID)->setValueNotifyingHost(value);
                found = true;
                break;
            }
        }

        if(!found)
        {
            File fullpath = userAppDataDirectory_tones.getFullPathName() + "/" + value + ".json";
            if(!fullpath.existsAsFile())fullpath = userAppDataDirectory_tones.getFullPathName() + "/" + value + ".nam";
            if(fullpath.existsAsFile())
            {
                configFiles.push_back(fullpath);
                float value = static_cast<float>(configFiles.size() - 1) / configFiles.size();
                apvts.getParameter(MODEL_ID)->setValueNotifyingHost(value);
            }
        }
    };

    oscReceiver.irCallback = [&] (juce::String value) {
        bool found = false;

        for(size_t i = 0; i < irFiles.size(); i++) {
            if(value == irFiles[i].getFileNameWithoutExtension()) {
                float value = static_cast<float>(i) / irFiles.size();
                apvts.getParameter(IR_ID)->setValueNotifyingHost(value);
                found = true;
                break;
            }
        }

        if(!found)
        {
            File fullpath = userAppDataDirectory_irs.getFullPathName() + "/" + value + ".wav";
            if(fullpath.existsAsFile())
            {
                irFiles.push_back(fullpath);
                float value = static_cast<float>(irFiles.size() - 1) / irFiles.size();
                apvts.getParameter(IR_ID)->setValueNotifyingHost(value);
            }
        }
    };

    oscReceiver.gainCallback =      [&] (float value) { apvts.getParameter(GAIN_ID)->setValueNotifyingHost(value); };
    oscReceiver.masterCallback =    [&] (float value) { apvts.getParameter(MASTER_ID)->setValueNotifyingHost(value); };
    oscReceiver.bassCallback =      [&] (float value) { apvts.getParameter(BASS_ID)->setValueNotifyingHost(value); };
    oscReceiver.midCallback =       [&] (float value) { apvts.getParameter(MID_ID)->setValueNotifyingHost(value); };
    oscReceiver.trebleCallback =    [&] (float value) { apvts.getParameter(TREBLE_ID)->setValueNotifyingHost(value); };
    oscReceiver.presenceCallback =  [&] (float value) { apvts.getParameter(PRESENCE_ID)->setValueNotifyingHost(value); };

    oscReceiver.delayCallback =         [&] (float value) { apvts.getParameter(DELAY_ID)->setValueNotifyingHost(value); };
    oscReceiver.delayWetLevelCallback = [&] (float value) { apvts.getParameter(DELAYWETLEVEL_ID)->setValueNotifyingHost(value); };
    oscReceiver.delayTimeCallback =     [&] (float value) { apvts.getParameter(DELAYTIME_ID)->setValueNotifyingHost(value); };
    oscReceiver.delayFeedbackCallback = [&] (float value) { apvts.getParameter(DELAYFEEDBACK_ID)->setValueNotifyingHost(value); };

    oscReceiver.chorusCallback =            [&] (float value) { apvts.getParameter(CHORUS_ID)->setValueNotifyingHost(value); };
    oscReceiver.chorusMixCallback =         [&] (float value) { apvts.getParameter(CHORUSMIX_ID)->setValueNotifyingHost(value); };
    oscReceiver.chorusRateCallback =        [&] (float value) { apvts.getParameter(CHORUSRATE_ID)->setValueNotifyingHost(value); };
    oscReceiver.chorusDepthCallback =       [&] (float value) { apvts.getParameter(CHORUSDEPTH_ID)->setValueNotifyingHost(value); };
    oscReceiver.chorusCentreDelayCallback = [&] (float value) { apvts.getParameter(CHORUSCENTREDELAY_ID)->setValueNotifyingHost(value); };
    oscReceiver.chorusFeedbackCallback =    [&] (float value) { apvts.getParameter(CHORUSFEEDBACK_ID)->setValueNotifyingHost(value); };

    oscReceiver.flangerCallback =            [&] (float value) { apvts.getParameter(FLANGER_ID)->setValueNotifyingHost(value); };
    oscReceiver.flangerMixCallback =         [&] (float value) { apvts.getParameter(FLANGERMIX_ID)->setValueNotifyingHost(value); };
    oscReceiver.flangerRateCallback =        [&] (float value) { apvts.getParameter(FLANGERRATE_ID)->setValueNotifyingHost(value); };
    oscReceiver.flangerDepthCallback =       [&] (float value) { apvts.getParameter(FLANGERDEPTH_ID)->setValueNotifyingHost(value); };
    oscReceiver.flangerCentreDelayCallback = [&] (float value) { apvts.getParameter(FLANGERCENTREDELAY_ID)->setValueNotifyingHost(value); };
    oscReceiver.flangerFeedbackCallback =    [&] (float value) { apvts.getParameter(FLANGERFEEDBACK_ID)->setValueNotifyingHost(value); };

    oscReceiver.reverbCallback =         [&] (float value) { apvts.getParameter(REVERB_ID)->setValueNotifyingHost(value); };
    oscReceiver.reverbWetLevelCallback = [&] (float value) { apvts.getParameter(REVERBWETLEVEL_ID)->setValueNotifyingHost(value); };
    oscReceiver.reverbDampingCallback =  [&] (float value) { apvts.getParameter(REVERBDAMPING_ID)->setValueNotifyingHost(value); };
    oscReceiver.reverbRoomSizeCallback = [&] (float value) { apvts.getParameter(REVERBROOMSIZE_ID)->setValueNotifyingHost(value); };

    oscReceiver.ampStateCallback =  [&] (bool value) { apvts.getParameter(AMPSTATE_ID)->setValueNotifyingHost(value ? 0.0f : 1.0f); };
    oscReceiver.lstmStateCallback = [&] (bool value) { apvts.getParameter(LSTMSTATE_ID)->setValueNotifyingHost(value ? 0.0f : 1.0f); };
    oscReceiver.irStateCallback =   [&] (bool value) { apvts.getParameter(IRSTATE_ID)->setValueNotifyingHost(value ? 0.0f : 1.0f); };
    oscReceiver.recordCallback =   [&] (bool value) { apvts.getParameter(RECORD_ID)->setValueNotifyingHost(value ? 0.0f : 1.0f); };

    apvts.addParameterListener (MODEL_ID, this);
    apvts.addParameterListener (IR_ID, this);

    apvts.addParameterListener (GAIN_ID, this);
    apvts.addParameterListener (MASTER_ID, this);
    apvts.addParameterListener (BASS_ID, this);
    apvts.addParameterListener (MID_ID, this);
    apvts.addParameterListener (TREBLE_ID, this);
    apvts.addParameterListener (PRESENCE_ID, this);

    apvts.addParameterListener (DELAY_ID, this);
    apvts.addParameterListener (DELAYWETLEVEL_ID, this);
    apvts.addParameterListener (DELAYTIME_ID, this);
    apvts.addParameterListener (DELAYFEEDBACK_ID, this);

    apvts.addParameterListener (CHORUS_ID, this);
    apvts.addParameterListener (CHORUSMIX_ID, this);
    apvts.addParameterListener (CHORUSRATE_ID, this);
    apvts.addParameterListener (CHORUSDEPTH_ID, this);
    apvts.addParameterListener (CHORUSCENTREDELAY_ID, this);
    apvts.addParameterListener (CHORUSFEEDBACK_ID, this);

    apvts.addParameterListener (FLANGER_ID, this);
    apvts.addParameterListener (FLANGERMIX_ID, this);
    apvts.addParameterListener (FLANGERRATE_ID, this);
    apvts.addParameterListener (FLANGERDEPTH_ID, this);
    apvts.addParameterListener (FLANGERCENTREDELAY_ID, this);
    apvts.addParameterListener (FLANGERFEEDBACK_ID, this);

    apvts.addParameterListener (REVERB_ID, this);
    apvts.addParameterListener (REVERBWETLEVEL_ID, this);
    apvts.addParameterListener (REVERBDAMPING_ID, this);
    apvts.addParameterListener (REVERBROOMSIZE_ID, this);

    apvts.addParameterListener (AMPSTATE_ID, this);
    apvts.addParameterListener (LSTMSTATE_ID, this);
    apvts.addParameterListener (IRSTATE_ID, this);
    apvts.addParameterListener (RECORD_ID, this);
}

juce::AudioProcessorValueTreeState::ParameterLayout NeuralPiAudioProcessor::createParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout params;

    // initialize parameters:
    params.add (std::make_unique<AudioParameterFloat>(MODEL_ID,     MODEL_NAME,     NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.0f));
    params.add (std::make_unique<AudioParameterFloat>(IR_ID,        IR_NAME,        NormalisableRange<float>(0.0f, 1.0f, 0.0001f), 0.0f));
    
    params.add (std::make_unique<AudioParameterFloat>(GAIN_ID,      GAIN_NAME,      NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    params.add (std::make_unique<AudioParameterFloat>(MASTER_ID,    MASTER_NAME,    NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    params.add (std::make_unique<AudioParameterFloat>(BASS_ID,      BASS_NAME,      NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    params.add (std::make_unique<AudioParameterFloat>(MID_ID,       MID_NAME,       NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    params.add (std::make_unique<AudioParameterFloat>(TREBLE_ID,    TREBLE_NAME,    NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    params.add (std::make_unique<AudioParameterFloat>(PRESENCE_ID,  PRESENCE_NAME,  NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.5f));
    
    params.add (std::make_unique<AudioParameterFloat>(DELAY_ID,         DELAY_NAME,         NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.add (std::make_unique<AudioParameterFloat>(DELAYWETLEVEL_ID, DELAYWETLEVEL_NAME, NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.add (std::make_unique<AudioParameterFloat>(DELAYTIME_ID,     DELAYTIME_NAME,     NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.add (std::make_unique<AudioParameterFloat>(DELAYFEEDBACK_ID, DELAYFEEDBACK_NAME, NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

    params.add (std::make_unique<AudioParameterFloat>(CHORUS_ID,            CHORUS_NAME,            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.add (std::make_unique<AudioParameterFloat>(CHORUSMIX_ID,         CHORUSMIX_NAME,         NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.add (std::make_unique<AudioParameterFloat>(CHORUSRATE_ID,        CHORUSRATE_NAME,        NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.add (std::make_unique<AudioParameterFloat>(CHORUSDEPTH_ID,       CHORUSDEPTH_NAME,       NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.add (std::make_unique<AudioParameterFloat>(CHORUSCENTREDELAY_ID, CHORUSCENTREDELAY_NAME, NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.add (std::make_unique<AudioParameterFloat>(CHORUSFEEDBACK_ID,    CHORUSFEEDBACK_NAME,    NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    params.add (std::make_unique<AudioParameterFloat>(FLANGER_ID,            FLANGER_NAME,            NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.add (std::make_unique<AudioParameterFloat>(FLANGERMIX_ID,         FLANGERMIX_NAME,         NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.add (std::make_unique<AudioParameterFloat>(FLANGERRATE_ID,        FLANGERRATE_NAME,        NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.add (std::make_unique<AudioParameterFloat>(FLANGERDEPTH_ID,       FLANGERDEPTH_NAME,       NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.add (std::make_unique<AudioParameterFloat>(FLANGERCENTREDELAY_ID, FLANGERCENTREDELAY_NAME, NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));
    params.add (std::make_unique<AudioParameterFloat>(FLANGERFEEDBACK_ID,    FLANGERFEEDBACK_NAME,    NormalisableRange<float>(0.0f, 1.0f, 0.01f), 0.0f));

    params.add (std::make_unique<AudioParameterFloat>(REVERB_ID,         REVERB_NAME,         NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.add (std::make_unique<AudioParameterFloat>(REVERBWETLEVEL_ID, REVERBWETLEVEL_NAME, NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.add (std::make_unique<AudioParameterFloat>(REVERBDAMPING_ID,  REVERBDAMPING_NAME,  NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.add (std::make_unique<AudioParameterFloat>(REVERBROOMSIZE_ID, REVERBROOMSIZE_NAME, NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

    params.add (std::make_unique<AudioParameterFloat>(AMPSTATE_ID,  AMPSTATE_NAME,  NormalisableRange<float>(0.0f, 1.0f, 1.0f), 1.0f));
    params.add (std::make_unique<AudioParameterFloat>(LSTMSTATE_ID, LSTMSTATE_NAME, NormalisableRange<float>(0.0f, 1.0f, 1.0f), 1.0f));
    params.add (std::make_unique<AudioParameterFloat>(IRSTATE_ID,   IRSTATE_NAME,   NormalisableRange<float>(0.0f, 1.0f, 1.0f), 1.0f));
    params.add (std::make_unique<AudioParameterFloat>(RECORD_ID,    RECORD_NAME,    NormalisableRange<float>(0.0f, 1.0f, 1.0f), 1.0f));

    return params;
}

void NeuralPiAudioProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    //std::ofstream out("/tmp/debug", std::ios::app);
    //out<<"parameter changed "<<parameterID<<": "<<newValue<<std::endl;

    
        
    if (parameterID == MODEL_ID)
    {
        model_index = jlimit(0, static_cast<int>(configFiles.size()-1), static_cast<int>(newValue * configFiles.size() + 0.5f));
        changeModel(configFiles[model_index]);
    }
    if (parameterID == IR_ID)
    {
        ir_index = jlimit(0, static_cast<int>(irFiles.size()-1), static_cast<int>(newValue * irFiles.size() + 0.5f));
        loadIR(irFiles[ir_index]);
    }
    if (parameterID == GAIN_ID)
        gain = newValue;
    if (parameterID == MASTER_ID)
        master = newValue;
    if (parameterID == BASS_ID)
    {
        float bass = (newValue - 0.5) * 24.0;
        eq4band.setBass(bass);
    }
    if (parameterID == MID_ID)
    {
        float mid = (newValue - 0.5) * 24.0;
        eq4band.setMid(mid);
    }
    if (parameterID == TREBLE_ID)
    {
        float treble = (newValue - 0.5) * 24.0;
        eq4band.setTreble(treble);
    }
    if (parameterID == PRESENCE_ID)
    {
        float presence = (newValue - 0.5) * 24.0;
        eq4band.setPresence(presence);
    }

    if (parameterID == DELAY_ID)
        set_delayParams(newValue);
    if (parameterID == DELAYWETLEVEL_ID)
        delay.setWetLevel(newValue);
    if (parameterID == DELAYTIME_ID)
        delay.setDelayTime(0, newValue);
    if (parameterID == DELAYFEEDBACK_ID)
        delay.setFeedback(newValue);

    if (parameterID == CHORUS_ID)
        set_chorusParams(newValue);
    if (parameterID == CHORUSMIX_ID)
        chorus.setMix(newValue);
    if (parameterID == CHORUSRATE_ID)
        chorus.setRate(static_cast<int>(newValue * 99));
    if (parameterID == CHORUSDEPTH_ID)
        chorus.setDepth(newValue);
    if (parameterID == CHORUSCENTREDELAY_ID)
        chorus.setCentreDelay(static_cast<int>(1 + newValue * 99));
    if (parameterID == CHORUSFEEDBACK_ID)
        chorus.setFeedback(newValue * 2 - 1);

    if (parameterID == FLANGER_ID)
        set_flangerParams(newValue);
    if (parameterID == FLANGERMIX_ID)
        flanger.setMix(newValue);
    if (parameterID == FLANGERRATE_ID)
        flanger.setRate(static_cast<int>(newValue * 99));
    if (parameterID == FLANGERDEPTH_ID)
        flanger.setDepth(newValue);
    if (parameterID == FLANGERCENTREDELAY_ID)
        flanger.setCentreDelay(static_cast<int>(1 + newValue * 99));
    if (parameterID == FLANGERFEEDBACK_ID)
        flanger.setFeedback(newValue * 2 - 1);

    if (parameterID == REVERB_ID)
        set_reverbParams(newValue);
    if (parameterID == REVERBWETLEVEL_ID)
    {
        auto rev_params = reverb.getParameters();
        rev_params.wetLevel = newValue;
        reverb.setParameters(rev_params);
    }
    if (parameterID == REVERBDAMPING_ID)
    {
        auto rev_params = reverb.getParameters();
        rev_params.damping = newValue;
        reverb.setParameters(rev_params);
    }
    if (parameterID == REVERBROOMSIZE_ID)
    {
        auto rev_params = reverb.getParameters();
        rev_params.roomSize = newValue;
        reverb.setParameters(rev_params);
    }

    if (parameterID == AMPSTATE_ID)
        ampState = newValue >= 0.5f;
    if (parameterID == LSTMSTATE_ID)
        lstmState = newValue >= 0.5f;
    if (parameterID == IRSTATE_ID)
        irState = newValue >= 0.5f;
    if (parameterID == RECORD_ID)
        recording = newValue >= 0.5f;
}


NeuralPiAudioProcessor::~NeuralPiAudioProcessor()
{
    apvts.removeParameterListener(MODEL_ID, this);
    apvts.removeParameterListener(IR_ID, this);

    apvts.removeParameterListener(GAIN_ID, this);
    apvts.removeParameterListener(MASTER_ID, this);
    apvts.removeParameterListener(BASS_ID, this);
    apvts.removeParameterListener(MID_ID, this);
    apvts.removeParameterListener(TREBLE_ID, this);
    apvts.removeParameterListener(PRESENCE_ID, this);
    
    apvts.removeParameterListener(DELAY_ID, this);
    apvts.removeParameterListener(DELAYWETLEVEL_ID, this);
    apvts.removeParameterListener(DELAYTIME_ID, this);
    apvts.removeParameterListener(DELAYFEEDBACK_ID, this);

    apvts.removeParameterListener(CHORUS_ID, this);
    apvts.removeParameterListener(CHORUSMIX_ID, this);
    apvts.removeParameterListener(CHORUSRATE_ID, this);
    apvts.removeParameterListener(CHORUSDEPTH_ID, this);
    apvts.removeParameterListener(CHORUSCENTREDELAY_ID, this);
    apvts.removeParameterListener(CHORUSFEEDBACK_ID, this);
    
    apvts.removeParameterListener(FLANGER_ID, this);
    apvts.removeParameterListener(FLANGERMIX_ID, this);
    apvts.removeParameterListener(FLANGERRATE_ID, this);
    apvts.removeParameterListener(FLANGERDEPTH_ID, this);
    apvts.removeParameterListener(FLANGERCENTREDELAY_ID, this);
    apvts.removeParameterListener(FLANGERFEEDBACK_ID, this);

    apvts.removeParameterListener(REVERB_ID, this);
    apvts.removeParameterListener(REVERBWETLEVEL_ID, this);
    apvts.removeParameterListener(REVERBDAMPING_ID, this);
    apvts.removeParameterListener(REVERBROOMSIZE_ID, this);

    apvts.removeParameterListener(AMPSTATE_ID, this);
    apvts.removeParameterListener(LSTMSTATE_ID, this);
    apvts.removeParameterListener(IRSTATE_ID, this);
    apvts.removeParameterListener(RECORD_ID, this);
}

//==============================================================================
const String NeuralPiAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NeuralPiAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool NeuralPiAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool NeuralPiAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double NeuralPiAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NeuralPiAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int NeuralPiAudioProcessor::getCurrentProgram()
{
    return 0;
}

void NeuralPiAudioProcessor::setCurrentProgram (int /*index*/)
{
}

const String NeuralPiAudioProcessor::getProgramName (int /*index*/)
{
    return {};
}

void NeuralPiAudioProcessor::changeProgramName (int /*index*/, const String& /*newName*/)
{
}

//==============================================================================
void NeuralPiAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    // set up DC blocker
    dcBlocker.coefficients = dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 35.0f);
    dsp::ProcessSpec spec{ sampleRate, static_cast<uint32> (samplesPerBlock), 2 };
    dcBlocker.prepare(spec);

    constexpr double targetSampleRate = 44100.0;
    resampler.prepareWithTargetSampleRate({ sampleRate, (uint32)samplesPerBlock, 1 }, targetSampleRate);

    //std::ofstream of("/tmp/debug",std::ios_base::ate);
    //of<<"Downsampling from "<<sampleRate<<" to "<<targetSampleRate<<std::endl;

    // Set up IR
    cabSimIR1.prepare(spec);
    cabSimIR2.prepare(spec);

    neuralNetwork1.reset();
    neuralNetwork2.reset();

    // fx chain
    delay.prepare(spec);
    reverb.prepare(spec);
    chorus.prepare(spec);
    flanger.prepare(spec);
}

void NeuralPiAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool NeuralPiAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif


void NeuralPiAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

    // Setup Audio Data
    const int numSamples = buffer.getNumSamples();
    const int numInputChannels = getTotalNumInputChannels();
    const int sampleRate = getSampleRate();

    dsp::AudioBlock<float> block = dsp::AudioBlock<float>(buffer).getSingleChannelBlock(0);
    dsp::ProcessContextReplacing<float> context(block);

    // Amp =============================================================================
    if (ampState) {
        if (model_loaded && lstmState) {
            if(currentNeuralNetwork == 0)
            {
                // Applying gain adjustment for snapshot models
                if (neuralNetwork1.input_size == 1) {
                    //buffer.applyGain(gain * 2.0); //TODO check
                    buffer.applyGain(0, 0, numSamples, gain * 2.5);
                } else {
                    buffer.applyGain(0, 0, numSamples, 1.5);
                }

                //auto block44k = resampler.processIn(block);

                auto readPointer = buffer.getReadPointer(0);
                auto writePointer = buffer.getWritePointer(0);

                //auto readPointer = block44k.getReadPointer(0);
                //auto writePointer = block44k.getWritePointer(0);

                neuralNetwork1.process(readPointer, gain, master, writePointer, numSamples);

                //resampler.processOut(block44k, block);
            }
            else
            {
                // Applying gain adjustment for snapshot models
                if (neuralNetwork2.input_size == 1) {
                    //buffer.applyGain(gain * 2.0); //TODO check
                    buffer.applyGain(0, 0, numSamples, gain * 2.5);
                } else {
                    buffer.applyGain(0, 0, numSamples, 1.5);
                }

                //auto block44k = resampler.processIn(block);

                auto readPointer = buffer.getReadPointer(0);
                auto writePointer = buffer.getWritePointer(0);

                //auto readPointer = block44k.getReadPointer(0);
                //auto writePointer = block44k.getWritePointer(0);

                neuralNetwork2.process(readPointer, gain, master, writePointer, numSamples);

                //resampler.processOut(block44k, block);
            }

        }

        dcBlocker.process(context);

        eq4band.process(buffer.getReadPointer(0), buffer.getWritePointer(0), midiMessages, numSamples, numInputChannels, sampleRate);

        // Process Delay, Reverb, Chorus and Flanger
        //if(delayValue < 0.01)
        //    context.isBypassed = true;
        //set_delayParams(delayValue);
        delay.process(context);
        //context.isBypassed = false;

        //if(chorusValue < 0.01)
        //    context.isBypassed = true;
        //set_chorusParams(chorusValue);
        chorus.process(context);
        //context.isBypassed = false;

        //if(flangerValue < 0.01)
        //    context.isBypassed = true;
        //set_flangerParams(flangerValue);
        flanger.process(context);
        //context.isBypassed = false;

        //if(reverbValue < 0.01)
        //    context.isBypassed = true;
        //set_reverbParams(reverbValue);
        reverb.process(context);
        //context.isBypassed = false;

        //    Master Volume 
		if (currentNeuralNetwork == 0 && (neuralNetwork1.input_size == 1 || neuralNetwork1.input_size == 2) || currentNeuralNetwork == 1 && (neuralNetwork2.input_size == 1 || neuralNetwork2.input_size == 2)) {
			buffer.applyGain(0, 0, numSamples, master * 2.0); // Adding volume range (2x) mainly for clean models
		}

        // Process IR
        if (ir_loaded && irState) {
            if(currentIR == 0)
            {
                cabSimIR1.process(context);
            }
            else
            {
                cabSimIR2.process(context);
            }

            // IR generally makes output quieter, add volume here to make ir on/off volume more even
            buffer.applyGain(0, 0, numSamples, 2.0);
        }
    }

    //Calculate averaged RMS over the last 10 seconds
    float currentRMS = buffer.getRMSLevel(1, 0, numSamples);
    float currentBufferDurationSeconds = static_cast<float>(numSamples) / sampleRate;
    averagedRMS = (averagedRMS * 10 + currentRMS * currentBufferDurationSeconds) / (10 + currentBufferDurationSeconds);

    //std::ofstream out("/tmp/debug", std::ios::app);
    //out<<"current: "<<currentRMS<<"\tduration: "<<currentBufferDurationSeconds<<"\taveraged: "<<averagedRMS<<std::endl;

    if(averagedRMS <= 0.1f && currentRMS < 0.3f) {
        buffer.copyFrom(1, 0, buffer, 0, 0, numSamples);
    } else {
        averagedRMS = std::max(averagedRMS, 0.1f);

        // store the sum in the left
        buffer.addFrom(0, 0, buffer, 1, 0, buffer.getNumSamples());

        // copy the combined left (0) to the right (1)
        buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());

        // apply 0.5 gain to both
        buffer.applyGain(0.5f);
    }

    if(recording)
    {
        if(!writerOutput)
        {
            WavAudioFormat format;
            outstreamOutput = std::make_unique<FileOutputStream>(juce::File("/udata/libs/output.wav"));
            writerOutput.reset(format.createWriterFor(outstreamOutput.get(), sampleRate, buffer.getNumChannels(), 24, {}, 0));
        }

        writerOutput->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
    }

    if(!recording && writerOutput)
    {
        writerOutput = nullptr;
        outstreamOutput = nullptr;

    }
}

//==============================================================================
void NeuralPiAudioProcessor::getStateInformation(MemoryBlock& destData)
{
    juce::ValueTree copyState = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml = copyState.createXml();
    copyXmlToBinary (*xml.get(), destData);
}

void NeuralPiAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml = getXmlFromBinary (data, sizeInBytes);
    juce::ValueTree copyState = juce::ValueTree::fromXml (*xml.get());
    apvts.replaceState (copyState);
}

void NeuralPiAudioProcessor::changeModel(File configFile)
{
    if(currentNeuralNetwork == 0)
    {
        loadConfig(configFile, neuralNetwork2);
        currentNeuralNetwork = 1;
    }
    else
    {
        loadConfig(configFile, neuralNetwork1);
        currentNeuralNetwork = 0;
    }
}

void NeuralPiAudioProcessor::loadConfig(File configFile, NeuralNetwork &out)
{
    try {
        out.reset();

        // Load the config file into the correct model
        out.loadConfig(configFile.getFullPathName());
        model_loaded = true;
    }
    catch (const std::exception& e) {
        DBG("Unable to load config file: " + configFile.getFullPathName());
        std::cout << e.what();
    }
}

void NeuralPiAudioProcessor::loadIR(File irFile)
{
    try {
        if(currentIR == 0)
        {
            cabSimIR2.load(irFile);
            currentIR = 1;
        }
        else
        {
            cabSimIR1.load(irFile);
            currentIR = 0;
        }

        ir_loaded = true;
    }
    catch (const std::exception& e) {
        DBG("Unable to load IR file: " + irFile.getFullPathName());
        std::cout << e.what();
    }
}

void NeuralPiAudioProcessor::resetDirectory(const File& file)
{
    configFiles.clear();
    if (file.isDirectory())
    {
        juce::Array<juce::File> results;
        file.findChildFiles(results, juce::File::findFiles, false, "*.json");
        for (int i = results.size(); --i >= 0;)
            configFiles.push_back(File(results.getReference(i).getFullPathName()));
    }
}

void NeuralPiAudioProcessor::resetDirectoryIR(const File& file)
{
    irFiles.clear();
    if (file.isDirectory())
    {
        juce::Array<juce::File> results;
        file.findChildFiles(results, juce::File::findFiles, false, "*.wav");
        for (int i = results.size(); --i >= 0;)
            irFiles.push_back(File(results.getReference(i).getFullPathName()));
    }
}

void NeuralPiAudioProcessor::setupDataDirectories()
{
    // User app data directory
    File userAppDataTempFile = userAppDataDirectory.getChildFile("tmp.pdl");

    File userAppDataTempFile_tones = userAppDataDirectory_tones.getChildFile("tmp.pdl");

    File userAppDataTempFile_irs = userAppDataDirectory_irs.getChildFile("tmp.pdl");

    // Create (and delete) temp file if necessary, so that user doesn't have
    // to manually create directories
    if (!userAppDataDirectory.exists()) {
        userAppDataTempFile.create();
    }
    if (userAppDataTempFile.existsAsFile()) {
        userAppDataTempFile.deleteFile();
    }

    if (!userAppDataDirectory_tones.exists()) {
        userAppDataTempFile_tones.create();
    }
    if (userAppDataTempFile_tones.existsAsFile()) {
        userAppDataTempFile_tones.deleteFile();
    }

    if (!userAppDataDirectory_irs.exists()) {
        userAppDataTempFile_irs.create();
    }
    if (userAppDataTempFile_irs.existsAsFile()) {
        userAppDataTempFile_irs.deleteFile();
    }
}

void NeuralPiAudioProcessor::installTones()
//====================================================================
// Description: Checks that the default tones
//  are installed to the NeuralPi directory, and if not, 
//  copy them from the binary data in the plugin to that directory.
//
//====================================================================
{
    // Default tones
    File ts9_tone = userAppDataDirectory_tones.getFullPathName() + "/TS9.json";
    File bjdirty_tone = userAppDataDirectory_tones.getFullPathName() + "/BluesJR.json";
    File ht40od_tone = userAppDataDirectory_tones.getFullPathName() + "/HT40_Overdrive.json";

    if (ts9_tone.existsAsFile() == false) {
        std::string string_command = ts9_tone.getFullPathName().toStdString();
        const char* char_ts9_tone = &string_command[0];

        std::ofstream myfile;
        myfile.open(char_ts9_tone);
        myfile << BinaryData::TS9_json;

        myfile.close();
    }

    if (bjdirty_tone.existsAsFile() == false) {
        std::string string_command = bjdirty_tone.getFullPathName().toStdString();
        const char* char_bjdirty = &string_command[0];

        std::ofstream myfile;
        myfile.open(char_bjdirty);
        myfile << BinaryData::BluesJr_json;

        myfile.close();
    }

    if (ht40od_tone.existsAsFile() == false) {
        std::string string_command = ht40od_tone.getFullPathName().toStdString();
        const char* char_ht40od = &string_command[0];

        std::ofstream myfile;
        myfile.open(char_ht40od);
        myfile << BinaryData::HT40_Overdrive_json;

        myfile.close();
    }
    
}

void NeuralPiAudioProcessor::set_delayParams(float paramValue)
{
    //auto& del = fxChain.template get<delayIndex>();
    delay.setWetLevel(paramValue);
    // Setting delay time as larger steps to minimize clicking, and to start delay time at a reasonable value
    if (paramValue < 0.25) {
        delay.setDelayTime(0, 0.25);
    } else if (paramValue < 0.5) {
        delay.setDelayTime(0, 0.5);
    } else if (paramValue < 0.75) {
        delay.setDelayTime(0, 0.75);
    } else {
        delay.setDelayTime(0, 1.0);
    }
    delay.setFeedback(0.8-paramValue/2);
}


void NeuralPiAudioProcessor::set_reverbParams(float paramValue)
{
    //auto& rev = fxChain.template get<reverbIndex>();
    auto rev_params = reverb.getParameters();

    // Sets reverb params as a function of a single reverb param value ( 0.0 to 1.0)
    rev_params.wetLevel = paramValue;
    rev_params.damping = 0.6 - paramValue/2; // decay is inverse of damping
    rev_params.roomSize = 0.8 - paramValue/2;
    //rev_params.width = paramValue;
    reverb.setParameters(rev_params);
}

void NeuralPiAudioProcessor::set_chorusParams(float paramValue)
{
    //auto& ch = fxChain.template get<chorusIndex>();

    // Sets chorus params as a function of a single chorus param value ( 0.0 to 1.0)
    chorus.setMix(paramValue);
    chorus.setRate(50); // 0 - 99
    chorus.setDepth(0.1);  //0.0f - 1.0f
    chorus.setCentreDelay(8);  //1 - 100
    chorus.setFeedback(0.1);  //-1.0f - 1.0f
}

void NeuralPiAudioProcessor::set_flangerParams(float paramValue)
{
    //auto& ch = fxChain.template get<chorusIndex>();

    // Sets flanger params as a function of a single chorus param value ( 0.0 to 1.0)
    flanger.setMix(paramValue);
    flanger.setRate(50); // 0 - 99
    flanger.setDepth(0.1);  //0.0f - 1.0f
    flanger.setCentreDelay(2);  //1 - 100
    flanger.setFeedback(1);  //-1.0f - 1.0f
}


//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NeuralPiAudioProcessor();
}
