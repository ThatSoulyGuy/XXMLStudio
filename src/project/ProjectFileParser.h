#ifndef PROJECTFILEPARSER_H
#define PROJECTFILEPARSER_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVariant>

namespace XXMLStudio {

/**
 * Parser for XXML project (.xxmlp) and solution (.xxmls) files.
 *
 * File format is INI-like with sections:
 *
 * [SectionName]
 * Key = Value
 *
 * Or for lists:
 * [SectionName]
 * item1
 * item2
 */
class ProjectFileParser
{
public:
    struct Section {
        QString name;
        QMap<QString, QString> values;
        QStringList items;  // For list-only sections
    };

    ProjectFileParser();

    bool parse(const QString& filePath);
    bool parseString(const QString& content);

    QString errorString() const { return m_errorString; }
    int errorLine() const { return m_errorLine; }

    // Access parsed data
    QStringList sectionNames() const;
    bool hasSection(const QString& name) const;
    Section section(const QString& name) const;

    QString value(const QString& section, const QString& key, const QString& defaultValue = QString()) const;
    QStringList items(const QString& section) const;

    // Writing
    static bool write(const QString& filePath, const QList<Section>& sections);
    static QString serialize(const QList<Section>& sections);

private:
    QMap<QString, Section> m_sections;
    QString m_errorString;
    int m_errorLine = 0;

    void parseLine(const QString& line, int lineNumber, QString& currentSection);
};

} // namespace XXMLStudio

#endif // PROJECTFILEPARSER_H
