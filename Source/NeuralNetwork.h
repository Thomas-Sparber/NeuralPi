#pragma once

#include <RTNeural/RTNeural.h>

#define NAM_SAMPLE_FLOAT
#include <NAM/dsp.h>

#include "../JuceLibraryCode/JuceHeader.h"

enum class NeuralNetworkType {
    NeuralPi,
    Proteus,
    NAM
};


template <int input_size, int hidden_size>
using NeuralPiLSTM = RTNeural::ModelT<float, input_size, 1,
        RTNeural::LSTMLayerT<float, input_size, hidden_size>,
        RTNeural::DenseT<float, hidden_size, 1>>;



class NeuralNetwork
{
public:
    NeuralNetwork() = default;

    void reset();
    void load_json(const juce::String &filename);

    void loadNAM(const juce::String &filename);

    void loadConfig(const juce::String &filename);

    void process(const float* inData, float* outData, int numSamples);
    void process(const float* inData, float param, float* outData, int numSamples);
    void process(const float* inData, float param1, float param2, float* outData, int numSamples);

    int input_size = 1;

    NeuralNetworkType type;
    
private:
    NeuralPiLSTM<1, 20> model;

    NeuralPiLSTM<2, 20> model_cond1;

    NeuralPiLSTM<3, 20> model_cond2;

    NeuralPiLSTM<1, 40> model_proteus;

    NeuralPiLSTM<2, 40> model_proteus_cond1;

    NeuralPiLSTM<3, 40> model_proteus_cond2;

    std::unique_ptr<nam::DSP> model_nam;
    
    // Pre-Allowcate arrays for feeding the models
    float inArray1[2] = { 0.0, 0.0 };
    float inArray2[3] = { 0.0, 0.0, 0.0 };
};
