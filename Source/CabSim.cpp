/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 7 End-User License
   Agreement and JUCE Privacy Policy.

   End User License Agreement: www.juce.com/juce-7-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include "CabSim.h"

template <typename Element>
class Queue
{
public:
    explicit Queue (int size)
        : fifo (size), storage (static_cast<size_t> (size)) {}

    bool push (Element& element) noexcept
    {
        if (fifo.getFreeSpace() == 0)
            return false;

        const auto writer = fifo.write (1);

        if (writer.blockSize1 != 0)
            storage[static_cast<size_t> (writer.startIndex1)] = std::move (element);
        else if (writer.blockSize2 != 0)
            storage[static_cast<size_t> (writer.startIndex2)] = std::move (element);

        return true;
    }

    template <typename Fn>
    void pop (Fn&& fn) { popN (1, std::forward<Fn> (fn)); }

    template <typename Fn>
    void popAll (Fn&& fn) { popN (fifo.getNumReady(), std::forward<Fn> (fn)); }

    bool hasPendingMessages() const noexcept { return fifo.getNumReady() > 0; }

private:
    template <typename Fn>
    void popN (int n, Fn&& fn)
    {
        fifo.read (n).forEach ([&] (int index)
                               {
                                   fn (storage[static_cast<size_t> (index)]);
                               });
    }

    AbstractFifo fifo;
    std::vector<Element> storage;
};

class BackgroundMessageQueue : private Thread
{
public:
    explicit BackgroundMessageQueue (int entries)
        : Thread ("CabSim background loader"), queue (entries)
    {}

    using IncomingCommand = juce::dsp::FixedSizeFunction<400, void()>;

    // Push functions here, and they'll be called later on a background thread.
    // This function is wait-free.
    // This function is only safe to call from a single thread at a time.
    bool push (IncomingCommand& command) { return queue.push (command); }

    void popAll()
    {
        const ScopedLock lock (popMutex);
        queue.popAll ([] (IncomingCommand& command) { command(); command = nullptr; });
    }

    using Thread::startThread;
    using Thread::stopThread;

private:
    void run() override
    {
        while (! threadShouldExit())
        {
            const auto tryPop = [&]
            {
                const ScopedLock lock (popMutex);

                if (! queue.hasPendingMessages())
                    return false;

                queue.pop ([] (IncomingCommand& command) { command(); command = nullptr;});
                return true;
            };

            if (! tryPop())
                sleep (10);
        }
    }

    CriticalSection popMutex;
    Queue<IncomingCommand> queue;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BackgroundMessageQueue)
};

struct CabSimMessageQueue::Impl  : public BackgroundMessageQueue
{
    using BackgroundMessageQueue::BackgroundMessageQueue;
};

CabSimMessageQueue::CabSimMessageQueue()
    : CabSimMessageQueue (1000)
{}

CabSimMessageQueue::CabSimMessageQueue (int entries)
    : pimpl (std::make_unique<Impl> (entries))
{
    pimpl->startThread();
}

CabSimMessageQueue::~CabSimMessageQueue() noexcept
{
    pimpl->stopThread (-1);
}

CabSimMessageQueue::CabSimMessageQueue (CabSimMessageQueue&&) noexcept = default;
CabSimMessageQueue& CabSimMessageQueue::operator= (CabSimMessageQueue&&) noexcept = default;

//==============================================================================
struct CabSimEngine
{
    CabSimEngine (const float* samples,
                       size_t numSamples,
                       size_t maxBlockSize)
        : blockSize ((size_t) nextPowerOfTwo ((int) maxBlockSize)),
          fftSize (blockSize > 128 ? 2 * blockSize : 4 * blockSize),
          fftObject (std::make_unique<juce::dsp::FFT> (juce::roundToInt (std::log2 (fftSize)))),
          numSegments (numSamples / (fftSize - blockSize) + 1u),
          numInputSegments ((blockSize > 128 ? numSegments : 3 * numSegments)),
          bufferInput      (1, static_cast<int> (fftSize)),
          bufferOutput     (1, static_cast<int> (fftSize * 2)),
          bufferTempOutput (1, static_cast<int> (fftSize * 2)),
          bufferOverlap    (1, static_cast<int> (fftSize))
    {
        bufferOutput.clear();

        auto updateSegmentsIfNecessary = [this] (size_t numSegmentsToUpdate,
                                                 std::vector<AudioBuffer<float>>& segments)
        {
            if (numSegmentsToUpdate == 0
                || numSegmentsToUpdate != (size_t) segments.size()
                || (size_t) segments[0].getNumSamples() != fftSize * 2)
            {
                segments.clear();

                for (size_t i = 0; i < numSegmentsToUpdate; ++i)
                    segments.push_back ({ 1, static_cast<int> (fftSize * 2) });
            }
        };

        updateSegmentsIfNecessary (numInputSegments, buffersInputSegments);
        updateSegmentsIfNecessary (numSegments,      buffersImpulseSegments);

        auto FFTTempObject = std::make_unique<juce::dsp::FFT> (juce::roundToInt (std::log2 (fftSize)));
        size_t currentPtr = 0;

        for (auto& buf : buffersImpulseSegments)
        {
            buf.clear();

            auto* impulseResponse = buf.getWritePointer (0);

            if (&buf == &buffersImpulseSegments.front())
                impulseResponse[0] = 1.0f;

            FloatVectorOperations::copy (impulseResponse,
                                         samples + currentPtr,
                                         static_cast<int> (juce::jmin (fftSize - blockSize, numSamples - currentPtr)));

            FFTTempObject->performRealOnlyForwardTransform (impulseResponse);
            prepareForCabSim (impulseResponse);

            currentPtr += (fftSize - blockSize);
        }

        reset();
    }

    void reset()
    {
        bufferInput.clear();
        bufferOverlap.clear();
        bufferTempOutput.clear();
        bufferOutput.clear();

        for (auto& buf : buffersInputSegments)
            buf.clear();

        currentSegment = 0;
        inputDataPos = 0;
    }

    void processSamples (const float* input, float* output, size_t numSamples)
    {
        // Overlap-add, zero latency CabSim algorithm with uniform partitioning
        size_t numSamplesProcessed = 0;

        auto indexStep = numInputSegments / numSegments;

        auto* inputData      = bufferInput.getWritePointer (0);
        auto* outputTempData = bufferTempOutput.getWritePointer (0);
        auto* outputData     = bufferOutput.getWritePointer (0);
        auto* overlapData    = bufferOverlap.getWritePointer (0);

        while (numSamplesProcessed < numSamples)
        {
            const bool inputDataWasEmpty = (inputDataPos == 0);
            auto numSamplesToProcess = juce::jmin (numSamples - numSamplesProcessed, blockSize - inputDataPos);

            FloatVectorOperations::copy (inputData + inputDataPos, input + numSamplesProcessed, static_cast<int> (numSamplesToProcess));

            auto* inputSegmentData = buffersInputSegments[currentSegment].getWritePointer (0);
            FloatVectorOperations::copy (inputSegmentData, inputData, static_cast<int> (fftSize));

            fftObject->performRealOnlyForwardTransform (inputSegmentData);
            prepareForCabSim (inputSegmentData);

            // Complex multiplication
            if (inputDataWasEmpty)
            {
                FloatVectorOperations::fill (outputTempData, 0, static_cast<int> (fftSize + 1));

                auto index = currentSegment;

                for (size_t i = 1; i < numSegments; ++i)
                {
                    index += indexStep;

                    if (index >= numInputSegments)
                        index -= numInputSegments;

                    CabSimProcessingAndAccumulate (buffersInputSegments[index].getWritePointer (0),
                                                        buffersImpulseSegments[i].getWritePointer (0),
                                                        outputTempData);
                }
            }

            FloatVectorOperations::copy (outputData, outputTempData, static_cast<int> (fftSize + 1));

            CabSimProcessingAndAccumulate (inputSegmentData,
                                                buffersImpulseSegments.front().getWritePointer (0),
                                                outputData);

            updateSymmetricFrequencyDomainData (outputData);
            fftObject->performRealOnlyInverseTransform (outputData);

            // Add overlap
            FloatVectorOperations::add (&output[numSamplesProcessed], &outputData[inputDataPos], &overlapData[inputDataPos], (int) numSamplesToProcess);

            // Input buffer full => Next block
            inputDataPos += numSamplesToProcess;

            if (inputDataPos == blockSize)
            {
                // Input buffer is empty again now
                FloatVectorOperations::fill (inputData, 0.0f, static_cast<int> (fftSize));

                inputDataPos = 0;

                // Extra step for segSize > blockSize
                FloatVectorOperations::add (&(outputData[blockSize]), &(overlapData[blockSize]), static_cast<int> (fftSize - 2 * blockSize));

                // Save the overlap
                FloatVectorOperations::copy (overlapData, &(outputData[blockSize]), static_cast<int> (fftSize - blockSize));

                currentSegment = (currentSegment > 0) ? (currentSegment - 1) : (numInputSegments - 1);
            }

            numSamplesProcessed += numSamplesToProcess;
        }
    }

    void processSamplesWithAddedLatency (const float* input, float* output, size_t numSamples)
    {
        // Overlap-add, zero latency CabSim algorithm with uniform partitioning
        size_t numSamplesProcessed = 0;

        auto indexStep = numInputSegments / numSegments;

        auto* inputData      = bufferInput.getWritePointer (0);
        auto* outputTempData = bufferTempOutput.getWritePointer (0);
        auto* outputData     = bufferOutput.getWritePointer (0);
        auto* overlapData    = bufferOverlap.getWritePointer (0);

        while (numSamplesProcessed < numSamples)
        {
            auto numSamplesToProcess = juce::jmin (numSamples - numSamplesProcessed, blockSize - inputDataPos);

            FloatVectorOperations::copy (inputData + inputDataPos, input + numSamplesProcessed, static_cast<int> (numSamplesToProcess));

            FloatVectorOperations::copy (output + numSamplesProcessed, outputData + inputDataPos, static_cast<int> (numSamplesToProcess));

            numSamplesProcessed += numSamplesToProcess;
            inputDataPos += numSamplesToProcess;

            // processing itself when needed (with latency)
            if (inputDataPos == blockSize)
            {
                // Copy input data in input segment
                auto* inputSegmentData = buffersInputSegments[currentSegment].getWritePointer (0);
                FloatVectorOperations::copy (inputSegmentData, inputData, static_cast<int> (fftSize));

                fftObject->performRealOnlyForwardTransform (inputSegmentData);
                prepareForCabSim (inputSegmentData);

                // Complex multiplication
                FloatVectorOperations::fill (outputTempData, 0, static_cast<int> (fftSize + 1));

                auto index = currentSegment;

                for (size_t i = 1; i < numSegments; ++i)
                {
                    index += indexStep;

                    if (index >= numInputSegments)
                        index -= numInputSegments;

                    CabSimProcessingAndAccumulate (buffersInputSegments[index].getWritePointer (0),
                                                        buffersImpulseSegments[i].getWritePointer (0),
                                                        outputTempData);
                }

                FloatVectorOperations::copy (outputData, outputTempData, static_cast<int> (fftSize + 1));

                CabSimProcessingAndAccumulate (inputSegmentData,
                                                    buffersImpulseSegments.front().getWritePointer (0),
                                                    outputData);

                updateSymmetricFrequencyDomainData (outputData);
                fftObject->performRealOnlyInverseTransform (outputData);

                // Add overlap
                FloatVectorOperations::add (outputData, overlapData, static_cast<int> (blockSize));

                // Input buffer is empty again now
                FloatVectorOperations::fill (inputData, 0.0f, static_cast<int> (fftSize));

                // Extra step for segSize > blockSize
                FloatVectorOperations::add (&(outputData[blockSize]), &(overlapData[blockSize]), static_cast<int> (fftSize - 2 * blockSize));

                // Save the overlap
                FloatVectorOperations::copy (overlapData, &(outputData[blockSize]), static_cast<int> (fftSize - blockSize));

                currentSegment = (currentSegment > 0) ? (currentSegment - 1) : (numInputSegments - 1);

                inputDataPos = 0;
            }
        }
    }

    // After each FFT, this function is called to allow CabSim to be performed with only 4 SIMD functions calls.
    void prepareForCabSim (float *samples) noexcept
    {
        auto FFTSizeDiv2 = fftSize / 2;

        for (size_t i = 0; i < FFTSizeDiv2; i++)
            samples[i] = samples[i << 1];

        samples[FFTSizeDiv2] = 0;

        for (size_t i = 1; i < FFTSizeDiv2; i++)
            samples[i + FFTSizeDiv2] = -samples[((fftSize - i) << 1) + 1];
    }

    // Does the CabSim operation itself only on half of the frequency domain samples.
    void CabSimProcessingAndAccumulate (const float *input, const float *impulse, float *output)
    {
        auto FFTSizeDiv2 = fftSize / 2;

        FloatVectorOperations::addWithMultiply      (output, input, impulse, static_cast<int> (FFTSizeDiv2));
        FloatVectorOperations::subtractWithMultiply (output, &(input[FFTSizeDiv2]), &(impulse[FFTSizeDiv2]), static_cast<int> (FFTSizeDiv2));

        FloatVectorOperations::addWithMultiply      (&(output[FFTSizeDiv2]), input, &(impulse[FFTSizeDiv2]), static_cast<int> (FFTSizeDiv2));
        FloatVectorOperations::addWithMultiply      (&(output[FFTSizeDiv2]), &(input[FFTSizeDiv2]), impulse, static_cast<int> (FFTSizeDiv2));

        output[fftSize] += input[fftSize] * impulse[fftSize];
    }

    // Undoes the re-organization of samples from the function prepareForCabSim.
    // Then takes the conjugate of the frequency domain first half of samples to fill the
    // second half, so that the inverse transform will return real samples in the time domain.
    void updateSymmetricFrequencyDomainData (float* samples) noexcept
    {
        auto FFTSizeDiv2 = fftSize / 2;

        for (size_t i = 1; i < FFTSizeDiv2; i++)
        {
            samples[(fftSize - i) << 1] = samples[i];
            samples[((fftSize - i) << 1) + 1] = -samples[FFTSizeDiv2 + i];
        }

        samples[1] = 0.f;

        for (size_t i = 1; i < FFTSizeDiv2; i++)
        {
            samples[i << 1] = samples[(fftSize - i) << 1];
            samples[(i << 1) + 1] = -samples[((fftSize - i) << 1) + 1];
        }
    }

    //==============================================================================
    const size_t blockSize;
    const size_t fftSize;
    const std::unique_ptr<juce::dsp::FFT> fftObject;
    const size_t numSegments;
    const size_t numInputSegments;
    size_t currentSegment = 0, inputDataPos = 0;

    AudioBuffer<float> bufferInput, bufferOutput, bufferTempOutput, bufferOverlap;
    std::vector<AudioBuffer<float>> buffersInputSegments, buffersImpulseSegments;
};

//==============================================================================
class MultichannelEngine
{
public:
    MultichannelEngine (const AudioBuffer<float>& buf,
                        int maxBlockSize,
                        int maxBufferSize,
                        CabSim::NonUniform headSizeIn,
                        bool isZeroDelayIn)
        : tailBuffer (1, maxBlockSize),
          latency (isZeroDelayIn ? 0 : maxBufferSize),
          irSize (buf.getNumSamples()),
          blockSize (maxBlockSize),
          isZeroDelay (isZeroDelayIn)
    {
        constexpr auto numChannels = 2;

        const auto makeEngine = [&] (int channel, int offset, int length, uint32 thisBlockSize)
        {
            return std::make_unique<CabSimEngine> (buf.getReadPointer (juce::jmin (buf.getNumChannels() - 1, channel), offset),
                                                        length,
                                                        static_cast<size_t> (thisBlockSize));
        };

        if (headSizeIn.headSizeInSamples == 0)
        {
            for (int i = 0; i < numChannels; ++i)
                head.emplace_back (makeEngine (i, 0, buf.getNumSamples(), static_cast<uint32> (maxBufferSize)));
        }
        else
        {
            const auto size = juce::jmin (buf.getNumSamples(), headSizeIn.headSizeInSamples);

            for (int i = 0; i < numChannels; ++i)
                head.emplace_back (makeEngine (i, 0, size, static_cast<uint32> (maxBufferSize)));

            const auto tailBufferSize = static_cast<uint32> (headSizeIn.headSizeInSamples + (isZeroDelay ? 0 : maxBufferSize));

            if (size != buf.getNumSamples())
                for (int i = 0; i < numChannels; ++i)
                    tail.emplace_back (makeEngine (i, size, buf.getNumSamples() - size, tailBufferSize));
        }
    }

    void reset()
    {
        for (const auto& e : head)
            e->reset();

        for (const auto& e : tail)
            e->reset();
    }

    void processSamples (const juce::dsp::AudioBlock<const float>& input, juce::dsp::AudioBlock<float>& output)
    {
        const auto numChannels = juce::jmin (head.size(), input.getNumChannels(), output.getNumChannels());
        const auto numSamples  = juce::jmin (input.getNumSamples(), output.getNumSamples());

        const juce::dsp::AudioBlock<float> fullTailBlock (tailBuffer);
        const auto tailBlock = fullTailBlock.getSubBlock (0, (size_t) numSamples);

        const auto isUniform = tail.empty();

        for (size_t channel = 0; channel < numChannels; ++channel)
        {
            if (! isUniform)
                tail[channel]->processSamplesWithAddedLatency (input.getChannelPointer (channel),
                                                               tailBlock.getChannelPointer (0),
                                                               numSamples);

            if (isZeroDelay)
                head[channel]->processSamples (input.getChannelPointer (channel),
                                               output.getChannelPointer (channel),
                                               numSamples);
            else
                head[channel]->processSamplesWithAddedLatency (input.getChannelPointer (channel),
                                                               output.getChannelPointer (channel),
                                                               numSamples);

            if (! isUniform)
                output.getSingleChannelBlock (channel) += tailBlock;
        }

        const auto numOutputChannels = output.getNumChannels();

        for (auto i = numChannels; i < numOutputChannels; ++i)
            output.getSingleChannelBlock (i).copyFrom (output.getSingleChannelBlock (0));
    }

    int getIRSize() const noexcept     { return irSize; }
    int getLatency() const noexcept    { return latency; }
    int getBlockSize() const noexcept  { return blockSize; }

private:
    std::vector<std::unique_ptr<CabSimEngine>> head, tail;
    AudioBuffer<float> tailBuffer;

    const int latency;
    const int irSize;
    const int blockSize;
    const bool isZeroDelay;
};

static AudioBuffer<float> fixNumChannels (const AudioBuffer<float>& buf, CabSim::Stereo stereo)
{
    const auto numChannels = juce::jmin (buf.getNumChannels(), stereo == CabSim::Stereo::yes ? 2 : 1);
    const auto numSamples = buf.getNumSamples();

    AudioBuffer<float> result (numChannels, buf.getNumSamples());

    for (auto channel = 0; channel != numChannels; ++channel)
        result.copyFrom (channel, 0, buf.getReadPointer (channel), numSamples);

    if (result.getNumSamples() == 0 || result.getNumChannels() == 0)
    {
        result.setSize (1, 1);
        result.setSample (0, 0, 1.0f);
    }

    return result;
}

static AudioBuffer<float> trimImpulseResponse (const AudioBuffer<float>& buf)
{
    const auto thresholdTrim = juce::Decibels::decibelsToGain (-80.0f);

    const auto numChannels = buf.getNumChannels();
    const auto numSamples = buf.getNumSamples();

    std::ptrdiff_t offsetBegin = numSamples;
    std::ptrdiff_t offsetEnd   = numSamples;

    for (auto channel = 0; channel < numChannels; ++channel)
    {
        const auto indexAboveThreshold = [&] (auto begin, auto end)
        {
            return std::distance (begin, std::find_if (begin, end, [&] (float sample)
            {
                return std::abs (sample) >= thresholdTrim;
            }));
        };

        const auto channelBegin = buf.getReadPointer (channel);
        const auto channelEnd = channelBegin + numSamples;
        const auto itStart = indexAboveThreshold (channelBegin, channelEnd);
        const auto itEnd = indexAboveThreshold (std::make_reverse_iterator (channelEnd),
                                                std::make_reverse_iterator (channelBegin));

        offsetBegin = juce::jmin (offsetBegin, itStart);
        offsetEnd   = juce::jmin (offsetEnd,   itEnd);
    }

    if (offsetBegin == numSamples)
    {
        auto result = AudioBuffer<float> (numChannels, 1);
        result.clear();
        return result;
    }

    const auto newLength = juce::jmax (1, numSamples - static_cast<int> (offsetBegin + offsetEnd));

    AudioBuffer<float> result (numChannels, newLength);

    for (auto channel = 0; channel < numChannels; ++channel)
    {
        result.copyFrom (channel,
                         0,
                         buf.getReadPointer (channel, static_cast<int> (offsetBegin)),
                         result.getNumSamples());
    }

    return result;
}

static float calculateNormalisationFactor (float sumSquaredMagnitude)
{
    if (sumSquaredMagnitude < 1e-8f)
        return 1.0f;

    return 0.125f / std::sqrt (sumSquaredMagnitude);
}

static void normaliseImpulseResponse (AudioBuffer<float>& buf)
{
    const auto numChannels = buf.getNumChannels();
    const auto numSamples  = buf.getNumSamples();
    const auto channelPtrs = buf.getArrayOfWritePointers();

    const auto maxSumSquaredMag = std::accumulate (channelPtrs, channelPtrs + numChannels, 0.0f, [numSamples] (auto max, auto* channel)
    {
        return jmax (max, std::accumulate (channel, channel + numSamples, 0.0f, [] (auto sum, auto samp)
        {
            return sum + (samp * samp);
        }));
    });

    const auto normalisationFactor = calculateNormalisationFactor (maxSumSquaredMag);

    std::for_each (channelPtrs, channelPtrs + numChannels, [normalisationFactor, numSamples] (auto* channel)
    {
        FloatVectorOperations::multiply (channel, normalisationFactor, numSamples);
    });
}

static AudioBuffer<float> resampleImpulseResponse (const AudioBuffer<float>& buf,
                                                   const double srcSampleRate,
                                                   const double destSampleRate)
{
    if (juce::approximatelyEqual (srcSampleRate, destSampleRate))
        return buf;

    const auto factorReading = srcSampleRate / destSampleRate;

    AudioBuffer<float> original = buf;
    MemoryAudioSource memorySource (original, false);
    ResamplingAudioSource resamplingSource (&memorySource, false, buf.getNumChannels());

    const auto finalSize = juce::roundToInt (juce::jmax (1.0, buf.getNumSamples() / factorReading));
    resamplingSource.setResamplingRatio (factorReading);
    resamplingSource.prepareToPlay (finalSize, srcSampleRate);

    AudioBuffer<float> result (buf.getNumChannels(), finalSize);
    resamplingSource.getNextAudioBlock ({ &result, 0, result.getNumSamples() });

    return result;
}

//==============================================================================
template <typename Element>
class TryLockedPtr
{
public:
    void set (std::unique_ptr<Element> p)
    {
        const SpinLock::ScopedLockType lock (mutex);
        ptr = std::move (p);
    }

    std::unique_ptr<MultichannelEngine> get()
    {
        const SpinLock::ScopedTryLockType lock (mutex);
        return lock.isLocked() ? std::move (ptr) : nullptr;
    }

private:
    std::unique_ptr<Element> ptr;
    SpinLock mutex;
};

struct BufferWithSampleRate
{
    BufferWithSampleRate() = default;

    BufferWithSampleRate (AudioBuffer<float>&& bufferIn, double sampleRateIn)
        : buffer (std::move (bufferIn)), sampleRate (sampleRateIn) {}

    AudioBuffer<float> buffer;
    double sampleRate = 0.0;
};

static BufferWithSampleRate loadStreamToBuffer (std::unique_ptr<InputStream> stream, size_t maxLength)
{
    AudioFormatManager manager;
    manager.registerBasicFormats();
    std::unique_ptr<AudioFormatReader> formatReader (manager.createReaderFor (std::move (stream)));

    if (formatReader == nullptr)
        return {};

    const auto fileLength = static_cast<size_t> (formatReader->lengthInSamples);
    const auto lengthToLoad = maxLength == 0 ? fileLength : juce::jmin (maxLength, fileLength);

    BufferWithSampleRate result { { juce::jlimit (1, 2, static_cast<int> (formatReader->numChannels)),
                                    static_cast<int> (lengthToLoad) },
                                  formatReader->sampleRate };

    formatReader->read (result.buffer.getArrayOfWritePointers(),
                        result.buffer.getNumChannels(),
                        0,
                        result.buffer.getNumSamples());

    return result;
}

// This class caches the data required to build a new CabSim engine
// (in particular, impulse response data and a ProcessSpec).
// Calls to `setProcessSpec` and `setImpulseResponse` construct a
// new engine, which can be retrieved by calling `getEngine`.
class CabSimEngineFactory
{
public:
    CabSimEngineFactory (CabSim::Latency requiredLatency,
                              CabSim::NonUniform requiredHeadSize)
        : latency  { (requiredLatency.latencyInSamples   <= 0) ? 0 : juce::jmax (64, nextPowerOfTwo (requiredLatency.latencyInSamples)) },
          headSize { (requiredHeadSize.headSizeInSamples <= 0) ? 0 : juce::jmax (64, nextPowerOfTwo (requiredHeadSize.headSizeInSamples)) },
          shouldBeZeroLatency (requiredLatency.latencyInSamples == 0)
    {}

    // It is safe to call this method simultaneously with other public
    // member functions.
    void setProcessSpec (const juce::dsp::ProcessSpec& spec)
    {
        const std::lock_guard<std::mutex> lock (mutex);
        processSpec = spec;

        engine.set (makeEngine());
    }

    // It is safe to call this method simultaneously with other public
    // member functions.
    void setImpulseResponse (BufferWithSampleRate&& buf,
                             CabSim::Stereo stereo,
                             CabSim::Trim trim,
                             CabSim::Normalise normalise)
    {
        const std::lock_guard<std::mutex> lock (mutex);
        wantsNormalise = normalise;
        originalSampleRate = buf.sampleRate;

        impulseResponse = [&]
        {
            auto corrected = fixNumChannels (buf.buffer, stereo);
            return trim == CabSim::Trim::yes ? trimImpulseResponse (corrected) : corrected;
        }();

        engine.set (makeEngine());
    }

    // Returns the most recently-created engine, or nullptr
    // if there is no pending engine, or if the engine is currently
    // being updated by one of the setter methods.
    // It is safe to call this simultaneously with other public
    // member functions.
    std::unique_ptr<MultichannelEngine> getEngine() { return engine.get(); }

private:
    std::unique_ptr<MultichannelEngine> makeEngine()
    {
        auto resampled = resampleImpulseResponse (impulseResponse, originalSampleRate, processSpec.sampleRate);

        if (wantsNormalise == CabSim::Normalise::yes)
            normaliseImpulseResponse (resampled);
        else
            resampled.applyGain ((float) (originalSampleRate / processSpec.sampleRate));

        const auto currentLatency = juce::jmax (processSpec.maximumBlockSize, (uint32) latency.latencyInSamples);
        const auto maxBufferSize = shouldBeZeroLatency ? static_cast<int> (processSpec.maximumBlockSize)
                                                       : nextPowerOfTwo (static_cast<int> (currentLatency));

        return std::make_unique<MultichannelEngine> (resampled,
                                                     processSpec.maximumBlockSize,
                                                     maxBufferSize,
                                                     headSize,
                                                     shouldBeZeroLatency);
    }

    static AudioBuffer<float> makeImpulseBuffer()
    {
        AudioBuffer<float> result (1, 1);
        result.setSample (0, 0, 1.0f);
        return result;
    }

    juce::dsp::ProcessSpec processSpec { 44100.0, 128, 2 };
    AudioBuffer<float> impulseResponse = makeImpulseBuffer();
    double originalSampleRate = processSpec.sampleRate;
    CabSim::Normalise wantsNormalise = CabSim::Normalise::no;
    const CabSim::Latency latency;
    const CabSim::NonUniform headSize;
    const bool shouldBeZeroLatency;

    TryLockedPtr<MultichannelEngine> engine;

    mutable std::mutex mutex;
};

static void setImpulseResponse (CabSimEngineFactory& factory,
                                const void* sourceData,
                                size_t sourceDataSize,
                                CabSim::Stereo stereo,
                                CabSim::Trim trim,
                                size_t size,
                                CabSim::Normalise normalise)
{
    factory.setImpulseResponse (loadStreamToBuffer (std::make_unique<MemoryInputStream> (sourceData, sourceDataSize, false), size),
                                stereo, trim, normalise);
}

static void setImpulseResponse (CabSimEngineFactory& factory,
                                const File& fileImpulseResponse,
                                CabSim::Stereo stereo,
                                CabSim::Trim trim,
                                size_t size,
                                CabSim::Normalise normalise)
{
    factory.setImpulseResponse (loadStreamToBuffer (std::make_unique<FileInputStream> (fileImpulseResponse), size),
                                stereo, trim, normalise);
}

// This class acts as a destination for CabSim engines which are loaded on
// a background thread.

// Deriving from `enable_shared_from_this` allows us to capture a reference to
// this object when adding commands to the background message queue.
// That way, we can avoid dangling references in the background thread in the case
// that a CabSim instance is deleted before the background message queue.
class CabSimEngineQueue final : public std::enable_shared_from_this<CabSimEngineQueue>
{
public:
    CabSimEngineQueue (BackgroundMessageQueue& queue,
                            CabSim::Latency latencyIn,
                            CabSim::NonUniform headSizeIn)
        : messageQueue (queue), factory (latencyIn, headSizeIn) {}

    void loadImpulseResponse (AudioBuffer<float>&& buffer,
                              double sr,
                              CabSim::Stereo stereo,
                              CabSim::Trim trim,
                              CabSim::Normalise normalise)
    {
        callLater ([b = std::move (buffer), sr, stereo, trim, normalise] (CabSimEngineFactory& f) mutable
        {
            f.setImpulseResponse ({ std::move (b), sr }, stereo, trim, normalise);
        });
    }

    void loadImpulseResponse (const void* sourceData,
                              size_t sourceDataSize,
                              CabSim::Stereo stereo,
                              CabSim::Trim trim,
                              size_t size,
                              CabSim::Normalise normalise)
    {
        callLater ([sourceData, sourceDataSize, stereo, trim, size, normalise] (CabSimEngineFactory& f) mutable
        {
            setImpulseResponse (f, sourceData, sourceDataSize, stereo, trim, size, normalise);
        });
    }

    void loadImpulseResponse (const File& fileImpulseResponse,
                              CabSim::Stereo stereo,
                              CabSim::Trim trim,
                              size_t size,
                              CabSim::Normalise normalise)
    {
        callLater ([fileImpulseResponse, stereo, trim, size, normalise] (CabSimEngineFactory& f) mutable
        {
            setImpulseResponse (f, fileImpulseResponse, stereo, trim, size, normalise);
        });
    }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        factory.setProcessSpec (spec);
    }

    // Call this regularly to try to resend any pending message.
    // This allows us to always apply the most recently requested
    // state (eventually), even if the message queue fills up.
    void postPendingCommand()
    {
        if (pendingCommand == nullptr)
            return;

        if (messageQueue.push (pendingCommand))
            pendingCommand = nullptr;
    }

    std::unique_ptr<MultichannelEngine> getEngine() { return factory.getEngine(); }

private:
    template <typename Fn>
    void callLater (Fn&& fn)
    {
        // If there was already a pending command (because the queue was full) we'll end up deleting it here.
        // Not much we can do about that!
        pendingCommand = [weak = weakFromThis(), callback = std::forward<Fn> (fn)]() mutable
        {
            if (auto t = weak.lock())
                callback (t->factory);
        };

        postPendingCommand();
    }

    std::weak_ptr<CabSimEngineQueue> weakFromThis() { return shared_from_this(); }

    BackgroundMessageQueue& messageQueue;
    CabSimEngineFactory factory;
    BackgroundMessageQueue::IncomingCommand pendingCommand;
};

class CrossoverMixer
{
public:
    void reset()
    {
        smoother.setCurrentAndTargetValue (1.0f);
    }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        smoother.reset (spec.sampleRate, 0.05);
        smootherBuffer.setSize (1, static_cast<int> (spec.maximumBlockSize));
        mixBuffer.setSize (static_cast<int> (spec.numChannels), static_cast<int> (spec.maximumBlockSize));
        reset();
    }

    template <typename ProcessCurrent, typename ProcessPrevious, typename NotifyDone>
    void processSamples (const juce::dsp::AudioBlock<const float>& input,
                         juce::dsp::AudioBlock<float>& output,
                         ProcessCurrent&& current,
                         ProcessPrevious&& previous,
                         NotifyDone&& notifyDone)
    {
        if (smoother.isSmoothing())
        {
            const auto numSamples = static_cast<int> (input.getNumSamples());

            for (auto sample = 0; sample != numSamples; ++sample)
                smootherBuffer.setSample (0, sample, smoother.getNextValue());

            juce::dsp::AudioBlock<float> mixBlock (mixBuffer);
            mixBlock.clear();
            previous (input, mixBlock);

            for (size_t channel = 0; channel != output.getNumChannels(); ++channel)
            {
                FloatVectorOperations::multiply (mixBlock.getChannelPointer (channel),
                                                 smootherBuffer.getReadPointer (0),
                                                 numSamples);
            }

            FloatVectorOperations::multiply (smootherBuffer.getWritePointer (0), -1.0f, numSamples);
            FloatVectorOperations::add (smootherBuffer.getWritePointer (0), 1.0f, numSamples);

            current (input, output);

            for (size_t channel = 0; channel != output.getNumChannels(); ++channel)
            {
                FloatVectorOperations::multiply (output.getChannelPointer (channel),
                                                 smootherBuffer.getReadPointer (0),
                                                 numSamples);
                FloatVectorOperations::add (output.getChannelPointer (channel),
                                            mixBlock.getChannelPointer (channel),
                                            numSamples);
            }

            if (! smoother.isSmoothing())
                notifyDone();
        }
        else
        {
            current (input, output);
        }
    }

    void beginTransition()
    {
        smoother.setCurrentAndTargetValue (1.0f);
        smoother.setTargetValue (0.0f);
    }

private:
    LinearSmoothedValue<float> smoother;
    AudioBuffer<float> smootherBuffer;
    AudioBuffer<float> mixBuffer;
};

using OptionalQueue = OptionalScopedPointer<CabSimMessageQueue>;

class CabSim::Impl
{
public:
    Impl (Latency requiredLatency,
          NonUniform requiredHeadSize,
          OptionalQueue&& queue)
        : messageQueue (std::move (queue)),
          engineQueue (std::make_shared<CabSimEngineQueue> (*messageQueue->pimpl,
                                                                 requiredLatency,
                                                                 requiredHeadSize))
    {}

    void reset()
    {
        mixer.reset();

        if (currentEngine != nullptr)
            currentEngine->reset();

        destroyPreviousEngine();
    }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        messageQueue->pimpl->popAll();
        mixer.prepare (spec);
        engineQueue->prepare (spec);

        if (auto newEngine = engineQueue->getEngine())
            currentEngine = std::move (newEngine);

        previousEngine = nullptr;
        jassert (currentEngine != nullptr);
    }

    void processSamples (const juce::dsp::AudioBlock<const float>& input, juce::dsp::AudioBlock<float>& output)
    {
        engineQueue->postPendingCommand();

        if (previousEngine == nullptr)
            installPendingEngine();

        mixer.processSamples (input,
                              output,
                              [this] (const juce::dsp::AudioBlock<const float>& in, juce::dsp::AudioBlock<float>& out)
                              {
                                  currentEngine->processSamples (in, out);
                              },
                              [this] (const juce::dsp::AudioBlock<const float>& in, juce::dsp::AudioBlock<float>& out)
                              {
                                  if (previousEngine != nullptr)
                                      previousEngine->processSamples (in, out);
                                  else
                                      out.copyFrom (in);
                              },
                              [this] { destroyPreviousEngine(); });
    }

    int getCurrentIRSize() const { return currentEngine != nullptr ? currentEngine->getIRSize() : 0; }

    int getLatency() const { return currentEngine != nullptr ? currentEngine->getLatency() : 0; }

    void loadImpulseResponse (AudioBuffer<float>&& buffer,
                              double originalSampleRate,
                              Stereo stereo,
                              Trim trim,
                              Normalise normalise)
    {
        engineQueue->loadImpulseResponse (std::move (buffer), originalSampleRate, stereo, trim, normalise);
    }

    void loadImpulseResponse (const void* sourceData,
                              size_t sourceDataSize,
                              Stereo stereo,
                              Trim trim,
                              size_t size,
                              Normalise normalise)
    {
        engineQueue->loadImpulseResponse (sourceData, sourceDataSize, stereo, trim, size, normalise);
    }

    void loadImpulseResponse (const File& fileImpulseResponse,
                              Stereo stereo,
                              Trim trim,
                              size_t size,
                              Normalise normalise)
    {
        engineQueue->loadImpulseResponse (fileImpulseResponse, stereo, trim, size, normalise);
    }

private:
    void destroyPreviousEngine()
    {
        // If the queue is full, we'll destroy this straight away
        BackgroundMessageQueue::IncomingCommand command = [p = std::move (previousEngine)]() mutable { p = nullptr; };
        messageQueue->pimpl->push (command);
    }

    void installNewEngine (std::unique_ptr<MultichannelEngine> newEngine)
    {
        destroyPreviousEngine();
        previousEngine = std::move (currentEngine);
        currentEngine = std::move (newEngine);
        mixer.beginTransition();
    }

    void installPendingEngine()
    {
        if (auto newEngine = engineQueue->getEngine())
            installNewEngine (std::move (newEngine));
    }

    OptionalQueue messageQueue;
    std::shared_ptr<CabSimEngineQueue> engineQueue;
    std::unique_ptr<MultichannelEngine> previousEngine, currentEngine;
    CrossoverMixer mixer;
};

//==============================================================================
void CabSim::Mixer::prepare (const juce::dsp::ProcessSpec& spec)
{
    for (auto& dry : volumeDry)
        dry.reset (spec.sampleRate, 0.05);

    for (auto& wet : volumeWet)
        wet.reset (spec.sampleRate, 0.05);

    sampleRate = spec.sampleRate;

    dryBlock = juce::dsp::AudioBlock<float> (dryBlockStorage,
                                  juce::jmin (spec.numChannels, 2u),
                                  spec.maximumBlockSize);

}

template <typename ProcessWet>
void CabSim::Mixer::processSamples (const juce::dsp::AudioBlock<const float>& input,
                                         juce::dsp::AudioBlock<float>& output,
                                         bool isBypassed,
                                         ProcessWet&& processWet) noexcept
{
    const auto numChannels = jmin (input.getNumChannels(), volumeDry.size());
    const auto numSamples  = jmin (input.getNumSamples(), output.getNumSamples());

    auto dry = dryBlock.getSubsetChannelBlock (0, numChannels);

    //if (volumeDry[0].isSmoothing())
    //{
        dry.copyFrom (input);

        for (size_t channel = 0; channel < numChannels; ++channel)
            volumeDry[channel].applyGain (dry.getChannelPointer (channel), (int) numSamples);

        processWet (input, output);

        for (size_t channel = 0; channel < numChannels; ++channel)
            volumeWet[channel].applyGain (output.getChannelPointer (channel), (int) numSamples);

        output += dry;

        //Apply volume correction because IR makes output quieter
        for (size_t channel = 0; channel < numChannels; ++channel)
            volumeCorrection[channel].applyGain (output.getChannelPointer (channel), (int) numSamples);
    //}
    //else
    //{
    //    if (! currentIsBypassed)
    //        processWet (input, output);
    //
    //    if (isBypassed != currentIsBypassed)
    //    {
    //        currentIsBypassed = isBypassed;
    //
    //        applyWetLevel();
    //    }
    //}
}

void CabSim::Mixer::reset() { dryBlock.clear(); }
#include <fstream>
void CabSim::Mixer::setWetLevel(float wetLevel)
{
    this->wetLevel = juce::jlimit(0.0f, 1.0f, wetLevel);

    applyWetLevel();
}

void CabSim::Mixer::applyWetLevel()
{
    for (size_t channel = 0; channel < volumeDry.size(); ++channel)
    {
        volumeDry[channel].setTargetValue (currentIsBypassed ? 1.0f - wetLevel : 1.0f);
        volumeDry[channel].reset (sampleRate, 0.05);
        volumeDry[channel].setTargetValue (currentIsBypassed ? 1.0f : 1.0f - wetLevel);

        volumeWet[channel].setTargetValue (currentIsBypassed ? wetLevel : 0.0f);
        volumeWet[channel].reset (sampleRate, 0.05);
        volumeWet[channel].setTargetValue (currentIsBypassed ? 0.0f : wetLevel);

        volumeCorrection[channel].setTargetValue (currentIsBypassed ? 1.0f + wetLevel : 1.0f);
        volumeCorrection[channel].reset (sampleRate, 0.05);
        volumeCorrection[channel].setTargetValue (currentIsBypassed ? 1.0f : 1.0f + wetLevel);
    }
}

//==============================================================================
CabSim::CabSim()
    : CabSim (Latency { 0 })
{}

CabSim::CabSim (CabSimMessageQueue& queue)
    : CabSim (Latency { 0 }, queue)
{}

CabSim::CabSim (const Latency& requiredLatency)
    : CabSim (requiredLatency,
                   {},
                   OptionalQueue { std::make_unique<CabSimMessageQueue>() })
{}

CabSim::CabSim (const NonUniform& nonUniform)
    : CabSim ({},
                   nonUniform,
                   OptionalQueue { std::make_unique<CabSimMessageQueue>() })
{}

CabSim::CabSim (const Latency& requiredLatency, CabSimMessageQueue& queue)
    : CabSim (requiredLatency, {}, OptionalQueue { queue })
{}

CabSim::CabSim (const NonUniform& nonUniform, CabSimMessageQueue& queue)
    : CabSim ({}, nonUniform, OptionalQueue { queue })
{}

CabSim::CabSim (const Latency& latency,
                          const NonUniform& nonUniform,
                          OptionalQueue&& queue)
    : pimpl (std::make_unique<Impl> (latency, nonUniform, std::move (queue)))
{}

CabSim::~CabSim() noexcept = default;

void CabSim::loadImpulseResponse (const void* sourceData,
                                       size_t sourceDataSize,
                                       Stereo stereo,
                                       Trim trim,
                                       size_t size,
                                       Normalise normalise)
{
    pimpl->loadImpulseResponse (sourceData, sourceDataSize, stereo, trim, size, normalise);
}

void CabSim::loadImpulseResponse (const File& fileImpulseResponse,
                                       Stereo stereo,
                                       Trim trim,
                                       size_t size,
                                       Normalise normalise)
{
    pimpl->loadImpulseResponse (fileImpulseResponse, stereo, trim, size, normalise);
}

void CabSim::loadImpulseResponse (AudioBuffer<float>&& buffer,
                                       double originalSampleRate,
                                       Stereo stereo,
                                       Trim trim,
                                       Normalise normalise)
{
    pimpl->loadImpulseResponse (std::move (buffer), originalSampleRate, stereo, trim, normalise);
}

void CabSim::prepare (const juce::dsp::ProcessSpec& spec)
{
    mixer.prepare (spec);
    pimpl->prepare (spec);
    isActive = true;
}

void CabSim::reset() noexcept
{
    mixer.reset();
    pimpl->reset();
}

void CabSim::processSamples (const juce::dsp::AudioBlock<const float>& input,
                                  juce::dsp::AudioBlock<float>& output,
                                  bool isBypassed) noexcept
{
    if (! isActive)
        return;

    jassert (input.getNumChannels() == output.getNumChannels());
    jassert (isPositiveAndBelow (input.getNumChannels(), static_cast<size_t> (3))); // only mono and stereo is supported

    mixer.processSamples (input, output, isBypassed, [this] (const auto& in, auto& out)
    {
        pimpl->processSamples (in, out);
    });
}

int CabSim::getCurrentIRSize() const { return pimpl->getCurrentIRSize(); }

int CabSim::getLatency() const { return pimpl->getLatency(); }

void CabSim::setWetLevel(float wetLevel)
{
    mixer.setWetLevel(wetLevel);
}