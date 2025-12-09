#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <QObject>
#include <QString>

namespace XXMLStudio {

class Project;

/**
 * Manages the currently open project.
 */
class ProjectManager : public QObject
{
    Q_OBJECT

public:
    explicit ProjectManager(QObject* parent = nullptr);
    ~ProjectManager();

    // Project lifecycle
    bool openProject(const QString& path);
    bool createProject(const QString& path, const QString& name);
    bool closeProject();
    bool saveProject();

    // Accessors
    Project* currentProject() const { return m_currentProject; }
    bool hasProject() const { return m_currentProject != nullptr; }
    QString projectPath() const;
    QString projectDir() const;

signals:
    void projectOpened(Project* project);
    void projectClosed();
    void projectModified();
    void error(const QString& message);

private:
    Project* m_currentProject = nullptr;
};

} // namespace XXMLStudio

#endif // PROJECTMANAGER_H
