#pragma once

#include <JuceHeader.h>
#include <atomic>

/**
 * Controls transport state (play/stop/record) and timing for the DAW.
 * Thread-safe for use from both audio and UI threads.
 */
class TransportController
{
public:
    enum class State
    {
        Stopped,
        Paused,
        Playing,
        Recording
    };
    
    TransportController();
    ~TransportController() = default;
    
    // Transport control
    void play();
    void stop();
    void record();
    void togglePlayPause();
    
    // Position control
    void setPosition(double timeInBeats);
    void setPositionInSamples(int64_t samples);
    double getPosition() const;
    int64_t getPositionInSamples() const;
    
    // State queries
    State getState() const;
    bool isPlaying() const;
    bool isRecording() const;
    bool isStopped() const;
    
    // Tempo control
    void setTempo(double bpm);
    double getTempo() const;
    
    // Loop control
    void setLoopEnabled(bool enabled);
    bool isLoopEnabled() const;
    void setLoopPoints(double startBeats, double endBeats);
    double getLoopStart() const;
    double getLoopEnd() const;
    
    // Audio thread processing
    void advancePosition(int numSamples, double sampleRate);
    
    // Conversion utilities
    double samplesToBeats(int64_t samples, double sampleRate) const;
    int64_t beatsToSamples(double beats, double sampleRate) const;
    double beatsToSeconds(double beats) const;
    double secondsToBeats(double seconds) const;
    
    // Listener interface for state changes
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void transportStateChanged(State newState) {}
        virtual void transportPositionChanged(double positionInBeats) {}
        virtual void tempoChanged(double newTempo) {}
    };
    
    void addListener(Listener* listener);
    void removeListener(Listener* listener);
    
private:
    // Thread-safe state
    std::atomic<State> state { State::Stopped };
    std::atomic<double> positionInBeats { 0.0 };
    std::atomic<int64_t> positionInSamples { 0 };
    std::atomic<double> tempo { 120.0 };
    std::atomic<bool> loopEnabled { false };
    
    // Loop points (accessed from UI thread only)
    double loopStartBeats = 0.0;
    double loopEndBeats = 4.0;
    
    // Listeners
    juce::ListenerList<Listener> listeners;
    
    void notifyStateChanged();
    void notifyPositionChanged();
    void notifyTempoChanged();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportController)
};
