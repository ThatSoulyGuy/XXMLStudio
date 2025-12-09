#include "ProjectManager.h"
#include "Project.h"

#include <QFileInfo>

namespace XXMLStudio {

ProjectManager::ProjectManager(QObject* parent)
    : QObject(parent)
{
}

ProjectManager::~ProjectManager()
{
    closeProject();
}

bool ProjectManager::openProject(const QString& path)
{
    // Close any existing project
    closeProject();

    // Create and load the project
    m_currentProject = new Project(this);

    connect(m_currentProject, &Project::modified,
            this, &ProjectManager::projectModified);

    if (!m_currentProject->load(path)) {
        emit error(QString("Failed to open project: %1").arg(path));
        delete m_currentProject;
        m_currentProject = nullptr;
        return false;
    }

    emit projectOpened(m_currentProject);
    return true;
}

bool ProjectManager::createProject(const QString& path, const QString& name)
{
    // Close any existing project
    closeProject();

    // Create new project
    m_currentProject = new Project(this);
    m_currentProject->setName(name);
    m_currentProject->setEntryPoint("src/Main.XXML");
    m_currentProject->addIncludePath("src");

    connect(m_currentProject, &Project::modified,
            this, &ProjectManager::projectModified);

    if (!m_currentProject->saveAs(path)) {
        emit error(QString("Failed to create project: %1").arg(path));
        delete m_currentProject;
        m_currentProject = nullptr;
        return false;
    }

    emit projectOpened(m_currentProject);
    return true;
}

bool ProjectManager::closeProject()
{
    if (!m_currentProject) {
        return true;
    }

    // Check for unsaved changes
    if (m_currentProject->isModified()) {
        // In a real implementation, prompt user to save
        // For now, just save automatically
        m_currentProject->save();
    }

    delete m_currentProject;
    m_currentProject = nullptr;

    emit projectClosed();
    return true;
}

bool ProjectManager::saveProject()
{
    if (!m_currentProject) {
        return false;
    }

    return m_currentProject->save();
}

QString ProjectManager::projectPath() const
{
    return m_currentProject ? m_currentProject->filePath() : QString();
}

QString ProjectManager::projectDir() const
{
    return m_currentProject ? m_currentProject->projectDir() : QString();
}

} // namespace XXMLStudio
