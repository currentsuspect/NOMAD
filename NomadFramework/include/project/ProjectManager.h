/**
 * @file ProjectManager.h
 * @brief Project system with JSON/XML serialization and versioning
 * @author Nomad Framework Team
 */

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <functional>
#include <string>

namespace nomad::project
{
    /**
     * @enum ProjectFormat
     * @brief Project file format enumeration
     */
    enum class ProjectFormat
    {
        JSON,
        XML,
        Binary
    };
    
    /**
     * @struct ProjectInfo
     * @brief Project information structure
     */
    struct ProjectInfo
    {
        std::string name;
        std::string version;
        std::string author;
        std::string description;
        std::string createdDate;
        std::string modifiedDate;
        double sampleRate = 44100.0;
        int bufferSize = 512;
        double tempo = 120.0;
        double timeSignatureNumerator = 4.0;
        double timeSignatureDenominator = 4.0;
        double duration = 0.0;
        std::string filePath;
        bool isDirty = false;
    };
    
    /**
     * @struct ResourceInfo
     * @brief Resource information structure
     */
    struct ResourceInfo
    {
        std::string id;
        std::string name;
        std::string type;
        std::string filePath;
        std::string hash;
        size_t size = 0;
        bool isLoaded = false;
        std::string metadata;
    };
    
    /**
     * @class ProjectManager
     * @brief Centralized project management system
     * 
     * Handles project creation, loading, saving, versioning, resource management,
     * and provides autosave and crash recovery functionality.
     */
    class ProjectManager
    {
    public:
        /**
         * @brief Constructor
         */
        ProjectManager();
        
        /**
         * @brief Destructor
         */
        ~ProjectManager();
        
        /**
         * @brief Initialize the project manager
         * @return true if successful
         */
        bool initialize();
        
        /**
         * @brief Shutdown the project manager
         */
        void shutdown();
        
        /**
         * @brief Create new project
         * @param name Project name
         * @param sampleRate Sample rate
         * @param bufferSize Buffer size
         * @return true if successful
         */
        bool createNewProject(const std::string& name, double sampleRate = 44100.0, int bufferSize = 512);
        
        /**
         * @brief Load project from file
         * @param filePath Path to project file
         * @param format Project format
         * @return true if successful
         */
        bool loadProject(const std::string& filePath, ProjectFormat format = ProjectFormat::JSON);
        
        /**
         * @brief Save project to file
         * @param filePath Path to project file
         * @param format Project format
         * @return true if successful
         */
        bool saveProject(const std::string& filePath, ProjectFormat format = ProjectFormat::JSON);
        
        /**
         * @brief Save current project
         * @return true if successful
         */
        bool saveCurrentProject();
        
        /**
         * @brief Close current project
         * @return true if successful
         */
        bool closeProject();
        
        /**
         * @brief Check if project is open
         * @return true if project is open
         */
        bool isProjectOpen() const { return projectOpen.load(); }
        
        /**
         * @brief Check if project is dirty
         * @return true if project has unsaved changes
         */
        bool isProjectDirty() const { return currentProject.isDirty; }
        
        /**
         * @brief Mark project as dirty
         * @param dirty True to mark as dirty
         */
        void setProjectDirty(bool dirty = true);
        
        /**
         * @brief Get current project info
         * @return Project information
         */
        const ProjectInfo& getCurrentProjectInfo() const { return currentProject; }
        
        /**
         * @brief Set project info
         * @param info Project information
         */
        void setProjectInfo(const ProjectInfo& info);
        
        /**
         * @brief Get project file path
         * @return Project file path
         */
        std::string getProjectFilePath() const { return currentProject.filePath; }
        
        /**
         * @brief Set project file path
         * @param filePath Project file path
         */
        void setProjectFilePath(const std::string& filePath);
        
        /**
         * @brief Add resource to project
         * @param resource Resource information
         * @return true if successful
         */
        bool addResource(const ResourceInfo& resource);
        
        /**
         * @brief Remove resource from project
         * @param resourceId Resource ID
         * @return true if successful
         */
        bool removeResource(const std::string& resourceId);
        
        /**
         * @brief Get resource by ID
         * @param resourceId Resource ID
         * @return Pointer to resource info, nullptr if not found
         */
        ResourceInfo* getResource(const std::string& resourceId);
        
        /**
         * @brief Get all resources
         * @return Vector of resource info pointers
         */
        std::vector<ResourceInfo*> getResources();
        
        /**
         * @brief Get resources by type
         * @param type Resource type
         * @return Vector of resource info pointers
         */
        std::vector<ResourceInfo*> getResourcesByType(const std::string& type);
        
        /**
         * @brief Load resource
         * @param resourceId Resource ID
         * @return true if successful
         */
        bool loadResource(const std::string& resourceId);
        
        /**
         * @brief Unload resource
         * @param resourceId Resource ID
         * @return true if successful
         */
        bool unloadResource(const std::string& resourceId);
        
        /**
         * @brief Set autosave enabled
         * @param enabled True to enable autosave
         */
        void setAutosaveEnabled(bool enabled);
        
        /**
         * @brief Check if autosave is enabled
         * @return true if enabled
         */
        bool isAutosaveEnabled() const { return autosaveEnabled.load(); }
        
        /**
         * @brief Set autosave interval
         * @param intervalMs Interval in milliseconds
         */
        void setAutosaveInterval(int intervalMs);
        
        /**
         * @brief Get autosave interval
         * @return Interval in milliseconds
         */
        int getAutosaveInterval() const { return autosaveInterval.load(); }
        
        /**
         * @brief Enable/disable crash recovery
         * @param enabled True to enable crash recovery
         */
        void setCrashRecoveryEnabled(bool enabled);
        
        /**
         * @brief Check if crash recovery is enabled
         * @return true if enabled
         */
        bool isCrashRecoveryEnabled() const { return crashRecoveryEnabled.load(); }
        
        /**
         * @brief Create backup of current project
         * @return true if successful
         */
        bool createBackup();
        
        /**
         * @brief Restore from backup
         * @param backupPath Path to backup file
         * @return true if successful
         */
        bool restoreFromBackup(const std::string& backupPath);
        
        /**
         * @brief Get available backups
         * @return Vector of backup file paths
         */
        std::vector<std::string> getAvailableBackups() const;
        
        /**
         * @brief Export project to different format
         * @param filePath Output file path
         * @param format Target format
         * @return true if successful
         */
        bool exportProject(const std::string& filePath, ProjectFormat format);
        
        /**
         * @brief Import project from different format
         * @param filePath Input file path
         * @param format Source format
         * @return true if successful
         */
        bool importProject(const std::string& filePath, ProjectFormat format);
        
        /**
         * @brief Add project callback
         * @param callback Function to call on project events
         */
        void addProjectCallback(std::function<void(const std::string&, const std::string&)> callback);
        
        /**
         * @brief Remove project callback
         * @param callback Function to remove
         */
        void removeProjectCallback(std::function<void(const std::string&, const std::string&)> callback);
        
        /**
         * @brief Get project statistics
         * @return Statistics structure
         */
        struct ProjectStats
        {
            int totalResources = 0;
            int loadedResources = 0;
            size_t totalResourceSize = 0;
            int totalBackups = 0;
            double lastSaveTime = 0.0;
            double lastAutosaveTime = 0.0;
        };
        
        ProjectStats getProjectStats() const;
        
    private:
        // Project state
        ProjectInfo currentProject;
        std::atomic<bool> projectOpen{false};
        std::atomic<bool> projectDirty{false};
        
        // Resources
        std::unordered_map<std::string, ResourceInfo> resources;
        mutable std::mutex resourceMutex;
        
        // Autosave
        std::atomic<bool> autosaveEnabled{true};
        std::atomic<int> autosaveInterval{300000}; // 5 minutes
        std::atomic<double> lastAutosaveTime{0.0};
        
        // Crash recovery
        std::atomic<bool> crashRecoveryEnabled{true};
        std::string crashRecoveryPath;
        
        // Callbacks
        std::vector<std::function<void(const std::string&, const std::string&)>> projectCallbacks;
        std::mutex callbackMutex;
        
        // Statistics
        mutable std::atomic<int> totalResources{0};
        mutable std::atomic<int> loadedResources{0};
        mutable std::atomic<size_t> totalResourceSize{0};
        mutable std::atomic<int> totalBackups{0};
        mutable std::atomic<double> lastSaveTime{0.0};
        
        // Internal methods
        bool saveProjectToJson(const std::string& filePath);
        bool loadProjectFromJson(const std::string& filePath);
        bool saveProjectToXml(const std::string& filePath);
        bool loadProjectFromXml(const std::string& filePath);
        bool saveProjectToBinary(const std::string& filePath);
        bool loadProjectFromBinary(const std::string& filePath);
        
        void updateProjectModifiedDate();
        void notifyProjectCallback(const std::string& event, const std::string& data);
        std::string generateResourceHash(const std::string& filePath) const;
        bool validateProjectVersion(const std::string& version) const;
        void updateStatistics();
    };
}