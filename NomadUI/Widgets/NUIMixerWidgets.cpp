#include "NUIMixerWidgets.h"

#include "../Graphics/NUIRenderer.h"
#include <algorithm>

namespace NomadUI {

Fader::Fader()
{
    setOrientation(NUISliderOrientation::Vertical);
}

PanKnob::PanKnob()
{
    setOrientation(NUISliderOrientation::Rotary);
    setRange(-1.0f, 1.0f);
    setValue(0.0f);
}

TrackLabel::TrackLabel()
    : text_("Track"), color_(NUIColor::fromHex(0xff6633ff))
{
}

void TrackLabel::onRender(NUIRenderer& renderer)
{
    (void)renderer;
}

void TrackLabel::setText(const std::string& text)
{
    text_ = text;
    repaint();
}

void TrackLabel::setColor(const NUIColor& color)
{
    color_ = color;
    repaint();
}

MuteButton::MuteButton()
{
    setOn(false);
}

SoloButton::SoloButton()
{
    setOn(false);
}

ArmButton::ArmButton()
{
    setOn(false);
}

InsertSlot::InsertSlot() = default;

void InsertSlot::onRender(NUIRenderer& renderer)
{
    (void)renderer;
}

bool InsertSlot::onMouseEvent(const NUIMouseEvent& event)
{
    if (event.type == NUIMouseEventType::ButtonDown)
    {
        if (onActivate_)
            onActivate_();
        return true;
    }
    return false;
}

void InsertSlot::setPluginName(const std::string& name)
{
    pluginName_ = name;
    repaint();
}

void InsertSlot::setOnActivate(std::function<void()> callback)
{
    onActivate_ = std::move(callback);
}

SendSlot::SendSlot()
    : amount_(0.0f)
{
}

void SendSlot::onRender(NUIRenderer& renderer)
{
    (void)renderer;
}

void SendSlot::setAmount(float amount)
{
    amount_ = std::clamp(amount, 0.0f, 1.0f);
    repaint();
}

MeterStrip::MeterStrip()
{
    setChannelCount(2);
}

ChannelStrip::ChannelStrip()
{
    fader_ = std::make_shared<Fader>();
    panKnob_ = std::make_shared<PanKnob>();
    trackLabel_ = std::make_shared<TrackLabel>();
    muteButton_ = std::make_shared<MuteButton>();
    soloButton_ = std::make_shared<SoloButton>();
    armButton_ = std::make_shared<ArmButton>();
    meterStrip_ = std::make_shared<MeterStrip>();

    addChild(fader_);
    addChild(panKnob_);
    addChild(trackLabel_);
    addChild(muteButton_);
    addChild(soloButton_);
    addChild(armButton_);
    addChild(meterStrip_);
}

void ChannelStrip::onRender(NUIRenderer& renderer)
{
    (void)renderer;
}

void ChannelStrip::addInsert()
{
    auto slot = std::make_shared<InsertSlot>();
    inserts_.push_back(slot);
    addChild(slot);
}

void ChannelStrip::addSend()
{
    auto slot = std::make_shared<SendSlot>();
    sends_.push_back(slot);
    addChild(slot);
}

MixerPanel::MixerPanel() = default;

void MixerPanel::onRender(NUIRenderer& renderer)
{
    (void)renderer;
}

void MixerPanel::addChannelStrip(std::shared_ptr<ChannelStrip> strip)
{
    if (!strip)
        return;
    channels_.push_back(strip);
    addChild(strip);
}

} // namespace NomadUI

