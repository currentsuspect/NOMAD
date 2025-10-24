#include "AudioDeviceManager.h"
#include "RtAudioBackend.h"
#include "NomadASIOBackend.h"

namespace Nomad {
namespace Audio {

AudioDeviceManager::AudioDeviceManager()
    : m_currentCallback(nullptr)
    , m_currentUserData(nullptr)
    , m_initialized(false)
    , m_wasRunning(false)
{
}

AudioDeviceManager::~AudioDeviceManager() {
    shutdown();
}

bool AudioDeviceManager::initialize() {
    if (m_initialized) {
        return true;
    }

    try {
        // Prefer NomadASIOBackend on Windows to leverage ASIO low-latency if available.
#if defined(_WIN32) || defined(WIN32)
        try {
            m_driver = std::make_unique<NomadASIOBackend>();
        } catch (...) {
            // Fallback to generic RtAudio backend if NomadASIOBackend cannot be constructed
            m_driver = std::make_unique<RtAudioBackend>();
        }
#else
        m_driver = std::make_unique<RtAudioBackend>();
#endif
        m_initialized = true;
        return true;
    } catch (...) {
        return false;
    }
}

void AudioDeviceManager::shutdown() {
    if (m_initialized) {
        closeStream();
        m_driver.reset();
        m_initialized = false;
    }
}

std::vector<AudioDeviceInfo> AudioDeviceManager::getDevices() const {
    if (!m_initialized || !m_driver) {
        return {};
    }
    return m_driver->getDevices();
}

AudioDeviceInfo AudioDeviceManager::getDefaultOutputDevice() const {
    if (!m_initialized || !m_driver) {
        return {};
    }

    uint32_t deviceId = m_driver->getDefaultOutputDevice();
    auto devices = m_driver->getDevices();
    
    for (const auto& device : devices) {
        if (device.id == deviceId) {
            return device;
        }
    }
    
    return {};
}

AudioDeviceInfo AudioDeviceManager::getDefaultInputDevice() const {
    if (!m_initialized || !m_driver) {
        return {};
    }

    uint32_t deviceId = m_driver->getDefaultInputDevice();
    auto devices = m_driver->getDevices();
    
    for (const auto& device : devices) {
        if (device.id == deviceId) {
            return device;
        }
    }
    
    return {};
}

bool AudioDeviceManager::openStream(const AudioStreamConfig& config, AudioCallback callback, void* userData) {
    if (!m_initialized || !m_driver) {
        return false;
    }

    // Validate configuration
    if (!validateDeviceConfig(config.deviceId, config.sampleRate)) {
        return false;
    }

    m_currentConfig = config;
    m_currentCallback = callback;
    m_currentUserData = userData;
    
    return m_driver->openStream(config, callback, userData);
}

void AudioDeviceManager::closeStream() {
    if (m_initialized && m_driver) {
        m_driver->closeStream();
        m_currentCallback = nullptr;
        m_currentUserData = nullptr;
    }
}

bool AudioDeviceManager::startStream() {
    if (!m_initialized || !m_driver) {
        return false;
    }
    return m_driver->startStream();
}

void AudioDeviceManager::stopStream() {
    if (m_initialized && m_driver) {
        m_driver->stopStream();
    }
}

bool AudioDeviceManager::isStreamRunning() const {
    if (!m_initialized || !m_driver) {
        return false;
    }
    return m_driver->isStreamRunning();
}

double AudioDeviceManager::getStreamLatency() const {
    if (!m_initialized || !m_driver) {
        return 0.0;
    }
    return m_driver->getStreamLatency();
}

bool AudioDeviceManager::switchDevice(uint32_t deviceId) {
    if (!m_initialized || !m_driver) {
        return false;
    }

    // Validate the new device
    if (!validateDeviceConfig(deviceId, m_currentConfig.sampleRate)) {
        return false;
    }

    // Remember if stream was running
    m_wasRunning = isStreamRunning();

    // Stop and close current stream
    if (m_wasRunning) {
        stopStream();
    }
    closeStream();

    // Update configuration with new device
    m_currentConfig.deviceId = deviceId;

    // Reopen stream with new device
    if (!m_driver->openStream(m_currentConfig, m_currentCallback, m_currentUserData)) {
        return false;
    }

    // Restart stream if it was running
    if (m_wasRunning) {
        return startStream();
    }

    return true;
}

bool AudioDeviceManager::setSampleRate(uint32_t sampleRate) {
    if (!m_initialized || !m_driver) {
        return false;
    }

    // Validate the new sample rate
    if (!validateDeviceConfig(m_currentConfig.deviceId, sampleRate)) {
        return false;
    }

    // Remember if stream was running
    m_wasRunning = isStreamRunning();

    // Stop and close current stream
    if (m_wasRunning) {
        stopStream();
    }
    closeStream();

    // Update configuration with new sample rate
    m_currentConfig.sampleRate = sampleRate;

    // Reopen stream with new sample rate
    if (!m_driver->openStream(m_currentConfig, m_currentCallback, m_currentUserData)) {
        return false;
    }

    // Restart stream if it was running
    if (m_wasRunning) {
        return startStream();
    }

    return true;
}

bool AudioDeviceManager::setBufferSize(uint32_t bufferSize) {
    if (!m_initialized || !m_driver) {
        return false;
    }

    // Validate buffer size (reasonable range: 64 to 8192 frames)
    if (bufferSize < 64 || bufferSize > 8192) {
        return false;
    }

    // Remember if stream was running
    m_wasRunning = isStreamRunning();

    // Stop and close current stream
    if (m_wasRunning) {
        stopStream();
    }
    closeStream();

    // Update configuration with new buffer size
    m_currentConfig.bufferSize = bufferSize;

    // Reopen stream with new buffer size
    if (!m_driver->openStream(m_currentConfig, m_currentCallback, m_currentUserData)) {
        return false;
    }

    // Restart stream if it was running
    if (m_wasRunning) {
        return startStream();
    }

    return true;
}

bool AudioDeviceManager::validateDeviceConfig(uint32_t deviceId, uint32_t sampleRate) const {
    if (!m_initialized || !m_driver) {
        return false;
    }

    auto devices = m_driver->getDevices();
    
    // Find the device
    for (const auto& device : devices) {
        if (device.id == deviceId) {
            // Check if device has output channels
            if (device.maxOutputChannels == 0) {
                return false;
            }

            // Check if sample rate is supported
            bool sampleRateSupported = false;
            for (uint32_t supportedRate : device.supportedSampleRates) {
                if (supportedRate == sampleRate) {
                    sampleRateSupported = true;
                    break;
                }
            }

            return sampleRateSupported;
        }
    }

    return false; // Device not found
}

const char* getVersion() {
    return "1.0.0";
}

const char* getBackendName() {
#if defined(__WINDOWS_NOMADASIO__)
    return "NomadASIO";
#endif
#if defined(__WINDOWS_WASAPI__)
    return "WASAPI";
#elif defined(__WINDOWS_DS__)
    return "DirectSound";
#elif defined(__MACOSX_CORE__)
    return "CoreAudio";
#elif defined(__LINUX_ALSA__)
    return "ALSA";
#elif defined(__UNIX_JACK__)
    return "JACK";
#else
    return "Unknown";
#endif
}

} // namespace Audio
} // namespace Nomad
