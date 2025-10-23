/**
 * @file Main.cpp
 * @brief NOMAD DAW - Main Application Entry Point
 * 
 * This is the main entry point for the NOMAD Digital Audio Workstation.
 * It initializes all core systems:
 * - NomadPlat: Platform abstraction (windowing, input, OpenGL)
 * - NomadUI: UI rendering framework
 * - NomadAudio: Real-time audio engine
 * 
 * @version 1.0.0
 * @license Proprietary
 */

#include "../NomadPlat/include/NomadPlatform.h"
#include "../NomadUI/Core/NUIComponent.h"
#include "../NomadUI/Core/NUICustomWindow.h"
#include "../NomadUI/Core/NUIThemeSystem.h"
#include "../NomadUI/Graphics/NUIRenderer.h"
#include "../NomadUI/Graphics/OpenGL/NUIRendererGL.h"
#include "../NomadUI/Platform/NUIPlatformBridge.h"
#include "../NomadAudio/include/NomadAudio.h"
#include "../NomadCore/include/NomadLog.h"
#include "TransportBar.h"
#include "AudioSettingsDialog.h"
#include "FileBrowser.h"
#include "AudioVisualizer.h"

#include <memory>
#include <iostream>

using namespace Nomad;
using namespace NomadUI;
using namespace Nomad::Audio;

/**
 * @brief Main application class
 * 
 * Manages the lifecycle of all NOMAD subsystems and the main event loop.
 */
/**
 * @brief Main content area for NOMAD DAW
 */
class NomadContent : public NomadUI::NUIComponent {
public:
    NomadContent() {
        // Create transport bar
        m_transportBar = std::make_shared<TransportBar>();
        addChild(m_transportBar);
        
        // Create file browser - starts right below transport bar accent strip
        m_fileBrowser = std::make_shared<NomadUI::FileBrowser>();
        // Initial positioning - will be updated in onResize
        m_fileBrowser->setBounds(NomadUI::NUIRect(0, 62, 280, 620)); // Right below transport bar (Y=62), no gap
        m_fileBrowser->setOnFileOpened([this](const NomadUI::FileItem& file) {
            Log::info("File opened: " + file.path);
            // TODO: Load audio file or project
        });
        addChild(m_fileBrowser);
        
        // Create compact audio meter - positioned inside transport bar (right side)
        m_audioVisualizer = std::make_shared<NomadUI::AudioVisualizer>();
        m_audioVisualizer->setBounds(NomadUI::NUIRect(1100, 46, 80, 40)); // Inside transport bar (Y=46, height=40) - centered
        m_audioVisualizer->setMode(NomadUI::AudioVisualizationMode::CompactMeter);
        m_audioVisualizer->setShowStereo(true);
        addChild(m_audioVisualizer);
    }
    
    void setAudioStatus(bool active) {
        m_audioActive = active;
    }
    
    TransportBar* getTransportBar() const {
        return m_transportBar.get();
    }
    
    std::shared_ptr<NomadUI::AudioVisualizer> getAudioVisualizer() const {
        return m_audioVisualizer;
    }
    
    
    void onRender(NomadUI::NUIRenderer& renderer) override {
        // Note: bounds are set by parent (NUICustomWindow) to be below the title bar
        NomadUI::NUIRect bounds = getBounds();
        float width = bounds.width;
        float height = bounds.height;
        
        // Transport bar height
        float transportHeight = 60.0f;
        
        // Get Liminal Dark v2.0 theme colors
        auto& themeManager = NomadUI::NUIThemeManager::getInstance();
        NomadUI::NUIColor textColor = themeManager.getColor("textPrimary");        // #e6e6eb - Soft white
        NomadUI::NUIColor accentColor = themeManager.getColor("accentCyan");       // #00bcd4 - Accent cyan
        NomadUI::NUIColor statusColor = m_audioActive ? 
            themeManager.getColor("accentLime") : themeManager.getColor("error");  // #9eff61 for active, #ff4d4d for error
        
        // Draw center panel with padding (below transport bar) - BACKGROUND FIRST
        // Use absolute coordinates since NUIComponent doesn't transform child coordinates
        NomadUI::NUIRect centerPanel(bounds.x + 20, bounds.y + transportHeight + 20, 
                                     width - 40, height - transportHeight - 40);
        renderer.fillRoundedRect(centerPanel, 8, themeManager.getColor("surfaceTertiary"));  // #25252a - Popups, transport bar, VU meter surface
        
        // Render children AFTER background (transport bar and other components on top)
        renderChildren(renderer);
        
        // Calculate center position using absolute coordinates
        float centerX = bounds.x + (width / 2.0f);
        float centerY = bounds.y + transportHeight + ((height - transportHeight) / 2.0f);
        
        // Draw welcome message with enhanced styling
        std::string welcomeMsg = "NOMAD DAW - Ready";
        auto welcomeSize = renderer.measureText(welcomeMsg, 32);
        float welcomeY = centerY - welcomeSize.height / 2.0f - 20; // Perfect vertical centering
        renderer.drawText(welcomeMsg, 
                         NomadUI::NUIPoint(centerX - welcomeSize.width / 2.0f, welcomeY), 
                         32, textColor);
        
        // Draw audio status with perfect alignment
        std::string statusText = m_audioActive ? "Audio: Active" : "Audio: Inactive";
        auto statusSize = renderer.measureText(statusText, 16);
        float statusY = centerY + 20; // Perfect spacing from welcome message
        renderer.drawText(statusText, 
                         NomadUI::NUIPoint(centerX - statusSize.width / 2.0f, statusY), 
                         16, statusColor);
        
        // Draw instructions with perfect alignment
        std::string infoText = "Press SPACE to play/pause | ESC to exit | F11 for fullscreen";
        auto infoSize = renderer.measureText(infoText, 14);
        float infoY = centerY + 50; // Perfect spacing from status
        renderer.drawText(infoText, 
                         NomadUI::NUIPoint(centerX - infoSize.width / 2.0f, infoY), 
                         14, themeManager.getColor("textSecondary"));
    }
    
    void onResize(int width, int height) override {
        // Update transport bar bounds
        // Since NUIComponent doesn't transform child coordinates, we need to position
        // the transport bar at the absolute position (content area Y offset + 0)
        if (m_transportBar) {
            float transportHeight = 60.0f;
            // Get our bounds to find the Y offset (should be 32 for title bar height)
            NomadUI::NUIRect contentBounds = getBounds();
            m_transportBar->setBounds(NomadUI::NUIRect(contentBounds.x, contentBounds.y, 
                                                       static_cast<float>(width), transportHeight));
            m_transportBar->onResize(width, static_cast<int>(transportHeight));
        }
        
        // Update file browser bounds to be responsive
        if (m_fileBrowser) {
            float fileBrowserWidth = std::min(280.0f, width * 0.25f); // 25% of width or max 280px
            float fileBrowserHeight = height - 62; // Full height minus transport bar (60px) + 2px gap
            NomadUI::NUIRect contentBounds = getBounds();
            m_fileBrowser->setBounds(NomadUI::NUIRect(contentBounds.x, contentBounds.y + 62, 
                                                      fileBrowserWidth, fileBrowserHeight));
        }
        
        // Update audio visualizer position
        if (m_audioVisualizer) {
            NomadUI::NUIRect contentBounds = getBounds();
            m_audioVisualizer->setBounds(NomadUI::NUIRect(contentBounds.x + width - 100, 
                                                          contentBounds.y + 10, 80, 40));
        }
        
        NomadUI::NUIComponent::onResize(width, height);
    }
    
private:
    std::shared_ptr<TransportBar> m_transportBar;
    std::shared_ptr<NomadUI::FileBrowser> m_fileBrowser;
    std::shared_ptr<NomadUI::AudioVisualizer> m_audioVisualizer;
    bool m_audioActive = false;
};

/**
 * @brief Root component that contains the custom window
 */
class NomadRootComponent : public NUIComponent {
public:
    NomadRootComponent() = default;
    
    void setCustomWindow(std::shared_ptr<NUICustomWindow> window) {
        m_customWindow = window;
        addChild(m_customWindow);
    }
    
    void setAudioSettingsDialog(std::shared_ptr<AudioSettingsDialog> dialog) {
        // Store reference to dialog for debugging
        m_audioSettingsDialog = dialog;
    }
    
    void onRender(NUIRenderer& renderer) override {
        // Debug output
        static bool firstRender = true;
        if (firstRender) {
            std::cout << "NomadRootComponent::onRender() called" << std::endl;
            std::cout << "  Bounds: " << getBounds().x << "," << getBounds().y << " " 
                      << getBounds().width << "x" << getBounds().height << std::endl;
            std::cout << "  Children count: " << getChildren().size() << std::endl;
            firstRender = false;
        }
        
        // Don't draw background here - let custom window handle it
        // Just render children (custom window and audio settings dialog)
        renderChildren(renderer);
        
        // Audio settings dialog is handled by its own render method
    }
    
    void onResize(int width, int height) override {
        if (m_customWindow) {
            m_customWindow->setBounds(NUIRect(0, 0, width, height));
        }
        
        // Resize all children (including audio settings dialog)
        for (auto& child : getChildren()) {
            if (child) {
                child->onResize(width, height);
            }
        }
        
        NUIComponent::onResize(width, height);
    }
    
private:
    std::shared_ptr<NUICustomWindow> m_customWindow;
    std::shared_ptr<AudioSettingsDialog> m_audioSettingsDialog;
};

/**
 * @brief Main application class
 * 
 * Manages the lifecycle of all NOMAD subsystems and the main event loop.
 */
class NomadApp {
public:
    NomadApp() 
        : m_running(false)
        , m_audioInitialized(false)
    {
    }

    ~NomadApp() {
        shutdown();
    }

    /**
     * @brief Initialize all subsystems
     */
    bool initialize() {
        std::stringstream ss;
        Log::info("NOMAD DAW v1.0.0 - Initializing...");

        // Initialize platform
        if (!Platform::initialize()) {
            Log::error("Failed to initialize platform");
            return false;
        }
        Log::info("Platform initialized");

        // Create window using NUIPlatformBridge
        m_window = std::make_unique<NUIPlatformBridge>();
        
        WindowDesc desc;
        desc.title = "NOMAD DAW v1.0";
        desc.width = 1280;
        desc.height = 720;
        desc.resizable = true;
        desc.decorated = false;  // Borderless for custom title bar

        if (!m_window->create(desc)) {
            Log::error("Failed to create window");
            return false;
        }
        
        ss.str("");
        ss << "Window created: " << desc.width << "x" << desc.height;
        Log::info(ss.str());

        // Create OpenGL context
        if (!m_window->createGLContext()) {
            Log::error("Failed to create OpenGL context");
            return false;
        }

        if (!m_window->makeContextCurrent()) {
            Log::error("Failed to make OpenGL context current");
            return false;
        }

        Log::info("OpenGL context created");

        // Initialize UI renderer (this will initialize GLAD internally)
        try {
            m_renderer = std::make_unique<NUIRendererGL>();
            if (!m_renderer->initialize(desc.width, desc.height)) {
                Log::error("Failed to initialize UI renderer");
                return false;
            }
            
            ss.str("");
            ss << "UI renderer initialized: " << m_renderer->getBackendName();
            Log::info(ss.str());
        }
        catch (const std::exception& e) {
            ss.str("");
            ss << "Exception during renderer initialization: " << e.what();
            Log::error(ss.str());
            return false;
        }

        // Initialize audio engine
        m_audioManager = std::make_unique<AudioDeviceManager>();
        if (!m_audioManager->initialize()) {
            Log::error("Failed to initialize audio engine");
            // Continue without audio for now
            m_audioInitialized = false;
        } else {
            ss.str("");
            ss << "Audio engine initialized: " << Audio::getBackendName();
            Log::info(ss.str());
            
            // Get default audio device
            auto defaultDevice = m_audioManager->getDefaultOutputDevice();
            ss.str("");
            ss << "Default audio device: " << defaultDevice.name;
            Log::info(ss.str());
            
            // Configure audio stream
            AudioStreamConfig config;
            config.deviceId = defaultDevice.id;
            config.sampleRate = 48000;
            config.bufferSize = 512;
            config.numInputChannels = 0;
            config.numOutputChannels = 2;

            // Open audio stream with a simple callback
            if (m_audioManager->openStream(config, audioCallback, this)) {
                ss.str("");
                ss << "Audio stream opened: " << config.sampleRate << " Hz, " 
                   << config.bufferSize << " samples, " << config.numOutputChannels << " channels";
                Log::info(ss.str());
                m_audioInitialized = true;
            } else {
                Log::warning("Failed to open audio stream");
                m_audioInitialized = false;
            }
        }

        // Initialize Nomad theme
        auto& themeManager = NUIThemeManager::getInstance();
        themeManager.setActiveTheme("nomad-dark");
        Log::info("Theme system initialized");
        
        // Create root component
        m_rootComponent = std::make_shared<NomadRootComponent>();
        m_rootComponent->setBounds(NUIRect(0, 0, desc.width, desc.height));
        
        // Create custom window with title bar
        m_customWindow = std::make_shared<NUICustomWindow>();
        m_customWindow->setTitle("NOMAD DAW v1.0");
        m_customWindow->setBounds(NUIRect(0, 0, desc.width, desc.height));
        
        // Create content area
        m_content = std::make_shared<NomadContent>();
        m_content->setAudioStatus(m_audioInitialized);
        m_customWindow->setContent(m_content.get());
        
        // Wire up transport bar callbacks
        if (m_content->getTransportBar()) {
            auto* transport = m_content->getTransportBar();
            
            transport->setOnPlay([this]() {
                Log::info("Transport: Play");
                if (m_audioManager) {
                    m_audioManager->startStream();
                }
            });
            
            transport->setOnPause([this]() {
                Log::info("Transport: Pause");
                if (m_audioManager) {
                    m_audioManager->stopStream();
                }
            });
            
            transport->setOnStop([this, transport]() {
                Log::info("Transport: Stop");
                if (m_audioManager) {
                    m_audioManager->stopStream();
                }
                // Reset position to 0
                transport->setPosition(0.0);
            });
            
            transport->setOnTempoChange([this](float bpm) {
                std::stringstream ss;
                ss << "Transport: Tempo changed to " << bpm << " BPM";
                Log::info(ss.str());
                // TODO: Update audio engine tempo
            });
        }
        
        // Create audio settings dialog
        m_audioSettingsDialog = std::make_shared<AudioSettingsDialog>(m_audioManager.get());
        m_audioSettingsDialog->setBounds(NUIRect(0, 0, desc.width, desc.height));
        m_audioSettingsDialog->setOnApply([this]() {
            Log::info("Audio settings applied");
            // Update audio status
            if (m_content) {
                m_content->setAudioStatus(m_audioManager->isStreamRunning());
            }
        });
        m_audioSettingsDialog->setOnCancel([this]() {
            Log::info("Audio settings cancelled");
        });
        // Add dialog to root component AFTER custom window so it renders on top
        Log::info("Audio settings dialog created");
        
        // Add custom window to root component
        m_rootComponent->setCustomWindow(m_customWindow);
        
        // Add audio settings dialog to root component (after custom window for proper z-ordering)
        m_rootComponent->addChild(m_audioSettingsDialog);
        m_rootComponent->setAudioSettingsDialog(m_audioSettingsDialog);
        
        // Debug: Verify dialog was added
        std::stringstream ss2;
        ss2 << "Dialog added to root component, pointer: " << m_audioSettingsDialog.get();
        Log::info(ss2.str());
        
        // Connect window and renderer to bridge
        m_window->setRootComponent(m_rootComponent.get());
        m_window->setRenderer(m_renderer.get());
        
        // Connect custom window to platform window for dragging and window controls
        m_customWindow->setWindowHandle(m_window.get());
        
        // Debug: Check if custom window has children (should have title bar)
        auto children = m_customWindow->getChildren();
        ss.str("");
        ss << "Custom window has " << children.size() << " children";
        Log::info(ss.str());
        
        Log::info("Custom window created");
        
        // Setup window callbacks
        setupCallbacks();

        m_running = true;
        Log::info("NOMAD DAW initialized successfully");
        return true;
    }

    /**
     * @brief Main application loop
     */
    void run() {
        if (!m_running) {
            Log::error("Cannot run - application not initialized");
            return;
        }

        m_window->show();
        Log::info("Entering main loop...");

        // Start audio if initialized
        if (m_audioInitialized && !m_audioManager->startStream()) {
            Log::warning("Failed to start audio stream");
        }

        // Main event loop
        while (m_running && m_window->processEvents()) {
            render();
            m_window->swapBuffers();
        }

        Log::info("Exiting main loop");
    }

    /**
     * @brief Shutdown all subsystems
     */
    void shutdown() {
        if (!m_running) {
            return;
        }

        Log::info("Shutting down NOMAD DAW...");

        // Stop and close audio
        if (m_audioInitialized && m_audioManager) {
            m_audioManager->stopStream();
            m_audioManager->closeStream();
            m_audioManager->shutdown();
            Log::info("Audio engine shutdown");
        }

        // Shutdown UI renderer
        if (m_renderer) {
            m_renderer->shutdown();
            Log::info("UI renderer shutdown");
        }

        // Destroy window
        if (m_window) {
            m_window->destroy();
            m_window.reset();
            Log::info("Window destroyed");
        }

        // Shutdown platform
        Platform::shutdown();
        Log::info("Platform shutdown");

        m_running = false;
        Log::info("NOMAD DAW shutdown complete");
    }

private:
    /**
     * @brief Setup window event callbacks
     */
    void setupCallbacks() {
        // Window resize callback
        m_window->setResizeCallback([this](int width, int height) {
            if (m_renderer) {
                m_renderer->resize(width, height);
            }
            std::stringstream ss;
            ss << "Window resized: " << width << "x" << height;
            Log::info(ss.str());
        });

        // Window close callback
        m_window->setCloseCallback([this]() {
            Log::info("Window close requested");
            m_running = false;
        });

        // Key callback
        m_window->setKeyCallback([this](int key, bool pressed) {
            // Debug: Log key presses
            if (pressed) {
                std::stringstream ss;
                ss << "Key pressed: " << key;
                Log::info(ss.str());
                std::cout << "Key pressed: " << key << " (P should be 80)" << std::endl;
            }
            
            // First, try to handle key events in the audio settings dialog if it's visible
            if (m_audioSettingsDialog && m_audioSettingsDialog->isVisible()) {
                NomadUI::NUIKeyEvent event;
                event.keyCode = static_cast<NomadUI::NUIKeyCode>(key);
                event.pressed = pressed;
                event.released = !pressed;
                
                if (m_audioSettingsDialog->onKeyEvent(event)) {
                    return; // Dialog handled the event
                }
            }
            
            // Handle global key shortcuts
            // Debug: Log all key presses
            if (pressed) {
                std::cout << "Key pressed: " << key << " (P should be 80)" << std::endl;
            }
            
            if (key == static_cast<int>(KeyCode::Escape) && pressed) {
                // If audio settings dialog is open, close it
                if (m_audioSettingsDialog && m_audioSettingsDialog->isVisible()) {
                    m_audioSettingsDialog->hide();
                } else if (m_customWindow && m_customWindow->isFullScreen()) {
                    Log::info("Escape key pressed - exiting fullscreen");
                    m_customWindow->exitFullScreen();
                } else {
                    Log::info("Escape key pressed - exiting");
                    m_running = false;
                }
            } else if (key == static_cast<int>(KeyCode::F11) && pressed) {
                if (m_customWindow) {
                    Log::info("F11 pressed - toggling fullscreen");
                    m_customWindow->toggleFullScreen();
                }
            } else if (key == static_cast<int>(KeyCode::Space) && pressed) {
                // Space bar to play/stop (not pause)
                if (m_content && m_content->getTransportBar()) {
                    auto* transport = m_content->getTransportBar();
                    if (transport->getState() == TransportState::Playing) {
                        transport->stop();
                    } else {
                        transport->play();
                    }
                }
            } else if (key == static_cast<int>(KeyCode::P) && pressed) {
                // P key to open audio settings (Preferences)
                Log::info("===== P KEY HANDLER START =====");
                if (m_audioSettingsDialog) {
                    Log::info("Dialog pointer is valid, calling show()");
                    m_audioSettingsDialog->show();
                    Log::info("show() method called successfully");
                } else {
                    Log::info("ERROR: m_audioSettingsDialog is NULL!");
                }
                Log::info("===== P KEY HANDLER END =====");
            }
        });
        
        // Mouse button and move callbacks are handled by NUIPlatformBridge
        // Events will be forwarded to root component and its children

        // DPI change callback
        m_window->setDPIChangeCallback([this](float dpiScale) {
            std::stringstream ss;
            ss << "DPI changed: " << dpiScale;
            Log::info(ss.str());
        });
    }

    /**
     * @brief Render a frame
     */
    void render() {
        if (!m_renderer || !m_rootComponent) {
            return;
        }

        // Clear background with same color as title bar and window for flush appearance
        auto& themeManager = NUIThemeManager::getInstance();
        NUIColor bgColor = themeManager.getColor("background"); // Match title bar color
        
        // Debug: Log the color once
        static bool colorLogged = false;
        if (!colorLogged) {
            std::stringstream ss;
            ss << "Clear color: R=" << bgColor.r << " G=" << bgColor.g << " B=" << bgColor.b;
            Log::info(ss.str());
            colorLogged = true;
        }
        
        m_renderer->clear(bgColor);

        m_renderer->beginFrame();

        // Render root component (which contains custom window)
        m_rootComponent->onRender(*m_renderer);

        m_renderer->endFrame();
    }

    /**
     * @brief Audio callback function
     */
    static int audioCallback(float* outputBuffer, const float* inputBuffer, 
                            uint32_t nFrames, double streamTime, void* userData) {
        NomadApp* app = static_cast<NomadApp*>(userData);
        
        // Generate a more dynamic test tone for visualization
        static double phase = 0.0;
        static double phase2 = 0.0;
        double frequency = 440.0; // A4 note
        double frequency2 = 880.0; // A5 note for stereo effect
        double sampleRate = 48000.0;
        double phaseIncrement = 2.0 * 3.14159265359 * frequency / sampleRate;
        double phaseIncrement2 = 2.0 * 3.14159265359 * frequency2 / sampleRate;
        
        // Add some variation to make it more interesting
        double time = streamTime;
        double amplitude = 0.3f + 0.2f * std::sin(time * 2.0); // Varying amplitude
        double stereoOffset = 0.1f * std::sin(time * 0.5); // Stereo movement
        
        for (uint32_t i = 0; i < nFrames; ++i) {
            float leftSample = static_cast<float>(std::sin(phase) * amplitude);
            float rightSample = static_cast<float>(std::sin(phase2 + stereoOffset) * amplitude);
            
            outputBuffer[i * 2] = leftSample;     // Left channel
            outputBuffer[i * 2 + 1] = rightSample; // Right channel
            
            phase += phaseIncrement;
            phase2 += phaseIncrement2;
        }
        
        // Send audio data to visualizer if available
        if (app && app->m_content) {
            // Get the audio visualizer from the content
            auto visualizer = app->m_content->getAudioVisualizer();
            if (visualizer) {
                visualizer->setAudioData(
                    outputBuffer, outputBuffer + 1, nFrames, sampleRate);
            }
        }
        
        return 0;
    }

private:
    std::unique_ptr<NUIPlatformBridge> m_window;
    std::unique_ptr<NUIRenderer> m_renderer;
    std::unique_ptr<AudioDeviceManager> m_audioManager;
    std::shared_ptr<NomadRootComponent> m_rootComponent;
    std::shared_ptr<NUICustomWindow> m_customWindow;
    std::shared_ptr<NomadContent> m_content;
    std::shared_ptr<AudioSettingsDialog> m_audioSettingsDialog;
    bool m_running;
    bool m_audioInitialized;
};

/**
 * @brief Application entry point
 */
int main(int argc, char* argv[]) {
    // Initialize logging with console logger
    Log::init(std::make_shared<ConsoleLogger>(LogLevel::Info));
    Log::setLevel(LogLevel::Info);

    try {
        NomadApp app;
        
        if (!app.initialize()) {
            Log::error("Failed to initialize NOMAD DAW");
            return 1;
        }

        app.run();

        Log::info("NOMAD DAW exited successfully");
        return 0;
    }
    catch (const std::exception& e) {
        std::stringstream ss;
        ss << "Fatal error: " << e.what();
        Log::error(ss.str());
        return 1;
    }
    catch (...) {
        Log::error("Unknown fatal error");
        return 1;
    }
}
