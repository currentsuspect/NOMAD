// © 2025 Nomad Studios — All Rights Reserved. Licensed for personal & educational use only.
#include "ProjectSerializer.h"
#include "../NomadCore/include/NomadLog.h"
#include <filesystem>
#include <fstream>

using namespace Nomad;
using namespace Nomad::Audio;

namespace {
    constexpr int PROJECT_VERSION = 1;
}

/**
 * @brief Serialize project state (tempo, playhead, and non-system tracks) to a JSON file.
 *
 * The written JSON contains the project version, tempo, playhead position, and an array of tracks.
 * Each serialized track includes: name, color, volume, pan, mute, solo, start, file, sampleRate, and channels.
 *
 * @param path Filesystem path to write the JSON project file.
 * @param trackManager Track manager supplying tracks to serialize; if null the save will fail.
 * @param tempo Project tempo to store.
 * @param playheadSeconds Playhead position in seconds to store.
 * @return true if the project was successfully written to disk, `false` on failure (for example when
 *         `trackManager` is null or the output file could not be opened).
 */
bool ProjectSerializer::save(const std::string& path,
                             const std::shared_ptr<TrackManager>& trackManager,
                             double tempo,
                             double playheadSeconds) {
    if (!trackManager) return false;

    JSON root = JSON::object();
    root.set("version", JSON(static_cast<double>(PROJECT_VERSION)));
    root.set("tempo", JSON(tempo));
    root.set("playhead", JSON(playheadSeconds));

    JSON tracksJson = JSON::array();
    for (size_t i = 0; i < trackManager->getTrackCount(); ++i) {
        auto track = trackManager->getTrack(i);
        if (!track || track->isSystemTrack()) continue;

        JSON t = JSON::object();
        t.set("name", JSON(track->getName()));
        t.set("color", JSON(static_cast<double>(track->getColor())));
        t.set("volume", JSON(track->getVolume()));
        t.set("pan", JSON(track->getPan()));
        t.set("mute", JSON(track->isMuted()));
        t.set("solo", JSON(track->isSoloed()));
        t.set("start", JSON(track->getStartPositionInTimeline()));
        t.set("file", JSON(track->getSourcePath()));
        t.set("sampleRate", JSON(static_cast<double>(track->getSampleRate())));
        t.set("channels", JSON(static_cast<double>(track->getNumChannels())));

        tracksJson.push(t);
    }

    root.set("tracks", tracksJson);

    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        Log::error("Project save failed (cannot open file): " + path);
        return false;
    }
    out << root.toString(2);
    Log::info("Project saved to " + path);
    return true;
}

/**
 * @brief Load project data from a JSON file into the provided TrackManager.
 *
 * Reads and parses the JSON at the given path, applies project-level values (tempo and playhead)
 * and reconstructs tracks into the provided TrackManager. Existing tracks are cleared before
 * loading. Per-track properties applied include name, color, volume, pan, mute, solo, start,
 * and an optional audio file which will be loaded if present (failures to load individual files
 * are logged and do not stop the overall load).
 *
 * @param path Filesystem path to the project JSON file.
 * @param trackManager TrackManager instance to populate; must be non-null.
 * @return LoadResult `ok` is `true` when the project was successfully loaded and applied.
 *         On failure (`ok` is `false`) the result will be empty (e.g., when `trackManager` is null,
 *         the file is missing or unreadable, or the JSON root is invalid). `tempo` and `playhead`
 *         in the returned result reflect values read from the file (or their defaults when present).
 */
ProjectSerializer::LoadResult ProjectSerializer::load(const std::string& path,
                                                      const std::shared_ptr<TrackManager>& trackManager) {
    LoadResult result;
    if (!trackManager) return result;
    if (!std::filesystem::exists(path)) {
        Log::warning("Project file not found: " + path);
        return result;
    }

    std::ifstream in(path);
    if (!in) {
        Log::error("Project load failed (cannot open file): " + path);
        return result;
    }
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    JSON root = JSON::parse(contents);
    if (!root.isObject()) {
        Log::error("Project load failed (invalid JSON root)");
        return result;
    }

    int version = root.has("version") ? root["version"].asInt() : 0;
    if (version != PROJECT_VERSION) {
        Log::warning("Project version mismatch");
    }

    result.tempo = root.has("tempo") ? root["tempo"].asNumber() : 120.0;
    result.playhead = root.has("playhead") ? root["playhead"].asNumber() : 0.0;

    // Clear existing tracks
    trackManager->clearAllTracks();

    const JSON& tracksJson = root["tracks"];
    if (tracksJson.isArray()) {
        for (size_t i = 0; i < tracksJson.size(); ++i) {
            const JSON& t = tracksJson[i];
            if (!t.isObject()) continue;

            std::string name = t.has("name") ? t["name"].asString() : "Track";
            auto track = trackManager->addTrack(name);

            if (t.has("color")) track->setColor(static_cast<uint32_t>(t["color"].asNumber()));
            if (t.has("volume")) track->setVolume(static_cast<float>(t["volume"].asNumber()));
            if (t.has("pan")) track->setPan(static_cast<float>(t["pan"].asNumber()));
            if (t.has("mute")) track->setMute(t["mute"].asBool());
            if (t.has("solo")) track->setSolo(t["solo"].asBool());
            if (t.has("start")) track->setStartPositionInTimeline(t["start"].asNumber());

            std::string file = t.has("file") ? t["file"].asString() : "";
            if (!file.empty()) {
                if (!track->loadAudioFile(file)) {
                    Log::warning("Failed to load clip: " + file);
                }
            }
        }
    }

    result.ok = true;
    Log::info("Project loaded from " + path);
    return result;
}