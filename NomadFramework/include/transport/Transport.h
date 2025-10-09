/**
 * @file Transport.h
 * @brief Sample-accurate transport system with thread-safe synchronization
 * @author Nomad Framework Team
 */

#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <mutex>
#include <vector>
#include <functional>
#include <memory>

namespace nomad::transport
{
    /**
     * @enum TransportState
     * @brief Transport state enumeration
     */
    enum class TransportState
    {
        Stopped,
        Playing,
        Paused,
        Recording
    };
    
    /**
     * @struct TransportInfo
     * @brief Transport information structure
     */
    struct TransportInfo
    {
        TransportState state = TransportState::Stopped;
        double currentTime = 0.0;           // Current time in seconds
        double currentBeat = 0.0;           // Current beat position
        double tempo = 120.0;               // Current tempo in BPM
        double timeSignatureNumerator = 4.0;   // Time signature numerator
        double timeSignatureDenominator = 4.0; // Time signature denominator
        bool isLooping = false;             // Loop enabled
        double loopStart = 0.0;             // Loop start time
        double loopEnd = 0.0;               // Loop end time
        bool isRecording = false;           // Recording state
    };
    
    /**
     * @class Transport
     * @brief Sample-accurate transport control system
     * 
     * Provides play/pause/stop/record functionality with tight audio-thread
     * synchronization, sample-accurate positioning, and thread-safe callbacks.
     */
    class Transport
    {
    public:
        /**
         * @brief Constructor
         * @param audioEngine Reference to the audio engine
         */
        Transport(nomad::audio::AudioEngine& audioEngine);
        
        /**
         * @brief Destructor
         */
        ~Transport();
        
        /**
         * @brief Initialize the transport system
         * @return true if successful
         */
        bool initialize();
        
        /**
         * @brief Shutdown the transport system
         */
        void shutdown();
        
        /**
         * @brief Start playback
         * @param fromBeginning If true, start from beginning
         */
        void play(bool fromBeginning = false);
        
        /**
         * @brief Pause playback
         */
        void pause();
        
        /**
         * @brief Stop playback and reset position
         */
        void stop();
        
        /**
         * @brief Start recording
         */
        void record();
        
        /**
         * @brief Stop recording
         */
        void stopRecording();
        
        /**
         * @brief Get current transport state
         * @return Current state
         */
        TransportState getState() const { return currentState.load(); }
        
        /**
         * @brief Check if transport is playing
         * @return True if playing
         */
        bool isPlaying() const { return currentState.load() == TransportState::Playing; }
        
        /**
         * @brief Check if transport is recording
         * @return True if recording
         */
        bool isRecording() const { return currentState.load() == TransportState::Recording; }
        
        /**
         * @brief Set current time position
         * @param time Time in seconds
         */
        void setTimePosition(double time);
        
        /**
         * @brief Get current time position
         * @return Time in seconds
         */
        double getTimePosition() const { return currentTime.load(); }
        
        /**
         * @brief Set current beat position
         * @param beat Beat position
         */
        void setBeatPosition(double beat);
        
        /**
         * @brief Get current beat position
         * @return Beat position
         */
        double getBeatPosition() const { return currentBeat.load(); }
        
        /**
         * @brief Set tempo
         * @param tempo Tempo in BPM
         */
        void setTempo(double tempo);
        
        /**
         * @brief Get tempo
         * @return Tempo in BPM
         */
        double getTempo() const { return currentTempo.load(); }
        
        /**
         * @brief Set time signature
         * @param numerator Time signature numerator
         * @param denominator Time signature denominator
         */
        void setTimeSignature(double numerator, double denominator);
        
        /**
         * @brief Get time signature numerator
         * @return Numerator
         */
        double getTimeSignatureNumerator() const { return timeSignatureNumerator.load(); }
        
        /**
         * @brief Get time signature denominator
         * @return Denominator
         */
        double getTimeSignatureDenominator() const { return timeSignatureDenominator.load(); }
        
        /**
         * @brief Set loop enabled
         * @param enabled True to enable looping
         */
        void setLoopEnabled(bool enabled);
        
        /**
         * @brief Check if looping is enabled
         * @return True if enabled
         */
        bool isLoopEnabled() const { return loopEnabled.load(); }
        
        /**
         * @brief Set loop range
         * @param start Loop start time in seconds
         * @param end Loop end time in seconds
         */
        void setLoopRange(double start, double end);
        
        /**
         * @brief Get loop start time
         * @return Start time in seconds
         */
        double getLoopStart() const { return loopStart.load(); }
        
        /**
         * @brief Get loop end time
         * @return End time in seconds
         */
        double getLoopEnd() const { return loopEnd.load(); }
        
        /**
         * @brief Add transport callback
         * @param callback Function to call on transport changes
         */
        void addTransportCallback(std::function<void(const TransportInfo&)> callback);
        
        /**
         * @brief Remove transport callback
         * @param callback Function to remove
         */
        void removeTransportCallback(std::function<void(const TransportInfo&)> callback);
        
        /**
         * @brief Process transport (call from audio thread)
         * @param numSamples Number of samples to process
         */
        void processTransport(int numSamples);
        
        /**
         * @brief Get current transport info
         * @return Transport information
         */
        TransportInfo getTransportInfo() const;
        
        /**
         * @brief Convert time to beats
         * @param time Time in seconds
         * @return Beats
         */
        double timeToBeats(double time) const;
        
        /**
         * @brief Convert beats to time
         * @param beats Beat position
         * @return Time in seconds
         */
        double beatsToTime(double beats) const;
        
        /**
         * @brief Get samples per beat
         * @return Samples per beat
         */
        int getSamplesPerBeat() const;
        
        /**
         * @brief Get samples per second
         * @return Samples per second
         */
        double getSamplesPerSecond() const;
        
    private:
        // Reference to audio engine
        nomad::audio::AudioEngine& audioEngine;
        
        // Transport state
        std::atomic<TransportState> currentState{TransportState::Stopped};
        std::atomic<double> currentTime{0.0};
        std::atomic<double> currentBeat{0.0};
        std::atomic<double> currentTempo{120.0};
        std::atomic<double> timeSignatureNumerator{4.0};
        std::atomic<double> timeSignatureDenominator{4.0};
        
        // Loop settings
        std::atomic<bool> loopEnabled{false};
        std::atomic<double> loopStart{0.0};
        std::atomic<double> loopEnd{0.0};
        
        // Callbacks
        std::vector<std::function<void(const TransportInfo&)>> transportCallbacks;
        std::mutex callbackMutex;
        
        // Internal state
        double lastProcessTime = 0.0;
        double lastProcessBeat = 0.0;
        
        // Internal methods
        void updateTimePosition(int numSamples);
        void checkLoopBoundary();
        void notifyCallbacks();
        void resetPosition();
    };
}