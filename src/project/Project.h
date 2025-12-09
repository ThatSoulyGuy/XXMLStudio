#ifndef PROJECT_H
#define PROJECT_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QMap>

namespace XXMLStudio {

/**
 * Represents a dependency in the project.
 */
struct Dependency {
    QString name;
    QString gitUrl;
    QString tag;        // Tag or branch
    QString localPath;  // Resolved local path after fetching
    QString commitHash; // Locked commit hash
};

/**
 * Build configuration (Debug/Release).
 */
struct BuildConfiguration {
    QString name;           // "Debug" or "Release"
    QString outputDir;
    QStringList flags;
    bool optimize = false;
    bool debugInfo = true;
};

/**
 * Run configuration for executing the project.
 */
struct RunConfiguration {
    QString name;
    QString executable;     // Path to executable (relative to project)
    QStringList arguments;
    QString workingDir;
    QMap<QString, QString> environment;
    bool pauseOnExit = true;
};

/**
 * Represents an XXML project (.xxmlp file).
 *
 * File format:
 *
 * [Project]
 * Name = MyProject
 * Version = 1.0.0
 * Type = Executable
 * EntryPoint = src/Main.XXML
 * OutputDir = build
 *
 * [IncludePaths]
 * src
 * lib
 *
 * [Dependencies]
 * json-lib = github.com/xxml/json@v1.2.0
 *
 * [Build.Debug]
 * Optimize = false
 * DebugInfo = true
 * OutputDir = build/debug
 *
 * [Build.Release]
 * Optimize = true
 * DebugInfo = false
 * OutputDir = build/release
 *
 * [Run.Default]
 * Arguments = --verbose
 * WorkingDir = .
 * PauseOnExit = true
 */
class Project : public QObject
{
    Q_OBJECT

public:
    enum class Type {
        Executable,
        Library
    };

    explicit Project(QObject* parent = nullptr);
    ~Project();

    // Load/Save
    bool load(const QString& path);
    bool save();
    bool saveAs(const QString& path);

    // Lock file
    bool loadLockFile();
    bool saveLockFile();

    // Project info
    QString name() const { return m_name; }
    void setName(const QString& name);

    QString version() const { return m_version; }
    void setVersion(const QString& version);

    QString filePath() const { return m_filePath; }
    QString projectDir() const;
    QString lockFilePath() const;

    Type type() const { return m_type; }
    void setType(Type type);

    // Build settings
    QString entryPoint() const { return m_entryPoint; }
    void setEntryPoint(const QString& path);

    QString outputDir() const { return m_outputDir; }
    void setOutputDir(const QString& dir);

    QStringList includePaths() const { return m_includePaths; }
    void setIncludePaths(const QStringList& paths);
    void addIncludePath(const QString& path);

    // Dependencies
    QList<Dependency> dependencies() const { return m_dependencies; }
    void addDependency(const Dependency& dep);
    void removeDependency(const QString& name);
    Dependency* findDependency(const QString& name);

    // Build configurations
    QList<BuildConfiguration> configurations() const { return m_configurations; }
    BuildConfiguration* activeConfiguration();
    BuildConfiguration* configuration(const QString& name);
    void addConfiguration(const BuildConfiguration& config);
    void setActiveConfigurationName(const QString& name);
    QString activeConfigurationName() const { return m_activeConfigName; }

    // Run configurations
    QList<RunConfiguration> runConfigurations() const { return m_runConfigurations; }
    RunConfiguration* activeRunConfiguration();
    void addRunConfiguration(const RunConfiguration& config);
    void setActiveRunConfigurationName(const QString& name);
    QString activeRunConfigurationName() const { return m_activeRunConfigName; }

    // State
    bool isModified() const { return m_modified; }

signals:
    void modified();
    void saved();
    void nameChanged(const QString& name);
    void dependenciesChanged();
    void configurationsChanged();

private:
    bool parseProjectFile(const QString& content);
    QString generateProjectFile() const;

    QString m_name;
    QString m_version = "0.1.0";
    QString m_filePath;
    Type m_type = Type::Executable;
    QString m_entryPoint;
    QString m_outputDir = "build";
    QStringList m_includePaths;
    QList<Dependency> m_dependencies;
    QList<BuildConfiguration> m_configurations;
    QList<RunConfiguration> m_runConfigurations;
    QString m_activeConfigName = "Debug";
    QString m_activeRunConfigName = "Default";
    bool m_modified = false;
};

} // namespace XXMLStudio

#endif // PROJECT_H
