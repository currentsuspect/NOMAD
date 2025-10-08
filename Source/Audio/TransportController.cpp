#include "TransportController.h"

TransportController::TransportController()
{
    // Enable loop by default with 4-bar loop (16 beats in 4/4 time)
    loopEnabled.store(true);
    loopStartBeats = 0.0;
    loopEndBeats = 16.0; // 4 bars * 4 beats
}

void TransportController::play()
{
    auto currentState = state.load();
    if (currentState == State::Stopped)
    {
        state.store(State::Playing);
        notifyStateChanged();
    }
}

void TransportController::stop()
{
    auto currentState = state.load();
    if (currentState != State::Stopped)
    {
        state.store(State::Stopped);
        notifyStateChanged();
    }
    
    // Always reset position to zero when stop is pressed
    positionInBeats.store(0.0);
    positionInSamples.store(0);
    notifyPositionChanged();
}

void TransportController::record()
{
    state.store(State::Recording);
    notifyStateChanged();
}

void TransportController::togglePlayPause()
{
    auto currentState = state.load();
    if (currentState == State::Playing || currentState == State::Recording)
    {
        // Pause - keep position
        state.store(State::Paused);
    }
    else
    {
        // Play from current position
        state.store(State::Playing);
    }
    notifyStateChanged();
}

void TransportController::setPosition(double timeInBeats)
{
    positionInBeats.store(timeInBeats);
    notifyPositionChanged();
}

void TransportController::setPositionInSamples(int64_t samples)
{
    positionInSamples.store(samples);
}

double TransportController::getPosition() const
{
    return positionInBeats.load();
}

int64_t TransportController::getPositionInSamples() const
{
    return positionInSamples.load();
}

TransportController::State TransportController::getState() const
{
    return state.load();
}

bool TransportController::isPlaying() const
{
    return state.load() == State::Playing;
}

bool TransportController::isRecording() const
{
    return state.load() == State::Recording;
}

bool TransportController::isStopped() const
{
    return state.load() == State::Stopped;
}

void TransportController::setTempo(double bpm)
{
    // Clamp tempo to reasonable range
    bpm = juce::jlimit(20.0, 999.0, bpm);
    tempo.store(bpm);
    notifyTempoChanged();
}

double TransportController::getTempo() const
{
    return tempo.load();
}

void TransportController::setLoopEnabled(bool enabled)
{
    loopEnabled.store(enabled);
}

bool TransportController::isLoopEnabled() const
{
    return loopEnabled.load();
}

void TransportController::setLoopPoints(double startBeats, double endBeats)
{
    loopStartBeats = startBeats;
    loopEndBeats = endBeats;
}

double TransportController::getLoopStart() const
{
    return loopStartBeats;
}

double TransportController::getLoopEnd() const
{
    return loopEndBeats;
}

void TransportController::advancePosition(int numSamples, double sampleRate)
{
    auto currentState = state.load();
    if (currentState == State::Stopped || currentState == State::Paused)
        return;
    
    // Update sample position
    int64_t currentSamples = positionInSamples.load();
    currentSamples += numSamples;
    positionInSamples.store(currentSamples);
    
    // Convert to beats
    double beats = samplesToBeats(currentSamples, sampleRate);
    
    // Handle looping
    if (loopEnabled.load())
    {
        double loopStart = loopStartBeats;
        double loopEnd = loopEndBeats;
        
        if (beats >= loopEnd)
        {
            // Wrap around to loop start
            double loopLength = loopEnd - loopStart;
            if (loopLength > 0.0)
            {
                beats = loopStart + std::fmod(beats - loopStart, loopLength);
                currentSamples = beatsToSamples(beats, sampleRate);
                positionInSamples.store(currentSamples);
            }
        }
    }
    
    positionInBeats.store(beats);
}

double TransportController::samplesToBeats(int64_t samples, double sampleRate) const
{
    if (sampleRate <= 0.0)
        return 0.0;
    
    double seconds = static_cast<double>(samples) / sampleRate;
    return secondsToBeats(seconds);
}

int64_t TransportController::beatsToSamples(double beats, double sampleRate) const
{
    double seconds = beatsToSeconds(beats);
    return static_cast<int64_t>(seconds * sampleRate);
}

double TransportController::beatsToSeconds(double beats) const
{
    double currentTempo = tempo.load();
    if (currentTempo <= 0.0)
        return 0.0;
    
    // beats * (60 seconds / minute) / (beats / minute) = seconds
    return beats * 60.0 / currentTempo;
}

double TransportController::secondsToBeats(double seconds) const
{
    double currentTempo = tempo.load();
    if (currentTempo <= 0.0)
        return 0.0;
    
    // seconds * (beats / minute) / (60 seconds / minute) = beats
    return seconds * currentTempo / 60.0;
}

void TransportController::addListener(Listener* listener)
{
    listeners.add(listener);
}

void TransportController::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

void TransportController::notifyStateChanged()
{
    auto currentState = state.load();
    listeners.call([currentState](Listener& l) { l.transportStateChanged(currentState); });
}

void TransportController::notifyPositionChanged()
{
    double position = positionInBeats.load();
    listeners.call([position](Listener& l) { l.transportPositionChanged(position); });
}

void TransportController::notifyTempoChanged()
{
    double currentTempo = tempo.load();
    listeners.call([currentTempo](Listener& l) { l.tempoChanged(currentTempo); });
}
