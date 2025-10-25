#include "NomadASIOBackend.h"
#include <iostream>
#include <windows.h>

namespace Nomad {
namespace Audio {

static const char* rtApiToString(RtAudio::Api api)
{
    switch (api) {
    case RtAudio::UNSPECIFIED: return "UNSPECIFIED";
    case RtAudio::LINUX_ALSA: return "LINUX_ALSA";
    case RtAudio::UNIX_JACK: return "UNIX_JACK";
    case RtAudio::MACOSX_CORE: return "MACOSX_CORE";
    case RtAudio::WINDOWS_ASIO: return "WINDOWS_ASIO";
    case RtAudio::WINDOWS_DS: return "WINDOWS_DS";
    case RtAudio::WINDOWS_WASAPI: return "WINDOWS_WASAPI";
    case RtAudio::RTAUDIO_DUMMY: return "RTAUDIO_DUMMY";
    default: return "UNKNOWN";
    }
}


NomadASIOBackend::NomadASIOBackend()
    : m_userCallback(nullptr)
    , m_userData(nullptr)
    , m_requestedAsio(false)
{
    // Try to request ASIO on Windows builds via RtAudio if RtAudio was compiled with ASIO support.
    // If RtAudio wasn't built with ASIO (no ASIO SDK at compile time), fall back to default.
    try {
        std::vector<RtAudio::Api> compiledApis;
        RtAudio::getCompiledApi(compiledApis);

        std::cout << "NomadASIOBackend: RtAudio compiled APIs (count=" << compiledApis.size() << "):";
        for (size_t i = 0; i < compiledApis.size(); ++i) {
            std::cout << " [" << i << "]=" << rtApiToString(compiledApis[i]);
        }
        std::cout << std::endl;
        std::cout.flush();

        bool asioCompiled = false;
        for (const auto& a : compiledApis) {
            if (a == RtAudio::WINDOWS_ASIO) {
                asioCompiled = true;
                break;
            }
        }

        if (asioCompiled) {
            // Check if ASIO is available before constructing
            if (isAsioAvailable()) {
                // Construct RtAudio requesting the ASIO API explicitly.
                m_rtAudio = std::make_unique<RtAudio>(RtAudio::WINDOWS_ASIO);
                m_requestedAsio = true;
                std::cout << "NomadASIOBackend: RtAudio compiled with ASIO support - requesting ASIO." << std::endl;
                std::cout.flush();
            } else {
                // No ASIO available; use default RtAudio
                m_rtAudio = std::make_unique<RtAudio>();
                m_requestedAsio = false;
                std::cout << "NomadASIOBackend: ASIO not available; falling back to default API." << std::endl;
                std::cout.flush();
            }
        } else {
            // No ASIO support compiled into RtAudio; use default RtAudio and fall back to WASAPI/other.
            m_rtAudio = std::make_unique<RtAudio>();
            m_requestedAsio = false;
            std::cout << "NomadASIOBackend: RtAudio NOT compiled with ASIO support; falling back to default API." << std::endl;
            std::cout.flush();
        }

        // Print the API RtAudio reports as current after construction
        try {
            RtAudio::Api current = m_rtAudio->getCurrentApi();
            std::cout << "NomadASIOBackend: RtAudio current API after construction: " << rtApiToString(current) << std::endl;
            std::cout.flush();
        } catch (...) {
            std::cout << "NomadASIOBackend: Unable to query RtAudio current API." << std::endl;
            std::cout.flush();
        }

    } catch (...) {
        // If anything goes wrong, fall back to default RtAudio
        m_rtAudio = std::make_unique<RtAudio>();
        m_requestedAsio = false;
        std::cerr << "NomadASIOBackend: Exception while checking RtAudio compiled APIs; using default API." << std::endl;
    }

    // Set error callback
    m_rtAudio->setErrorCallback([](RtAudioErrorType type, const std::string& errorText) {
        if (type != RTAUDIO_NO_ERROR && type != RTAUDIO_WARNING) {
            std::cerr << "RtAudio Error: " << errorText << std::endl;
        }
    });
}

bool NomadASIOBackend::isAsioAvailable() {
    try {
        std::unique_ptr<RtAudio> testRtAudio = std::make_unique<RtAudio>(RtAudio::WINDOWS_ASIO);
        std::vector<unsigned int> deviceIds = testRtAudio->getDeviceIds();
        std::cout << "NomadASIOBackend: ASIO devices available: " << deviceIds.size() << std::endl;
        std::cout.flush();
        return true;
    } catch (...) {
        std::cout << "NomadASIOBackend: ASIO not available." << std::endl;
        std::cout.flush();
        return false;
    }
}

NomadASIOBackend::~NomadASIOBackend() {
    closeStream();
}

std::vector<AudioDeviceInfo> NomadASIOBackend::getDevices() {
    try {
        std::vector<AudioDeviceInfo> devices;

        std::vector<unsigned int> deviceIds = m_rtAudio->getDeviceIds();
        for (unsigned int id : deviceIds) {
            RtAudio::DeviceInfo rtInfo = m_rtAudio->getDeviceInfo(id);
            if (rtInfo.outputChannels == 0 && rtInfo.inputChannels == 0) continue;

            AudioDeviceInfo info;
            info.id = id;
            info.name = rtInfo.name;
            info.maxInputChannels = rtInfo.inputChannels;
            info.maxOutputChannels = rtInfo.outputChannels;
            info.supportedSampleRates = rtInfo.sampleRates;
            info.preferredSampleRate = rtInfo.preferredSampleRate;
            info.isDefaultInput = rtInfo.isDefaultInput;
            info.isDefaultOutput = rtInfo.isDefaultOutput;
            devices.push_back(info);
        }

        return devices;
    } catch (const std::exception& e) {
        std::cerr << "NomadASIOBackend::getDevices: Exception: " << e.what() << std::endl;
        std::cerr << "Falling back to default RtAudio" << std::endl;
        // Fall back to default RtAudio
        m_rtAudio = std::make_unique<RtAudio>();
        m_requestedAsio = false;
        // Retry with default API
        return getDevicesFallback();
    } catch (...) {
        std::cerr << "NomadASIOBackend::getDevices: Unknown exception, falling back to default RtAudio" << std::endl;
        m_rtAudio = std::make_unique<RtAudio>();
        m_requestedAsio = false;
        return getDevicesFallback();
    }
}

std::vector<AudioDeviceInfo> NomadASIOBackend::getDevicesFallback() {
    std::vector<AudioDeviceInfo> devices;

    std::vector<unsigned int> deviceIds = m_rtAudio->getDeviceIds();
    for (unsigned int id : deviceIds) {
        RtAudio::DeviceInfo rtInfo = m_rtAudio->getDeviceInfo(id);
        if (rtInfo.outputChannels == 0 && rtInfo.inputChannels == 0) continue;

        AudioDeviceInfo info;
        info.id = id;
        info.name = rtInfo.name;
        info.maxInputChannels = rtInfo.inputChannels;
        info.maxOutputChannels = rtInfo.outputChannels;
        info.supportedSampleRates = rtInfo.sampleRates;
        info.preferredSampleRate = rtInfo.preferredSampleRate;
        info.isDefaultInput = rtInfo.isDefaultInput;
        info.isDefaultOutput = rtInfo.isDefaultOutput;
        devices.push_back(info);
    }

    return devices;
}

uint32_t NomadASIOBackend::getDefaultOutputDevice() {
    std::cout << "NomadASIOBackend::getDefaultOutputDevice: Getting device IDs" << std::endl;
    std::cout.flush();
    std::vector<unsigned int> deviceIds = m_rtAudio->getDeviceIds();
    std::cout << "NomadASIOBackend::getDefaultOutputDevice: Found " << deviceIds.size() << " devices" << std::endl;
    std::cout.flush();
    for (unsigned int id : deviceIds) {
        std::cout << "NomadASIOBackend::getDefaultOutputDevice: Querying device " << id << std::endl;
        std::cout.flush();
        RtAudio::DeviceInfo info = m_rtAudio->getDeviceInfo(id);
        std::cout << "NomadASIOBackend::getDefaultOutputDevice: Device " << id << " name: " << info.name << std::endl;
        std::cout.flush();
        if (info.isDefaultOutput) {
            std::cout << "NomadASIOBackend::getDefaultOutputDevice: Found default output device " << id << std::endl;
            std::cout.flush();
            return id;
        }
    }
    std::cout << "NomadASIOBackend::getDefaultOutputDevice: No default found, returning first" << std::endl;
    std::cout.flush();
    return deviceIds.empty() ? 0 : deviceIds[0];
}

uint32_t NomadASIOBackend::getDefaultOutputDeviceFallback() {
    std::vector<unsigned int> deviceIds = m_rtAudio->getDeviceIds();
    for (unsigned int id : deviceIds) {
        RtAudio::DeviceInfo info = m_rtAudio->getDeviceInfo(id);
        if (info.isDefaultOutput) return id;
    }
    return deviceIds.empty() ? 0 : deviceIds[0];
}

uint32_t NomadASIOBackend::getDefaultInputDevice() {
    try {
        std::vector<unsigned int> deviceIds = m_rtAudio->getDeviceIds();
        for (unsigned int id : deviceIds) {
            RtAudio::DeviceInfo info = m_rtAudio->getDeviceInfo(id);
            if (info.isDefaultInput) return id;
        }
        return deviceIds.empty() ? 0 : deviceIds[0];
    } catch (const std::exception& e) {
        std::cerr << "NomadASIOBackend::getDefaultInputDevice: Exception: " << e.what() << std::endl;
        std::cerr << "Falling back to default RtAudio" << std::endl;
        m_rtAudio = std::make_unique<RtAudio>();
        m_requestedAsio = false;
        return getDefaultInputDeviceFallback();
    } catch (...) {
        std::cerr << "NomadASIOBackend::getDefaultInputDevice: Unknown exception, falling back to default RtAudio" << std::endl;
        m_rtAudio = std::make_unique<RtAudio>();
        m_requestedAsio = false;
        return getDefaultInputDeviceFallback();
    }
}

uint32_t NomadASIOBackend::getDefaultInputDeviceFallback() {
    std::vector<unsigned int> deviceIds = m_rtAudio->getDeviceIds();
    for (unsigned int id : deviceIds) {
        RtAudio::DeviceInfo info = m_rtAudio->getDeviceInfo(id);
        if (info.isDefaultInput) return id;
    }
    return deviceIds.empty() ? 0 : deviceIds[0];
}

bool NomadASIOBackend::openStream(const AudioStreamConfig& config, AudioCallback callback, void* userData) {
    std::cout << "NomadASIOBackend::openStream: Starting stream open" << std::endl;
    std::cout << "  Device ID: " << config.deviceId << std::endl;
    std::cout << "  Sample Rate: " << config.sampleRate << std::endl;
    std::cout << "  Buffer Size: " << config.bufferSize << std::endl;
    std::cout << "  Output Channels: " << config.numOutputChannels << std::endl;
    std::cout << "  Input Channels: " << config.numInputChannels << std::endl;
    std::cout.flush();

    if (m_rtAudio->isStreamOpen()) {
        std::cout << "NomadASIOBackend::openStream: Closing existing stream" << std::endl;
        closeStream();
    }

    m_userCallback = callback;
    m_userData = userData;

    RtAudio::StreamParameters outputParams;
    outputParams.deviceId = config.deviceId;
    outputParams.nChannels = config.numOutputChannels;
    outputParams.firstChannel = 0;

    RtAudio::StreamParameters* inputParams = nullptr;
    RtAudio::StreamParameters inputParamsData;
    if (config.numInputChannels > 0) {
        inputParamsData.deviceId = config.deviceId;
        inputParamsData.nChannels = config.numInputChannels;
        inputParamsData.firstChannel = 0;
        inputParams = &inputParamsData;
    }

    unsigned int bufferFrames = config.bufferSize;
    unsigned int sampleRate = config.sampleRate;

    std::cout << "NomadASIOBackend::openStream: Calling RtAudio::openStream" << std::endl;
    std::cout.flush();

    RtAudioErrorType error = m_rtAudio->openStream(
        &outputParams,
        inputParams,
        RTAUDIO_FLOAT32,
        sampleRate,
        &bufferFrames,
        &NomadASIOBackend::rtAudioCallback,
        this
    );

    std::cout << "NomadASIOBackend::openStream: RtAudio::openStream returned error: " << error << std::endl;
    std::cout << "  Actual buffer frames: " << bufferFrames << std::endl;
    std::cout.flush();

    if (error != RTAUDIO_NO_ERROR) {
        std::cerr << "NomadASIOBackend::openStream: Failed to open stream" << std::endl;
    }

    return (error == RTAUDIO_NO_ERROR);
}

void NomadASIOBackend::closeStream() {
    if (m_rtAudio->isStreamOpen()) {
        if (m_rtAudio->isStreamRunning()) stopStream();
        m_rtAudio->closeStream();
    }
}

bool NomadASIOBackend::startStream() {
    if (!m_rtAudio->isStreamOpen()) return false;
    RtAudioErrorType error = m_rtAudio->startStream();
    return (error == RTAUDIO_NO_ERROR);
}

void NomadASIOBackend::stopStream() {
    if (m_rtAudio->isStreamRunning()) m_rtAudio->stopStream();
}

bool NomadASIOBackend::isStreamRunning() const {
    return m_rtAudio->isStreamRunning();
}

double NomadASIOBackend::getStreamLatency() const {
    if (!m_rtAudio->isStreamOpen()) return 0.0;
    return m_rtAudio->getStreamLatency();
}

int NomadASIOBackend::rtAudioCallback(
    void* outputBuffer,
    void* inputBuffer,
    unsigned int numFrames,
    double streamTime,
    RtAudioStreamStatus status,
    void* userData
) {
    std::cout << "NomadASIOBackend::rtAudioCallback: Called with numFrames=" << numFrames
              << ", streamTime=" << streamTime << ", status=" << status << std::endl;
    std::cout.flush();

    (void)status;
    NomadASIOBackend* backend = static_cast<NomadASIOBackend*>(userData);
    if (!backend) {
        std::cerr << "NomadASIOBackend::rtAudioCallback: Backend is null!" << std::endl;
        return 1;
    }

    if (backend->m_userCallback) {
        std::cout << "NomadASIOBackend::rtAudioCallback: Calling user callback" << std::endl;
        std::cout.flush();
        int result = backend->m_userCallback(
            static_cast<float*>(outputBuffer),
            static_cast<const float*>(inputBuffer),
            numFrames,
            streamTime,
            backend->m_userData
        );
        std::cout << "NomadASIOBackend::rtAudioCallback: User callback returned " << result << std::endl;
        std::cout.flush();
        return result;
    } else {
        std::cerr << "NomadASIOBackend::rtAudioCallback: No user callback set!" << std::endl;
        return 0;
    }
}

} // namespace Audio
} // namespace Nomad
