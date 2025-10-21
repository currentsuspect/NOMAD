#include "RtAudioBackend.h"
#include <stdexcept>
#include <string>

namespace Nomad {
namespace Audio {

RtAudioBackend::RtAudioBackend()
    : m_userCallback(nullptr)
    , m_userData(nullptr)
{
    m_rtAudio = std::make_unique<RtAudio>();
    
    // Set error callback
    m_rtAudio->setErrorCallback([](RtAudioErrorType type, const std::string& errorText) {
        // Log error but don't throw - RtAudio v6 uses callbacks
        if (type != RTAUDIO_NO_ERROR && type != RTAUDIO_WARNING) {
            std::cerr << "RtAudio Error: " << errorText << std::endl;
        }
    });
}

RtAudioBackend::~RtAudioBackend() {
    closeStream();
}

std::vector<AudioDeviceInfo> RtAudioBackend::getDevices() {
    std::vector<AudioDeviceInfo> devices;
    
    std::vector<unsigned int> deviceIds = m_rtAudio->getDeviceIds();
    for (unsigned int id : deviceIds) {
        RtAudio::DeviceInfo rtInfo = m_rtAudio->getDeviceInfo(id);
        
        // Skip invalid devices
        if (rtInfo.outputChannels == 0 && rtInfo.inputChannels == 0) {
            continue;
        }
        
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

uint32_t RtAudioBackend::getDefaultOutputDevice() {
    std::vector<unsigned int> deviceIds = m_rtAudio->getDeviceIds();
    for (unsigned int id : deviceIds) {
        RtAudio::DeviceInfo info = m_rtAudio->getDeviceInfo(id);
        if (info.isDefaultOutput) {
            return id;
        }
    }
    return deviceIds.empty() ? 0 : deviceIds[0];
}

uint32_t RtAudioBackend::getDefaultInputDevice() {
    std::vector<unsigned int> deviceIds = m_rtAudio->getDeviceIds();
    for (unsigned int id : deviceIds) {
        RtAudio::DeviceInfo info = m_rtAudio->getDeviceInfo(id);
        if (info.isDefaultInput) {
            return id;
        }
    }
    return deviceIds.empty() ? 0 : deviceIds[0];
}

bool RtAudioBackend::openStream(const AudioStreamConfig& config, AudioCallback callback, void* userData) {
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
        &RtAudioBackend::rtAudioCallback,
        this
    );
    
    return (error == RTAUDIO_NO_ERROR);
}

void RtAudioBackend::closeStream() {
    if (m_rtAudio->isStreamOpen()) {
        if (m_rtAudio->isStreamRunning()) {
            stopStream();
        }
        m_rtAudio->closeStream();
    }
}

bool RtAudioBackend::startStream() {
    if (!m_rtAudio->isStreamOpen()) {
        return false;
    }

    RtAudioErrorType error = m_rtAudio->startStream();
    return (error == RTAUDIO_NO_ERROR);
}

void RtAudioBackend::stopStream() {
    if (m_rtAudio->isStreamRunning()) {
        m_rtAudio->stopStream();
    }
}

bool RtAudioBackend::isStreamRunning() const {
    return m_rtAudio->isStreamRunning();
}

double RtAudioBackend::getStreamLatency() const {
    if (!m_rtAudio->isStreamOpen()) {
        return 0.0;
    }
    return m_rtAudio->getStreamLatency();
}

int RtAudioBackend::rtAudioCallback(
    void* outputBuffer,
    void* inputBuffer,
    unsigned int numFrames,
    double streamTime,
    RtAudioStreamStatus status,
    void* userData
) {
    (void)status; // Unused parameter
    
    RtAudioBackend* backend = static_cast<RtAudioBackend*>(userData);
    
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
