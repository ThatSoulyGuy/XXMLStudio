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
    QString tag;         // Tag or branch
    QString localPath;   // Resolved path in project's Library folder
    QString cachePath;   // Path in global cache (source for Library copy)
    QString commitHash;  // Locked commit hash
    QStringList dllFiles; // DLL filenames stored in .dlls/ subfolder
};

/**
 * Optimization level for builds.
 */
enum class OptimizationLevel {
    O0,     // No optimization (fastest compile, easiest debugging)
    O1,     // Basic optimization
    O2,     // Full optimization (recommended for release)
    O3,     // Aggressive optimization (may increase code size)
    Os      // Optimize for size
};

/**
 * Build configuration (Debug/Release).
 */
struct BuildConfiguration {
    QString name;           // "Debug" or "Release"
    QString outputDir;
    QStringList flags;      // Additional custom flags
    OptimizationLevel optimization = OptimizationLevel::O0;
    bool debugInfo = true;

    // Helper to get the compiler flag for the optimization level
    QString optimizationFlag() const {
        switch (optimization) {
            case OptimizationLevel::O0: return QString();  // No flag needed
            case OptimizationLevel::O1: return "-O1";
            case OptimizationLevel::O2: return "-O2";
            case OptimizationLevel::O3: return "-O3";
            case OptimizationLevel::Os: return "-Os";
        }
        return QString();
    }

    // Helper to get display string for optimization level
    static QString optimizationLevelToString(OptimizationLevel level) {
        switch (level) {
            case OptimizationLevel::O0: return "None (O0)";
            case OptimizationLevel::O1: return "Basic (O1)";
            case OptimizationLevel::O2: return "Full (O2)";
            case OptimizationLevel::O3: return "Aggressive (O3)";
            case OptimizationLevel::Os: return "Size (Os)";
        }
        return "None";
    }

    // Helper to parse optimization level from string
    static OptimizationLevel optimizationLevelFromString(const QString& str) {
        QString s = str.toLower().trimmed();
        if (s == "o1" || s == "1" || s == "basic") return OptimizationLevel::O1;
        if (s == "o2" || s == "2" || s == "full" || s == "true") return OptimizationLevel::O2;
        if (s == "o3" || s == "3" || s == "aggressive") return OptimizationLevel::O3;
        if (s == "os" || s == "s" || s == "size") return OptimizationLevel::Os;
        return OptimizationLevel::O0;  // Default: no optimization
    }
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
 * Optimization = O0       ; Options: O0, O1, O2, O3, Os
 * DebugInfo = true
 * OutputDir = build/debug
 *
 * [Build.Release]
 * Optimization = O2       ; Full optimization for release
 * DebugInfo = false
 * OutputDir = build/release
 *
 * [Build.ReleaseSize]
 * Optimization = Os       ; Size-optimized build
 * DebugInfo = false
 * OutputDir = build/release-small
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

    // Compilation entrypoint - compiler is invoked from this file's directory for #import resolution
    QString compilationEntryPoint() const { return m_compilationEntryPoint; }
    void setCompilationEntryPoint(const QString& path);

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
    QString m_compilationEntryPoint;
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
