#include "Solution.h"
#include "ProjectFileParser.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>

namespace XXMLStudio {

Solution::Solution(QObject* parent)
    : QObject(parent)
{
}

Solution::~Solution()
{
}

QString Solution::solutionDir() const
{
    return QFileInfo(m_filePath).absolutePath();
}

bool Solution::load(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    m_filePath = path;

    if (!parseFile(content)) {
        return false;
    }

    m_modified = false;
    return true;
}

bool Solution::save()
{
    if (m_filePath.isEmpty()) {
        return false;
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << generateFile();
    file.close();

    m_modified = false;
    emit saved();
    return true;
}

bool Solution::saveAs(const QString& path)
{
    m_filePath = path;
    return save();
}

void Solution::setName(const QString& name)
{
    if (m_name != name) {
        m_name = name;
        m_modified = true;
        emit nameChanged(name);
        emit modified();
    }
}

void Solution::setVersion(const QString& version)
{
    if (m_version != version) {
        m_version = version;
        m_modified = true;
        emit modified();
    }
}

void Solution::addProject(const QString& name, const QString& relativePath)
{
    // Check if project already exists
    for (const auto& proj : m_projects) {
        if (proj.name == name) {
            return;
        }
    }

    ProjectReference ref;
    ref.name = name;
    ref.relativePath = relativePath;
    m_projects.append(ref);

    // Set as startup/active if first project
    if (m_projects.size() == 1) {
        m_startupProject = name;
        m_activeProject = name;
    }

    m_modified = true;
    emit projectsChanged();
    emit modified();
}

void Solution::removeProject(const QString& name)
{
    for (int i = 0; i < m_projects.size(); ++i) {
        if (m_projects[i].name == name) {
            m_projects.removeAt(i);
            m_modified = true;

            // Update startup/active if removed
            if (m_startupProject == name) {
                m_startupProject = m_projects.isEmpty() ? QString() : m_projects[0].name;
                emit startupProjectChanged(m_startupProject);
            }
            if (m_activeProject == name) {
                m_activeProject = m_projects.isEmpty() ? QString() : m_projects[0].name;
                emit activeProjectChanged(m_activeProject);
            }

            emit projectsChanged();
            emit modified();
            return;
        }
    }
}

bool Solution::hasProject(const QString& name) const
{
    for (const auto& proj : m_projects) {
        if (proj.name == name) {
            return true;
        }
    }
    return false;
}

QString Solution::projectPath(const QString& name) const
{
    for (const auto& proj : m_projects) {
        if (proj.name == name) {
            return QDir(solutionDir()).absoluteFilePath(proj.relativePath);
        }
    }
    return QString();
}

void Solution::setStartupProject(const QString& name)
{
    if (m_startupProject != name && hasProject(name)) {
        m_startupProject = name;
        m_modified = true;
        emit startupProjectChanged(name);
        emit modified();
    }
}

void Solution::setActiveProject(const QString& name)
{
    if (m_activeProject != name && hasProject(name)) {
        m_activeProject = name;
        emit activeProjectChanged(name);
    }
}

bool Solution::parseFile(const QString& content)
{
    ProjectFileParser parser;
    if (!parser.parseString(content)) {
        return false;
    }

    m_projects.clear();

    // [Solution] section
    if (parser.hasSection("Solution")) {
        m_name = parser.value("Solution", "Name", "Untitled");
        m_version = parser.value("Solution", "Version", "1.0.0");
    }

    // [Projects] section
    if (parser.hasSection("Projects")) {
        auto section = parser.section("Projects");
        for (auto it = section.values.constBegin(); it != section.values.constEnd(); ++it) {
            ProjectReference ref;
            ref.name = it.key();
            ref.relativePath = it.value();
            m_projects.append(ref);
        }
    }

    // [Settings] section
    if (parser.hasSection("Settings")) {
        m_startupProject = parser.value("Settings", "StartupProject");
        m_activeProject = parser.value("Settings", "ActiveProject");
    }

    // Default to first project if not specified
    if (!m_projects.isEmpty()) {
        if (m_startupProject.isEmpty() || !hasProject(m_startupProject)) {
            m_startupProject = m_projects[0].name;
        }
        if (m_activeProject.isEmpty() || !hasProject(m_activeProject)) {
            m_activeProject = m_projects[0].name;
        }
    }

    return true;
}

QString Solution::generateFile() const
{
    QList<ProjectFileParser::Section> sections;

    // [Solution] section
    ProjectFileParser::Section solutionSection;
    solutionSection.name = "Solution";
    solutionSection.values["Name"] = m_name;
    solutionSection.values["Version"] = m_version;
    sections.append(solutionSection);

    // [Projects] section
    if (!m_projects.isEmpty()) {
        ProjectFileParser::Section projectsSection;
        projectsSection.name = "Projects";
        for (const ProjectReference& ref : m_projects) {
            projectsSection.values[ref.name] = ref.relativePath;
        }
        sections.append(projectsSection);
    }

    // [Settings] section
    ProjectFileParser::Section settingsSection;
    settingsSection.name = "Settings";
    if (!m_startupProject.isEmpty()) {
        settingsSection.values["StartupProject"] = m_startupProject;
    }
    if (!m_activeProject.isEmpty()) {
        settingsSection.values["ActiveProject"] = m_activeProject;
    }
    sections.append(settingsSection);

    return ProjectFileParser::serialize(sections);
}

} // namespace XXMLStudio
