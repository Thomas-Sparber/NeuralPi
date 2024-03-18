#include "NeuralNetwork.h"

using Vec2d = std::vector<std::vector<float>>;

Vec2d transpose(const Vec2d& x)
{
    auto outer_size = x.size();
    auto inner_size = x[0].size();
    Vec2d y(inner_size, std::vector<float>(outer_size, 0.0f));

    for (size_t i = 0; i < outer_size; ++i)
    {
        for (size_t j = 0; j < inner_size; ++j)
            y[j][i] = x[i][j];
    }

    return y;
}

template <typename T1>
void set_weights_NeuralPi(T1 model, const nlohmann::json &weights_json)
{
    // Initialize the correct model
    auto& lstm = (*model).template get<0>();
    auto& dense = (*model).template get<1>();

    Vec2d lstm_weights_ih = weights_json["/state_dict/rec.weight_ih_l0"_json_pointer];
    lstm.setWVals(transpose(lstm_weights_ih));

    Vec2d lstm_weights_hh = weights_json["/state_dict/rec.weight_hh_l0"_json_pointer];
    lstm.setUVals(transpose(lstm_weights_hh));

    std::vector<float> lstm_bias_ih = weights_json["/state_dict/rec.bias_ih_l0"_json_pointer];
    std::vector<float> lstm_bias_hh = weights_json["/state_dict/rec.bias_hh_l0"_json_pointer];
    for (int i = 0; i < 80; ++i)
        lstm_bias_hh[i] += lstm_bias_ih[i];
    lstm.setBVals(lstm_bias_hh);

    Vec2d dense_weights = weights_json["/state_dict/lin.weight"_json_pointer];
    dense.setWeights(dense_weights);

    std::vector<float> dense_bias = weights_json["/state_dict/lin.bias"_json_pointer];
    dense.setBias(dense_bias.data());
}

template <typename T1>
void set_weights_Proteus(T1 model, const nlohmann::json &weights_json)
{
    // Initialize the correct model
    auto& lstm = (*model).template get<0>();
    auto& dense = (*model).template get<1>();

    Vec2d lstm_weights_ih = weights_json["/state_dict/rec.weight_ih_l0"_json_pointer];
    lstm.setWVals(transpose(lstm_weights_ih));

    Vec2d lstm_weights_hh = weights_json["/state_dict/rec.weight_hh_l0"_json_pointer];
    lstm.setUVals(transpose(lstm_weights_hh));

    std::vector<float> lstm_bias_ih = weights_json["/state_dict/rec.bias_ih_l0"_json_pointer];
    std::vector<float> lstm_bias_hh = weights_json["/state_dict/rec.bias_hh_l0"_json_pointer];
    for (int i = 0; i < 160; ++i)
        lstm_bias_hh[i] += lstm_bias_ih[i];
    lstm.setBVals(lstm_bias_hh);

    Vec2d dense_weights = weights_json["/state_dict/lin.weight"_json_pointer];
    dense.setWeights(dense_weights);

    std::vector<float> dense_bias = weights_json["/state_dict/lin.bias"_json_pointer];
    dense.setBias(dense_bias.data());
}

void NeuralNetwork::load_json(const juce::String &filename)
{
    // Read in the JSON file
    std::ifstream i2(filename.toUTF8());
	nlohmann::json weights_json;
	i2 >> weights_json;

    // Get the input size of the JSON file
	input_size = weights_json["/model_data/input_size"_json_pointer];

    int hidden_size = weights_json["/model_data/hidden_size"_json_pointer];

    if(hidden_size == 20)
    {
        type = NeuralNetworkType::NeuralPi;
        
        // Load the appropriate model
        if (input_size == 1) {
            set_weights_NeuralPi(&model, weights_json);
        }
        else if (input_size == 2) {
            set_weights_NeuralPi(&model_cond1, weights_json);
        } 
        else if (input_size == 3) {
            set_weights_NeuralPi(&model_cond2, weights_json);
        }
    }
    else if(hidden_size == 40)
    {
        type = NeuralNetworkType::Proteus;

        // Load the appropriate model
        if (input_size == 1) {
            set_weights_Proteus(&model_proteus, weights_json);
        }
        else if (input_size == 2) {
            set_weights_Proteus(&model_proteus_cond1, weights_json);
        } 
        else if (input_size == 3) {
            set_weights_Proteus(&model_proteus_cond2, weights_json);
        }
    }
}

void NeuralNetwork::loadConfig(const juce::String &filename)
{
    if(filename.toLowerCase().endsWith(".json"))
    {
        load_json(filename);
    }
}


void NeuralNetwork::reset()
{
    if(type == NeuralNetworkType::NeuralPi)
    {
        model.reset();
        model_cond1.reset();
        model_cond2.reset();
    }
    else if(type == NeuralNetworkType::Proteus)
    {
        model_proteus.reset();
        model_proteus_cond1.reset();
        model_proteus_cond2.reset();
    }
}

void NeuralNetwork::process(const float* inData, float param1, float param2, float* outData, int numSamples)
{
    if(input_size == 1)
    {
        if(type == NeuralNetworkType::NeuralPi)
        {
            for (int i = 0; i < numSamples; ++i)
                outData[i] = model.forward(inData + i) + inData[i];
        }
        else if(type == NeuralNetworkType::Proteus)
        {
            for (int i = 0; i < numSamples; ++i)
                outData[i] = model_proteus.forward(inData + i) + inData[i];
        }
    }
    else if(input_size == 2)
    {
        if(type == NeuralNetworkType::NeuralPi)
        {
            for (int i = 0; i < numSamples; ++i) {
                inArray1[0] = inData[i];
                inArray1[1] = param1;
                outData[i] = model_cond1.forward(inArray1) + inData[i];
            }
        }
        else if(type == NeuralNetworkType::Proteus)
        {
            for (int i = 0; i < numSamples; ++i) {
                inArray1[0] = inData[i];
                inArray1[1] = param1;
                outData[i] = model_proteus_cond1.forward(inArray1) + inData[i];
            }
        }
    }
    else if(input_size == 3)
    {
        if(type == NeuralNetworkType::NeuralPi)
        {
            for (int i = 0; i < numSamples; ++i) {
                inArray2[0] = inData[i];
                inArray2[1] = param1;
                inArray2[2] = param2;
                outData[i] = model_cond2.forward(inArray2) + inData[i];
            }
        }
        else if(type == NeuralNetworkType::Proteus)
        {
            for (int i = 0; i < numSamples; ++i) {
                inArray2[0] = inData[i];
                inArray2[1] = param1;
                inArray2[2] = param2;
                outData[i] = model_proteus_cond2.forward(inArray2) + inData[i];
            }
        }
    }
}