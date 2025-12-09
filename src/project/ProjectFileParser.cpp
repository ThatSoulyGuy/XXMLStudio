#include "ProjectFileParser.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

namespace XXMLStudio {

ProjectFileParser::ProjectFileParser()
{
}

bool ProjectFileParser::parse(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_errorString = QString("Cannot open file: %1").arg(file.errorString());
        return false;
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    return parseString(content);
}

bool ProjectFileParser::parseString(const QString& content)
{
    m_sections.clear();
    m_errorString.clear();
    m_errorLine = 0;

    QString currentSection;
    int lineNumber = 0;

    QStringList lines = content.split('\n');
    for (const QString& rawLine : lines) {
        lineNumber++;
        parseLine(rawLine, lineNumber, currentSection);
        if (!m_errorString.isEmpty()) {
            return false;
        }
    }

    return true;
}

void ProjectFileParser::parseLine(const QString& rawLine, int lineNumber, QString& currentSection)
{
    QString line = rawLine.trimmed();

    // Skip empty lines and comments
    if (line.isEmpty() || line.startsWith('#') || line.startsWith(';') || line.startsWith("//")) {
        return;
    }

    // Section header: [SectionName] or [Section.SubSection]
    static QRegularExpression sectionRegex(R"(^\[([^\]]+)\]$)");
    QRegularExpressionMatch sectionMatch = sectionRegex.match(line);
    if (sectionMatch.hasMatch()) {
        currentSection = sectionMatch.captured(1).trimmed();
        if (!m_sections.contains(currentSection)) {
            Section section;
            section.name = currentSection;
            m_sections[currentSection] = section;
        }
        return;
    }

    // Must be in a section
    if (currentSection.isEmpty()) {
        m_errorString = QString("Line %1: Content outside of section").arg(lineNumber);
        m_errorLine = lineNumber;
        return;
    }

    // Key = Value pair
    static QRegularExpression kvRegex(R"(^([^=]+)=(.*)$)");
    QRegularExpressionMatch kvMatch = kvRegex.match(line);
    if (kvMatch.hasMatch()) {
        QString key = kvMatch.captured(1).trimmed();
        QString value = kvMatch.captured(2).trimmed();

        // Remove quotes if present
        if ((value.startsWith('"') && value.endsWith('"')) ||
            (value.startsWith('\'') && value.endsWith('\''))) {
            value = value.mid(1, value.length() - 2);
        }

        m_sections[currentSection].values[key] = value;
        return;
    }

    // Plain item (for list sections)
    m_sections[currentSection].items.append(line);
}

QStringList ProjectFileParser::sectionNames() const
{
    return m_sections.keys();
}

bool ProjectFileParser::hasSection(const QString& name) const
{
    return m_sections.contains(name);
}

ProjectFileParser::Section ProjectFileParser::section(const QString& name) const
{
    return m_sections.value(name);
}

QString ProjectFileParser::value(const QString& section, const QString& key, const QString& defaultValue) const
{
    if (!m_sections.contains(section)) {
        return defaultValue;
    }
    return m_sections[section].values.value(key, defaultValue);
}

QStringList ProjectFileParser::items(const QString& section) const
{
    if (!m_sections.contains(section)) {
        return QStringList();
    }
    return m_sections[section].items;
}

bool ProjectFileParser::write(const QString& filePath, const QList<Section>& sections)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out << serialize(sections);
    file.close();

    return true;
}

QString ProjectFileParser::serialize(const QList<Section>& sections)
{
    QString result;
    QTextStream out(&result);

    bool first = true;
    for (const Section& section : sections) {
        if (!first) {
            out << "\n";
        }
        first = false;

        out << "[" << section.name << "]\n";

        // Write key-value pairs
        for (auto it = section.values.constBegin(); it != section.values.constEnd(); ++it) {
            QString value = it.value();
            // Quote values with spaces
            if (value.contains(' ') || value.contains('=')) {
                value = QString("\"%1\"").arg(value);
            }
            out << it.key() << " = " << value << "\n";
        }

        // Write list items
        for (const QString& item : section.items) {
            out << item << "\n";
        }
    }

    return result;
}

} // namespace XXMLStudio
