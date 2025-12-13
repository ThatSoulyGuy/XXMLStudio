#include "Settings.h"
#include <QFontDatabase>

namespace XXMLStudio {

Settings::Settings(QObject* parent)
    : QObject(parent)
    , m_settings(QSettings::IniFormat, QSettings::UserScope, "XXML", "XXMLStudio")
{
}

Settings::~Settings()
{
    sync();
}

// =============================================================================
// Window state
// =============================================================================

QSize Settings::windowSize() const
{
    return m_settings.value("Window/size", QSize(1280, 800)).toSize();
}

void Settings::setWindowSize(const QSize& size)
{
    m_settings.setValue("Window/size", size);
}

QPoint Settings::windowPosition() const
{
    return m_settings.value("Window/position", QPoint(100, 100)).toPoint();
}

void Settings::setWindowPosition(const QPoint& pos)
{
    m_settings.setValue("Window/position", pos);
}

bool Settings::windowMaximized() const
{
    return m_settings.value("Window/maximized", false).toBool();
}

void Settings::setWindowMaximized(bool maximized)
{
    m_settings.setValue("Window/maximized", maximized);
}

QByteArray Settings::windowState() const
{
    return m_settings.value("Window/state").toByteArray();
}

void Settings::setWindowState(const QByteArray& state)
{
    m_settings.setValue("Window/state", state);
}

QByteArray Settings::windowGeometry() const
{
    return m_settings.value("Window/geometry").toByteArray();
}

void Settings::setWindowGeometry(const QByteArray& geometry)
{
    m_settings.setValue("Window/geometry", geometry);
}

// =============================================================================
// Recent files/projects
// =============================================================================

QStringList Settings::recentProjects() const
{
    return m_settings.value("RecentProjects/list").toStringList();
}

void Settings::addRecentProject(const QString& path)
{
    QStringList recent = recentProjects();
    recent.removeAll(path);
    recent.prepend(path);
    while (recent.size() > MAX_RECENT_PROJECTS) {
        recent.removeLast();
    }
    m_settings.setValue("RecentProjects/list", recent);
}

void Settings::clearRecentProjects()
{
    m_settings.setValue("RecentProjects/list", QStringList());
}

QString Settings::lastOpenedProject() const
{
    return m_settings.value("RecentProjects/last").toString();
}

void Settings::setLastOpenedProject(const QString& path)
{
    m_settings.setValue("RecentProjects/last", path);
}

// =============================================================================
// Editor settings
// =============================================================================

QFont Settings::defaultEditorFont()
{
    QFont font;
#ifdef Q_OS_WIN
    // Try modern fonts first, with good fallbacks
    if (QFontDatabase::hasFamily("JetBrains Mono")) {
        font = QFont("JetBrains Mono", 11);
    } else if (QFontDatabase::hasFamily("Cascadia Code")) {
        font = QFont("Cascadia Code", 11);
    } else if (QFontDatabase::hasFamily("Fira Code")) {
        font = QFont("Fira Code", 11);
    } else {
        font = QFont("Consolas", 11);
    }
#elif defined(Q_OS_MAC)
    if (QFontDatabase::hasFamily("JetBrains Mono")) {
        font = QFont("JetBrains Mono", 13);
    } else if (QFontDatabase::hasFamily("SF Mono")) {
        font = QFont("SF Mono", 13);
    } else {
        font = QFont("Menlo", 13);
    }
#else
    if (QFontDatabase::hasFamily("JetBrains Mono")) {
        font = QFont("JetBrains Mono", 11);
    } else if (QFontDatabase::hasFamily("Fira Code")) {
        font = QFont("Fira Code", 11);
    } else if (QFontDatabase::hasFamily("Source Code Pro")) {
        font = QFont("Source Code Pro", 11);
    } else {
        font = QFont("Monospace", 11);
    }
#endif
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    return font;
}

QFont Settings::editorFont() const
{
    QFont defaultFont = defaultEditorFont();
    QString family = m_settings.value("Editor/fontFamily", defaultFont.family()).toString();
    int size = m_settings.value("Editor/fontSize", defaultFont.pointSize()).toInt();
    QFont font(family, size);
    font.setStyleHint(QFont::Monospace);
    return font;
}

void Settings::setEditorFont(const QFont& font)
{
    m_settings.setValue("Editor/fontFamily", font.family());
    m_settings.setValue("Editor/fontSize", font.pointSize());
    emit editorFontChanged(font);
    emit editorSettingsChanged();
}

int Settings::editorFontSize() const
{
    return m_settings.value("Editor/fontSize", defaultEditorFont().pointSize()).toInt();
}

void Settings::setEditorFontSize(int size)
{
    m_settings.setValue("Editor/fontSize", size);
    emit editorFontChanged(editorFont());
    emit editorSettingsChanged();
}

int Settings::tabWidth() const
{
    return m_settings.value("Editor/tabWidth", 4).toInt();
}

void Settings::setTabWidth(int width)
{
    m_settings.setValue("Editor/tabWidth", width);
    emit tabWidthChanged(width);
    emit editorSettingsChanged();
}

bool Settings::useSpacesForTabs() const
{
    return m_settings.value("Editor/useSpaces", true).toBool();
}

void Settings::setUseSpacesForTabs(bool useSpaces)
{
    m_settings.setValue("Editor/useSpaces", useSpaces);
    emit editorSettingsChanged();
}

bool Settings::showLineNumbers() const
{
    return m_settings.value("Editor/showLineNumbers", true).toBool();
}

void Settings::setShowLineNumbers(bool show)
{
    m_settings.setValue("Editor/showLineNumbers", show);
    emit editorSettingsChanged();
}

bool Settings::highlightCurrentLine() const
{
    return m_settings.value("Editor/highlightCurrentLine", true).toBool();
}

void Settings::setHighlightCurrentLine(bool highlight)
{
    m_settings.setValue("Editor/highlightCurrentLine", highlight);
    emit editorSettingsChanged();
}

bool Settings::wordWrap() const
{
    return m_settings.value("Editor/wordWrap", false).toBool();
}

void Settings::setWordWrap(bool wrap)
{
    m_settings.setValue("Editor/wordWrap", wrap);
    emit editorSettingsChanged();
}

// =============================================================================
// Build settings
// =============================================================================

QString Settings::activeConfiguration() const
{
    return m_settings.value("Build/activeConfiguration", "Debug").toString();
}

void Settings::setActiveConfiguration(const QString& config)
{
    m_settings.setValue("Build/activeConfiguration", config);
}

bool Settings::buildBeforeRun() const
{
    return m_settings.value("Build/buildBeforeRun", true).toBool();
}

void Settings::setBuildBeforeRun(bool build)
{
    m_settings.setValue("Build/buildBeforeRun", build);
}

bool Settings::saveBeforeBuild() const
{
    return m_settings.value("Build/saveBeforeBuild", true).toBool();
}

void Settings::setSaveBeforeBuild(bool save)
{
    m_settings.setValue("Build/saveBeforeBuild", save);
}

// =============================================================================
// Toolchain
// =============================================================================

QString Settings::toolchainPath() const
{
    return m_settings.value("Toolchain/path").toString();
}

void Settings::setToolchainPath(const QString& path)
{
    m_settings.setValue("Toolchain/path", path);
}

QString Settings::customCompilerPath() const
{
    return m_settings.value("Toolchain/customCompilerPath").toString();
}

void Settings::setCustomCompilerPath(const QString& path)
{
    m_settings.setValue("Toolchain/customCompilerPath", path);
}

QString Settings::customLspServerPath() const
{
    return m_settings.value("Toolchain/customLspServerPath").toString();
}

void Settings::setCustomLspServerPath(const QString& path)
{
    m_settings.setValue("Toolchain/customLspServerPath", path);
}

bool Settings::useCustomToolchain() const
{
    return m_settings.value("Toolchain/useCustom", false).toBool();
}

void Settings::setUseCustomToolchain(bool use)
{
    m_settings.setValue("Toolchain/useCustom", use);
}

// =============================================================================
// General
// =============================================================================

bool Settings::restoreSessionOnStartup() const
{
    return m_settings.value("General/restoreSession", true).toBool();
}

void Settings::setRestoreSessionOnStartup(bool restore)
{
    m_settings.setValue("General/restoreSession", restore);
}

bool Settings::promptToResumeProject() const
{
    return m_settings.value("General/promptToResume", true).toBool();
}

void Settings::setPromptToResumeProject(bool prompt)
{
    m_settings.setValue("General/promptToResume", prompt);
}

QString Settings::theme() const
{
    return m_settings.value("General/theme", "dark").toString();
}

void Settings::setTheme(const QString& theme)
{
    m_settings.setValue("General/theme", theme);
    emit themeChanged(theme);
}

int Settings::syntaxTheme() const
{
    return m_settings.value("Editor/syntaxTheme", 2).toInt();  // Default to VSCodeDark (2)
}

void Settings::setSyntaxTheme(int theme)
{
    m_settings.setValue("Editor/syntaxTheme", theme);
    emit syntaxThemeChanged(theme);
}

void Settings::sync()
{
    m_settings.sync();
}

} // namespace XXMLStudio
