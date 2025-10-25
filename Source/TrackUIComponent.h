#pragma once

#include "../NomadAudio/include/Track.h"
#include "../NomadUI/Core/NUIComponent.h"
#include "../NomadUI/Core/NUILabel.h"
#include "../NomadUI/Core/NUIButton.h"
#include "../NomadUI/Core/NUISlider.h"
#include <memory>

namespace Nomad {
namespace Audio {

/**
 * @brief UI wrapper for Track class
 *
 * Provides UI interface for a Track, including controls for
 * volume, pan, mute, solo, and record functionality.
 */
class TrackUIComponent : public NomadUI::NUIComponent {
public:
    TrackUIComponent(std::shared_ptr<Track> track);
    ~TrackUIComponent() override;

    std::shared_ptr<Track> getTrack() const { return m_track; }

protected:
    void onRender(NomadUI::NUIRenderer& renderer) override;
    void onResize(int width, int height) override;
    bool onMouseEvent(const NomadUI::NUIMouseEvent& event) override;
    void onMouseEnter();
    void onMouseLeave();
    void onUpdate(double deltaTime);

private:
    std::shared_ptr<Track> m_track;

    // UI Components
    std::shared_ptr<NomadUI::NUILabel> m_nameLabel;
    std::shared_ptr<NomadUI::NUIButton> m_muteButton;
    std::shared_ptr<NomadUI::NUIButton> m_soloButton;
    std::shared_ptr<NomadUI::NUIButton> m_recordButton;

    // UI callbacks
    void onVolumeChanged(float volume);
    void onPanChanged(float pan);
    void onMuteToggled();
    void onSoloToggled();
    void onRecordToggled();

    // Waveform rendering
    void drawWaveform(NomadUI::NUIRenderer& renderer, const NomadUI::NUIRect& bounds);

    // UI state
    void updateUI();
    void updateTrackNameColors(); // Update track name with bright colors based on number
};

} // namespace Audio
} // namespace Nomad
