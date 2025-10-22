/**
 * @file TransportBar.cpp
 * @brief Transport bar implementation
 */

#include "TransportBar.h"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace Nomad {

TransportBar::TransportBar()
    : NomadUI::NUIComponent()
    , m_state(TransportState::Stopped)
    , m_tempo(120.0f)
    , m_position(0.0)
{
    createIcons();
    createButtons();
    
    // Create labels
    m_tempoLabel = std::make_shared<NomadUI::NUILabel>();
    m_tempoLabel->setAlignment(NomadUI::NUILabel::Alignment::Center);
    addChild(m_tempoLabel);
    
    m_positionLabel = std::make_shared<NomadUI::NUILabel>();
    m_positionLabel->setAlignment(NomadUI::NUILabel::Alignment::Center);
    addChild(m_positionLabel);
    
    updateLabels();
    updateButtonStates();
}

void TransportBar::createIcons() {
    // Play icon (triangle)
    const char* playSvg = R"(
        <svg viewBox="0 0 24 24" fill="currentColor">
            <path d="M8 5v14l11-7z"/>
        </svg>
    )";
    m_playIcon = std::make_shared<NomadUI::NUIIcon>(playSvg);
    m_playIcon->setIconSize(NomadUI::NUIIconSize::Medium);
    m_playIcon->setColorFromTheme("textPrimary");
    
    // Pause icon (two bars)
    const char* pauseSvg = R"(
        <svg viewBox="0 0 24 24" fill="currentColor">
            <path d="M6 4h4v16H6V4zm8 0h4v16h-4V4z"/>
        </svg>
    )";
    m_pauseIcon = std::make_shared<NomadUI::NUIIcon>(pauseSvg);
    m_pauseIcon->setIconSize(NomadUI::NUIIconSize::Medium);
    m_pauseIcon->setColorFromTheme("textPrimary");
    
    // Stop icon (square)
    const char* stopSvg = R"(
        <svg viewBox="0 0 24 24" fill="currentColor">
            <rect x="6" y="6" width="12" height="12"/>
        </svg>
    )";
    m_stopIcon = std::make_shared<NomadUI::NUIIcon>(stopSvg);
    m_stopIcon->setIconSize(NomadUI::NUIIconSize::Medium);
    m_stopIcon->setColorFromTheme("textPrimary");
    
    // Record icon (circle)
    const char* recordSvg = R"(
        <svg viewBox="0 0 24 24" fill="currentColor">
            <circle cx="12" cy="12" r="8"/>
        </svg>
    )";
    m_recordIcon = std::make_shared<NomadUI::NUIIcon>(recordSvg);
    m_recordIcon->setIconSize(NomadUI::NUIIconSize::Medium);
    m_recordIcon->setColor(NomadUI::NUIColor(0.9f, 0.2f, 0.2f, 1.0f)); // Red
}

void TransportBar::createButtons() {
    // Play/Pause button
    m_playButton = std::make_shared<NomadUI::NUIButton>();
    m_playButton->setText("");
    m_playButton->setSize(40, 40);
    m_playButton->setOnClick([this]() {
        togglePlayPause();
    });
    addChild(m_playButton);
    
    // Stop button
    m_stopButton = std::make_shared<NomadUI::NUIButton>();
    m_stopButton->setText("");
    m_stopButton->setSize(40, 40);
    m_stopButton->setOnClick([this]() {
        stop();
    });
    addChild(m_stopButton);
    
    // Record button (for future use)
    m_recordButton = std::make_shared<NomadUI::NUIButton>();
    m_recordButton->setText("");
    m_recordButton->setSize(40, 40);
    m_recordButton->setEnabled(false); // Disabled for now
    addChild(m_recordButton);
}

void TransportBar::play() {
    if (m_state != TransportState::Playing) {
        m_state = TransportState::Playing;
        updateButtonStates();
        if (m_onPlay) {
            m_onPlay();
        }
    }
}

void TransportBar::pause() {
    if (m_state == TransportState::Playing) {
        m_state = TransportState::Paused;
        updateButtonStates();
        if (m_onPause) {
            m_onPause();
        }
    }
}

void TransportBar::stop() {
    if (m_state != TransportState::Stopped) {
        m_state = TransportState::Stopped;
        m_position = 0.0;
        updateButtonStates();
        updateLabels();
        if (m_onStop) {
            m_onStop();
        }
    }
}

void TransportBar::togglePlayPause() {
    if (m_state == TransportState::Playing) {
        pause();
    } else {
        play();
    }
}

void TransportBar::setTempo(float bpm) {
    m_tempo = std::max(20.0f, std::min(999.0f, bpm));
    updateLabels();
    if (m_onTempoChange) {
        m_onTempoChange(m_tempo);
    }
}

void TransportBar::setPosition(double seconds) {
    m_position = std::max(0.0, seconds);
    updateLabels();
}

void TransportBar::updateButtonStates() {
    // Update play button text based on state
    if (m_state == TransportState::Playing) {
        m_playButton->setText("||"); // Pause symbol
    } else {
        m_playButton->setText("▶"); // Play symbol
    }
    
    // Update stop button
    m_stopButton->setText("■"); // Stop symbol
    m_stopButton->setEnabled(m_state != TransportState::Stopped);
    
    // Update record button
    m_recordButton->setText("●"); // Record symbol
}

void TransportBar::updateLabels() {
    // Update tempo label
    std::stringstream tempoSs;
    tempoSs << std::fixed << std::setprecision(1) << m_tempo << " BPM";
    m_tempoLabel->setText(tempoSs.str());
    
    // Update position label
    m_positionLabel->setText(formatTime(m_position));
}

std::string TransportBar::formatTime(double seconds) const {
    int totalSeconds = static_cast<int>(seconds);
    int minutes = totalSeconds / 60;
    int secs = totalSeconds % 60;
    int millis = static_cast<int>((seconds - totalSeconds) * 100);
    
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(2) << minutes << ":"
       << std::setw(2) << secs << "."
       << std::setw(2) << millis;
    return ss.str();
}

void TransportBar::layoutComponents() {
    NomadUI::NUIRect bounds = getBounds();
    float padding = 8.0f;
    float buttonSize = 40.0f;
    float spacing = 8.0f;
    
    // Center vertically offset
    float centerOffsetY = (bounds.height - buttonSize) / 2.0f;
    
    // Layout buttons from left using utility helpers
    float x = padding;
    
    // Play button - using NUIAbsolute helper for cleaner code
    m_playButton->setBounds(NomadUI::NUIAbsolute(bounds, x, centerOffsetY, buttonSize, buttonSize));
    x += buttonSize + spacing;
    
    // Stop button
    m_stopButton->setBounds(NomadUI::NUIAbsolute(bounds, x, centerOffsetY, buttonSize, buttonSize));
    x += buttonSize + spacing;
    
    // Record button
    m_recordButton->setBounds(NomadUI::NUIAbsolute(bounds, x, centerOffsetY, buttonSize, buttonSize));
    x += buttonSize + spacing * 2;
    
    // Position label (larger, centered)
    float positionWidth = 120.0f;
    float positionHeight = 30.0f;
    float positionOffsetY = (bounds.height - positionHeight) / 2.0f;
    m_positionLabel->setBounds(NomadUI::NUIAbsolute(bounds, x, positionOffsetY, positionWidth, positionHeight));
    x += positionWidth + spacing * 2;
    
    // Tempo label (right side)
    float tempoWidth = 100.0f;
    float tempoHeight = 24.0f;
    float tempoOffsetY = (bounds.height - tempoHeight) / 2.0f;
    float tempoOffsetX = bounds.width - tempoWidth - padding;
    m_tempoLabel->setBounds(NomadUI::NUIAbsolute(bounds, tempoOffsetX, tempoOffsetY, tempoWidth, tempoHeight));
}

void TransportBar::onRender(NomadUI::NUIRenderer& renderer) {
    NomadUI::NUIRect bounds = getBounds();
    
    // Get theme colors
    auto& themeManager = NomadUI::NUIThemeManager::getInstance();
    NomadUI::NUIColor bgColor = themeManager.getColor("surfaceTertiary");
    NomadUI::NUIColor borderColor = themeManager.getColor("borderSubtle");
    
    // Draw background
    renderer.fillRect(bounds, bgColor);
    
    // Draw top border
    renderer.drawLine(
        NomadUI::NUIPoint(bounds.x, bounds.y),
        NomadUI::NUIPoint(bounds.x + bounds.width, bounds.y),
        1.0f,
        borderColor
    );
    
    // Render children (buttons and labels)
    renderChildren(renderer);
}

void TransportBar::onResize(int width, int height) {
    // Don't reset bounds here - parent has already set the correct position
    // Just update the size while preserving x,y position
    NomadUI::NUIRect currentBounds = getBounds();
    setBounds(NomadUI::NUIRect(currentBounds.x, currentBounds.y, width, height));
    layoutComponents();
    NomadUI::NUIComponent::onResize(width, height);
}

bool TransportBar::onMouseEvent(const NomadUI::NUIMouseEvent& event) {
    // Let children handle mouse events first
    return NomadUI::NUIComponent::onMouseEvent(event);
}

} // namespace Nomad
