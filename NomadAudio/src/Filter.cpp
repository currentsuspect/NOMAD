// ============================================================================
// © 2025 Nomad Studios — Audio DSP Filter Library
// ============================================================================

#include "Filter.h"
#include <cstring>

#ifdef Filter
#undef Filter
#endif

namespace Nomad {
namespace Audio {
namespace DSP {

// ============================================================================
// Constructor & initialization
/**
 * @brief Constructs a Filter configured for the given audio sample rate.
 *
 * @param sampleRate Sample rate in hertz used for internal coefficient and timing calculations.
 */

Filter::Filter(float sampleRate)
    : m_sampleRate(sampleRate)
{
    prepare(sampleRate);
    reset();
}

/**
 * @brief Prepare the filter for a new sample rate and initialize internal state.
 *
 * Updates the filter's sample-rate-dependent constants, resets internal processing
 * state, resizes the oversampling buffer if oversampling is enabled, and
 * initializes filter coefficients.
 *
 * @param sampleRate Sample rate in Hz to configure the filter for (must be > 0).
 */
void Filter::prepare(float sampleRate)
{
    m_sampleRate = sampleRate;
    m_nyquist = sampleRate * NYQUIST_MARGIN;
    m_inverseSampleRate = 1.0f / sampleRate;
    
    // Reset states
    reset();
    
    // Resize oversampling buffer if needed
    if (m_oversampling != OversamplingFactor::None)
    {
        uint32_t bufferSize = 1024 * static_cast<int>(m_oversampling);
        m_oversampleBuffer.resize(bufferSize);
    }
    
    // Initialize coefficients
    updateCoefficients();
}

// ============================================================================
// Parameter setters
/**
 * @brief Set the filter response type and schedule coefficient recomputation.
 *
 * Updates the filter type used to compute biquad coefficients. If `smooth` is false,
 * coefficients are recalculated immediately; otherwise the change is marked so that
 * coefficients will be updated later with smoothing applied.
 *
 * @param type Desired filter type.
 * @param smooth If true, defer coefficient updates to allow smoothing; if false, apply immediately.
 */

void Filter::setType(FilterType type, bool smooth)
{
    if (m_type != type)
    {
        m_type = type;
        if (!smooth)
        {
            // Immediate update for type changes (optional smoothing)
            updateCoefficients();
        }
        m_needsUpdate = true;
    }
}

/**
 * @brief Set the filter's target cutoff frequency with optional modulation.
 *
 * Sets the internal target cutoff frequency after applying a modulation amount,
 * constraining the result to the audible range [20 Hz, nyquist]. If the new
 * target differs from the previous target by more than 0.01 Hz, the method
 * records the modulation amount and marks parameters as changed so coefficients
 * will be updated.
 *
 * @param frequency Base cutoff frequency in Hz.
 * @param modulate Modulation amount in the range [-1, 1]; ±1 corresponds to ±2 octaves applied to the base frequency.
 */
void Filter::setCutoff(float frequency, float modulate)
{
    // Clamp with modulation range
    float minCutoff = 20.0f;
    float maxCutoff = m_nyquist;
    
    // Apply modulation (-1 to 1 gives ±2 octaves)
    float modulatedFreq = frequency * std::pow(2.0f, modulate * 2.0f);
    
    // Convert to log scale for smoother interpolation
    float logFreq = std::log10(std::max(modulatedFreq, 0.1f));
    float logMin = std::log10(minCutoff);
    float logMax = std::log10(maxCutoff);
    
    float newCutoff = std::clamp(std::pow(10.0f, logFreq), minCutoff, maxCutoff);
    
    if (std::abs(newCutoff - m_targetCutoff) > 0.01f)
    {
        m_targetCutoff = newCutoff;
        m_cutoffMod = modulate;
        m_parametersChanged = true;
    }
}

/**
 * @brief Sets the filter's target resonance with optional modulation.
 *
 * Applies modulation to the supplied resonance, clamps the result to [0.1, 10],
 * and updates the stored target resonance and modulation state when the
 * adjusted value differs from the current target by more than 0.001, marking
 * parameters for an update.
 *
 * @param resonance Base resonance value before modulation.
 * @param modulate Modulation amount applied multiplicatively as (1 + modulate * 0.5).
 */
void Filter::setResonance(float resonance, float modulate)
{
    float newResonance = std::clamp(resonance * (1.0f + modulate * 0.5f), 0.1f, 10.0f);
    
    if (std::abs(newResonance - m_targetResonance) > 0.001f)
    {
        m_targetResonance = newResonance;
        m_resonanceMod = modulate;
        m_parametersChanged = true;
    }
}

/**
 * @brief Sets the filter target gain in decibels, clamped to the allowed range.
 *
 * @param gain Desired gain in dB; values outside [DB_MIN, DB_MAX] are clamped.
 *
 * Updates the internal target gain and marks parameters as changed when the
 * clamped value differs from the current target by more than 0.01 dB.
 */
void Filter::setGain(float gain)
{
    float newGain = std::clamp(gain, DB_MIN, DB_MAX);
    
    if (std::abs(newGain - m_targetGain) > 0.01f)
    {
        m_targetGain = newGain;
        m_parametersChanged = true;
    }
}

/**
 * @brief Set the filter's slope (filter order) and schedule coefficient recalculation if changed.
 *
 * Updates the configured slope and marks the filter coefficients as needing an update so the
 * new slope takes effect on the next coefficient recomputation.
 *
 * @param slope Desired filter slope (e.g., 12 dB/octave, 24 dB/octave) as a FilterSlope value.
 */
void Filter::setSlope(FilterSlope slope)
{
    if (m_slope != slope)
    {
        m_slope = slope;
        m_needsUpdate = true;
    }
}

/**
 * @brief Sets the pre-filter drive amount and the saturation curve.
 *
 * @param drive Desired drive amount applied to the input before filtering. Values are clamped to the range [0.0, 1.0].
 * @param saturation Saturation curve to use when applying drive (e.g., SoftClip, HardClip, Tanh, Asymmetric).
 */
void Filter::setDrive(float drive, SaturationType saturation)
{
    m_drive = std::clamp(drive, 0.0f, 1.0f);
    m_saturationType = saturation;
}

/**
 * @brief Configure the oversampling factor used by the filter.
 *
 * When the factor changes, updates internal state: if oversampling is enabled,
 * resizes the internal oversample buffer to 1024 * factor samples and clears
 * the internal upsample/downsample filter states.
 *
 * @param factor Desired oversampling factor; use OversamplingFactor::None to disable oversampling.
 */
void Filter::setOversampling(OversamplingFactor factor)
{
    if (m_oversampling != factor)
    {
        m_oversampling = factor;
        
        if (factor != OversamplingFactor::None)
        {
            uint32_t bufferSize = 1024 * static_cast<int>(factor);
            m_oversampleBuffer.resize(bufferSize);
        }
        
        // Reset filter states for oversampling
        std::memset(m_upsampleFilter, 0, sizeof(m_upsampleFilter));
        std::memset(m_downsampleFilter, 0, sizeof(m_downsampleFilter));
    }
}

/**
 * @brief Configure parameter smoothing duration and interpolation type.
 *
 * Sets the smoothing time (in milliseconds) and the smoothing interpolation mode,
 * and updates the internal smoothing coefficient used to interpolate parameters
 * and coefficients over time.
 *
 * @param timeMs Smoothing time in milliseconds; negative values are clamped to 0.
 * @param type Interpolation type used for smoothing (Linear, Exponential, Cosine, etc.).
 */
void Filter::setSmoothing(float timeMs, SmoothingType type)
{
    m_smoothingTimeMs = std::max(0.0f, timeMs);
    m_smoothingType = type;
    
    // Calculate smoothing coefficient
    float timeConstant = m_smoothingTimeMs * 0.001f;
    float samples = timeConstant * m_sampleRate;
    m_smoothingAlpha = std::exp(-1.0f / samples);
}

/**
 * @brief Enable or disable stereo parameter linking.
 *
 * When enabled, the right channel's filter coefficients and parameter updates are kept
 * identical to the left channel. When disabled, left and right channels can have
 * independent coefficients and parameter updates.
 *
 * @param linked If `true`, link right channel to left; if `false`, allow independent channels.
 */
void Filter::setStereoLinked(bool linked)
{
    m_stereoLinked = linked;
}

/**
 * @brief Enable or disable self-oscillation behavior for resonance handling.
 *
 * When enabled, the filter allows resonance to approach higher values used for
 * self-oscillation behavior; when disabled, resonance is constrained to the
 * normal stability limits.
 *
 * @param enabled `true` to enable self-oscillation behavior, `false` to disable it.
 */
void Filter::setSelfOscillation(bool enabled)
{
    m_selfOscillation = enabled;
}

// ============================================================================
// Core processing
/**
 * @brief Process a single mono input sample through the filter.
 *
 * Updates smoothed parameters, applies drive and the selected saturation curve before filtering,
 * and processes the sample using the configured oversampling and biquad path.
 *
 * @param input Input sample to process.
 * @return float Processed output sample.
 */

float Filter::process(float input)
{
    // Update parameters with smoothing
    updateInternal();
    
    // Apply drive/saturation before filtering
    float processed = input;
    if (m_drive > 0.0f && m_saturationType != SaturationType::None)
    {
        processed = applySaturation(processed * (1.0f + m_drive * 3.0f));
    }
    
    // Process with oversampling if enabled
    if (m_oversampling != OversamplingFactor::None)
    {
        processOversampled(processed, m_state[0]);
    }
    else
    {
        processSample(processed, m_state[0], m_coeffs[0]);
    }
    
    return processed;
}

/**
 * @brief Processes a stereo input pair through the filter and returns the filtered outputs.
 *
 * The function applies configured drive and saturation to both inputs before filtering.
 * Each channel is then processed using either the oversampled path (if enabled) or the standard biquad path.
 * If stereo linking is enabled, the right channel uses the same coefficients as the left.
 *
 * @return Pair where `first` is the processed left sample and `second` is the processed right sample.
 */
std::pair<float, float> Filter::processStereo(float left, float right)
{
    updateInternal();
    
    // Apply drive to both channels
    if (m_drive > 0.0f && m_saturationType != SaturationType::None)
    {
        float driveFactor = 1.0f + m_drive * 3.0f;
        left = applySaturation(left * driveFactor);
        right = applySaturation(right * driveFactor);
    }
    
    // Process left channel
    float outLeft = left;
    if (m_oversampling != OversamplingFactor::None)
    {
        processOversampled(outLeft, m_state[0]);
    }
    else
    {
        processSample(outLeft, m_state[0], m_coeffs[0]);
    }
    
    // Process right channel (use same coefficients if linked)
    float outRight = right;
    const auto& rightCoeffs = m_stereoLinked ? m_coeffs[0] : m_coeffs[1];
    if (m_oversampling != OversamplingFactor::None)
    {
        // For simplicity in stereo oversampling, we'll process normally
        processSample(outRight, m_state[1], rightCoeffs);
    }
    else
    {
        processSample(outRight, m_state[1], rightCoeffs);
    }
    
    return {outLeft, outRight};
}

/**
 * @brief Processes a mono buffer of samples in-place through the filter.
 *
 * Updates internal parameters once for the entire block, applies drive and the
 * selected saturation to each sample, then filters each sample using the
 * configured processing path (standard or oversampled).
 *
 * @param samples Pointer to a contiguous array of mono samples to process in-place.
 * @param numSamples Number of samples in the array; if zero the function returns immediately.
 */
void Filter::processBlock(float* samples, uint32_t numSamples)
{
    if (numSamples == 0) return;
    
    // Update parameters once per block (optimization)
    updateInternal();
    
    for (uint32_t i = 0; i < numSamples; ++i)
    {
        float sample = samples[i];
        
        // Apply drive
        if (m_drive > 0.0f && m_saturationType != SaturationType::None)
        {
            sample = applySaturation(sample * (1.0f + m_drive * 3.0f));
        }
        
        // Process
        if (m_oversampling != OversamplingFactor::None)
        {
            processOversampled(sample, m_state[0]);
        }
        else
        {
            processSample(sample, m_state[0], m_coeffs[0]);
        }
        
        samples[i] = sample;
    }
}

/**
 * @brief Processes a block of interleaved stereo samples in-place.
 *
 * Processes up to numSamples samples from the separate left and right buffers,
 * applying drive/saturation, filtering (with optional oversampling), and per-channel state. If stereo linking is enabled, the right channel uses the left channel's coefficients.
 *
 * @param left Pointer to the left-channel sample buffer; must contain at least numSamples samples. Processed samples are written back into this buffer.
 * @param right Pointer to the right-channel sample buffer; must contain at least numSamples samples. Processed samples are written back into this buffer.
 * @param numSamples Number of samples to process from each buffer. If zero, the function returns immediately.
 */
void Filter::processBlockStereo(float* left, float* right, uint32_t numSamples)
{
    if (numSamples == 0) return;
    
    updateInternal();
    
    const auto& rightCoeffs = m_stereoLinked ? m_coeffs[0] : m_coeffs[1];
    
    for (uint32_t i = 0; i < numSamples; ++i)
    {
        // Process left
        float leftSample = left[i];
        if (m_drive > 0.0f && m_saturationType != SaturationType::None)
        {
            leftSample = applySaturation(leftSample * (1.0f + m_drive * 3.0f));
        }
        
        if (m_oversampling != OversamplingFactor::None)
        {
            processOversampled(leftSample, m_state[0]);
        }
        else
        {
            processSample(leftSample, m_state[0], m_coeffs[0]);
        }
        
        // Process right
        float rightSample = right[i];
        if (m_drive > 0.0f && m_saturationType != SaturationType::None)
        {
            rightSample = applySaturation(rightSample * (1.0f + m_drive * 3.0f));
        }
        
        if (m_oversampling != OversamplingFactor::None)
        {
            processSample(rightSample, m_state[1], rightCoeffs);
        }
        else
        {
            processSample(rightSample, m_state[1], rightCoeffs);
        }
        
        left[i] = leftSample;
        right[i] = rightSample;
    }
}

// ============================================================================
// Internal processing methods
/**
 * @brief Filters a single sample in-place using a biquad Direct Form II transposed structure.
 *
 * Processes the provided sample through the biquad defined by `coeffs`, updates the
 * filter `state` delay elements, and stores the filtered value back into `sample`.
 *
 * @param sample Reference to the input sample; replaced with the filtered output.
 * @param state Per-channel filter state containing delay elements that are updated.
 * @param coeffs Biquad filter coefficients (b0, b1, b2, a1, a2) used for processing.
 */

void Filter::processSample(float& sample, FilterState& state, const BiquadCoefficients& coeffs)
{
    // Direct Form II transposed implementation (more efficient)
    float output = coeffs.b0 * sample + state.x1;
    state.x1 = coeffs.b1 * sample - coeffs.a1 * output + state.x2;
    state.x2 = coeffs.b2 * sample - coeffs.a2 * output;
    
    sample = output;
}

/**
 * @brief Applies a simple 2x oversampling processing path to a single sample.
 *
 * When the filter's oversampling mode is TwoX, runs a basic upsample→filter→downsample sequence that advances the provided filter state
 * and replaces the input sample with the downsampled result. When oversampling is not TwoX, this function has no effect.
 *
 * @param sample Reference to the sample to process; overwritten with the processed (downsampled) value when oversampling is active.
 * @param state Per-channel filter state (delay elements) which is updated by the internal filtering steps.
 */
void Filter::processOversampled(float& sample, FilterState& state)
{
    // Simple 2x oversampling implementation
    if (m_oversampling == OversamplingFactor::TwoX)
    {
        // Upsample: insert zero and filter
        float upsampled[2];
        
        // First sample
        upsampled[0] = sample;
        processSample(upsampled[0], state, m_coeffs[0]);
        
        // Interpolated sample
        upsampled[1] = sample;  // Simple hold (better interpolation can be added)
        processSample(upsampled[1], state, m_coeffs[0]);
        
        // Downsample: average (simple low-pass)
        sample = (upsampled[0] + upsampled[1]) * 0.5f;
    }
}

// ============================================================================
// Coefficient calculation methods
/**
 * @brief Compute and apply pending biquad coefficients, marking them as up-to-date.
 *
 * Ensures the filter's coefficient set reflects the current parameter targets and
 * clears the internal flag that indicates an update is required.
 */

void Filter::updateCoefficients()
{
    updateCoefficientsInternal();
    m_needsUpdate = false;
}

/**
 * @brief Compute and set the target biquad coefficients from current filter parameters.
 *
 * Calculates normalized frequency, resonance (respecting self-oscillation limits), and linear gain,
 * then populates the class's target biquad coefficients for the configured filter type.
 *
 * If stereo linking is enabled, the right-channel target coefficients are copied from the left.
 * After computing targets, coefficient smoothing is applied via updateSmoothing().
 */
void Filter::updateCoefficientsInternal()
{
    // Calculate normalized frequency
    float w0 = TWO_PI * m_currentCutoff * m_inverseSampleRate;
    float sinW0 = std::sin(w0);
    float cosW0 = std::cos(w0);
    
    // Clamp resonance based on self-oscillation
    float Q = m_selfOscillation ? 
        std::min(m_currentResonance, 0.99f) : 
        std::clamp(m_currentResonance, 0.1f, 10.0f);
    
    float alpha = sinW0 / (2.0f * Q);
    
    // Convert gain from dB to linear
    float A = std::pow(10.0f, m_currentGain / 40.0f);
    
    // Calculate coefficients based on filter type
    switch (m_type)
    {
    case FilterType::LowPass:
        calculateLowPass(m_targetCoeffs[0], w0, alpha, A);
        break;
        
    case FilterType::HighPass:
        calculateHighPass(m_targetCoeffs[0], w0, alpha, A);
        break;
        
    case FilterType::BandPass:
        calculateBandPass(m_targetCoeffs[0], w0, alpha);
        break;
        
    case FilterType::Notch:
        calculateNotch(m_targetCoeffs[0], w0, alpha);
        break;
        
    case FilterType::LowShelf:
        calculateLowShelf(m_targetCoeffs[0], w0, alpha, A);
        break;
        
    case FilterType::HighShelf:
        calculateHighShelf(m_targetCoeffs[0], w0, alpha, A);
        break;
        
    case FilterType::Peak:
        calculatePeak(m_targetCoeffs[0], w0, alpha, A);
        break;
        
    case FilterType::AllPass:
        calculateAllPass(m_targetCoeffs[0], w0, alpha);
        break;
    }
    
    // Copy to right channel if linked
    if (m_stereoLinked)
    {
        m_targetCoeffs[1] = m_targetCoeffs[0];
    }
    
    // Apply coefficient smoothing
    updateSmoothing();
}

/**
 * @brief Computes normalized biquad coefficients for a 2nd-order low-pass filter.
 *
 * Populates `coeffs` with normalized feedforward (b0, b1, b2) and feedback (a1, a2)
 * coefficients using the common RBJ cookbook formulation for a low-pass biquad.
 *
 * @param coeffs Destination structure whose `b0`, `b1`, `b2`, `a1`, and `a2` members will be set.
 * @param w0 Normalized angular center/cutoff frequency in radians (0..pi).
 * @param alpha Frequency-dependent bandwidth/shape term (typically = sin(w0)/(2*Q)).
 * @param A Linear gain parameter (present for API parity with shelving/peaking calculators; ignored for low-pass).
 */
void Filter::calculateLowPass(BiquadCoefficients& coeffs, float w0, float alpha, float A) const noexcept
{
    float b0 = (1.0f - std::cos(w0)) / 2.0f;
    float b1 = 1.0f - std::cos(w0);
    float b2 = b0;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * std::cos(w0);
    float a2 = 1.0f - alpha;
    
    coeffs.b0 = b0 / a0;
    coeffs.b1 = b1 / a0;
    coeffs.b2 = b2 / a0;
    coeffs.a1 = a1 / a0;
    coeffs.a2 = a2 / a0;
}

/**
 * @brief Compute and populate biquad coefficients for a second-order high-pass filter.
 *
 * Populates `coeffs` with normalized coefficients (a0 scaled to 1) for a digital
 * second-order high-pass biquad defined by the normalized angular frequency and
 * bandwidth parameter.
 *
 * @param coeffs Output structure to receive `b0`, `b1`, `b2`, `a1`, and `a2`.
 * @param w0 Normalized angular frequency in radians (0 < w0 < π).
 * @param alpha Bandwidth-related coefficient (commonly derived from Q).
 * @param A Linear gain parameter; present for API uniformity and not used by the high-pass design.
 */
void Filter::calculateHighPass(BiquadCoefficients& coeffs, float w0, float alpha, float A) const noexcept
{
    float b0 = (1.0f + std::cos(w0)) / 2.0f;
    float b1 = -(1.0f + std::cos(w0));
    float b2 = b0;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * std::cos(w0);
    float a2 = 1.0f - alpha;
    
    coeffs.b0 = b0 / a0;
    coeffs.b1 = b1 / a0;
    coeffs.b2 = b2 / a0;
    coeffs.a1 = a1 / a0;
    coeffs.a2 = a2 / a0;
}

/**
 * @brief Populates `coeffs` with biquad coefficients for a band-pass filter (constant skirt gain).
 *
 * Fills the provided BiquadCoefficients with the normalized feedforward and feedback
 * coefficients for a band-pass biquad whose peak gain corresponds to the filter Q.
 *
 * @param[out] coeffs Destination structure to receive `b0`, `b1`, `b2`, `a1`, and `a2`.
 * @param w0 Normalized angular center frequency in radians per sample.
 * @param alpha Filter bandwidth-related factor (commonly computed as `sin(w0)/(2*Q)`).
 */
void Filter::calculateBandPass(BiquadCoefficients& coeffs, float w0, float alpha) const noexcept
{
    // Constant skirt gain (peak gain = Q)
    float b0 = alpha;
    float b1 = 0.0f;
    float b2 = -alpha;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * std::cos(w0);
    float a2 = 1.0f - alpha;
    
    coeffs.b0 = b0 / a0;
    coeffs.b1 = b1 / a0;
    coeffs.b2 = b2 / a0;
    coeffs.a1 = a1 / a0;
    coeffs.a2 = a2 / a0;
}

/**
 * @brief Compute normalized biquad coefficients for a second-order notch (band-stop) filter.
 *
 * Populates the supplied BiquadCoefficients with coefficients normalized by a0 for a notch
 * section defined by the normalized angular frequency w0 and bandwidth factor alpha.
 *
 * @param[out] coeffs Destination to receive normalized coefficients (b0, b1, b2, a1, a2).
 * @param w0 Normalized angular center frequency in radians (0 to π).
 * @param alpha Bandwidth-related factor (commonly sin(w0)/(2*Q)).
 */
void Filter::calculateNotch(BiquadCoefficients& coeffs, float w0, float alpha) const noexcept
{
    float b0 = 1.0f;
    float b1 = -2.0f * std::cos(w0);
    float b2 = 1.0f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * std::cos(w0);
    float a2 = 1.0f - alpha;
    
    coeffs.b0 = b0 / a0;
    coeffs.b1 = b1 / a0;
    coeffs.b2 = b2 / a0;
    coeffs.a1 = a1 / a0;
    coeffs.a2 = a2 / a0;
}

/**
 * @brief Computes normalized biquad coefficients for a low-shelf filter and stores them in `coeffs`.
 *
 * @param coeffs Destination structure that will receive the normalized filter coefficients (b0, b1, b2, a1, a2).
 * @param w0 Normalized angular center frequency in radians (0..pi).
 * @param alpha Filter bandwidth/shape parameter (commonly derived from Q or bandwidth).
 * @param A Linear gain factor for the shelf (A = 10^(gain_dB/40)), must be greater than 0.
 */
void Filter::calculateLowShelf(BiquadCoefficients& coeffs, float w0, float alpha, float A) const noexcept
{
    float sqrtA = std::sqrt(A);
    float aPlus1 = A + 1.0f;
    float aMinus1 = A - 1.0f;
    float cosW0 = std::cos(w0);
    
    float b0 = A * (aPlus1 - aMinus1 * cosW0 + 2.0f * sqrtA * alpha);
    float b1 = 2.0f * A * (aMinus1 - aPlus1 * cosW0);
    float b2 = A * (aPlus1 - aMinus1 * cosW0 - 2.0f * sqrtA * alpha);
    float a0 = aPlus1 + aMinus1 * cosW0 + 2.0f * sqrtA * alpha;
    float a1 = -2.0f * (aMinus1 + aPlus1 * cosW0);
    float a2 = aPlus1 + aMinus1 * cosW0 - 2.0f * sqrtA * alpha;
    
    coeffs.b0 = b0 / a0;
    coeffs.b1 = b1 / a0;
    coeffs.b2 = b2 / a0;
    coeffs.a1 = a1 / a0;
    coeffs.a2 = a2 / a0;
}

/**
 * @brief Populates biquad coefficients for a high-shelf filter response.
 *
 * Computes normalized feedforward and feedback coefficients (b0, b1, b2, a1, a2)
 * for a second-order high-shelf filter and writes them into `coeffs`.
 *
 * @param coeffs Destination structure to receive the normalized biquad coefficients.
 * @param w0 Normalized angular frequency (radians/sample).
 * @param alpha Bandwidth / shape parameter (typically derived from Q or bandwidth).
 * @param A Linear shelf gain factor.
 *
 * @note Coefficients are normalized by `a0` so that the implemented filter uses
 *       the form with a1 and a2 as the negative feedback coefficients stored here.
 */
void Filter::calculateHighShelf(BiquadCoefficients& coeffs, float w0, float alpha, float A) const noexcept
{
    float sqrtA = std::sqrt(A);
    float aPlus1 = A + 1.0f;
    float aMinus1 = A - 1.0f;
    float cosW0 = std::cos(w0);
    
    float b0 = A * (aPlus1 + aMinus1 * cosW0 + 2.0f * sqrtA * alpha);
    float b1 = -2.0f * A * (aMinus1 + aPlus1 * cosW0);
    float b2 = A * (aPlus1 + aMinus1 * cosW0 - 2.0f * sqrtA * alpha);
    float a0 = aPlus1 - aMinus1 * cosW0 + 2.0f * sqrtA * alpha;
    float a1 = 2.0f * (aMinus1 - aPlus1 * cosW0);
    float a2 = aPlus1 - aMinus1 * cosW0 - 2.0f * sqrtA * alpha;
    
    coeffs.b0 = b0 / a0;
    coeffs.b1 = b1 / a0;
    coeffs.b2 = b2 / a0;
    coeffs.a1 = a1 / a0;
    coeffs.a2 = a2 / a0;
}

/**
 * @brief Computes normalized biquad coefficients for a peaking (peak/notch) filter.
 *
 * Populates the provided BiquadCoefficients with coefficients normalized by `a0`
 * for a peaking-equalizer response using the given center frequency, bandwidth
 * and linear gain.
 *
 * @param coeffs Output structure to receive `b0`, `b1`, `b2`, `a1`, and `a2`.
 * @param w0 Normalized angular center frequency in radians (0..pi).
 * @param alpha Bandwidth-related term (typically sin(w0)/(2*Q) or similar).
 * @param A Linear gain factor (sqrt of peak gain in linear scale).
 */
void Filter::calculatePeak(BiquadCoefficients& coeffs, float w0, float alpha, float A) const noexcept
{
    float b0 = 1.0f + alpha * A;
    float b1 = -2.0f * std::cos(w0);
    float b2 = 1.0f - alpha * A;
    float a0 = 1.0f + alpha / A;
    float a1 = -2.0f * std::cos(w0);
    float a2 = 1.0f - alpha / A;
    
    coeffs.b0 = b0 / a0;
    coeffs.b1 = b1 / a0;
    coeffs.b2 = b2 / a0;
    coeffs.a1 = a1 / a0;
    coeffs.a2 = a2 / a0;
}

/**
 * @brief Computes normalized biquad coefficients for a second-order all-pass filter.
 *
 * Populates `coeffs` with the filter coefficients (b0, b1, b2, a1, a2) for an all-pass
 * response at the given normalized radian frequency and bandwidth factor; coefficients
 * are normalized by `a0`.
 *
 * @param coeffs Output structure that receives the normalized biquad coefficients.
 * @param w0 Normalized radian center frequency (in radians/sample).
 * @param alpha Bandwidth-related coefficient (commonly computed from Q or bandwidth).
 */
void Filter::calculateAllPass(BiquadCoefficients& coeffs, float w0, float alpha) const noexcept
{
    float b0 = 1.0f - alpha;
    float b1 = -2.0f * std::cos(w0);
    float b2 = 1.0f + alpha;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * std::cos(w0);
    float a2 = 1.0f - alpha;
    
    coeffs.b0 = b0 / a0;
    coeffs.b1 = b1 / a0;
    coeffs.b2 = b2 / a0;
    coeffs.a1 = a1 / a0;
    coeffs.a2 = a2 / a0;
}

// ============================================================================
// Parameter smoothing
/**
 * @brief Advances smoothed filter parameters and updates coefficients when needed.
 *
 * Advances the internal smoothed values for cutoff frequency, resonance (Q), and gain
 * toward their configured target values using the configured smoothing factor.
 * If the change for cutoff exceeds 0.1 Hz, resonance exceeds 0.001, or gain exceeds 0.01 dB,
 * the function marks the coefficients for recalculation and updates them.
 * Coefficients are also updated when the internal update flag is set.
 */

void Filter::updateInternal()
{
    bool needsCoefficientUpdate = false;
    
    // Smooth cutoff
    float newCutoff = interpolateParameter(m_currentCutoff, m_targetCutoff, m_smoothingAlpha);
    if (std::abs(newCutoff - m_currentCutoff) > 0.1f)
    {
        m_currentCutoff = newCutoff;
        needsCoefficientUpdate = true;
    }
    
    // Smooth resonance
    float newResonance = interpolateParameter(m_currentResonance, m_targetResonance, m_smoothingAlpha);
    if (std::abs(newResonance - m_currentResonance) > 0.001f)
    {
        m_currentResonance = newResonance;
        needsCoefficientUpdate = true;
    }
    
    // Smooth gain
    float newGain = interpolateParameter(m_currentGain, m_targetGain, m_smoothingAlpha);
    if (std::abs(newGain - m_currentGain) > 0.01f)
    {
        m_currentGain = newGain;
        needsCoefficientUpdate = true;
    }
    
    // Update coefficients if needed
    if (needsCoefficientUpdate || m_needsUpdate)
    {
        updateCoefficientsInternal();
    }
}

/**
 * @brief Smooths the active biquad coefficients toward their target values.
 *
 * Applies the configured smoothing/interpolation (via interpolateParameter and m_smoothingAlpha)
 * to each biquad coefficient (b0, b1, b2, a1, a2) for both channels so the running coefficients
 * gradually approach m_targetCoeffs.
 */
void Filter::updateSmoothing()
{
    // Smooth coefficients towards target
    for (int i = 0; i < 2; ++i)
    {
        m_coeffs[i].b0 = interpolateParameter(m_coeffs[i].b0, m_targetCoeffs[i].b0, m_smoothingAlpha);
        m_coeffs[i].b1 = interpolateParameter(m_coeffs[i].b1, m_targetCoeffs[i].b1, m_smoothingAlpha);
        m_coeffs[i].b2 = interpolateParameter(m_coeffs[i].b2, m_targetCoeffs[i].b2, m_smoothingAlpha);
        m_coeffs[i].a1 = interpolateParameter(m_coeffs[i].a1, m_targetCoeffs[i].a1, m_smoothingAlpha);
        m_coeffs[i].a2 = interpolateParameter(m_coeffs[i].a2, m_targetCoeffs[i].a2, m_smoothingAlpha);
    }
}

/**
 * @brief Interpolates a parameter value toward a target using the configured smoothing mode.
 *
 * Interpolation mode is selected by the filter's current smoothing type:
 * - Linear: straight linear interpolation.
 * - Exponential: exponential-style smoothing that biases toward the start for larger alpha.
 * - Cosine: smooth cosine easing between start and target.
 *
 * @param start Current/source value.
 * @param target Desired/target value.
 * @param alpha Smoothing coefficient in [0, 1]; `1.0` yields `start`, `0.0` yields `target`.
 * @return float Interpolated value between `start` and `target` according to the smoothing mode.
 */
float Filter::interpolateParameter(float start, float target, float alpha) const noexcept
{
    switch (m_smoothingType)
    {
    case SmoothingType::Linear:
        return start + (target - start) * (1.0f - alpha);
        
    case SmoothingType::Exponential:
        return target + (start - target) * alpha;
        
    case SmoothingType::Cosine:
    {
        float t = 1.0f - alpha;
        float cosine = (1.0f - std::cos(t * PI)) * 0.5f;
        return start + (target - start) * cosine;
    }
        
    default:
        return target;  // No smoothing
    }
}

// ============================================================================
// Saturation
/**
 * @brief Apply the filter's configured saturation curve to a single sample.
 *
 * The applied curve depends on the instance's selected SaturationType and
 * shapes the input sample to produce soft/hard clipping, a tanh curve, or a
 * gentle asymmetric saturation.
 *
 * @param sample Input audio sample to process.
 * @return float Processed sample after saturation.

float Filter::applySaturation(float sample) const noexcept
{
    switch (m_saturationType)
    {
    case SaturationType::SoftClip:
    {
        // Cubic soft clipper
        if (sample > 1.0f)
            return 2.0f / 3.0f;
        else if (sample < -1.0f)
            return -2.0f / 3.0f;
        else
            return sample - sample * sample * sample / 3.0f;
    }
        
    case SaturationType::HardClip:
        return std::clamp(sample, -1.0f, 1.0f);
        
    case SaturationType::Tanh:
        return std::tanh(sample);
        
    case SaturationType::Asymmetric:
        // Gentle asymmetric saturation
        if (sample > 0.0f)
            return 1.0f - std::exp(-sample);
        else
            return -1.0f + std::exp(sample);
        
    default:
        return sample;
    }
}

// ============================================================================
// State management
/**
 * @brief Resets all internal filter state and clears oversampling filter buffers.
 *
 * Resets per-channel biquad state (delay elements) to their initial zeroed state and
 * clears internal upsample/downsample filter memory used by the oversampling path.
 */

void Filter::reset()
{
    for (auto& state : m_state)
    {
        state.reset();
    }
    
    // Reset oversampling filters
    std::memset(m_upsampleFilter, 0, sizeof(m_upsampleFilter));
    std::memset(m_downsampleFilter, 0, sizeof(m_downsampleFilter));
}

/**
 * @brief Resets the internal filter state for a single channel.
 *
 * Clears delay elements (internal state) for the specified channel.
 *
 * @param channel Channel index to reset; 0 for left, 1 for right. Values outside [0,1] are ignored.
 */
void Filter::reset(int channel)
{
    if (channel >= 0 && channel < 2)
    {
        m_state[channel].reset();
    }
}

// ============================================================================
// Analysis methods
/**
 * @brief Compute the filter's magnitude response at a given frequency.
 *
 * Evaluates the current left-channel biquad transfer function at the specified
 * frequency and returns its magnitude in decibels.
 *
 * @param frequency Frequency in Hertz at which to evaluate the response.
 * @return float Magnitude in decibels (dB).
 */

float Filter::getMagnitudeResponse(float frequency) const
{
    // Convert to normalized frequency
    float w = TWO_PI * frequency * m_inverseSampleRate;
    std::complex<float> z = std::exp(std::complex<float>(0.0f, w));
    
    // Evaluate transfer function H(z) = (b0 + b1*z^-1 + b2*z^-2) / (1 + a1*z^-1 + a2*z^-2)
    std::complex<float> numerator = m_coeffs[0].b0 + m_coeffs[0].b1 / z + m_coeffs[0].b2 / (z * z);
    std::complex<float> denominator = 1.0f + m_coeffs[0].a1 / z + m_coeffs[0].a2 / (z * z);
    std::complex<float> H = numerator / denominator;
    
    return 20.0f * std::log10(std::abs(H));
}

/**
 * @brief Computes the filter's phase response at a given frequency.
 *
 * @param frequency Frequency in Hz at which to evaluate the response.
 * @return float Phase angle of the filter's complex frequency response in radians (range approximately (-\f$\pi\f$, \f$\pi\f$]).
 */
float Filter::getPhaseResponse(float frequency) const
{
    float w = TWO_PI * frequency * m_inverseSampleRate;
    std::complex<float> z = std::exp(std::complex<float>(0.0f, w));
    
    std::complex<float> numerator = m_coeffs[0].b0 + m_coeffs[0].b1 / z + m_coeffs[0].b2 / (z * z);
    std::complex<float> denominator = 1.0f + m_coeffs[0].a1 / z + m_coeffs[0].a2 / (z * z);
    std::complex<float> H = numerator / denominator;
    
    return std::arg(H);
}

/**
 * @brief Estimates the filter's group delay at a given frequency.
 *
 * Computes a numerical approximation of group delay by measuring the phase change
 * around the specified frequency using a 1 Hz finite difference and phase unwrapping.
 *
 * @param frequency Frequency in Hz at which to evaluate group delay.
 * @return float Group delay in seconds at the specified frequency.
 */
float Filter::getGroupDelay(float frequency) const
{
    // Numerical approximation of group delay
    const float delta = 1.0f;  // 1 Hz difference
    
    float phase1 = getPhaseResponse(frequency);
    float phase2 = getPhaseResponse(frequency + delta);
    
    float phaseDiff = phase2 - phase1;
    
    // Unwrap phase difference
    while (phaseDiff > PI) phaseDiff -= TWO_PI;
    while (phaseDiff < -PI) phaseDiff += TWO_PI;
    
    return -phaseDiff / (TWO_PI * delta);
}

/**
 * @brief Checks whether the filter is currently in a self-oscillating state.
 *
 * @return `true` if self-oscillation is enabled and the current resonance is greater than or equal to 0.99, `false` otherwise.
 */
bool Filter::isSelfOscillating() const noexcept
{
    return m_selfOscillation && m_currentResonance >= 0.99f;
}

// ============================================================================
// Helper methods
/**
 * @brief Constrains pole coefficients to ensure filter stability.
 *
 * If the pole radius (sqrt(a1^2 + a2^2)) exceeds 0.999, scales `a1` and `a2`
 * in place so the effective pole radius is limited to 0.999.
 */

void Filter::BiquadCoefficients::normalize() noexcept
{
    // Ensure stability by checking pole radius
    float poleRadius = std::sqrt(a1 * a1 + a2 * a2);
    if (poleRadius > 0.999f)
    {
        // Scale back poles to unit circle
        float scale = 0.999f / poleRadius;
        a1 *= scale;
        a2 *= scale * scale;
    }
}

/**
 * @brief Clears the filter state delay elements for a channel.
 *
 * Sets all internal delay samples (previous inputs x1, x2 and previous outputs y1, y2)
 * to 0.0f to return the biquad state to silence.
 */
void Filter::FilterState::reset() noexcept
{
    x1 = x2 = y1 = y2 = 0.0f;
}

/**
 * @brief Resize the oversampling buffer if the requested size differs.
 *
 * Allocates a new float array of length @p newSize and replaces the current buffer when the size changes.
 * The previous buffer contents are discarded. If @p newSize equals the current size, no action is taken.
 *
 * @param newSize Desired buffer size in elements.
 */
void Filter::OversampledBuffer::resize(uint32_t newSize)
{
    if (newSize != size)
    {
        buffer = std::make_unique<float[]>(newSize);
        size = newSize;
    }
}

} // namespace DSP
} // namespace Audio
} // namespace Nomad