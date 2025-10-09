/**
 * @file ProjectManager.cpp
 * @brief Project system implementation
 */

#include "project/ProjectManager.h"
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

namespace nomad::project
{
    ProjectManager::ProjectManager()
    {
        // Set default crash recovery path
        crashRecoveryPath = "crash_recovery/";
    }
    
    ProjectManager::~ProjectManager()
    {
        shutdown();
    }
    
    bool ProjectManager::initialize()
    {
        try
        {
            // Create crash recovery directory
            std::filesystem::create_directories(crashRecoveryPath);
            
            // Initialize project state
            projectOpen = false;
            projectDirty = false;
            
            return true;
        }
        catch (const std::exception& e)
        {
            return false;
        }
    }
    
    void ProjectManager::shutdown()
    {
        if (projectOpen.load())
        {
            closeProject();
        }
    }
    
    bool ProjectManager::createNewProject(const std::string& name, double sampleRate, int bufferSize)
    {
        try
        {
            // Close current project if open
            if (projectOpen.load())
            {
                closeProject();
            }
            
            // Create new project
            currentProject = ProjectInfo();
            currentProject.name = name;
            currentProject.version = "1.0.0";
            currentProject.sampleRate = sampleRate;
            currentProject.bufferSize = bufferSize;
            currentProject.tempo = 120.0;
            currentProject.timeSignatureNumerator = 4.0;
            currentProject.timeSignatureDenominator = 4.0;
            
            // Set creation date
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
            currentProject.createdDate = ss.str();
            currentProject.modifiedDate = currentProject.createdDate;
            
            projectOpen = true;
            projectDirty = false;
            
            notifyProjectCallback("project_created", name);
            return true;
        }
        catch (const std::exception& e)
        {
            return false;
        }
    }
    
    bool ProjectManager::loadProject(const std::string& filePath, ProjectFormat format)
    {
        try
        {
            // Close current project if open
            if (projectOpen.load())
            {
                closeProject();
            }
            
            bool success = false;
            switch (format)
            {
                case ProjectFormat::JSON:
                    success = loadProjectFromJson(filePath);
                    break;
                case ProjectFormat::XML:
                    success = loadProjectFromXml(filePath);
                    break;
                case ProjectFormat::Binary:
                    success = loadProjectFromBinary(filePath);
                    break;
            }
            
            if (success)
            {
                currentProject.filePath = filePath;
                projectOpen = true;
                projectDirty = false;
                notifyProjectCallback("project_loaded", filePath);
            }
            
            return success;
        }
        catch (const std::exception& e)
        {
            return false;
        }
    }
    
    bool ProjectManager::saveProject(const std::string& filePath, ProjectFormat format)
    {
        try
        {
            bool success = false;
            switch (format)
            {
                case ProjectFormat::JSON:
                    success = saveProjectToJson(filePath);
                    break;
                case ProjectFormat::XML:
                    success = saveProjectToXml(filePath);
                    break;
                case ProjectFormat::Binary:
                    success = saveProjectToBinary(filePath);
                    break;
            }
            
            if (success)
            {
                currentProject.filePath = filePath;
                updateProjectModifiedDate();
                projectDirty = false;
                lastSaveTime = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
                notifyProjectCallback("project_saved", filePath);
            }
            
            return success;
        }
        catch (const std::exception& e)
        {
            return false;
        }
    }
    
    bool ProjectManager::saveCurrentProject()
    {
        if (!projectOpen.load() || currentProject.filePath.empty())
            return false;
        
        return saveProject(currentProject.filePath, ProjectFormat::JSON);
    }
    
    bool ProjectManager::closeProject()
    {
        if (!projectOpen.load())
            return true;
        
        try
        {
            // Unload all resources
            for (auto& pair : resources)
            {
                pair.second.isLoaded = false;
            }
            
            // Clear project data
            currentProject = ProjectInfo();
            resources.clear();
            
            projectOpen = false;
            projectDirty = false;
            
            notifyProjectCallback("project_closed", "");
            return true;
        }
        catch (const std::exception& e)
        {
            return false;
        }
    }
    
    void ProjectManager::setProjectDirty(bool dirty)
    {
        projectDirty = dirty;
        if (dirty)
        {
            updateProjectModifiedDate();
        }
    }
    
    void ProjectManager::setProjectInfo(const ProjectInfo& info)
    {
        currentProject = info;
        setProjectDirty(true);
    }
    
    void ProjectManager::setProjectFilePath(const std::string& filePath)
    {
        currentProject.filePath = filePath;
    }
    
    bool ProjectManager::addResource(const ResourceInfo& resource)
    {
        std::lock_guard<std::mutex> lock(resourceMutex);
        
        resources[resource.id] = resource;
        updateStatistics();
        notifyProjectCallback("resource_added", resource.id);
        return true;
    }
    
    bool ProjectManager::removeResource(const std::string& resourceId)
    {
        std::lock_guard<std::mutex> lock(resourceMutex);
        
        auto it = resources.find(resourceId);
        if (it != resources.end())
        {
            resources.erase(it);
            updateStatistics();
            notifyProjectCallback("resource_removed", resourceId);
            return true;
        }
        
        return false;
    }
    
    ResourceInfo* ProjectManager::getResource(const std::string& resourceId)
    {
        std::lock_guard<std::mutex> lock(resourceMutex);
        
        auto it = resources.find(resourceId);
        if (it != resources.end())
        {
            return &it->second;
        }
        
        return nullptr;
    }
    
    std::vector<ResourceInfo*> ProjectManager::getResources()
    {
        std::lock_guard<std::mutex> lock(resourceMutex);
        
        std::vector<ResourceInfo*> resourceList;
        for (auto& pair : resources)
        {
            resourceList.push_back(&pair.second);
        }
        
        return resourceList;
    }
    
    std::vector<ResourceInfo*> ProjectManager::getResourcesByType(const std::string& type)
    {
        std::lock_guard<std::mutex> lock(resourceMutex);
        
        std::vector<ResourceInfo*> resourceList;
        for (auto& pair : resources)
        {
            if (pair.second.type == type)
            {
                resourceList.push_back(&pair.second);
            }
        }
        
        return resourceList;
    }
    
    bool ProjectManager::loadResource(const std::string& resourceId)
    {
        auto* resource = getResource(resourceId);
        if (resource)
        {
            resource->isLoaded = true;
            updateStatistics();
            notifyProjectCallback("resource_loaded", resourceId);
            return true;
        }
        return false;
    }
    
    bool ProjectManager::unloadResource(const std::string& resourceId)
    {
        auto* resource = getResource(resourceId);
        if (resource)
        {
            resource->isLoaded = false;
            updateStatistics();
            notifyProjectCallback("resource_unloaded", resourceId);
            return true;
        }
        return false;
    }
    
    void ProjectManager::setAutosaveEnabled(bool enabled)
    {
        autosaveEnabled = enabled;
    }
    
    void ProjectManager::setAutosaveInterval(int intervalMs)
    {
        autosaveInterval = intervalMs;
    }
    
    void ProjectManager::setCrashRecoveryEnabled(bool enabled)
    {
        crashRecoveryEnabled = enabled;
    }
    
    bool ProjectManager::createBackup()
    {
        if (!projectOpen.load())
            return false;
        
        try
        {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << "backup_" << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S") << ".json";
            
            std::string backupPath = crashRecoveryPath + ss.str();
            bool success = saveProject(backupPath, ProjectFormat::JSON);
            
            if (success)
            {
                totalBackups++;
                notifyProjectCallback("backup_created", backupPath);
            }
            
            return success;
        }
        catch (const std::exception& e)
        {
            return false;
        }
    }
    
    bool ProjectManager::restoreFromBackup(const std::string& backupPath)
    {
        return loadProject(backupPath, ProjectFormat::JSON);
    }
    
    std::vector<std::string> ProjectManager::getAvailableBackups() const
    {
        std::vector<std::string> backups;
        
        try
        {
            for (const auto& entry : std::filesystem::directory_iterator(crashRecoveryPath))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".json")
                {
                    backups.push_back(entry.path().string());
                }
            }
        }
        catch (const std::exception& e)
        {
            // Log error
        }
        
        return backups;
    }
    
    bool ProjectManager::exportProject(const std::string& filePath, ProjectFormat format)
    {
        return saveProject(filePath, format);
    }
    
    bool ProjectManager::importProject(const std::string& filePath, ProjectFormat format)
    {
        return loadProject(filePath, format);
    }
    
    void ProjectManager::addProjectCallback(std::function<void(const std::string&, const std::string&)> callback)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        projectCallbacks.push_back(callback);
    }
    
    void ProjectManager::removeProjectCallback(std::function<void(const std::string&, const std::string&)> callback)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        projectCallbacks.erase(
            std::remove(projectCallbacks.begin(), projectCallbacks.end(), callback),
            projectCallbacks.end()
        );
    }
    
    ProjectManager::ProjectStats ProjectManager::getProjectStats() const
    {
        ProjectStats stats;
        stats.totalResources = totalResources.load();
        stats.loadedResources = loadedResources.load();
        stats.totalResourceSize = totalResourceSize.load();
        stats.totalBackups = totalBackups.load();
        stats.lastSaveTime = lastSaveTime.load();
        stats.lastAutosaveTime = lastAutosaveTime.load();
        return stats;
    }
    
    bool ProjectManager::saveProjectToJson(const std::string& filePath)
    {
        try
        {
            juce::DynamicObject::Ptr json = new juce::DynamicObject();
            json->setProperty("name", currentProject.name);
            json->setProperty("version", currentProject.version);
            json->setProperty("author", currentProject.author);
            json->setProperty("description", currentProject.description);
            json->setProperty("createdDate", currentProject.createdDate);
            json->setProperty("modifiedDate", currentProject.modifiedDate);
            json->setProperty("sampleRate", currentProject.sampleRate);
            json->setProperty("bufferSize", currentProject.bufferSize);
            json->setProperty("tempo", currentProject.tempo);
            json->setProperty("timeSignatureNumerator", currentProject.timeSignatureNumerator);
            json->setProperty("timeSignatureDenominator", currentProject.timeSignatureDenominator);
            json->setProperty("duration", currentProject.duration);
            
            // Save resources
            juce::Array<juce::var> resourcesArray;
            for (const auto& pair : resources)
            {
                juce::DynamicObject::Ptr resourceJson = new juce::DynamicObject();
                resourceJson->setProperty("id", pair.second.id);
                resourceJson->setProperty("name", pair.second.name);
                resourceJson->setProperty("type", pair.second.type);
                resourceJson->setProperty("filePath", pair.second.filePath);
                resourceJson->setProperty("hash", pair.second.hash);
                resourceJson->setProperty("size", static_cast<int>(pair.second.size));
                resourceJson->setProperty("isLoaded", pair.second.isLoaded);
                resourceJson->setProperty("metadata", pair.second.metadata);
                resourcesArray.add(resourceJson.get());
            }
            json->setProperty("resources", resourcesArray);
            
            juce::String jsonString = juce::JSON::toString(juce::var(json.get()));
            
            std::ofstream file(filePath);
            if (file.is_open())
            {
                file << jsonString.toStdString();
                file.close();
                return true;
            }
        }
        catch (const std::exception& e)
        {
            // Log error
        }
        return false;
    }
    
    bool ProjectManager::loadProjectFromJson(const std::string& filePath)
    {
        try
        {
            std::ifstream file(filePath);
            if (!file.is_open())
                return false;
            
            std::string jsonString((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
            
            juce::var json = juce::JSON::parse(jsonString);
            if (json.isObject())
            {
                auto* obj = json.getDynamicObject();
                currentProject.name = obj->getProperty("name").toString().toStdString();
                currentProject.version = obj->getProperty("version").toString().toStdString();
                currentProject.author = obj->getProperty("author").toString().toStdString();
                currentProject.description = obj->getProperty("description").toString().toStdString();
                currentProject.createdDate = obj->getProperty("createdDate").toString().toStdString();
                currentProject.modifiedDate = obj->getProperty("modifiedDate").toString().toStdString();
                currentProject.sampleRate = obj->getProperty("sampleRate");
                currentProject.bufferSize = obj->getProperty("bufferSize");
                currentProject.tempo = obj->getProperty("tempo");
                currentProject.timeSignatureNumerator = obj->getProperty("timeSignatureNumerator");
                currentProject.timeSignatureDenominator = obj->getProperty("timeSignatureDenominator");
                currentProject.duration = obj->getProperty("duration");
                
                // Load resources
                juce::var resourcesVar = obj->getProperty("resources");
                if (resourcesVar.isArray())
                {
                    auto* resourcesArray = resourcesVar.getArray();
                    for (const auto& resourceVar : *resourcesArray)
                    {
                        if (resourceVar.isObject())
                        {
                            auto* resourceObj = resourceVar.getDynamicObject();
                            ResourceInfo resource;
                            resource.id = resourceObj->getProperty("id").toString().toStdString();
                            resource.name = resourceObj->getProperty("name").toString().toStdString();
                            resource.type = resourceObj->getProperty("type").toString().toStdString();
                            resource.filePath = resourceObj->getProperty("filePath").toString().toStdString();
                            resource.hash = resourceObj->getProperty("hash").toString().toStdString();
                            resource.size = resourceObj->getProperty("size");
                            resource.isLoaded = resourceObj->getProperty("isLoaded");
                            resource.metadata = resourceObj->getProperty("metadata").toString().toStdString();
                            resources[resource.id] = resource;
                        }
                    }
                }
                
                updateStatistics();
                return true;
            }
        }
        catch (const std::exception& e)
        {
            // Log error
        }
        return false;
    }
    
    bool ProjectManager::saveProjectToXml(const std::string& filePath)
    {
        try
        {
            auto* xml = new juce::XmlElement("Project");
            xml->setAttribute("name", currentProject.name);
            xml->setAttribute("version", currentProject.version);
            xml->setAttribute("author", currentProject.author);
            xml->setAttribute("description", currentProject.description);
            xml->setAttribute("createdDate", currentProject.createdDate);
            xml->setAttribute("modifiedDate", currentProject.modifiedDate);
            xml->setAttribute("sampleRate", currentProject.sampleRate);
            xml->setAttribute("bufferSize", currentProject.bufferSize);
            xml->setAttribute("tempo", currentProject.tempo);
            xml->setAttribute("timeSignatureNumerator", currentProject.timeSignatureNumerator);
            xml->setAttribute("timeSignatureDenominator", currentProject.timeSignatureDenominator);
            xml->setAttribute("duration", currentProject.duration);
            
            // Save resources
            auto* resourcesXml = new juce::XmlElement("Resources");
            for (const auto& pair : resources)
            {
                auto* resourceXml = new juce::XmlElement("Resource");
                resourceXml->setAttribute("id", pair.second.id);
                resourceXml->setAttribute("name", pair.second.name);
                resourceXml->setAttribute("type", pair.second.type);
                resourceXml->setAttribute("filePath", pair.second.filePath);
                resourceXml->setAttribute("hash", pair.second.hash);
                resourceXml->setAttribute("size", static_cast<int>(pair.second.size));
                resourceXml->setAttribute("isLoaded", pair.second.isLoaded);
                resourceXml->setAttribute("metadata", pair.second.metadata);
                resourcesXml->addChildElement(resourceXml);
            }
            xml->addChildElement(resourcesXml);
            
            return xml->writeToFile(juce::File(filePath), "");
        }
        catch (const std::exception& e)
        {
            return false;
        }
    }
    
    bool ProjectManager::loadProjectFromXml(const std::string& filePath)
    {
        try
        {
            auto xml = juce::XmlDocument::parse(juce::File(filePath));
            if (xml && xml->getTagName() == "Project")
            {
                currentProject.name = xml->getStringAttribute("name").toStdString();
                currentProject.version = xml->getStringAttribute("version").toStdString();
                currentProject.author = xml->getStringAttribute("author").toStdString();
                currentProject.description = xml->getStringAttribute("description").toStdString();
                currentProject.createdDate = xml->getStringAttribute("createdDate").toStdString();
                currentProject.modifiedDate = xml->getStringAttribute("modifiedDate").toStdString();
                currentProject.sampleRate = xml->getDoubleAttribute("sampleRate");
                currentProject.bufferSize = xml->getIntAttribute("bufferSize");
                currentProject.tempo = xml->getDoubleAttribute("tempo");
                currentProject.timeSignatureNumerator = xml->getDoubleAttribute("timeSignatureNumerator");
                currentProject.timeSignatureDenominator = xml->getDoubleAttribute("timeSignatureDenominator");
                currentProject.duration = xml->getDoubleAttribute("duration");
                
                // Load resources
                auto* resourcesXml = xml->getChildByName("Resources");
                if (resourcesXml)
                {
                    for (auto* resourceXml : resourcesXml->getChildIterator())
                    {
                        if (resourceXml->getTagName() == "Resource")
                        {
                            ResourceInfo resource;
                            resource.id = resourceXml->getStringAttribute("id").toStdString();
                            resource.name = resourceXml->getStringAttribute("name").toStdString();
                            resource.type = resourceXml->getStringAttribute("type").toStdString();
                            resource.filePath = resourceXml->getStringAttribute("filePath").toStdString();
                            resource.hash = resourceXml->getStringAttribute("hash").toStdString();
                            resource.size = resourceXml->getIntAttribute("size");
                            resource.isLoaded = resourceXml->getBoolAttribute("isLoaded");
                            resource.metadata = resourceXml->getStringAttribute("metadata").toStdString();
                            resources[resource.id] = resource;
                        }
                    }
                }
                
                updateStatistics();
                return true;
            }
        }
        catch (const std::exception& e)
        {
            // Log error
        }
        return false;
    }
    
    bool ProjectManager::saveProjectToBinary(const std::string& filePath)
    {
        // Binary format implementation would go here
        // For now, fall back to JSON
        return saveProjectToJson(filePath);
    }
    
    bool ProjectManager::loadProjectFromBinary(const std::string& filePath)
    {
        // Binary format implementation would go here
        // For now, fall back to JSON
        return loadProjectFromJson(filePath);
    }
    
    void ProjectManager::updateProjectModifiedDate()
    {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        currentProject.modifiedDate = ss.str();
    }
    
    void ProjectManager::notifyProjectCallback(const std::string& event, const std::string& data)
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        for (auto& callback : projectCallbacks)
        {
            callback(event, data);
        }
    }
    
    std::string ProjectManager::generateResourceHash(const std::string& filePath) const
    {
        // Simple hash implementation - in production, use a proper hash function
        std::hash<std::string> hasher;
        return std::to_string(hasher(filePath));
    }
    
    bool ProjectManager::validateProjectVersion(const std::string& version) const
    {
        // Simple version validation - in production, use proper semantic versioning
        return !version.empty() && version[0] >= '0' && version[0] <= '9';
    }
    
    void ProjectManager::updateStatistics()
    {
        int total = 0;
        int loaded = 0;
        size_t totalSize = 0;
        
        for (const auto& pair : resources)
        {
            total++;
            if (pair.second.isLoaded)
                loaded++;
            totalSize += pair.second.size;
        }
        
        totalResources = total;
        loadedResources = loaded;
        totalResourceSize = totalSize;
    }
}