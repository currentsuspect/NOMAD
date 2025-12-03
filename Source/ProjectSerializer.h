// © 2025 Nomad Studios — All Rights Reserved. Licensed for personal & educational use only.
#pragma once

#include "../NomadAudio/include/TrackManager.h"
#include "../NomadCore/include/NomadJSON.h"
#include <string>
#include <memory>

/**
 * Load result for ProjectSerializer::load.
 * Contains status and any tempo/playhead values loaded from a project file.
 *
 * @var ok True if the load operation succeeded, false otherwise.
 * @var tempo Tempo value loaded from the project; defaults to 120.0 if not present.
 * @var playhead Playhead position in seconds loaded from the project; defaults to 0.0 if not present.
 */
/**
 * Serialize and write project state to the specified file path.
 *
 * @param path Filesystem path where the project will be saved.
 * @param trackManager Track manager whose tracks and state will be persisted.
 * @param tempo Project tempo to save.
 * @param playheadSeconds Playhead position in seconds to save.
 * @return true if the project was saved successfully, false otherwise.
 */
/**
 * Load project state from the specified file into the provided track manager.
 *
 * @param path Filesystem path of the project file to load.
 * @param trackManager Track manager to populate with the loaded tracks and state.
 * @return LoadResult containing `ok` (success flag), and any loaded `tempo` and `playhead` values.
 */
class ProjectSerializer {
public:
    struct LoadResult {
        bool ok{false};
        double tempo{120.0};
        double playhead{0.0};
    };

    static bool save(const std::string& path,
                     const std::shared_ptr<Nomad::Audio::TrackManager>& trackManager,
                     double tempo,
                     double playheadSeconds);

    static LoadResult load(const std::string& path,
                           const std::shared_ptr<Nomad::Audio::TrackManager>& trackManager);
};