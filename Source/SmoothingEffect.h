#pragma once

#include <functional>
#include <map>
#include <string>

#include "../JuceLibraryCode/JuceHeader.h"

#include <fstream>

template<class Effect>
class SmoothingEffect
{

public:
    template<typename ...ArgList>
    SmoothingEffect(ArgList ...args) :
        effect(args...),
        setParameterFunctions(),
        parameterValues(),
        parameterTargets(),
        parameterContdowns(),
        parameterSteps(),
        stepsToTarget(0)
    {}

    void addParameter(const std::string &name, std::function<void(Effect&,float)> setParameterFunction, float initialValue=0.0f)
    {
        setParameterFunctions[name] = setParameterFunction;
        parameterValues[name] = initialValue;
        parameterTargets[name] = initialValue;
        parameterContdowns[name] = 0;
        parameterSteps[name] = 0.0f;
    }

    void setParameter(const std::string &name, float value)
    {
        parameterSteps[name] = (value - parameterValues[name]) / (float) this->stepsToTarget;
        parameterTargets[name] = value;
        parameterContdowns[name] = stepsToTarget;
    }

    void setParameterImmidiately(const std::string &name, float value)
    {
        parameterSteps[name] = value - parameterValues[name];
        parameterTargets[name] = value;
        parameterContdowns[name] = 1;
    }

    void prepare(const dsp::ProcessSpec &spec, double rampLengthInSeconds=1)
    {
        effect.prepare(spec);
        stepsToTarget = std::floor(rampLengthInSeconds * spec.sampleRate);
    }

    void process(const juce::dsp::ProcessContextReplacing<float> &context) noexcept
    {
        const juce::dsp::AudioBlock<const float> &input = context.getInputBlock();
        juce::dsp::AudioBlock<float>& output = context.getOutputBlock();
        const auto numSamples = juce::jmin(input.getNumSamples(), output.getNumSamples());
        
        for(auto &countdownIt : parameterContdowns)
        {
            if(countdownIt.second != 0)
            {
                const std::string &name = countdownIt.first;

                int countdowns = juce::jmin((int)numSamples, countdownIt.second);
                countdownIt.second -= countdowns;

                auto valueIt = parameterValues.find(name);
                float currentValue = valueIt->second + parameterSteps[name] * countdowns;
                if(countdownIt.second == 0)currentValue = parameterTargets[name];
                valueIt->second = currentValue;

                setParameterFunctions[name](effect, currentValue);
            }
        }

        effect.process(context);
    }

private:
    Effect effect;
    std::map<std::string, std::function<void(Effect&,float)>> setParameterFunctions;
    std::map<std::string, float> parameterValues;
    std::map<std::string, float> parameterTargets;
    std::map<std::string, int> parameterContdowns;
    std::map<std::string, float> parameterSteps;
    int stepsToTarget;

}; //end class SmoothingEffect