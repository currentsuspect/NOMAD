#include "NomadASIOBackend.h"
#include <iostream>

namespace Nomad {
namespace Audio {

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

        bool asioCompiled = false;
        for (const auto& a : compiledApis) {
            if (a == RtAudio::WINDOWS_ASIO) {
                asioCompiled = true;
                break;
            }
        }

        if (asioCompiled) {
            // Construct RtAudio requesting the ASIO API explicitly.
            m_rtAudio = std::make_unique<RtAudio>(RtAudio::WINDOWS_ASIO);
            m_requestedAsio = true;
            std::cerr << "NomadASIOBackend: RtAudio compiled with ASIO support - requesting ASIO." << std::endl;
        } else {
            // No ASIO support compiled into RtAudio; use default RtAudio and fall back to WASAPI/other.
            m_rtAudio = std::make_unique<RtAudio>();
            m_requestedAsio = false;
            std::cerr << "NomadASIOBackend: RtAudio NOT compiled with ASIO support; falling back to default API." << std::endl;
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

NomadASIOBackend::~NomadASIOBackend() {
    closeStream();
}

std::vector<AudioDeviceInfo> NomadASIOBackend::getDevices() {
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
    std::vector<unsigned int> deviceIds = m_rtAudio->getDeviceIds();
    for (unsigned int id : deviceIds) {
        RtAudio::DeviceInfo info = m_rtAudio->getDeviceInfo(id);
        if (info.isDefaultOutput) return id;
    }
    return deviceIds.empty() ? 0 : deviceIds[0];
}

uint32_t NomadASIOBackend::getDefaultInputDevice() {
    std::vector<unsigned int> deviceIds = m_rtAudio->getDeviceIds();
    for (unsigned int id : deviceIds) {
        RtAudio::DeviceInfo info = m_rtAudio->getDeviceInfo(id);
        if (info.isDefaultInput) return id;
    }
    return deviceIds.empty() ? 0 : deviceIds[0];
}

bool NomadASIOBackend::openStream(const AudioStreamConfig& config, AudioCallback callback, void* userData) {
    if (m_rtAudio->isStreamOpen()) {
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

    RtAudioErrorType error = m_rtAudio->openStream(
        &outputParams,
        inputParams,
        RTAUDIO_FLOAT32,
        sampleRate,
        &bufferFrames,
        &NomadASIOBackend::rtAudioCallback,
        this
    );

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
    (void)status;
    NomadASIOBackend* backend = static_cast<NomadASIOBackend*>(userData);
    if (backend->m_userCallback) {
        return backend->m_userCallback(
            static_cast<float*>(outputBuffer),
            static_cast<const float*>(inputBuffer),
            numFrames,
            streamTime,
            backend->m_userData
        );
    }
    return 0;
}

} // namespace Audio
} // namespace Nomad
