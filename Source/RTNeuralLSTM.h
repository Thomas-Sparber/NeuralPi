#pragma once

#include <RTNeural/RTNeural.h>

enum class RT_LSTM_Type {
    NeuralPi,
    Proteus
};

class RT_LSTM
{
public:
    RT_LSTM() = default;

    void reset();
    void load_json(const char* filename);

    template <typename T1>
    void set_weights_NeuralPi(T1 model, const char* filename);

    template <typename T1>
    void set_weights_Proteus(T1 model, const char* filename);

    void process(const float* inData, float* outData, int numSamples);
    void process(const float* inData, float param, float* outData, int numSamples);
    void process(const float* inData, float param1, float param2, float* outData, int numSamples);

    int input_size = 1;

    RT_LSTM_Type type;
    
private:
    RTNeural::ModelT<float, 1, 1,
        RTNeural::LSTMLayerT<float, 1, 20>,
        RTNeural::DenseT<float, 20, 1>> model;

    RTNeural::ModelT<float, 2, 1,
        RTNeural::LSTMLayerT<float, 2, 20>,
        RTNeural::DenseT<float, 20, 1>> model_cond1;

    RTNeural::ModelT<float, 3, 1,
        RTNeural::LSTMLayerT<float, 3, 20>,
        RTNeural::DenseT<float, 20, 1>> model_cond2;

    RTNeural::ModelT<float, 1, 1,
        RTNeural::LSTMLayerT<float, 1, 40>,
        RTNeural::DenseT<float, 40, 1>> model_proteus;

    RTNeural::ModelT<float, 2, 1,
        RTNeural::LSTMLayerT<float, 2, 40>,
        RTNeural::DenseT<float, 40, 1>> model_proteus_cond1;

    RTNeural::ModelT<float, 3, 1,
        RTNeural::LSTMLayerT<float, 3, 40>,
        RTNeural::DenseT<float, 40, 1>> model_proteus_cond2;
    
    // Pre-Allowcate arrays for feeding the models
    float inArray1[2] = { 0.0, 0.0 };
    float inArray2[3] = { 0.0, 0.0, 0.0 };
};
