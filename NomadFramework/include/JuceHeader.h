/**
 * @file JuceHeader.h
 * @brief Minimal mock JUCE header for testing purposes
 */

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <queue>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cmath>
#include <random>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>

// Mock JUCE classes
namespace juce
{
    class String
    {
    public:
        String() = default;
        String(const char* str) : data(str) {}
        String(const std::string& str) : data(str) {}
        
        const char* toRawUTF8() const { return data.c_str(); }
        std::string toStdString() const { return data; }
        bool isEmpty() const { return data.empty(); }
        int length() const { return static_cast<int>(data.length()); }
        String substring(int start, int end = -1) const;
        
        String& operator=(const char* str) { data = str; return *this; }
        String& operator=(const std::string& str) { data = str; return *this; }
        
        bool operator==(const String& other) const { return data == other.data; }
        bool operator!=(const String& other) const { return data != other.data; }
        
    private:
        std::string data;
    };
    
    class AudioBuffer
    {
    public:
        AudioBuffer(int channels, int samples) : numChannels(channels), numSamples(samples)
        {
            data.resize(channels * samples, 0.0f);
            setupPointers();
        }
        
        int getNumChannels() const { return numChannels; }
        int getNumSamples() const { return numSamples; }
        
        float* getWritePointer(int channel) { return &data[channel * numSamples]; }
        const float* getWritePointer(int channel) const { return &data[channel * numSamples]; }
        float* const* getArrayOfWritePointers() { return writePointers.data(); }
        const float* const* getArrayOfReadPointers() const { return readPointers.data(); }
        
        void clear() { std::fill(data.begin(), data.end(), 0.0f); }
        
    private:
        int numChannels;
        int numSamples;
        std::vector<float> data;
        std::vector<float*> writePointers;
        std::vector<const float*> readPointers;
        
        void setupPointers()
        {
            writePointers.resize(numChannels);
            readPointers.resize(numChannels);
            for (int i = 0; i < numChannels; ++i)
            {
                writePointers[i] = &data[i * numSamples];
                readPointers[i] = &data[i * numSamples];
            }
        }
    };
    
    class AudioIODeviceCallback
    {
    public:
        virtual ~AudioIODeviceCallback() = default;
        virtual void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                    int numInputChannels,
                                                    float* const* outputChannelData,
                                                    int numOutputChannels,
                                                    int numSamples,
                                                    const AudioIODeviceCallbackContext& context) = 0;
        virtual void audioDeviceAboutToStart(AudioIODevice* device) {}
        virtual void audioDeviceStopped() {}
    };
    
    class AudioIODevice
    {
    public:
        virtual ~AudioIODevice() = default;
        virtual double getCurrentSampleRate() const = 0;
        virtual int getCurrentBufferSizeSamples() const = 0;
        virtual void setCurrentSampleRate(double rate) = 0;
        virtual void setCurrentBufferSizeSamples(int samples) = 0;
    };
    
    class AudioIODeviceCallbackContext
    {
    public:
        double hostTimeNs = 0.0;
    };
    
    class AudioProcessor
    {
    public:
        virtual ~AudioProcessor() = default;
        virtual void prepareToPlay(double sampleRate, int samplesPerBlock) = 0;
        virtual void releaseResources() = 0;
        virtual void processBlock(AudioBuffer& buffer, MidiBuffer& midiMessages) = 0;
        virtual int getNumParameters() const { return 0; }
        virtual float getParameter(int index) const { return 0.0f; }
        virtual void setParameter(int index, float value) {}
        virtual const String getParameterName(int index) const { return String(); }
        virtual const String getParameterText(int index, float value) const { return String(); }
        virtual void getStateInformation(MemoryBlock& destData) {}
        virtual void setStateInformation(const void* data, int sizeInBytes) {}
    };
    
    class MidiBuffer
    {
    public:
        MidiBuffer() = default;
    };
    
    class MemoryBlock
    {
    public:
        MemoryBlock() = default;
        MemoryBlock(const void* data, size_t size) : block(static_cast<const char*>(data), static_cast<const char*>(data) + size) {}
        
        void* getData() { return block.data(); }
        const void* getData() const { return block.data(); }
        size_t getSize() const { return block.size(); }
        
    private:
        std::vector<char> block;
    };
    
    class AudioProcessorGraph : public AudioProcessor
    {
    public:
        struct NodeID
        {
            int uid = -1;
            bool operator==(const NodeID& other) const { return uid == other.uid; }
            bool operator!=(const NodeID& other) const { return uid != other.uid; }
        };
        
        AudioProcessorGraph() : nextNodeId(1) {}
        
        void prepareToPlay(double sampleRate, int samplesPerBlock) override {}
        void releaseResources() override {}
        void processBlock(AudioBuffer& buffer, MidiBuffer& midiMessages) override {}
        
        NodeID addNode(std::unique_ptr<AudioProcessor> processor)
        {
            NodeID id;
            id.uid = nextNodeId++;
            nodes[id.uid] = std::move(processor);
            return id;
        }
        
        bool removeNode(NodeID nodeId)
        {
            auto it = nodes.find(nodeId.uid);
            if (it != nodes.end())
            {
                nodes.erase(it);
                return true;
            }
            return false;
        }
        
        bool addConnection(const std::pair<NodeID, int>& source, const std::pair<NodeID, int>& dest)
        {
            return true;
        }
        
        bool removeConnection(const std::pair<NodeID, int>& source, const std::pair<NodeID, int>& dest)
        {
            return true;
        }
        
        int getLatencySamples() const { return 0; }
        
    private:
        std::unordered_map<int, std::unique_ptr<AudioProcessor>> nodes;
        int nextNodeId;
    };
    
    class AudioDeviceManager
    {
    public:
        AudioDeviceManager() = default;
        
        String initialise(int numInputChannels, int numOutputChannels, const void* preferredDevice, bool selectDefaultDeviceOnFailure)
        {
            return String();
        }
        
        void closeAudioDevice() {}
        void addAudioCallback(AudioIODeviceCallback* callback) {}
        void removeAudioCallback(AudioIODeviceCallback* callback) {}
        AudioIODevice* getCurrentAudioDevice() { return nullptr; }
    };
    
    class MidiInput
    {
    public:
        struct DeviceInfo
        {
            String name;
        };
        
        static std::vector<DeviceInfo> getAvailableDevices()
        {
            return {};
        }
        
        static MidiInput* openDevice(const String& deviceName, MidiInputCallback* callback)
        {
            return nullptr;
        }
        
        void start() {}
        void stop() {}
        String getName() const { return String(); }
    };
    
    class MidiOutput
    {
    public:
        struct DeviceInfo
        {
            String name;
        };
        
        static std::vector<DeviceInfo> getAvailableDevices()
        {
            return {};
        }
        
        static MidiOutput* openDevice(const String& deviceName)
        {
            return nullptr;
        }
        
        void startBackgroundThread() {}
        void stop() {}
        void sendMessageNow(const MidiMessage& message) {}
        String getName() const { return String(); }
    };
    
    class MidiInputCallback
    {
    public:
        virtual ~MidiInputCallback() = default;
        virtual void handleIncomingMidiMessage(MidiInput* source, const MidiMessage& message) = 0;
    };
    
    class MidiMessage
    {
    public:
        MidiMessage() = default;
        MidiMessage(uint8_t status, uint8_t data1, uint8_t data2, double timestamp = 0.0)
            : statusByte(status), data1(data1), data2(data2), timeStamp(timestamp) {}
        
        uint8_t getChannel() const { return statusByte & 0x0F; }
        bool isMidiClock() const { return statusByte == 0xF8; }
        bool isSongPositionPointer() const { return statusByte == 0xF2; }
        int getSongPositionPointerMidiBeat() const { return data1 | (data2 << 7); }
        
        const uint8_t* getRawData() const { return &statusByte; }
        int getRawDataSize() const { return 3; }
        double getTimeStamp() const { return timeStamp; }
        
    private:
        uint8_t statusByte = 0;
        uint8_t data1 = 0;
        uint8_t data2 = 0;
        double timeStamp = 0.0;
    };
    
    class AudioPluginInstance : public AudioProcessor
    {
    public:
        virtual ~AudioPluginInstance() = default;
    };
    
    class AudioPluginFormatManager
    {
    public:
        void addDefaultFormats() {}
        AudioPluginInstance* createPluginInstance(const PluginDescription& description, double sampleRate, int bufferSize, String& errorMessage)
        {
            return nullptr;
        }
        std::vector<AudioPluginFormat*> getFormats() { return {}; }
    };
    
    class AudioPluginFormat
    {
    public:
        virtual ~AudioPluginFormat() = default;
    };
    
    class PluginDescription
    {
    public:
        String name;
        String manufacturerName;
        String version;
        String category;
        String pluginFormatName;
    };
    
    class XmlElement
    {
    public:
        XmlElement(const String& tagName) : tag(tagName) {}
        virtual ~XmlElement() = default;
        
        void setAttribute(const String& name, const String& value) { attributes[name] = value; }
        void setAttribute(const String& name, int value) { attributes[name] = std::to_string(value); }
        void setAttribute(const String& name, double value) { attributes[name] = std::to_string(value); }
        void setAttribute(const String& name, bool value) { attributes[name] = value ? "true" : "false"; }
        
        String getStringAttribute(const String& name) const
        {
            auto it = attributes.find(name);
            return it != attributes.end() ? it->second : String();
        }
        
        int getIntAttribute(const String& name) const
        {
            auto it = attributes.find(name);
            return it != attributes.end() ? std::stoi(it->second.toStdString()) : 0;
        }
        
        double getDoubleAttribute(const String& name) const
        {
            auto it = attributes.find(name);
            return it != attributes.end() ? std::stod(it->second.toStdString()) : 0.0;
        }
        
        bool getBoolAttribute(const String& name) const
        {
            auto it = attributes.find(name);
            return it != attributes.end() ? (it->second == "true") : false;
        }
        
        void addChildElement(XmlElement* child) { children.push_back(std::unique_ptr<XmlElement>(child)); }
        XmlElement* getChildByName(const String& name) const
        {
            for (const auto& child : children)
            {
                if (child->getTagName() == name)
                    return child.get();
            }
            return nullptr;
        }
        
        std::vector<XmlElement*> getChildIterator() const
        {
            std::vector<XmlElement*> result;
            for (const auto& child : children)
            {
                result.push_back(child.get());
            }
            return result;
        }
        
        String getTagName() const { return tag; }
        bool writeToFile(const File& file, const String& dtdToUse) { return true; }
        
    private:
        String tag;
        std::unordered_map<String, String> attributes;
        std::vector<std::unique_ptr<XmlElement>> children;
    };
    
    class File
    {
    public:
        File(const String& path) : filePath(path) {}
        
        bool exists() const { return std::filesystem::exists(filePath.toStdString()); }
        bool isDirectory() const { return std::filesystem::is_directory(filePath.toStdString()); }
        String getFullPathName() const { return filePath; }
        String getFileExtension() const
        {
            std::string path = filePath.toStdString();
            size_t pos = path.find_last_of('.');
            return pos != std::string::npos ? String(path.substr(pos)) : String();
        }
        
    private:
        String filePath;
    };
    
    class XmlDocument
    {
    public:
        XmlDocument(const File& file) : file(file) {}
        
        std::unique_ptr<XmlElement> parse()
        {
            return std::make_unique<XmlElement>("root");
        }
    };
    
    class DynamicObject
    {
    public:
        void setProperty(const String& name, const var& value) { properties[name] = value; }
        var getProperty(const String& name) const
        {
            auto it = properties.find(name);
            return it != properties.end() ? it->second : var();
        }
        
    private:
        std::unordered_map<String, var> properties;
    };
    
    class var
    {
    public:
        var() = default;
        var(const String& str) : value(str) {}
        var(double d) : value(d) {}
        var(int i) : value(i) {}
        var(bool b) : value(b) {}
        
        String toString() const { return std::holds_alternative<String>(value) ? std::get<String>(value) : String(); }
        double operator double() const { return std::holds_alternative<double>(value) ? std::get<double>(value) : 0.0; }
        int operator int() const { return std::holds_alternative<int>(value) ? std::get<int>(value) : 0; }
        bool operator bool() const { return std::holds_alternative<bool>(value) ? std::get<bool>(value) : false; }
        
        bool isObject() const { return std::holds_alternative<DynamicObject*>(value); }
        DynamicObject* getDynamicObject() const { return std::holds_alternative<DynamicObject*>(value) ? std::get<DynamicObject*>(value) : nullptr; }
        bool isArray() const { return std::holds_alternative<Array<var>*>(value); }
        Array<var>* getArray() const { return std::holds_alternative<Array<var>*>(value) ? std::get<Array<var>*>(value) : nullptr; }
        
    private:
        std::variant<String, double, int, bool, DynamicObject*, Array<var>*> value;
    };
    
    template<typename T>
    class Array
    {
    public:
        void add(const T& item) { items.push_back(item); }
        size_t size() const { return items.size(); }
        T& operator[](size_t index) { return items[index]; }
        const T& operator[](size_t index) const { return items[index]; }
        
        auto begin() { return items.begin(); }
        auto end() { return items.end(); }
        auto begin() const { return items.begin(); }
        auto end() const { return items.end(); }
        
    private:
        std::vector<T> items;
    };
    
    class JSON
    {
    public:
        static String toString(const var& object) { return String("{}"); }
        static var parse(const String& jsonString) { return var(); }
    };
    
    class ScopedJuceInitialiser_GUI
    {
    public:
        ScopedJuceInitialiser_GUI() = default;
    };
    
    class JUCEApplication
    {
    public:
        virtual ~JUCEApplication() = default;
        virtual const String getApplicationName() = 0;
        virtual const String getApplicationVersion() = 0;
        virtual bool moreThanOneInstanceAllowed() { return true; }
        virtual void initialise(const String& commandLine) = 0;
        virtual void shutdown() = 0;
        virtual void systemRequestedQuit() = 0;
        virtual void anotherInstanceStarted(const String& commandLine) {}
    };
    
    class FloatVectorOperations
    {
    public:
        static void clear(float* dest, int num) { std::fill(dest, dest + num, 0.0f); }
    };
    
    class MathConstants
    {
    public:
        static constexpr double pi = 3.14159265358979323846;
    };
    
    // Mock utility functions
    template<typename T>
    T jlimit(T min, T max, T value) { return std::max(min, std::min(max, value)); }
    
    // Mock JUCE application macro
    #define START_JUCE_APPLICATION(AppClass) \
        int main(int argc, char** argv) \
        { \
            AppClass app; \
            app.initialise(String()); \
            app.systemRequestedQuit(); \
            app.shutdown(); \
            return 0; \
        }
}