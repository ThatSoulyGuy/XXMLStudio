#include "Project.h"
#include "ProjectFileParser.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>

namespace XXMLStudio {

Project::Project(QObject* parent)
    : QObject(parent)
{
    // Default configurations
    BuildConfiguration debugConfig;
    debugConfig.name = "Debug";
    debugConfig.outputDir = "build/debug";
    debugConfig.debugInfo = true;
    debugConfig.optimization = OptimizationLevel::O0;  // No optimization for debugging
    m_configurations.append(debugConfig);

    BuildConfiguration releaseConfig;
    releaseConfig.name = "Release";
    releaseConfig.outputDir = "build/release";
    releaseConfig.debugInfo = false;
    releaseConfig.optimization = OptimizationLevel::O2;  // Full optimization for release
    m_configurations.append(releaseConfig);

    // Default run configuration
    RunConfiguration defaultRun;
    defaultRun.name = "Default";
    defaultRun.workingDir = ".";
    defaultRun.pauseOnExit = true;
    m_runConfigurations.append(defaultRun);
}

Project::~Project()
{
}

QString Project::projectDir() const
{
    return QFileInfo(m_filePath).absolutePath();
}

QString Project::lockFilePath() const
{
    return m_filePath + ".lock";
}

bool Project::load(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    m_filePath = path;

    if (!parseProjectFile(content)) {
        return false;
    }

    // Try to load lock file
    loadLockFile();

    m_modified = false;
    return true;
}

bool Project::save()
{
    if (m_filePath.isEmpty()) {
        return false;
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << generateProjectFile();
    file.close();

    m_modified = false;
    emit saved();
    return true;
}

bool Project::saveAs(const QString& path)
{
    m_filePath = path;
    return save();
}

bool Project::loadLockFile()
{
    ProjectFileParser parser;
    if (!parser.parse(lockFilePath())) {
        return false;
    }

    // Read locked dependency versions
    if (parser.hasSection("Dependencies")) {
        auto section = parser.section("Dependencies");
        for (auto it = section.values.constBegin(); it != section.values.constEnd(); ++it) {
            Dependency* dep = findDependency(it.key());
            if (dep) {
                dep->commitHash = it.value();
            }
        }
    }

    return true;
}

bool Project::saveLockFile()
{
    QList<ProjectFileParser::Section> sections;

    // Lock file header
    ProjectFileParser::Section headerSection;
    headerSection.name = "LockFile";
    headerSection.values["Version"] = "1";
    headerSection.values["Project"] = m_name;
    sections.append(headerSection);

    // Locked dependencies
    if (!m_dependencies.isEmpty()) {
        ProjectFileParser::Section depSection;
        depSection.name = "Dependencies";
        for (const Dependency& dep : m_dependencies) {
            if (!dep.commitHash.isEmpty()) {
                depSection.values[dep.name] = dep.commitHash;
            }
        }
        sections.append(depSection);
    }

    return ProjectFileParser::write(lockFilePath(), sections);
}

void Project::setName(const QString& name)
{
    if (m_name != name) {
        m_name = name;
        m_modified = true;
        emit nameChanged(name);
        emit modified();
    }
}

void Project::setVersion(const QString& version)
{
    if (m_version != version) {
        m_version = version;
        m_modified = true;
        emit modified();
    }
}

void Project::setType(Type type)
{
    if (m_type != type) {
        m_type = type;
        m_modified = true;
        emit modified();
    }
}

void Project::setEntryPoint(const QString& path)
{
    if (m_entryPoint != path) {
        m_entryPoint = path;
        m_modified = true;
        emit modified();
    }
}

void Project::setCompilationEntryPoint(const QString& path)
{
    if (m_compilationEntryPoint != path) {
        m_compilationEntryPoint = path;
        m_modified = true;
        emit modified();
    }
}

void Project::setOutputDir(const QString& dir)
{
    if (m_outputDir != dir) {
        m_outputDir = dir;
        m_modified = true;
        emit modified();
    }
}

void Project::setIncludePaths(const QStringList& paths)
{
    if (m_includePaths != paths) {
        m_includePaths = paths;
        m_modified = true;
        emit modified();
    }
}

void Project::addIncludePath(const QString& path)
{
    if (!m_includePaths.contains(path)) {
        m_includePaths.append(path);
        m_modified = true;
        emit modified();
    }
}

void Project::addDependency(const Dependency& dep)
{
    m_dependencies.append(dep);
    m_modified = true;
    emit dependenciesChanged();
    emit modified();
}

void Project::removeDependency(const QString& name)
{
    for (int i = 0; i < m_dependencies.size(); ++i) {
        if (m_dependencies[i].name == name) {
            m_dependencies.removeAt(i);
            m_modified = true;
            emit dependenciesChanged();
            emit modified();
            return;
        }
    }
}

Dependency* Project::findDependency(const QString& name)
{
    for (auto& dep : m_dependencies) {
        if (dep.name == name) {
            return &dep;
        }
    }
    return nullptr;
}

BuildConfiguration* Project::activeConfiguration()
{
    return configuration(m_activeConfigName);
}

BuildConfiguration* Project::configuration(const QString& name)
{
    for (auto& config : m_configurations) {
        if (config.name == name) {
            return &config;
        }
    }
    return m_configurations.isEmpty() ? nullptr : &m_configurations[0];
}

void Project::addConfiguration(const BuildConfiguration& config)
{
    m_configurations.append(config);
    m_modified = true;
    emit configurationsChanged();
    emit modified();
}

void Project::setActiveConfigurationName(const QString& name)
{
    if (m_activeConfigName != name) {
        m_activeConfigName = name;
        emit configurationsChanged();
    }
}

RunConfiguration* Project::activeRunConfiguration()
{
    for (auto& config : m_runConfigurations) {
        if (config.name == m_activeRunConfigName) {
            return &config;
        }
    }
    return m_runConfigurations.isEmpty() ? nullptr : &m_runConfigurations[0];
}

void Project::addRunConfiguration(const RunConfiguration& config)
{
    m_runConfigurations.append(config);
    m_modified = true;
    emit modified();
}

void Project::setActiveRunConfigurationName(const QString& name)
{
    m_activeRunConfigName = name;
}

bool Project::parseProjectFile(const QString& content)
{
    ProjectFileParser parser;
    if (!parser.parseString(content)) {
        return false;
    }

    // Clear existing configurations (will reload from file)
    m_configurations.clear();
    m_runConfigurations.clear();
    m_dependencies.clear();

    // [Project] section
    if (parser.hasSection("Project")) {
        m_name = parser.value("Project", "Name", "Untitled");
        m_version = parser.value("Project", "Version", "0.1.0");
        m_entryPoint = parser.value("Project", "EntryPoint");
        m_compilationEntryPoint = parser.value("Project", "CompilationEntryPoint");
        m_outputDir = parser.value("Project", "OutputDir", "build");
        m_activeConfigName = parser.value("Project", "ActiveConfig", "Debug");
        m_activeRunConfigName = parser.value("Project", "ActiveRunConfig", "Default");

        QString typeStr = parser.value("Project", "Type", "Executable");
        m_type = (typeStr.toLower() == "library") ? Type::Library : Type::Executable;
    }

    // [IncludePaths] section
    m_includePaths = parser.items("IncludePaths");

    // [Dependencies] section
    if (parser.hasSection("Dependencies")) {
        auto section = parser.section("Dependencies");
        for (auto it = section.values.constBegin(); it != section.values.constEnd(); ++it) {
            Dependency dep;
            dep.name = it.key();

            // Parse "github.com/user/repo@v1.0.0" format
            QString value = it.value();
            int atPos = value.indexOf('@');
            if (atPos != -1) {
                dep.gitUrl = "https://" + value.left(atPos);
                dep.tag = value.mid(atPos + 1);
            } else {
                dep.gitUrl = "https://" + value;
            }
            m_dependencies.append(dep);
        }
    }

    // [Build.*] sections
    for (const QString& sectionName : parser.sectionNames()) {
        if (sectionName.startsWith("Build.")) {
            QString configName = sectionName.mid(6);
            BuildConfiguration config;
            config.name = configName;
            config.outputDir = parser.value(sectionName, "OutputDir", "build/" + configName.toLower());

            // Parse optimization level - supports both old "Optimize" boolean and new "Optimization" level
            QString optValue = parser.value(sectionName, "Optimization");
            if (!optValue.isEmpty()) {
                config.optimization = BuildConfiguration::optimizationLevelFromString(optValue);
            } else {
                // Fallback to old "Optimize" boolean for backwards compatibility
                QString oldOptimize = parser.value(sectionName, "Optimize", "false");
                config.optimization = (oldOptimize.toLower() == "true")
                    ? OptimizationLevel::O2
                    : OptimizationLevel::O0;
            }

            config.debugInfo = parser.value(sectionName, "DebugInfo", "true").toLower() == "true";

            QString flags = parser.value(sectionName, "Flags");
            if (!flags.isEmpty()) {
                config.flags = flags.split(' ', Qt::SkipEmptyParts);
            }

            m_configurations.append(config);
        }
    }

    // Add default configurations if none defined
    if (m_configurations.isEmpty()) {
        BuildConfiguration debugConfig;
        debugConfig.name = "Debug";
        debugConfig.outputDir = "build/debug";
        debugConfig.debugInfo = true;
        debugConfig.optimization = OptimizationLevel::O0;
        m_configurations.append(debugConfig);

        BuildConfiguration releaseConfig;
        releaseConfig.name = "Release";
        releaseConfig.outputDir = "build/release";
        releaseConfig.debugInfo = false;
        releaseConfig.optimization = OptimizationLevel::O2;
        m_configurations.append(releaseConfig);
    }

    // [Run.*] sections
    for (const QString& sectionName : parser.sectionNames()) {
        if (sectionName.startsWith("Run.")) {
            QString configName = sectionName.mid(4);
            RunConfiguration config;
            config.name = configName;
            config.executable = parser.value(sectionName, "Executable");
            config.workingDir = parser.value(sectionName, "WorkingDir", ".");
            config.pauseOnExit = parser.value(sectionName, "PauseOnExit", "true").toLower() == "true";

            QString args = parser.value(sectionName, "Arguments");
            if (!args.isEmpty()) {
                config.arguments = args.split(' ', Qt::SkipEmptyParts);
            }

            m_runConfigurations.append(config);
        }
    }

    // Add default run configuration if none defined
    if (m_runConfigurations.isEmpty()) {
        RunConfiguration defaultRun;
        defaultRun.name = "Default";
        defaultRun.workingDir = ".";
        defaultRun.pauseOnExit = true;
        m_runConfigurations.append(defaultRun);
    }

    return true;
}

QString Project::generateProjectFile() const
{
    QList<ProjectFileParser::Section> sections;

    // [Project] section
    ProjectFileParser::Section projectSection;
    projectSection.name = "Project";
    projectSection.values["Name"] = m_name;
    projectSection.values["Version"] = m_version;
    projectSection.values["Type"] = (m_type == Type::Library) ? "Library" : "Executable";
    if (!m_entryPoint.isEmpty()) {
        projectSection.values["EntryPoint"] = m_entryPoint;
    }
    if (!m_compilationEntryPoint.isEmpty()) {
        projectSection.values["CompilationEntryPoint"] = m_compilationEntryPoint;
    }
    projectSection.values["OutputDir"] = m_outputDir;
    projectSection.values["ActiveConfig"] = m_activeConfigName;
    projectSection.values["ActiveRunConfig"] = m_activeRunConfigName;
    sections.append(projectSection);

    // [IncludePaths] section
    if (!m_includePaths.isEmpty()) {
        ProjectFileParser::Section includeSection;
        includeSection.name = "IncludePaths";
        includeSection.items = m_includePaths;
        sections.append(includeSection);
    }

    // [Dependencies] section
    if (!m_dependencies.isEmpty()) {
        ProjectFileParser::Section depSection;
        depSection.name = "Dependencies";
        for (const Dependency& dep : m_dependencies) {
            QString gitPath = dep.gitUrl;
            if (gitPath.startsWith("https://")) {
                gitPath = gitPath.mid(8);
            }
            if (!dep.tag.isEmpty()) {
                depSection.values[dep.name] = gitPath + "@" + dep.tag;
            } else {
                depSection.values[dep.name] = gitPath;
            }
        }
        sections.append(depSection);
    }

    // [Build.*] sections
    for (const BuildConfiguration& config : m_configurations) {
        ProjectFileParser::Section buildSection;
        buildSection.name = "Build." + config.name;
        buildSection.values["OutputDir"] = config.outputDir;

        // Serialize optimization level
        switch (config.optimization) {
            case OptimizationLevel::O0: buildSection.values["Optimization"] = "O0"; break;
            case OptimizationLevel::O1: buildSection.values["Optimization"] = "O1"; break;
            case OptimizationLevel::O2: buildSection.values["Optimization"] = "O2"; break;
            case OptimizationLevel::O3: buildSection.values["Optimization"] = "O3"; break;
            case OptimizationLevel::Os: buildSection.values["Optimization"] = "Os"; break;
        }

        buildSection.values["DebugInfo"] = config.debugInfo ? "true" : "false";
        if (!config.flags.isEmpty()) {
            buildSection.values["Flags"] = config.flags.join(' ');
        }
        sections.append(buildSection);
    }

    // [Run.*] sections
    for (const RunConfiguration& config : m_runConfigurations) {
        ProjectFileParser::Section runSection;
        runSection.name = "Run." + config.name;
        if (!config.executable.isEmpty()) {
            runSection.values["Executable"] = config.executable;
        }
        if (!config.arguments.isEmpty()) {
            runSection.values["Arguments"] = config.arguments.join(' ');
        }
        runSection.values["WorkingDir"] = config.workingDir;
        runSection.values["PauseOnExit"] = config.pauseOnExit ? "true" : "false";
        sections.append(runSection);
    }

    return ProjectFileParser::serialize(sections);
}

} // namespace XXMLStudio
