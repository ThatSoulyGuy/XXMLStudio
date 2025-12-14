#include "Application.h"
#include "Settings.h"
#include "ui/MainWindow.h"

#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QStyleFactory>

namespace XXMLStudio {

Application* Application::s_instance = nullptr;

Application* Application::instance()
{
    return s_instance;
}

void Application::create(int& argc, char** argv)
{
    if (!s_instance) {
        s_instance = new Application(argc, argv);
    }
}

void Application::destroy()
{
    delete s_instance;
    s_instance = nullptr;
}

Application::Application(int& argc, char** argv)
    : QObject(nullptr)
{
    // Set singleton instance immediately so it's available during initialization
    s_instance = this;

    // Create Qt application
    m_qtApp = std::make_unique<QApplication>(argc, argv);
    m_qtApp->setApplicationName("XXMLStudio");
    m_qtApp->setApplicationVersion("0.1.0");
    m_qtApp->setOrganizationName("XXML");
    m_qtApp->setOrganizationDomain("xxml.dev");

    // Use Fusion style as base for our dark theme
    m_qtApp->setStyle(QStyleFactory::create("Fusion"));

    initialize();
}

Application::~Application()
{
    // MainWindow is deleted by Qt's parent-child mechanism
    m_mainWindow = nullptr;
}

void Application::initialize()
{
    // Create settings manager
    m_settings = std::make_unique<Settings>();

    // Apply dark theme
    applyDarkTheme();

    // Create main window
    createMainWindow();

    // Setup application connections
    setupConnections();
}

void Application::createMainWindow()
{
    m_mainWindow = new MainWindow();
    m_mainWindow->show();
}

void Application::setupConnections()
{
    connect(m_qtApp.get(), &QApplication::aboutToQuit,
            this, &Application::aboutToQuit);
}

int Application::run()
{
    return m_qtApp->exec();
}

void Application::quit()
{
    emit aboutToQuit();
    m_qtApp->quit();
}

void Application::loadStylesheet(const QString& path)
{
    QFile file(path);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QString stylesheet = file.readAll();
        m_qtApp->setStyleSheet(stylesheet);
        file.close();
    }
}

void Application::applyDarkTheme()
{
    // Load the dark theme stylesheet from resources
    loadStylesheet(":/themes/dark.qss");

    // Set dark palette as fallback - icons use Mid, Light, Dark colors
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(30, 30, 30));
    darkPalette.setColor(QPalette::WindowText, QColor(212, 212, 212));
    darkPalette.setColor(QPalette::Base, QColor(37, 37, 38));
    darkPalette.setColor(QPalette::AlternateBase, QColor(45, 45, 45));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(37, 37, 38));
    darkPalette.setColor(QPalette::ToolTipText, QColor(212, 212, 212));
    darkPalette.setColor(QPalette::Text, QColor(212, 212, 212));
    darkPalette.setColor(QPalette::Button, QColor(60, 60, 60));
    darkPalette.setColor(QPalette::ButtonText, QColor(212, 212, 212));
    darkPalette.setColor(QPalette::BrightText, Qt::white);
    darkPalette.setColor(QPalette::Link, QColor(0, 122, 204));
    darkPalette.setColor(QPalette::Highlight, QColor(0, 122, 204));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);
    // These colors are used for drawing icons and indicators
    darkPalette.setColor(QPalette::Light, QColor(180, 180, 180));
    darkPalette.setColor(QPalette::Midlight, QColor(150, 150, 150));
    darkPalette.setColor(QPalette::Mid, QColor(140, 140, 140));
    darkPalette.setColor(QPalette::Dark, QColor(80, 80, 80));
    darkPalette.setColor(QPalette::Shadow, QColor(20, 20, 20));
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(101, 101, 101));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(101, 101, 101));
    darkPalette.setColor(QPalette::Disabled, QPalette::Light, QColor(80, 80, 80));

    m_qtApp->setPalette(darkPalette);
}

QString Application::appDir() const
{
    return m_qtApp->applicationDirPath();
}

QString Application::toolchainDir() const
{
#ifdef Q_OS_MACOS
    // macOS app bundle: XXMLStudio.app/Contents/Resources/toolchain
    return appDir() + "/../Resources/toolchain";
#else
    // Windows/Linux: toolchain directory alongside executable
    return appDir() + "/toolchain";
#endif
}

QString Application::compilerPath() const
{
#ifdef Q_OS_WIN
    return toolchainDir() + "/bin/xxml.exe";
#else
    // macOS/Linux: bundled toolchain has bin/ subdirectory
    return toolchainDir() + "/bin/xxml";
#endif
}

QString Application::lspServerPath() const
{
#ifdef Q_OS_WIN
    return toolchainDir() + "/bin/xxml-lsp.exe";
#else
    // macOS/Linux: bundled toolchain has bin/ subdirectory
    return toolchainDir() + "/bin/xxml-lsp";
#endif
}

QString Application::userDataDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

QString Application::cacheDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
}

} // namespace XXMLStudio
