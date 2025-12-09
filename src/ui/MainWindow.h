#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDockWidget>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>

namespace XXMLStudio {

class EditorTabWidget;
class CodeEditor;
class ProjectExplorer;
class ProblemsPanel;
class BuildOutputPanel;
class TerminalPanel;
class OutlinePanel;
class ProjectManager;
class Project;
class BuildManager;
class ProcessRunner;
class FindReplaceDialog;
class BookmarkManager;
class LSPClient;

/**
 * Main window of the XXMLStudio IDE.
 * Contains the central editor widget and dock panels.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    // Editor access
    EditorTabWidget* editorTabs() const { return m_editorTabs; }

    // File operations
    void openFile(const QString& path);
    void openProject(const QString& path);

public slots:
    // File menu actions
    void newFile();
    void newProject();
    void openFileDialog();
    void openProjectDialog();
    void saveFile();
    void saveFileAs();
    void saveAll();
    void closeFile();

    // Edit menu actions
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void selectAll();
    void findReplace();
    void goToLine();
    void toggleBookmark();
    void nextBookmark();
    void previousBookmark();

    // Build menu actions
    void buildProject();
    void rebuildProject();
    void cleanProject();
    void cancelBuild();
    void runProject();
    void runWithoutBuilding();

    // View menu actions
    void resetLayout();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUi();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupDockWidgets();
    void setupCentralWidget();
    void setupConnections();

    void createActions();
    void createFileMenu();
    void createEditMenu();
    void createViewMenu();
    void createBuildMenu();
    void createRunMenu();
    void createToolsMenu();
    void createHelpMenu();

    void saveWindowState();
    void restoreWindowState();

    // Central widget
    EditorTabWidget* m_editorTabs = nullptr;

    // Dock widgets
    QDockWidget* m_projectExplorerDock = nullptr;
    QDockWidget* m_problemsDock = nullptr;
    QDockWidget* m_buildOutputDock = nullptr;
    QDockWidget* m_terminalDock = nullptr;
    QDockWidget* m_outlineDock = nullptr;

    // Panel widgets
    ProjectExplorer* m_projectExplorer = nullptr;
    ProblemsPanel* m_problemsPanel = nullptr;
    BuildOutputPanel* m_buildOutputPanel = nullptr;
    TerminalPanel* m_terminalPanel = nullptr;
    OutlinePanel* m_outlinePanel = nullptr;

    // Toolbars
    QToolBar* m_mainToolBar = nullptr;
    QToolBar* m_buildToolBar = nullptr;

    // Project management
    ProjectManager* m_projectManager = nullptr;

    // Build system
    BuildManager* m_buildManager = nullptr;
    ProcessRunner* m_processRunner = nullptr;

    // Dialogs
    FindReplaceDialog* m_findReplaceDialog = nullptr;

    // LSP Client
    LSPClient* m_lspClient = nullptr;

    // Bookmark Manager
    BookmarkManager* m_bookmarkManager = nullptr;

    // Status bar widgets
    QLabel* m_cursorPositionLabel = nullptr;
    QLabel* m_encodingLabel = nullptr;
    QLabel* m_lspStatusLabel = nullptr;

    // Actions - File
    QAction* m_newFileAction = nullptr;
    QAction* m_newProjectAction = nullptr;
    QAction* m_openFileAction = nullptr;
    QAction* m_openProjectAction = nullptr;
    QAction* m_saveAction = nullptr;
    QAction* m_saveAsAction = nullptr;
    QAction* m_saveAllAction = nullptr;
    QAction* m_closeFileAction = nullptr;
    QAction* m_exitAction = nullptr;

    // Actions - Edit
    QAction* m_undoAction = nullptr;
    QAction* m_redoAction = nullptr;
    QAction* m_cutAction = nullptr;
    QAction* m_copyAction = nullptr;
    QAction* m_pasteAction = nullptr;
    QAction* m_selectAllAction = nullptr;
    QAction* m_findReplaceAction = nullptr;
    QAction* m_goToLineAction = nullptr;
    QAction* m_toggleBookmarkAction = nullptr;
    QAction* m_nextBookmarkAction = nullptr;
    QAction* m_prevBookmarkAction = nullptr;

    // Actions - Build
    QAction* m_buildAction = nullptr;
    QAction* m_rebuildAction = nullptr;
    QAction* m_cleanAction = nullptr;
    QAction* m_cancelBuildAction = nullptr;

    // Actions - Run
    QAction* m_runAction = nullptr;
    QAction* m_runWithoutBuildAction = nullptr;
};

} // namespace XXMLStudio

#endif // MAINWINDOW_H
