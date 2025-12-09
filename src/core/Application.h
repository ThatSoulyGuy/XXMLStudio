#ifndef APPLICATION_H
#define APPLICATION_H

#include <QApplication>
#include <QObject>
#include <memory>

namespace XXMLStudio {

class Settings;
class MainWindow;

/**
 * Application singleton that manages the IDE lifecycle.
 * Provides access to global services like settings and the main window.
 */
class Application : public QObject
{
    Q_OBJECT

public:
    static Application* instance();
    static void create(int& argc, char** argv);
    static void destroy();

    // Accessors
    QApplication* qtApp() const { return m_qtApp.get(); }
    Settings* settings() const { return m_settings.get(); }
    MainWindow* mainWindow() const { return m_mainWindow; }

    // Application lifecycle
    int run();
    void quit();

    // Theme management
    void loadStylesheet(const QString& path);
    void applyDarkTheme();

    // Paths
    QString appDir() const;
    QString toolchainDir() const;
    QString compilerPath() const;
    QString lspServerPath() const;
    QString userDataDir() const;
    QString cacheDir() const;

signals:
    void aboutToQuit();

private:
    explicit Application(int& argc, char** argv);
    ~Application();

    static Application* s_instance;

    std::unique_ptr<QApplication> m_qtApp;
    std::unique_ptr<Settings> m_settings;
    MainWindow* m_mainWindow = nullptr;

    void initialize();
    void createMainWindow();
    void setupConnections();
};

} // namespace XXMLStudio

#endif // APPLICATION_H
