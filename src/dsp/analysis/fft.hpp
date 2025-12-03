/**
 * @file fft.hpp
 * @brief FFT analysis utilities for Nomad DAW
 * @author Nomad Team
 * @date 2025
 * 
 * This file provides FFT-based analysis tools:
 * - Radix-2 FFT implementation
 * - Spectrum analyzer
 * - Window functions
 */

#pragma once

#include "../../core/base/types.hpp"
#include <vector>
#include <complex>
#include <cmath>
#include <algorithm>

namespace nomad::dsp {

//=============================================================================
// Window Functions
//=============================================================================

/**
 * @brief Window function types
 */
enum class WindowType : u8 {
    Rectangular,
    Hann,
    Hamming,
    Blackman,
    BlackmanHarris,
    FlatTop
};

/**
 * @brief Generate window function coefficients
 * @param type Window type
 * @param size Window size
 * @return Vector of window coefficients
 */
[[nodiscard]] inline std::vector<f32> generateWindow(WindowType type, u32 size) {
    std::vector<f32> window(size);
    const f64 pi = 3.14159265358979323846;
    const f64 N = static_cast<f64>(size);
    
    for (u32 i = 0; i < size; ++i) {
        const f64 n = static_cast<f64>(i);
        
        switch (type) {
            case WindowType::Rectangular:
                window[i] = 1.0f;
                break;
                
            case WindowType::Hann:
                window[i] = static_cast<f32>(0.5 * (1.0 - std::cos(2.0 * pi * n / (N - 1))));
                break;
                
            case WindowType::Hamming:
                window[i] = static_cast<f32>(0.54 - 0.46 * std::cos(2.0 * pi * n / (N - 1)));
                break;
                
            case WindowType::Blackman:
                window[i] = static_cast<f32>(
                    0.42 - 0.5 * std::cos(2.0 * pi * n / (N - 1))
                         + 0.08 * std::cos(4.0 * pi * n / (N - 1))
                );
                break;
                
            case WindowType::BlackmanHarris:
                window[i] = static_cast<f32>(
                    0.35875 - 0.48829 * std::cos(2.0 * pi * n / (N - 1))
                           + 0.14128 * std::cos(4.0 * pi * n / (N - 1))
                           - 0.01168 * std::cos(6.0 * pi * n / (N - 1))
                );
                break;
                
            case WindowType::FlatTop:
                window[i] = static_cast<f32>(
                    0.21557895 - 0.41663158 * std::cos(2.0 * pi * n / (N - 1))
                              + 0.277263158 * std::cos(4.0 * pi * n / (N - 1))
                              - 0.083578947 * std::cos(6.0 * pi * n / (N - 1))
                              + 0.006947368 * std::cos(8.0 * pi * n / (N - 1))
                );
                break;
        }
    }
    
    return window;
}

//=============================================================================
// FFT Implementation
//=============================================================================

/**
 * @brief Radix-2 FFT implementation
 * 
 * In-place Cooley-Tukey FFT algorithm. Size must be power of 2.
 */
class FFT {
public:
    using Complex = std::complex<f64>;
    
    /**
     * @brief Prepare FFT for given size
     * @param size FFT size (must be power of 2)
     */
    void prepare(u32 size) {
        m_size = size;
        m_logSize = static_cast<u32>(std::log2(size));
        
        // Pre-compute twiddle factors
        m_twiddles.resize(size / 2);
        const f64 twoPi = 6.283185307179586476925286766559;
        for (u32 i = 0; i < size / 2; ++i) {
            const f64 angle = -twoPi * i / size;
            m_twiddles[i] = Complex(std::cos(angle), std::sin(angle));
        }
        
        // Pre-compute bit-reversal indices
        m_bitReversal.resize(size);
        for (u32 i = 0; i < size; ++i) {
            m_bitReversal[i] = reverseBits(i, m_logSize);
        }
        
        // Allocate working buffer
        m_buffer.resize(size);
    }
    
    /**
     * @brief Perform forward FFT (time → frequency)
     * @param input Input samples (time domain)
     * @param output Output spectrum (frequency domain)
     */
    void forward(const f32* input, Complex* output) {
        // Bit-reversal permutation
        for (u32 i = 0; i < m_size; ++i) {
            m_buffer[m_bitReversal[i]] = Complex(input[i], 0.0);
        }
        
        // Cooley-Tukey decimation-in-time
        for (u32 stage = 1; stage <= m_logSize; ++stage) {
            const u32 halfStage = 1u << (stage - 1);
            const u32 stageSize = 1u << stage;
            const u32 twiddleStep = m_size >> stage;
            
            for (u32 k = 0; k < m_size; k += stageSize) {
                for (u32 j = 0; j < halfStage; ++j) {
                    const Complex& twiddle = m_twiddles[j * twiddleStep];
                    const Complex t = twiddle * m_buffer[k + j + halfStage];
                    const Complex u = m_buffer[k + j];
                    
                    m_buffer[k + j] = u + t;
                    m_buffer[k + j + halfStage] = u - t;
                }
            }
        }
        
        // Copy to output
        for (u32 i = 0; i < m_size; ++i) {
            output[i] = m_buffer[i];
        }
    }
    
    /**
     * @brief Perform inverse FFT (frequency → time)
     * @param input Input spectrum (frequency domain)
     * @param output Output samples (time domain)
     */
    void inverse(const Complex* input, f32* output) {
        // Conjugate input
        for (u32 i = 0; i < m_size; ++i) {
            m_buffer[m_bitReversal[i]] = std::conj(input[i]);
        }
        
        // Same butterfly as forward
        for (u32 stage = 1; stage <= m_logSize; ++stage) {
            const u32 halfStage = 1u << (stage - 1);
            const u32 stageSize = 1u << stage;
            const u32 twiddleStep = m_size >> stage;
            
            for (u32 k = 0; k < m_size; k += stageSize) {
                for (u32 j = 0; j < halfStage; ++j) {
                    const Complex& twiddle = m_twiddles[j * twiddleStep];
                    const Complex t = twiddle * m_buffer[k + j + halfStage];
                    const Complex u = m_buffer[k + j];
                    
                    m_buffer[k + j] = u + t;
                    m_buffer[k + j + halfStage] = u - t;
                }
            }
        }
        
        // Conjugate and scale output
        const f64 scale = 1.0 / m_size;
        for (u32 i = 0; i < m_size; ++i) {
            output[i] = static_cast<f32>(m_buffer[i].real() * scale);
        }
    }
    
    /**
     * @brief Perform real-valued FFT (optimized for real input)
     * @param input Input samples (real, time domain)
     * @param output Output spectrum (only positive frequencies, N/2+1 bins)
     */
    void forwardReal(const f32* input, Complex* output) {
        forward(input, output);
        // Note: For real input, output[N-k] = conj(output[k])
        // So we only need bins 0 to N/2
    }
    
    [[nodiscard]] u32 size() const noexcept { return m_size; }

private:
    [[nodiscard]] static u32 reverseBits(u32 value, u32 numBits) noexcept {
        u32 result = 0;
        for (u32 i = 0; i < numBits; ++i) {
            result = (result << 1) | (value & 1);
            value >>= 1;
        }
        return result;
    }
    
    u32 m_size = 0;
    u32 m_logSize = 0;
    std::vector<Complex> m_twiddles;
    std::vector<u32> m_bitReversal;
    std::vector<Complex> m_buffer;
};

//=============================================================================
// Spectrum Analyzer
//=============================================================================

/**
 * @brief Real-time spectrum analyzer
 * 
 * Provides smoothed magnitude spectrum for visualization.
 */
class SpectrumAnalyzer {
public:
    /**
     * @brief Prepare analyzer
     * @param fftSize FFT size (power of 2)
     * @param sampleRate Sample rate in Hz
     */
    void prepare(u32 fftSize, f64 sampleRate) {
        m_fftSize = fftSize;
        m_sampleRate = sampleRate;
        
        m_fft.prepare(fftSize);
        m_window = generateWindow(WindowType::BlackmanHarris, fftSize);
        
        m_inputBuffer.resize(fftSize, 0.0f);
        m_spectrum.resize(fftSize);
        m_magnitudes.resize(fftSize / 2 + 1, 0.0f);
        m_smoothedMagnitudes.resize(fftSize / 2 + 1, 0.0f);
        
        m_inputIndex = 0;
    }
    
    /**
     * @brief Push samples into analyzer
     * @param samples Input samples
     * @param numSamples Number of samples
     */
    void pushSamples(const f32* samples, u32 numSamples) {
        for (u32 i = 0; i < numSamples; ++i) {
            m_inputBuffer[m_inputIndex] = samples[i];
            m_inputIndex = (m_inputIndex + 1) % m_fftSize;
        }
    }
    
    /**
     * @brief Process FFT and update spectrum
     */
    void process() {
        // Apply window and copy to temp buffer
        std::vector<f32> windowed(m_fftSize);
        for (u32 i = 0; i < m_fftSize; ++i) {
            const u32 idx = (m_inputIndex + i) % m_fftSize;
            windowed[i] = m_inputBuffer[idx] * m_window[i];
        }
        
        // Perform FFT
        m_fft.forward(windowed.data(), m_spectrum.data());
        
        // Calculate magnitudes (dB)
        const f64 scale = 2.0 / m_fftSize;
        for (u32 i = 0; i <= m_fftSize / 2; ++i) {
            const f64 mag = std::abs(m_spectrum[i]) * scale;
            const f32 magDb = mag > 1e-10 
                ? static_cast<f32>(20.0 * std::log10(mag))
                : -200.0f;
            m_magnitudes[i] = magDb;
        }
        
        // Smooth magnitudes (exponential decay)
        for (u32 i = 0; i <= m_fftSize / 2; ++i) {
            if (m_magnitudes[i] > m_smoothedMagnitudes[i]) {
                // Fast attack
                m_smoothedMagnitudes[i] = m_magnitudes[i];
            } else {
                // Slow decay
                m_smoothedMagnitudes[i] += m_decayRate * (m_magnitudes[i] - m_smoothedMagnitudes[i]);
            }
        }
    }
    
    /**
     * @brief Get magnitude at frequency bin (in dB)
     */
    [[nodiscard]] f32 getMagnitude(u32 bin) const noexcept {
        if (bin <= m_fftSize / 2) {
            return m_smoothedMagnitudes[bin];
        }
        return -200.0f;
    }
    
    /**
     * @brief Get frequency at bin
     */
    [[nodiscard]] f64 getBinFrequency(u32 bin) const noexcept {
        return static_cast<f64>(bin) * m_sampleRate / m_fftSize;
    }
    
    /**
     * @brief Get bin for frequency
     */
    [[nodiscard]] u32 getFrequencyBin(f64 frequency) const noexcept {
        return static_cast<u32>(frequency * m_fftSize / m_sampleRate);
    }
    
    /**
     * @brief Get all smoothed magnitudes
     */
    [[nodiscard]] const std::vector<f32>& getMagnitudes() const noexcept {
        return m_smoothedMagnitudes;
    }
    
    /**
     * @brief Set decay rate for smoothing
     * @param rate Decay rate (0 = no decay, 1 = instant)
     */
    void setDecayRate(f32 rate) noexcept {
        m_decayRate = std::clamp(rate, 0.0f, 1.0f);
    }
    
    [[nodiscard]] u32 numBins() const noexcept { return m_fftSize / 2 + 1; }
    [[nodiscard]] f64 sampleRate() const noexcept { return m_sampleRate; }

private:
    u32 m_fftSize = 0;
    f64 m_sampleRate = 44100.0;
    f32 m_decayRate = 0.1f;
    
    FFT m_fft;
    std::vector<f32> m_window;
    std::vector<f32> m_inputBuffer;
    std::vector<FFT::Complex> m_spectrum;
    std::vector<f32> m_magnitudes;
    std::vector<f32> m_smoothedMagnitudes;
    u32 m_inputIndex = 0;
};

} // namespace nomad::dsp
