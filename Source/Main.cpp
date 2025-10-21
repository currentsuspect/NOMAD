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
    NomadContent() = default;
    
    void setAudioStatus(bool active) {
        m_audioActive = active;
    }
    
    void onRender(NomadUI::NUIRenderer& renderer) override {
        // Note: bounds are set by parent (NUICustomWindow) to be below the title bar
        NomadUI::NUIRect bounds = getBounds();
        float width = bounds.width;
        float height = bounds.height;
        
        // Get theme colors
        auto& themeManager = NomadUI::NUIThemeManager::getInstance();
        // Use same background as title bar for flush, seamless appearance
        NomadUI::NUIColor contentBg = themeManager.getColor("background");
        NomadUI::NUIColor textColor = themeManager.getColor("textPrimary");
        NomadUI::NUIColor accentColor = themeManager.getColor("primary");
        NomadUI::NUIColor statusColor = m_audioActive ? 
            themeManager.getColor("success") : themeManager.getColor("error");
        
        // Don't draw content background - let window background show through for flush look
        // renderer.fillRect(bounds, contentBg);
        
        // Draw center panel with padding
        NomadUI::NUIRect centerPanel(bounds.x + 20, bounds.y + 20, 
                                     width - 40, height - 40);
        renderer.fillRoundedRect(centerPanel, 8, themeManager.getColor("surfaceRaised"));
        
        // Calculate center position within content area
        float centerX = bounds.x + (width / 2.0f);
        float centerY = bounds.y + (height / 2.0f);
        
        // Draw welcome message
        std::string welcomeMsg = "NOMAD DAW - Ready";
        auto welcomeSize = renderer.measureText(welcomeMsg, 32);
        renderer.drawText(welcomeMsg, 
                         NomadUI::NUIPoint(centerX - welcomeSize.width / 2.0f, 
                                          centerY - welcomeSize.height / 2.0f), 
                         32, textColor);
        
        // Draw audio status
        std::string statusText = m_audioActive ? "Audio: Active" : "Audio: Inactive";
        auto statusSize = renderer.measureText(statusText, 16);
        renderer.drawText(statusText, 
                         NomadUI::NUIPoint(centerX - statusSize.width / 2.0f, centerY + 40), 
                         16, statusColor);
        
        // Draw instructions
        std::string infoText = "Press ESC to exit | F11 for fullscreen | Drag title bar to move";
        auto infoSize = renderer.measureText(infoText, 14);
        renderer.drawText(infoText, 
                         NomadUI::NUIPoint(centerX - infoSize.width / 2.0f, centerY + 70), 
                         14, themeManager.getColor("textSecondary"));
    }
    
private:
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
        // Just render children (custom window)
        renderChildren(renderer);
    }
    
    void onResize(int width, int height) override {
        if (m_customWindow) {
            m_customWindow->setBounds(NUIRect(0, 0, width, height));
        }
        NUIComponent::onResize(width, height);
    }
    
private:
    std::shared_ptr<NUICustomWindow> m_customWindow;
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
        
        // Add custom window to root component
        m_rootComponent->setCustomWindow(m_customWindow);
        
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
            if (m_rootComponent) {
                m_rootComponent->onResize(width, height);
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
            if (key == static_cast<int>(KeyCode::Escape) && pressed) {
                if (m_customWindow && m_customWindow->isFullScreen()) {
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
            }
        });
        
        // Mouse button callback for custom window interaction
        m_window->setMouseButtonCallback([this](int button, bool pressed) {
            // NUIPlatformBridge will handle forwarding to root component
        });
        
        // Mouse move callback for custom window interaction
        m_window->setMouseMoveCallback([this](int x, int y) {
            // NUIPlatformBridge will handle forwarding to root component
        });

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
        // For now, just output silence
        for (uint32_t i = 0; i < nFrames * 2; ++i) {
            outputBuffer[i] = 0.0f;
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
