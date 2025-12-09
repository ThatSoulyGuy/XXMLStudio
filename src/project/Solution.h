#ifndef SOLUTION_H
#define SOLUTION_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>

namespace XXMLStudio {

class Project;

/**
 * Reference to a project within a solution.
 */
struct ProjectReference {
    QString name;
    QString relativePath;  // Relative path to .xxmlp file
    bool isLoaded = false;
};

/**
 * Represents an XXML solution (.xxmls file).
 * A solution contains multiple projects.
 *
 * File format:
 *
 * [Solution]
 * Name = MySolution
 * Version = 1.0.0
 *
 * [Projects]
 * MyApp = MyApp/MyApp.xxmlp
 * MyLib = MyLib/MyLib.xxmlp
 *
 * [Settings]
 * StartupProject = MyApp
 * ActiveProject = MyApp
 */
class Solution : public QObject
{
    Q_OBJECT

public:
    explicit Solution(QObject* parent = nullptr);
    ~Solution();

    // Load/Save
    bool load(const QString& path);
    bool save();
    bool saveAs(const QString& path);

    // Solution info
    QString name() const { return m_name; }
    void setName(const QString& name);

    QString version() const { return m_version; }
    void setVersion(const QString& version);

    QString filePath() const { return m_filePath; }
    QString solutionDir() const;

    // Project management
    QList<ProjectReference> projectReferences() const { return m_projects; }
    void addProject(const QString& name, const QString& relativePath);
    void removeProject(const QString& name);
    bool hasProject(const QString& name) const;
    QString projectPath(const QString& name) const;  // Returns absolute path

    // Active/Startup project
    QString startupProject() const { return m_startupProject; }
    void setStartupProject(const QString& name);

    QString activeProject() const { return m_activeProject; }
    void setActiveProject(const QString& name);

    // State
    bool isModified() const { return m_modified; }

signals:
    void modified();
    void saved();
    void nameChanged(const QString& name);
    void projectsChanged();
    void activeProjectChanged(const QString& name);
    void startupProjectChanged(const QString& name);

private:
    bool parseFile(const QString& content);
    QString generateFile() const;

    QString m_name;
    QString m_version = "1.0.0";
    QString m_filePath;
    QList<ProjectReference> m_projects;
    QString m_startupProject;
    QString m_activeProject;
    bool m_modified = false;
};

} // namespace XXMLStudio

#endif // SOLUTION_H
