#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QFont>
#include <QSize>
#include <QPoint>

namespace XXMLStudio {

/**
 * Settings manager with typed accessors for all IDE settings.
 * Wraps QSettings and provides signals for settings changes.
 */
class Settings : public QObject
{
    Q_OBJECT

public:
    explicit Settings(QObject* parent = nullptr);
    ~Settings();

    // Window state
    QSize windowSize() const;
    void setWindowSize(const QSize& size);
    QPoint windowPosition() const;
    void setWindowPosition(const QPoint& pos);
    bool windowMaximized() const;
    void setWindowMaximized(bool maximized);
    QByteArray windowState() const;
    void setWindowState(const QByteArray& state);
    QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray& geometry);

    // Recent files/projects
    QStringList recentProjects() const;
    void addRecentProject(const QString& path);
    void clearRecentProjects();
    QString lastOpenedProject() const;
    void setLastOpenedProject(const QString& path);

    // Editor settings
    QFont editorFont() const;
    void setEditorFont(const QFont& font);
    int editorFontSize() const;
    void setEditorFontSize(int size);
    int tabWidth() const;
    void setTabWidth(int width);
    bool useSpacesForTabs() const;
    void setUseSpacesForTabs(bool useSpaces);
    bool showLineNumbers() const;
    void setShowLineNumbers(bool show);
    bool highlightCurrentLine() const;
    void setHighlightCurrentLine(bool highlight);
    bool wordWrap() const;
    void setWordWrap(bool wrap);

    // Build settings
    QString activeConfiguration() const;
    void setActiveConfiguration(const QString& config);
    bool buildBeforeRun() const;
    void setBuildBeforeRun(bool build);
    bool saveBeforeBuild() const;
    void setSaveBeforeBuild(bool save);

    // Toolchain
    QString toolchainPath() const;
    void setToolchainPath(const QString& path);
    QString customCompilerPath() const;
    void setCustomCompilerPath(const QString& path);
    QString customLspServerPath() const;
    void setCustomLspServerPath(const QString& path);
    bool useCustomToolchain() const;
    void setUseCustomToolchain(bool use);

    // General
    bool restoreSessionOnStartup() const;
    void setRestoreSessionOnStartup(bool restore);
    bool promptToResumeProject() const;
    void setPromptToResumeProject(bool prompt);
    QString theme() const;
    void setTheme(const QString& theme);

    // Syntax highlighting theme (0=Darcula, 1=QtCreator, 2=VSCodeDark)
    int syntaxTheme() const;
    void setSyntaxTheme(int theme);

    // Sync to disk
    void sync();

signals:
    void editorFontChanged(const QFont& font);
    void tabWidthChanged(int width);
    void editorSettingsChanged();
    void themeChanged(const QString& theme);
    void syntaxThemeChanged(int theme);

private:
    QSettings m_settings;
    static constexpr int MAX_RECENT_PROJECTS = 10;

    // Default values
    static QFont defaultEditorFont();
};

} // namespace XXMLStudio

#endif // SETTINGS_H
