/**
 * @file Transport.cpp
 * @brief Sample-accurate transport system implementation
 */

#include "transport/Transport.h"
#include "audio/AudioEngine.h"

namespace nomad::transport
{
    Transport::Transport(nomad::audio::AudioEngine& audioEngine)
        : audioEngine(audioEngine)
    {
    }
    
    Transport::~Transport()
    {
        shutdown();
    }
    
    bool Transport::initialize()
    {
        try
        {
            // Initialize transport state
            currentState = TransportState::Stopped;
            currentTime = 0.0;
            currentBeat = 0.0;
            currentTempo = 120.0;
            timeSignatureNumerator = 4.0;
            timeSignatureDenominator = 4.0;
            loopEnabled = false;
            loopStart = 0.0;
            loopEnd = 0.0;
            
            return true;
        }
        catch (const std::exception& e)
        {
            return false;
        }
    }
    
    void Transport::shutdown()
    {
        stop();
    }
    
    void Transport::play(bool fromBeginning)
    {
        if (fromBeginning)
        {
            resetPosition();
        }
        
        currentState = TransportState::Playing;
        notifyCallbacks();
    }
    
    void Transport::pause()
    {
        currentState = TransportState::Paused;
        notifyCallbacks();
    }
    
    void Transport::stop()
    {
        currentState = TransportState::Stopped;
        resetPosition();
        notifyCallbacks();
    }
    
    void Transport::record()
    {
        currentState = TransportState::Recording;
        notifyCallbacks();
    }
    
    void Transport::stopRecording()
    {
        if (currentState.load() == TransportState::Recording)
        {
            currentState = TransportState::Playing;
            notifyCallbacks();
        }
    }
    
    void Transport::setTimePosition(double time)
    {
        currentTime = time;
        currentBeat = timeToBeats(time);
    }
    
    void Transport::setBeatPosition(double beat)
    {
        currentBeat = beat;
        currentTime = beatsToTime(beat);
    }
    
    void Transport::setTempo(double tempo)
    {
        currentTempo = tempo;
    }
    
    void Transport::setTimeSignature(double numerator, double denominator)
    {
        timeSignatureNumerator = numerator;
        timeSignatureDenominator = denominator;
    }
    
    void Transport::setLoopEnabled(bool enabled)
    {
        loopEnabled = enabled;
    }
    
    void Transport::setLoopRange(double start, double end)
    {
        loopStart = start;
        loopEnd = end;
    }
    
    void Transport::addTransportCallback(std::function<void(const TransportInfo&)> callback)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        transportCallbacks.push_back(callback);
    }
    
    void Transport::removeTransportCallback(std::function<void(const TransportInfo&)> callback)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        transportCallbacks.erase(
            std::remove(transportCallbacks.begin(), transportCallbacks.end(), callback),
            transportCallbacks.end()
        );
    }
    
    void Transport::processTransport(int numSamples)
    {
        if (currentState.load() == TransportState::Stopped)
            return;
        
        updateTimePosition(numSamples);
        checkLoopBoundary();
    }
    
    TransportInfo Transport::getTransportInfo() const
    {
        TransportInfo info;
        info.state = currentState.load();
        info.currentTime = currentTime.load();
        info.currentBeat = currentBeat.load();
        info.tempo = currentTempo.load();
        info.timeSignatureNumerator = timeSignatureNumerator.load();
        info.timeSignatureDenominator = timeSignatureDenominator.load();
        info.isLooping = loopEnabled.load();
        info.loopStart = loopStart.load();
        info.loopEnd = loopEnd.load();
        info.isRecording = (currentState.load() == TransportState::Recording);
        return info;
    }
    
    double Transport::timeToBeats(double time) const
    {
        double tempo = currentTempo.load();
        double beatsPerSecond = tempo / 60.0;
        return time * beatsPerSecond;
    }
    
    double Transport::beatsToTime(double beats) const
    {
        double tempo = currentTempo.load();
        double beatsPerSecond = tempo / 60.0;
        return beats / beatsPerSecond;
    }
    
    int Transport::getSamplesPerBeat() const
    {
        double tempo = currentTempo.load();
        double sampleRate = audioEngine.getSampleRate();
        double beatsPerSecond = tempo / 60.0;
        return static_cast<int>(sampleRate / beatsPerSecond);
    }
    
    double Transport::getSamplesPerSecond() const
    {
        return audioEngine.getSampleRate();
    }
    
    void Transport::updateTimePosition(int numSamples)
    {
        double sampleRate = audioEngine.getSampleRate();
        double timeIncrement = numSamples / sampleRate;
        
        double newTime = currentTime.load() + timeIncrement;
        currentTime = newTime;
        
        double newBeat = timeToBeats(newTime);
        currentBeat = newBeat;
        
        lastProcessTime = newTime;
        lastProcessBeat = newBeat;
    }
    
    void Transport::checkLoopBoundary()
    {
        if (!loopEnabled.load())
            return;
        
        double currentTimeValue = currentTime.load();
        double loopStartValue = loopStart.load();
        double loopEndValue = loopEnd.load();
        
        if (currentTimeValue >= loopEndValue)
        {
            currentTime = loopStartValue;
            currentBeat = timeToBeats(loopStartValue);
        }
    }
    
    void Transport::notifyCallbacks()
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        TransportInfo info = getTransportInfo();
        
        for (auto& callback : transportCallbacks)
        {
            callback(info);
        }
    }
    
    void Transport::resetPosition()
    {
        currentTime = 0.0;
        currentBeat = 0.0;
        lastProcessTime = 0.0;
        lastProcessBeat = 0.0;
    }
}