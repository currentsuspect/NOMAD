# Requirements Document

## Introduction

The current mixer implementation has several architectural and functional issues that need to be addressed. This spec focuses on fixing critical bugs and design flaws in the Mixer, MixerChannel, and related components to ensure proper audio routing, processing, and metering functionality.

## Requirements

### Requirement 1: Fix Audio Routing Architecture

**User Story:** As a developer, I want the mixer to properly route audio from sources through channels to outputs, so that audio processing works correctly.

#### Acceptance Criteria

1. WHEN audio is processed through the mixer THEN each channel SHALL receive audio from its designated source (not from the internal buffer)
2. WHEN multiple channels are active THEN the mixer SHALL sum their outputs correctly without double-processing
3. WHEN a channel processes audio THEN it SHALL NOT copy the entire internal buffer as input
4. IF a channel has no audio source THEN it SHALL process silence or its own generated audio

### Requirement 2: Fix Metering Implementation

**User Story:** As a user, I want accurate level meters on each channel, so that I can monitor audio levels properly.

#### Acceptance Criteria

1. WHEN MixerChannel::updateMetering is called THEN the method SHALL exist and compile
2. WHEN audio passes through a channel THEN peak and RMS levels SHALL be calculated correctly
3. WHEN no audio is present THEN meters SHALL decay naturally over time
4. WHEN sample rate changes THEN metering decay SHALL adjust accordingly

### Requirement 3: Fix Solo/Mute Logic

**User Story:** As a user, I want solo and mute buttons to work correctly, so that I can isolate or silence specific channels.

#### Acceptance Criteria

1. WHEN a channel is muted THEN it SHALL output silence regardless of solo state
2. WHEN any channel is soloed THEN only soloed channels SHALL be audible
3. WHEN solo is disabled on all channels THEN all unmuted channels SHALL be audible
4. WHEN handleSoloStateChanged is called THEN it SHALL NOT override user-set mute states permanently

### Requirement 4: Fix Gain and Pan Processing

**User Story:** As a user, I want gain and pan controls to work correctly, so that I can balance my mix.

#### Acceptance Criteria

1. WHEN processing mono audio THEN pan SHALL create proper stereo image using equal-power panning
2. WHEN processing stereo audio THEN pan SHALL adjust left/right balance correctly
3. WHEN gain is adjusted THEN smoothing SHALL prevent clicks and pops
4. WHEN processing multiple channels THEN gain smoothing SHALL work independently per channel

### Requirement 5: Fix Master Channel Processing

**User Story:** As a user, I want the master channel to process the final mix correctly, so that master effects and gain work properly.

#### Acceptance Criteria

1. WHEN master gain is applied THEN it SHALL be applied only once to the final mix
2. WHEN master channel processes audio THEN it SHALL process the summed output of all channels
3. WHEN master gain smoothing is used THEN it SHALL apply the same value to all samples in a buffer
4. IF master channel has effects THEN they SHALL process the complete mix

### Requirement 6: Fix Thread Safety

**User Story:** As a developer, I want thread-safe channel management, so that adding/removing channels doesn't cause crashes.

#### Acceptance Criteria

1. WHEN channels are added or removed THEN the operation SHALL be protected by a mutex
2. WHEN audio is processing THEN channel iteration SHALL be safe from concurrent modifications
3. WHEN getChannel is called THEN it SHALL safely return nullptr if index is invalid
4. WHEN the mixer is destroyed THEN all channels SHALL be safely cleaned up

### Requirement 7: Fix Effects Processing

**User Story:** As a user, I want effects to process audio correctly without unnecessary buffer copies, so that CPU usage is minimized.

#### Acceptance Criteria

1. WHEN effects process audio THEN they SHALL process in-place without unnecessary buffer copies
2. WHEN no effects are active THEN audio SHALL pass through without processing overhead
3. WHEN effects are bypassed THEN audio SHALL pass through unmodified
4. WHEN MIDI messages are present THEN they SHALL be properly handled by effects

### Requirement 8: Fix Audio Source Integration

**User Story:** As a developer, I want channels to receive audio from proper sources, so that the mixer can actually mix multiple audio streams.

#### Acceptance Criteria

1. WHEN a channel is created THEN it SHALL have a way to receive audio from a source
2. WHEN audio is routed to a channel THEN it SHALL process that specific audio stream
3. WHEN multiple sources are active THEN each SHALL route to its designated channel
4. IF no source is connected THEN the channel SHALL process silence
