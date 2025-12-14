#include "MainWindow.h"
#include "core/Application.h"
#include "core/Settings.h"
#include "core/IconUtils.h"
#include "editor/EditorTabWidget.h"
#include "editor/CodeEditor.h"
#include "editor/BookmarkManager.h"
#include "panels/ProjectExplorer.h"
#include "panels/ProblemsPanel.h"
#include "panels/BuildOutputPanel.h"
#include "panels/TerminalPanel.h"
#include "panels/OutlinePanel.h"
#include "project/ProjectManager.h"
#include "project/Project.h"
#include "build/BuildManager.h"
#include "build/ProcessRunner.h"
#include "build/OutputParser.h"
#include "build/ToolchainLocator.h"
#include "lsp/LSPClient.h"
#include "lsp/LSPProtocol.h"
#include "dialogs/NewProjectDialog.h"
#include "dialogs/GoToLineDialog.h"
#include "dialogs/FindReplaceDialog.h"
#include "dialogs/SettingsDialog.h"
#include "dialogs/ResumeProjectDialog.h"
#include "dialogs/DependencyDialog.h"
#include "project/DependencyManager.h"
#include "git/GitManager.h"
#include "git/GitStatusModel.h"
#include "panels/GitChangesPanel.h"
#include "panels/GitHistoryPanel.h"
#include "panels/GitFileDecorator.h"
#include "widgets/GitBranchWidget.h"
#include "widgets/GitStatusIndicator.h"

#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <functional>
#include <memory>
#include <QFile>
#include <QStandardPaths>
#include <QDateTime>

// Debug logging to file
static void logToFile(const QString& message) {
    static QString logPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/xxmlstudio_debug.log";
    QFile file(logPath);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << " " << message << "\n";
        file.close();
    }
}
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QShortcut>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextStream>

namespace XXMLStudio {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    // Clear and init log file
    QString logPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/xxmlstudio_debug.log";
    QFile logFile(logPath);
    logFile.remove();  // Clear old log
    logToFile(QString("=== XXMLStudio started === Log file: %1").arg(logPath));

    setWindowTitle("XXML Studio");
    setMinimumSize(800, 600);

    setupUi();
    restoreWindowState();

    // Check if we should resume a previous project (after window is shown)
    QMetaObject::invokeMethod(this, &MainWindow::checkResumeProject, Qt::QueuedConnection);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUi()
{
    // Create project manager
    m_projectManager = new ProjectManager(this);

    // Create build system
    m_buildManager = new BuildManager(this);
    m_processRunner = new ProcessRunner(this);

    // Create bookmark manager
    m_bookmarkManager = new BookmarkManager(this);

    // Create LSP client
    m_lspClient = new LSPClient(this);

    // Create Git manager
    m_gitManager = new GitManager(this);

    createActions();
    setupMenuBar();
    setupToolBar();
    setupCentralWidget();
    setupDockWidgets();
    setupStatusBar();
    setupConnections();

    // Start LSP client - find the LSP server using ToolchainLocator
    ToolchainLocator toolchainLocator;
    QString lspPath = toolchainLocator.lspServerPath();
    logToFile(QString("LSP: ToolchainLocator found: %1").arg(lspPath.isEmpty() ? "nothing" : lspPath));

    if (!lspPath.isEmpty() && QFileInfo::exists(lspPath)) {
        logToFile(QString("LSP: Starting server at: %1").arg(lspPath));
        m_lspClient->start(lspPath);
    } else {
        logToFile("LSP: Server NOT found in any search path!");
        statusBar()->showMessage(tr("LSP server not found. Install XXML toolchain or check PATH."), 5000);
    }
}

void MainWindow::createActions()
{
    // File actions
    m_newFileAction = new QAction(tr("New File"), this);
    m_newFileAction->setShortcut(QKeySequence::New);
    m_newFileAction->setIcon(IconUtils::loadForDarkBackground(":/icons/NewDocument.svg"));

    m_newProjectAction = new QAction(tr("New Project..."), this);
    m_newProjectAction->setShortcut(QKeySequence("Ctrl+Shift+N"));

    m_openFileAction = new QAction(tr("Open File..."), this);
    m_openFileAction->setShortcut(QKeySequence::Open);
    m_openFileAction->setIcon(IconUtils::loadForDarkBackground(":/icons/OpenFile.svg"));

    m_openProjectAction = new QAction(tr("Open Project..."), this);
    m_openProjectAction->setShortcut(QKeySequence("Ctrl+Shift+O"));

    m_saveAction = new QAction(tr("Save"), this);
    m_saveAction->setShortcut(QKeySequence::Save);
    m_saveAction->setIcon(IconUtils::loadForDarkBackground(":/icons/Save.svg"));

    m_saveAsAction = new QAction(tr("Save As..."), this);
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    m_saveAsAction->setIcon(IconUtils::loadForDarkBackground(":/icons/SaveAs.svg"));

    m_saveAllAction = new QAction(tr("Save All"), this);
    m_saveAllAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
    m_saveAllAction->setIcon(IconUtils::loadForDarkBackground(":/icons/SaveAll.svg"));

    m_closeFileAction = new QAction(tr("Close"), this);
    m_closeFileAction->setShortcut(QKeySequence::Close);

    m_exitAction = new QAction(tr("Exit"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);

    // Edit actions
    m_undoAction = new QAction(tr("Undo"), this);
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setIcon(IconUtils::loadForDarkBackground(":/icons/Undo.svg"));

    m_redoAction = new QAction(tr("Redo"), this);
    m_redoAction->setShortcut(QKeySequence::Redo);
    m_redoAction->setIcon(IconUtils::loadForDarkBackground(":/icons/Redo.svg"));

    m_cutAction = new QAction(tr("Cut"), this);
    m_cutAction->setShortcut(QKeySequence::Cut);

    m_copyAction = new QAction(tr("Copy"), this);
    m_copyAction->setShortcut(QKeySequence::Copy);

    m_pasteAction = new QAction(tr("Paste"), this);
    m_pasteAction->setShortcut(QKeySequence::Paste);

    m_selectAllAction = new QAction(tr("Select All"), this);
    m_selectAllAction->setShortcut(QKeySequence::SelectAll);

    m_findReplaceAction = new QAction(tr("Find and Replace..."), this);
    m_findReplaceAction->setShortcut(QKeySequence::Find);

    m_goToLineAction = new QAction(tr("Go to Line..."), this);
    m_goToLineAction->setShortcut(QKeySequence("Ctrl+G"));

    // Bookmark actions
    m_toggleBookmarkAction = new QAction(tr("Toggle Bookmark"), this);
    m_toggleBookmarkAction->setShortcut(QKeySequence("Ctrl+B"));

    m_nextBookmarkAction = new QAction(tr("Next Bookmark"), this);
    m_nextBookmarkAction->setShortcut(QKeySequence("F2"));

    m_prevBookmarkAction = new QAction(tr("Previous Bookmark"), this);
    m_prevBookmarkAction->setShortcut(QKeySequence("Shift+F2"));

    // Build actions
    m_buildAction = new QAction(tr("Build Project"), this);
    m_buildAction->setShortcut(QKeySequence("F7"));
    m_buildAction->setIcon(IconUtils::loadForDarkBackground(":/icons/BuildSolution.svg"));

    m_rebuildAction = new QAction(tr("Rebuild Project"), this);
    m_rebuildAction->setShortcut(QKeySequence("Ctrl+Shift+B"));
    m_rebuildAction->setIcon(IconUtils::loadForDarkBackground(":/icons/BuildSelection.svg"));

    m_cleanAction = new QAction(tr("Clean Project"), this);
    m_cleanAction->setIcon(IconUtils::loadForDarkBackground(":/icons/CleanData.svg"));

    m_cancelBuildAction = new QAction(tr("Cancel Build"), this);
    m_cancelBuildAction->setShortcut(QKeySequence("Ctrl+Pause"));
    m_cancelBuildAction->setEnabled(false);
    m_cancelBuildAction->setIcon(IconUtils::loadForDarkBackground(":/icons/CancelBuild.svg"));

    // Run actions
    m_runAction = new QAction(tr("Run"), this);
    m_runAction->setShortcut(QKeySequence("F5"));
    m_runAction->setIcon(IconUtils::loadForDarkBackground(":/icons/Run.svg"));

    m_pauseAction = new QAction(tr("Pause"), this);
    m_pauseAction->setShortcut(QKeySequence("F6"));
    m_pauseAction->setIcon(IconUtils::loadForDarkBackground(":/icons/Pause.svg"));
    m_pauseAction->setEnabled(false);
    m_pauseAction->setVisible(false);
    m_pauseAction->setCheckable(true);

    m_stopAction = new QAction(tr("Stop"), this);
    m_stopAction->setShortcut(QKeySequence("Shift+F5"));
    m_stopAction->setIcon(IconUtils::loadForDarkBackground(":/icons/Stop.svg"));
    m_stopAction->setEnabled(false);
    m_stopAction->setVisible(false);

    m_runWithoutBuildAction = new QAction(tr("Run Without Building"), this);
    m_runWithoutBuildAction->setShortcut(QKeySequence("Ctrl+F5"));
    m_runWithoutBuildAction->setIcon(IconUtils::loadForDarkBackground(":/icons/RunOutline.svg"));

    // Project actions
    m_manageDependenciesAction = new QAction(tr("Manage Dependencies..."), this);
    m_manageDependenciesAction->setIcon(IconUtils::loadForDarkBackground(":/icons/AddReference.svg"));
    m_manageDependenciesAction->setEnabled(false);
}

void MainWindow::setupMenuBar()
{
    createFileMenu();
    createEditMenu();
    createViewMenu();
    createProjectMenu();
    createBuildMenu();
    createRunMenu();
    createGitMenu();
    createToolsMenu();
    createHelpMenu();
}

void MainWindow::createFileMenu()
{
    QMenu* fileMenu = menuBar()->addMenu(tr("&File"));

    fileMenu->addAction(m_newFileAction);
    fileMenu->addAction(m_newProjectAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_openFileAction);
    fileMenu->addAction(m_openProjectAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_saveAction);
    fileMenu->addAction(m_saveAsAction);
    fileMenu->addAction(m_saveAllAction);
    fileMenu->addSeparator();
    fileMenu->addAction(m_closeFileAction);
    fileMenu->addSeparator();

    // Recent projects submenu
    m_recentProjectsMenu = fileMenu->addMenu(tr("Recent Projects"));
    updateRecentProjectsMenu();

    fileMenu->addSeparator();
    fileMenu->addAction(m_exitAction);
}

void MainWindow::createEditMenu()
{
    QMenu* editMenu = menuBar()->addMenu(tr("&Edit"));

    editMenu->addAction(m_undoAction);
    editMenu->addAction(m_redoAction);
    editMenu->addSeparator();
    editMenu->addAction(m_cutAction);
    editMenu->addAction(m_copyAction);
    editMenu->addAction(m_pasteAction);
    editMenu->addSeparator();
    editMenu->addAction(m_selectAllAction);
    editMenu->addSeparator();
    editMenu->addAction(m_findReplaceAction);
    editMenu->addAction(m_goToLineAction);
    editMenu->addSeparator();
    editMenu->addAction(m_toggleBookmarkAction);
    editMenu->addAction(m_nextBookmarkAction);
    editMenu->addAction(m_prevBookmarkAction);
}

void MainWindow::createViewMenu()
{
    QMenu* viewMenu = menuBar()->addMenu(tr("&View"));

    // Dock widget visibility toggles will be added after docks are created
    viewMenu->addAction(tr("Project Explorer"), this, [this]() {
        m_projectExplorerDock->setVisible(!m_projectExplorerDock->isVisible());
    });
    viewMenu->addAction(tr("Outline"), this, [this]() {
        m_outlineDock->setVisible(!m_outlineDock->isVisible());
    });
    viewMenu->addAction(tr("Git Changes"), this, [this]() {
        m_gitChangesDock->setVisible(!m_gitChangesDock->isVisible());
        if (m_gitChangesDock->isVisible()) {
            m_gitChangesDock->raise();
        }
    });
    viewMenu->addAction(tr("Problems"), this, [this]() {
        m_problemsDock->setVisible(!m_problemsDock->isVisible());
    });
    viewMenu->addAction(tr("Build Output"), this, [this]() {
        m_buildOutputDock->setVisible(!m_buildOutputDock->isVisible());
    });
    viewMenu->addAction(tr("Terminal"), this, [this]() {
        m_terminalDock->setVisible(!m_terminalDock->isVisible());
    });
    viewMenu->addAction(tr("Git History"), this, [this]() {
        m_gitHistoryDock->setVisible(!m_gitHistoryDock->isVisible());
        if (m_gitHistoryDock->isVisible()) {
            m_gitHistoryDock->raise();
            m_gitManager->getLog(100);  // Refresh history when shown
        }
    });

    viewMenu->addSeparator();

    QAction* resetLayoutAction = viewMenu->addAction(tr("Reset Layout"));
    connect(resetLayoutAction, &QAction::triggered, this, &MainWindow::resetLayout);
}

void MainWindow::createProjectMenu()
{
    QMenu* projectMenu = menuBar()->addMenu(tr("&Project"));

    projectMenu->addAction(m_manageDependenciesAction);
}

void MainWindow::createBuildMenu()
{
    QMenu* buildMenu = menuBar()->addMenu(tr("&Build"));

    buildMenu->addAction(m_buildAction);
    buildMenu->addAction(m_rebuildAction);
    buildMenu->addAction(m_cleanAction);
    buildMenu->addSeparator();
    buildMenu->addAction(m_cancelBuildAction);
}

void MainWindow::createRunMenu()
{
    QMenu* runMenu = menuBar()->addMenu(tr("&Run"));

    runMenu->addAction(m_runAction);
    runMenu->addAction(m_runWithoutBuildAction);
}

void MainWindow::createGitMenu()
{
    QMenu* gitMenu = menuBar()->addMenu(tr("&Git"));

    QAction* commitAction = gitMenu->addAction(tr("Commit..."));
    commitAction->setShortcut(QKeySequence("Ctrl+K"));
    commitAction->setIcon(IconUtils::loadForDarkBackground(":/icons/Commit.svg"));
    connect(commitAction, &QAction::triggered, this, [this]() {
        if (m_gitChangesDock) {
            m_gitChangesDock->raise();
            m_gitChangesDock->show();
        }
    });

    gitMenu->addSeparator();

    QAction* fetchAction = gitMenu->addAction(tr("Fetch"));
    fetchAction->setIcon(IconUtils::loadForDarkBackground(":/icons/CloudDownload.svg"));
    connect(fetchAction, &QAction::triggered, this, [this]() {
        m_gitManager->fetch();
    });

    QAction* pullAction = gitMenu->addAction(tr("Pull"));
    pullAction->setShortcut(QKeySequence("Ctrl+Shift+P"));
    pullAction->setIcon(IconUtils::loadForDarkBackground(":/icons/BrowsePrevious.svg"));
    connect(pullAction, &QAction::triggered, this, [this]() {
        m_gitManager->pull();
    });

    QAction* pushAction = gitMenu->addAction(tr("Push"));
    pushAction->setShortcut(QKeySequence("Ctrl+Shift+U"));
    pushAction->setIcon(IconUtils::loadForDarkBackground(":/icons/BrowseNext.svg"));
    connect(pushAction, &QAction::triggered, this, [this]() {
        m_gitManager->push();
    });

    gitMenu->addSeparator();

    QAction* branchesAction = gitMenu->addAction(tr("Branches..."));
    branchesAction->setIcon(IconUtils::loadForDarkBackground(":/icons/Branch.svg"));
    connect(branchesAction, &QAction::triggered, this, [this]() {
        m_gitManager->getBranches();
    });

    gitMenu->addSeparator();

    QAction* historyAction = gitMenu->addAction(tr("View History"));
    historyAction->setShortcut(QKeySequence("Ctrl+Shift+H"));
    historyAction->setIcon(IconUtils::loadForDarkBackground(":/icons/ActionLog.svg"));
    connect(historyAction, &QAction::triggered, this, [this]() {
        if (m_gitHistoryDock) {
            m_gitHistoryDock->raise();
            m_gitHistoryDock->show();
            m_gitManager->getLog(100);
        }
    });

    QAction* changesAction = gitMenu->addAction(tr("View Changes"));
    changesAction->setIcon(IconUtils::loadForDarkBackground(":/icons/Changeset.svg"));
    connect(changesAction, &QAction::triggered, this, [this]() {
        if (m_gitChangesDock) {
            m_gitChangesDock->raise();
            m_gitChangesDock->show();
        }
    });
}

void MainWindow::createToolsMenu()
{
    QMenu* toolsMenu = menuBar()->addMenu(tr("&Tools"));

    QAction* settingsAction = toolsMenu->addAction(tr("Settings..."));
    connect(settingsAction, &QAction::triggered, this, [this]() {
        SettingsDialog dialog(Application::instance()->settings(), this);
        dialog.exec();
    });
}

void MainWindow::createHelpMenu()
{
    QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));

    QAction* aboutAction = helpMenu->addAction(tr("About XXML Studio"));
    connect(aboutAction, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, tr("About XXML Studio"),
            tr("<h3>XXML Studio</h3>"
               "<p>Version 0.1.0</p>"
               "<p>An integrated development environment for the XXML programming language.</p>"));
    });

    QAction* aboutQtAction = helpMenu->addAction(tr("About Qt"));
    connect(aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);
}

void MainWindow::updateRecentProjectsMenu()
{
    if (!m_recentProjectsMenu) {
        return;
    }

    m_recentProjectsMenu->clear();

    Settings* settings = Application::instance()->settings();
    QStringList recentProjects = settings->recentProjects();

    if (recentProjects.isEmpty()) {
        QAction* noRecent = m_recentProjectsMenu->addAction(tr("(No recent projects)"));
        noRecent->setEnabled(false);
        return;
    }

    for (const QString& projectPath : recentProjects) {
        QFileInfo info(projectPath);
        QString displayName = info.fileName();
        if (displayName.endsWith(".xxmlp")) {
            displayName = displayName.left(displayName.length() - 6);
        }

        QAction* action = m_recentProjectsMenu->addAction(displayName);
        action->setToolTip(projectPath);
        action->setData(projectPath);

        connect(action, &QAction::triggered, this, [this, projectPath]() {
            openProject(projectPath);
        });
    }

    m_recentProjectsMenu->addSeparator();
    QAction* clearAction = m_recentProjectsMenu->addAction(tr("Clear Recent Projects"));
    connect(clearAction, &QAction::triggered, this, [this]() {
        Settings* settings = Application::instance()->settings();
        settings->clearRecentProjects();
        updateRecentProjectsMenu();
    });
}

void MainWindow::setupToolBar()
{
    m_mainToolBar = addToolBar(tr("Main"));
    m_mainToolBar->setObjectName("MainToolBar");
    m_mainToolBar->setMovable(false);
    m_mainToolBar->setIconSize(QSize(18, 18));

    m_mainToolBar->addAction(m_undoAction);
    m_mainToolBar->addAction(m_redoAction);
    m_mainToolBar->addSeparator();

    // Build configuration selector
    m_configComboBox = new QComboBox(this);
    m_configComboBox->setMinimumWidth(100);
    m_configComboBox->setToolTip(tr("Build Configuration"));
    m_configComboBox->setEnabled(false);  // Disabled until project is opened
    m_mainToolBar->addWidget(m_configComboBox);

    // Git branch selector
    m_gitBranchWidget = new GitBranchWidget(this);
    m_gitBranchWidget->setGitManager(m_gitManager);
    m_mainToolBar->addWidget(m_gitBranchWidget);

    // VS 2022 style - no build button in toolbar, only run/pause/stop
    m_mainToolBar->addAction(m_runAction);
    m_mainToolBar->addAction(m_pauseAction);
    m_mainToolBar->addAction(m_stopAction);

    // Project actions
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(m_manageDependenciesAction);
}

void MainWindow::setupCentralWidget()
{
    m_editorTabs = new EditorTabWidget(this);
    setCentralWidget(m_editorTabs);
}

void MainWindow::setupDockWidgets()
{
    // Enable dock nesting and tabbing
    setDockOptions(QMainWindow::AnimatedDocks |
                   QMainWindow::AllowNestedDocks |
                   QMainWindow::AllowTabbedDocks);

    // Project Explorer (left)
    m_projectExplorer = new ProjectExplorer(this);
    m_projectExplorerDock = new QDockWidget(tr("Project Explorer"), this);
    m_projectExplorerDock->setObjectName("ProjectExplorerDock");
    m_projectExplorerDock->setWidget(m_projectExplorer);
    addDockWidget(Qt::LeftDockWidgetArea, m_projectExplorerDock);

    // Set up Git file decorator for Project Explorer
    m_gitFileDecorator = new GitFileDecorator(this);
    m_gitFileDecorator->setGitManager(m_gitManager);
    m_projectExplorer->setGitFileDecorator(m_gitFileDecorator);

    // Outline (left, tabbed with Project Explorer)
    m_outlinePanel = new OutlinePanel(this);
    m_outlineDock = new QDockWidget(tr("Outline"), this);
    m_outlineDock->setObjectName("OutlineDock");
    m_outlineDock->setWidget(m_outlinePanel);
    tabifyDockWidget(m_projectExplorerDock, m_outlineDock);
    m_projectExplorerDock->raise(); // Show Project Explorer by default

    // Problems Panel (bottom)
    m_problemsPanel = new ProblemsPanel(this);
    m_problemsDock = new QDockWidget(tr("Problems"), this);
    m_problemsDock->setObjectName("ProblemsDock");
    m_problemsDock->setWidget(m_problemsPanel);
    addDockWidget(Qt::BottomDockWidgetArea, m_problemsDock);

    // Build Output (bottom, tabbed with Problems)
    m_buildOutputPanel = new BuildOutputPanel(this);
    m_buildOutputDock = new QDockWidget(tr("Build Output"), this);
    m_buildOutputDock->setObjectName("BuildOutputDock");
    m_buildOutputDock->setWidget(m_buildOutputPanel);
    tabifyDockWidget(m_problemsDock, m_buildOutputDock);

    // Terminal (bottom, tabbed)
    m_terminalPanel = new TerminalPanel(this);
    m_terminalDock = new QDockWidget(tr("Terminal"), this);
    m_terminalDock->setObjectName("TerminalDock");
    m_terminalDock->setWidget(m_terminalPanel);
    tabifyDockWidget(m_buildOutputDock, m_terminalDock);
    m_problemsDock->raise(); // Show Problems by default

    // Git Changes Panel (left, tabbed with Outline)
    m_gitChangesPanel = new GitChangesPanel(this);
    m_gitChangesPanel->setGitManager(m_gitManager);
    m_gitChangesDock = new QDockWidget(tr("Git Changes"), this);
    m_gitChangesDock->setObjectName("GitChangesDock");
    m_gitChangesDock->setWidget(m_gitChangesPanel);
    tabifyDockWidget(m_outlineDock, m_gitChangesDock);
    m_projectExplorerDock->raise(); // Keep Project Explorer as default

    // Git History Panel (bottom, tabbed with Terminal)
    m_gitHistoryPanel = new GitHistoryPanel(this);
    m_gitHistoryPanel->setGitManager(m_gitManager);
    m_gitHistoryDock = new QDockWidget(tr("Git History"), this);
    m_gitHistoryDock->setObjectName("GitHistoryDock");
    m_gitHistoryDock->setWidget(m_gitHistoryPanel);
    tabifyDockWidget(m_terminalDock, m_gitHistoryDock);
    m_problemsDock->raise(); // Keep Problems as default

    // Set initial sizes
    resizeDocks({m_projectExplorerDock}, {250}, Qt::Horizontal);
    resizeDocks({m_problemsDock}, {200}, Qt::Vertical);
}

void MainWindow::setupStatusBar()
{
    // VS 2022 style status bar layout: [Message] ... [Git] [Ln X, Col Y] [CRLF] [UTF-8] [LSP: Status]

    // Git status indicator (clickable, shows branch and sync status)
    m_gitStatusIndicator = new GitStatusIndicator(this);
    m_gitStatusIndicator->setGitManager(m_gitManager);

    m_cursorPositionLabel = new QLabel(tr("Ln 1, Col 1"), this);
    m_cursorPositionLabel->setMinimumWidth(100);

    m_lineEndingsLabel = new QLabel(tr("CRLF"), this);
    m_lineEndingsLabel->setMinimumWidth(50);

    m_encodingLabel = new QLabel(tr("UTF-8"), this);
    m_encodingLabel->setMinimumWidth(60);

    m_lspStatusLabel = new QLabel(tr("LSP: Disconnected"), this);
    m_lspStatusLabel->setMinimumWidth(120);

    statusBar()->addPermanentWidget(m_gitStatusIndicator);
    statusBar()->addPermanentWidget(m_cursorPositionLabel);
    statusBar()->addPermanentWidget(m_lineEndingsLabel);
    statusBar()->addPermanentWidget(m_encodingLabel);
    statusBar()->addPermanentWidget(m_lspStatusLabel);

    statusBar()->showMessage(tr("Ready"));

    // Set initial status bar color (idle = purple)
    updateStatusBarColor(IDEState::Idle);
}

void MainWindow::setupConnections()
{
    // File actions
    connect(m_newFileAction, &QAction::triggered, this, &MainWindow::newFile);
    connect(m_newProjectAction, &QAction::triggered, this, &MainWindow::newProject);
    connect(m_openFileAction, &QAction::triggered, this, &MainWindow::openFileDialog);
    connect(m_openProjectAction, &QAction::triggered, this, &MainWindow::openProjectDialog);
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    connect(m_saveAsAction, &QAction::triggered, this, &MainWindow::saveFileAs);
    connect(m_saveAllAction, &QAction::triggered, this, &MainWindow::saveAll);
    connect(m_closeFileAction, &QAction::triggered, this, &MainWindow::closeFile);
    connect(m_exitAction, &QAction::triggered, this, &QMainWindow::close);

    // Edit actions
    connect(m_undoAction, &QAction::triggered, this, &MainWindow::undo);
    connect(m_redoAction, &QAction::triggered, this, &MainWindow::redo);
    connect(m_cutAction, &QAction::triggered, this, &MainWindow::cut);
    connect(m_copyAction, &QAction::triggered, this, &MainWindow::copy);
    connect(m_pasteAction, &QAction::triggered, this, &MainWindow::paste);
    connect(m_selectAllAction, &QAction::triggered, this, &MainWindow::selectAll);
    connect(m_findReplaceAction, &QAction::triggered, this, &MainWindow::findReplace);
    connect(m_goToLineAction, &QAction::triggered, this, &MainWindow::goToLine);
    connect(m_toggleBookmarkAction, &QAction::triggered, this, &MainWindow::toggleBookmark);
    connect(m_nextBookmarkAction, &QAction::triggered, this, &MainWindow::nextBookmark);
    connect(m_prevBookmarkAction, &QAction::triggered, this, &MainWindow::previousBookmark);

    // Build actions
    connect(m_buildAction, &QAction::triggered, this, &MainWindow::buildProject);
    connect(m_rebuildAction, &QAction::triggered, this, &MainWindow::rebuildProject);
    connect(m_cleanAction, &QAction::triggered, this, &MainWindow::cleanProject);
    connect(m_cancelBuildAction, &QAction::triggered, this, &MainWindow::cancelBuild);

    // Build configuration selector
    connect(m_configComboBox, &QComboBox::currentTextChanged, this, [this](const QString& configName) {
        Project* project = m_projectManager->currentProject();
        if (project && !configName.isEmpty()) {
            project->setActiveConfigurationName(configName);
            statusBar()->showMessage(tr("Build configuration: %1").arg(configName), 2000);
        }
    });

    // Run actions
    connect(m_runAction, &QAction::triggered, this, &MainWindow::runProject);
    connect(m_runWithoutBuildAction, &QAction::triggered, this, &MainWindow::runWithoutBuilding);
    connect(m_stopAction, &QAction::triggered, m_processRunner, &ProcessRunner::stop);
    connect(m_pauseAction, &QAction::toggled, this, [this](bool checked) {
        if (checked) {
            m_processRunner->pause();
        } else {
            m_processRunner->resume();
        }
    });

    // Project actions
    connect(m_manageDependenciesAction, &QAction::triggered, this, [this]() {
        if (!m_projectManager->hasProject()) {
            return;
        }
        DependencyDialog dialog(m_projectManager->currentProject(),
                                m_buildManager->dependencyManager(), this);
        dialog.exec();
    });

    // Process runner signals - toggle run/pause/stop visibility
    connect(m_processRunner, &ProcessRunner::started, this, [this]() {
        m_runAction->setVisible(false);
        m_pauseAction->setVisible(true);
        m_pauseAction->setEnabled(true);
        m_pauseAction->setChecked(false);
        m_stopAction->setVisible(true);
        m_stopAction->setEnabled(true);
        statusBar()->showMessage(tr("Program running..."));

        // Update status bar color to orange (running)
        updateStatusBarColor(IDEState::Running);
    });

    connect(m_processRunner, &ProcessRunner::finished, this, [this](int exitCode) {
        m_runAction->setVisible(true);
        m_pauseAction->setVisible(false);
        m_pauseAction->setEnabled(false);
        m_stopAction->setVisible(false);
        m_stopAction->setEnabled(false);
        statusBar()->showMessage(tr("Program exited with code %1").arg(exitCode), 5000);

        // Revert status bar color to project loaded (blue)
        updateStatusBarColor(IDEState::ProjectLoaded);
    });

    // Project manager signals
    connect(m_projectManager, &ProjectManager::projectOpened, this, [this](Project* project) {
        setWindowTitle(QString("XXML Studio - %1").arg(project->name()));
        m_projectExplorer->setRootPath(project->projectDir());
        statusBar()->showMessage(tr("Project opened: %1").arg(project->name()), 3000);

        // Populate build configuration combo box
        m_configComboBox->blockSignals(true);
        m_configComboBox->clear();
        for (const BuildConfiguration& config : project->configurations()) {
            m_configComboBox->addItem(config.name);
        }
        // Select the active configuration
        int index = m_configComboBox->findText(project->activeConfigurationName());
        if (index >= 0) {
            m_configComboBox->setCurrentIndex(index);
        }
        m_configComboBox->blockSignals(false);
        m_configComboBox->setEnabled(true);

        // Enable project-specific actions
        m_manageDependenciesAction->setEnabled(true);

        // Set up Git integration for this project directory
        m_gitManager->setRepositoryPath(project->projectDir());

        // Update status bar color to blue (project loaded)
        updateStatusBarColor(IDEState::ProjectLoaded);
    });

    connect(m_projectManager, &ProjectManager::projectClosed, this, [this]() {
        setWindowTitle("XXML Studio");
        m_projectExplorer->setRootPath(QString());
        statusBar()->showMessage(tr("Project closed"), 3000);

        // Clear and disable configuration combo box
        m_configComboBox->clear();
        m_configComboBox->setEnabled(false);

        // Disable project-specific actions
        m_manageDependenciesAction->setEnabled(false);

        // Clear Git integration
        m_gitManager->setRepositoryPath(QString());

        // Update status bar color to purple (idle)
        updateStatusBarColor(IDEState::Idle);
    });

    connect(m_projectManager, &ProjectManager::error, this, [this](const QString& message) {
        QMessageBox::warning(this, tr("Project Error"), message);
    });

    // Project explorer signals
    connect(m_projectExplorer, &ProjectExplorer::fileDoubleClicked, this, &MainWindow::openFile);
    connect(m_projectExplorer, &ProjectExplorer::openFileRequested, this, &MainWindow::openFileDialog);
    connect(m_projectExplorer, &ProjectExplorer::saveFileRequested, this, &MainWindow::saveFile);
    connect(m_projectExplorer, &ProjectExplorer::setCompilationEntrypointRequested,
            this, &MainWindow::setCompilationEntrypoint);

    // Build manager signals
    connect(m_buildManager, &BuildManager::buildStarted, this, [this]() {
        m_buildAction->setEnabled(false);
        m_rebuildAction->setEnabled(false);
        m_cancelBuildAction->setEnabled(true);
        m_buildOutputDock->raise();

        // Update status bar color to blue (building)
        updateStatusBarColor(IDEState::Building);
    });

    connect(m_buildManager, &BuildManager::buildOutput, m_buildOutputPanel, &BuildOutputPanel::appendText);

    connect(m_buildManager, &BuildManager::problemFound, this, [this](const BuildProblem& problem) {
        m_problemsPanel->addProblem(
            problem.file,
            problem.line,
            problem.column,
            problem.severityString(),
            problem.message
        );
    });

    connect(m_buildManager, &BuildManager::buildFinished, this, [this](bool success) {
        m_buildAction->setEnabled(true);
        m_rebuildAction->setEnabled(true);
        m_cancelBuildAction->setEnabled(false);

        if (success) {
            statusBar()->showMessage(tr("Build succeeded"), 5000);
        } else {
            statusBar()->showMessage(tr("Build failed"), 5000);
            m_problemsDock->raise();
        }

        // Revert status bar color to project loaded (blue)
        updateStatusBarColor(IDEState::ProjectLoaded);
    });

    // Process runner signals
    connect(m_processRunner, &ProcessRunner::output, m_terminalPanel, &TerminalPanel::appendText);
    connect(m_processRunner, &ProcessRunner::errorOutput, m_terminalPanel, &TerminalPanel::appendText);

    connect(m_processRunner, &ProcessRunner::started, this, [this]() {
        m_runAction->setEnabled(false);
        m_terminalDock->raise();
    });

    connect(m_processRunner, &ProcessRunner::finished, this, [this](int exitCode) {
        Q_UNUSED(exitCode);
        m_runAction->setEnabled(true);
    });

    // Editor tab signals
    connect(m_editorTabs, &EditorTabWidget::cursorPositionChanged, this, [this](int line, int column) {
        m_cursorPositionLabel->setText(tr("Ln %1, Col %2").arg(line).arg(column));
    });

    connect(m_editorTabs, &EditorTabWidget::currentEditorChanged, this, [this](CodeEditor* editor) {
        if (editor && m_lspClient->isReady()) {
            // Request document symbols for outline
            QString uri = "file:///" + editor->filePath().replace("\\", "/");
            m_lspClient->requestDocumentSymbols(uri);

            // Update bookmark display
            QList<int> bookmarks = m_bookmarkManager->bookmarksForFile(editor->filePath());
            editor->setBookmarkedLines(bookmarks);
        }
        m_outlinePanel->clear();

        // Update line endings label
        updateLineEndingsLabel();
    });

    // Helper lambda to setup LSP for an editor
    auto setupEditorLSP = [this](CodeEditor* editor) {
        if (!editor || !m_lspClient->isReady()) return;

        QString path = editor->filePath();
        if (path.isEmpty()) return;

        QString uri = "file:///" + path;
        uri.replace("\\", "/");

        logToFile(QString("LSP: Opening document %1").arg(uri));
        m_lspClient->openDocument(uri, "xxml", 1, editor->toPlainText());

        // Use a shared_ptr to track document version per editor
        auto docVersion = std::make_shared<int>(1);

        // Document change -> LSP sync
        connect(editor, &CodeEditor::documentChanged, this, [this, editor, docVersion]() {
            if (!m_lspClient->isReady()) return;

            QString filePath = editor->filePath();
            if (filePath.isEmpty()) return;

            QString uri = "file:///" + filePath;
            uri.replace("\\", "/");
            logToFile(QString("LSP: Document changed %1 version %2").arg(uri).arg(*docVersion + 1));
            m_lspClient->changeDocument(uri, ++(*docVersion), editor->toPlainText());
        });

        // Completion request -> LSP
        connect(editor, &CodeEditor::completionRequested, this, [this, editor](int line, int character) {
            if (!m_lspClient->isReady()) return;

            QString filePath = editor->filePath();
            if (filePath.isEmpty()) return;

            QString uri = "file:///" + filePath;
            uri.replace("\\", "/");
            logToFile(QString("LSP: Requesting completion at line %1 char %2").arg(line).arg(character));
            m_lspClient->requestCompletion(uri, line, character);
        });
    };

    connect(m_editorTabs, &EditorTabWidget::fileOpened, this, [this, setupEditorLSP](const QString& path) {
        CodeEditor* editor = m_editorTabs->editorForFile(path);
        setupEditorLSP(editor);
    });

    // When LSP becomes ready, set up all already-open editors
    connect(m_lspClient, &LSPClient::initialized, this, [this, setupEditorLSP]() {
        logToFile(QString("LSP: Initialized, setting up %1 editors").arg(m_editorTabs->count()));
        for (int i = 0; i < m_editorTabs->count(); ++i) {
            CodeEditor* editor = m_editorTabs->editorAt(i);
            setupEditorLSP(editor);
        }
    });

    connect(m_editorTabs, &EditorTabWidget::fileClosed, this, [this](const QString& path) {
        if (m_lspClient->isReady()) {
            QString uri = "file:///" + path;
            uri.replace("\\", "/");
            m_lspClient->closeDocument(uri);
        }
    });

    // Bookmark manager signals
    connect(m_bookmarkManager, &BookmarkManager::bookmarksChanged, this, [this]() {
        CodeEditor* editor = m_editorTabs->currentEditor();
        if (editor) {
            QList<int> bookmarks = m_bookmarkManager->bookmarksForFile(editor->filePath());
            editor->setBookmarkedLines(bookmarks);
        }
    });

    // LSP client signals
    connect(m_lspClient, &LSPClient::stateChanged, this, [this](LSPClient::State state) {
        switch (state) {
            case LSPClient::State::Disconnected:
                m_lspStatusLabel->setText(tr("LSP: Disconnected"));
                break;
            case LSPClient::State::Connecting:
                m_lspStatusLabel->setText(tr("LSP: Connecting..."));
                break;
            case LSPClient::State::Initializing:
                m_lspStatusLabel->setText(tr("LSP: Initializing..."));
                break;
            case LSPClient::State::Ready:
                m_lspStatusLabel->setText(tr("LSP: Ready"));
                break;
            case LSPClient::State::ShuttingDown:
                m_lspStatusLabel->setText(tr("LSP: Stopping..."));
                break;
        }
    });

    // LSP completion received -> route to editor
    connect(m_lspClient, &LSPClient::completionReceived, this, [this](const QString& uri, const QList<LSPCompletionItem>& items) {
        logToFile(QString("MainWindow::completionReceived: %1 items for URI: %2").arg(items.size()).arg(uri));
        qDebug() << "MainWindow: Received" << items.size() << "completions for" << uri;

        // Convert URI to file path
        QString path = uri;
        if (path.startsWith("file:///")) {
            path = path.mid(8);
        }
#ifdef Q_OS_WIN
        // Windows: convert forward slashes to backslashes
        path.replace("/", "\\");
#endif
        logToFile(QString("MainWindow: Converted path: %1").arg(path));

        // Find editor for this file
        CodeEditor* editor = m_editorTabs->editorForFile(path);
        logToFile(QString("MainWindow: editorForFile(%1) returned %2").arg(path).arg(editor ? "editor" : "null"));

#ifdef Q_OS_WIN
        // Windows: try both cases for drive letter
        if (!editor) {
            // Try with lowercase drive letter
            QString altPath = path;
            if (altPath.length() > 1 && altPath[1] == ':') {
                altPath[0] = altPath[0].toLower();
                editor = m_editorTabs->editorForFile(altPath);
                logToFile(QString("MainWindow: editorForFile(%1) returned %2").arg(altPath).arg(editor ? "editor" : "null"));
            }
        }
        if (!editor) {
            // Try with uppercase drive letter
            QString altPath = path;
            if (altPath.length() > 1 && altPath[1] == ':') {
                altPath[0] = altPath[0].toUpper();
                editor = m_editorTabs->editorForFile(altPath);
                logToFile(QString("MainWindow: editorForFile(%1) returned %2").arg(altPath).arg(editor ? "editor" : "null"));
            }
        }
#endif
        if (editor) {
            logToFile(QString("MainWindow: Calling showCompletions with %1 items").arg(items.size()));
            editor->showCompletions(items);
        } else {
            logToFile(QString("MainWindow: Editor not found for any path variant"));
        }
    });

    connect(m_lspClient, &LSPClient::diagnosticsReceived, this, [this](const QString& uri, const QList<LSPDiagnostic>& diagnostics) {
        logToFile(QString("LSP: Received %1 diagnostics for %2").arg(diagnostics.size()).arg(uri));

        // Convert URI to file path
        QString path = uri;
        if (path.startsWith("file:///")) {
            path = path.mid(8);
        }
#ifdef Q_OS_WIN
        // Windows: convert forward slashes to backslashes
        path.replace("/", "\\");
#endif

        // Find editor for this file
        CodeEditor* editor = m_editorTabs->editorForFile(path);
#ifdef Q_OS_WIN
        // Windows: try both cases for drive letter
        if (!editor) {
            QString altPath = path;
            if (altPath.length() > 1 && altPath[1] == ':') {
                altPath[0] = altPath[0].toLower();
                editor = m_editorTabs->editorForFile(altPath);
            }
        }
        if (!editor) {
            QString altPath = path;
            if (altPath.length() > 1 && altPath[1] == ':') {
                altPath[0] = altPath[0].toUpper();
                editor = m_editorTabs->editorForFile(altPath);
            }
        }
#endif
        if (editor) {
            // Convert LSP diagnostics to editor diagnostics
            QList<Diagnostic> editorDiagnostics;
            for (const LSPDiagnostic& lspDiag : diagnostics) {
                Diagnostic diag;
                diag.startLine = lspDiag.range.start.line + 1;  // LSP is 0-based
                diag.startColumn = lspDiag.range.start.character + 1;
                diag.endLine = lspDiag.range.end.line + 1;
                diag.endColumn = lspDiag.range.end.character + 1;
                diag.message = lspDiag.message;

                switch (lspDiag.severity) {
                    case DiagnosticSeverity::Error:
                        diag.severity = Diagnostic::Severity::Error;
                        break;
                    case DiagnosticSeverity::Warning:
                        diag.severity = Diagnostic::Severity::Warning;
                        break;
                    case DiagnosticSeverity::Information:
                        diag.severity = Diagnostic::Severity::Info;
                        break;
                    case DiagnosticSeverity::Hint:
                        diag.severity = Diagnostic::Severity::Hint;
                        break;
                }
                editorDiagnostics.append(diag);
            }
            editor->setDiagnostics(editorDiagnostics);
        }

        // Also update Problems Panel
        // First clear existing problems for this file, then add new ones
        m_problemsPanel->clearProblemsForFile(path);
        for (const LSPDiagnostic& lspDiag : diagnostics) {
            QString severity;
            switch (lspDiag.severity) {
                case DiagnosticSeverity::Error:
                    severity = "Error";
                    break;
                case DiagnosticSeverity::Warning:
                    severity = "Warning";
                    break;
                case DiagnosticSeverity::Information:
                    severity = "Info";
                    break;
                case DiagnosticSeverity::Hint:
                    severity = "Hint";
                    break;
            }
            m_problemsPanel->addProblem(
                path,
                lspDiag.range.start.line + 1,
                lspDiag.range.start.character + 1,
                severity,
                lspDiag.message
            );
        }
    });

    connect(m_lspClient, &LSPClient::documentSymbolsReceived, this, [this](const QString& uri, const QList<LSPDocumentSymbol>& symbols) {
        // Convert LSP symbols to outline panel symbols
        QList<DocumentSymbol> outlineSymbols;

        std::function<DocumentSymbol(const LSPDocumentSymbol&)> convertSymbol = [&](const LSPDocumentSymbol& lspSym) -> DocumentSymbol {
            DocumentSymbol sym;
            sym.name = lspSym.name;
            sym.line = lspSym.selectionRange.start.line + 1;
            sym.column = lspSym.selectionRange.start.character + 1;
            sym.endLine = lspSym.range.end.line + 1;
            sym.endColumn = lspSym.range.end.character + 1;

            // Convert LSP symbol kind to outline kind
            switch (lspSym.kind) {
                case LSPSymbolKind::File: sym.kind = DocumentSymbol::File; break;
                case LSPSymbolKind::Module: sym.kind = DocumentSymbol::Module; break;
                case LSPSymbolKind::Namespace: sym.kind = DocumentSymbol::Namespace; break;
                case LSPSymbolKind::Package: sym.kind = DocumentSymbol::Package; break;
                case LSPSymbolKind::Class: sym.kind = DocumentSymbol::Class; break;
                case LSPSymbolKind::Method: sym.kind = DocumentSymbol::Method; break;
                case LSPSymbolKind::Property: sym.kind = DocumentSymbol::Property; break;
                case LSPSymbolKind::Field: sym.kind = DocumentSymbol::Field; break;
                case LSPSymbolKind::Constructor: sym.kind = DocumentSymbol::Constructor; break;
                case LSPSymbolKind::Enum: sym.kind = DocumentSymbol::Enum; break;
                case LSPSymbolKind::Interface: sym.kind = DocumentSymbol::Interface; break;
                case LSPSymbolKind::Function: sym.kind = DocumentSymbol::Function; break;
                case LSPSymbolKind::Variable: sym.kind = DocumentSymbol::Variable; break;
                case LSPSymbolKind::Constant: sym.kind = DocumentSymbol::Constant; break;
                case LSPSymbolKind::Struct: sym.kind = DocumentSymbol::Struct; break;
                case LSPSymbolKind::Event: sym.kind = DocumentSymbol::Event; break;
                default: sym.kind = DocumentSymbol::Variable; break;
            }

            for (const auto& child : lspSym.children) {
                sym.children.append(convertSymbol(child));
            }
            return sym;
        };

        for (const auto& lspSym : symbols) {
            outlineSymbols.append(convertSymbol(lspSym));
        }

        m_outlinePanel->setSymbols(outlineSymbols);
    });

    // Outline panel signals
    connect(m_outlinePanel, &OutlinePanel::symbolDoubleClicked, this, [this](int line, int column) {
        CodeEditor* editor = m_editorTabs->currentEditor();
        if (editor) {
            editor->goToPosition(line, column);
            editor->setFocus();
        }
    });

    // Settings connections
    Settings* settings = Application::instance()->settings();
    connect(settings, &Settings::syntaxThemeChanged, this, [this](int themeIndex) {
        SyntaxTheme theme = static_cast<SyntaxTheme>(themeIndex);
        // Update all open editors
        for (int i = 0; i < m_editorTabs->count(); ++i) {
            CodeEditor* editor = m_editorTabs->editorAt(i);
            if (editor) {
                editor->setSyntaxTheme(theme);
            }
        }
    });

    // Git status indicator click -> show Git Changes panel
    connect(m_gitStatusIndicator, &GitStatusIndicator::clicked, this, [this]() {
        if (m_gitChangesDock) {
            m_gitChangesDock->raise();
            m_gitChangesDock->show();
        }
    });

    // Git manager signals
    connect(m_gitManager, &GitManager::operationStarted, this, [this](const QString& operation) {
        statusBar()->showMessage(tr("Git: %1...").arg(operation), 0);
    });

    connect(m_gitManager, &GitManager::operationError, this, [this](const QString& error) {
        statusBar()->showMessage(tr("Git error: %1").arg(error), 5000);
    });

    connect(m_gitManager, &GitManager::commitCompleted, this, [this](bool success, const QString& hash, const QString& error) {
        if (success) {
            statusBar()->showMessage(tr("Committed: %1").arg(hash), 5000);
            QMessageBox::information(this, tr("Commit Successful"),
                                     tr("Changes have been committed.\n\nCommit hash: %1").arg(hash));
        } else {
            QMessageBox::warning(this, tr("Commit Failed"), error);
        }
    });

    connect(m_gitManager, &GitManager::pushCompleted, this, [this](bool success, const QString& error) {
        if (success) {
            statusBar()->showMessage(tr("Push completed successfully"), 5000);
            QMessageBox::information(this, tr("Push Successful"),
                                     tr("Changes have been pushed to the remote repository."));
        } else {
            QMessageBox::warning(this, tr("Push Failed"), error);
        }
    });

    connect(m_gitManager, &GitManager::pullCompleted, this, [this](bool success, const QString& error) {
        if (success) {
            statusBar()->showMessage(tr("Pull completed successfully"), 5000);
            QMessageBox::information(this, tr("Pull Successful"),
                                     tr("Changes have been pulled from the remote repository."));
        } else {
            QMessageBox::warning(this, tr("Pull Failed"), error);
        }
    });

    connect(m_gitManager, &GitManager::fetchCompleted, this, [this](bool success, const QString& error) {
        if (success) {
            statusBar()->showMessage(tr("Fetch completed successfully"), 5000);
        } else {
            QMessageBox::warning(this, tr("Fetch Failed"), error);
        }
    });

    connect(m_gitManager, &GitManager::remoteAdded, this, [this](bool success, const QString& name, const QString& error) {
        qDebug() << "[MainWindow] remoteAdded: success=" << success << "name=" << name << "error=" << error;
        if (success) {
            statusBar()->showMessage(tr("Remote '%1' added, pushing...").arg(name), 5000);
        }
        // Note: errors are handled by GitChangesPanel::onRemoteAdded
    });

    connect(m_gitManager, &GitManager::operationProgress, this, [this](const QString& message) {
        statusBar()->showMessage(message, 3000);
    });

    // Stage/Unstage operations
    connect(m_gitManager, &GitManager::stageCompleted, this, [this](bool success, const QString& error) {
        if (success) {
            statusBar()->showMessage(tr("Files staged successfully"), 3000);
        } else {
            QMessageBox::warning(this, tr("Stage Failed"), error);
        }
    });

    connect(m_gitManager, &GitManager::unstageCompleted, this, [this](bool success, const QString& error) {
        if (success) {
            statusBar()->showMessage(tr("Files unstaged successfully"), 3000);
        } else {
            QMessageBox::warning(this, tr("Unstage Failed"), error);
        }
    });

    connect(m_gitManager, &GitManager::discardCompleted, this, [this](bool success, const QString& error) {
        if (success) {
            statusBar()->showMessage(tr("Changes discarded"), 3000);
            QMessageBox::information(this, tr("Changes Discarded"),
                                     tr("Selected changes have been discarded."));
        } else {
            QMessageBox::warning(this, tr("Discard Failed"), error);
        }
    });

    // Branch operations
    connect(m_gitManager, &GitManager::branchCheckoutCompleted, this, [this](bool success, const QString& branch, const QString& error) {
        if (success) {
            statusBar()->showMessage(tr("Switched to branch: %1").arg(branch), 5000);
            QMessageBox::information(this, tr("Branch Switched"),
                                     tr("Switched to branch '%1'.").arg(branch));
        } else {
            QMessageBox::warning(this, tr("Checkout Failed"), error);
        }
    });

    connect(m_gitManager, &GitManager::branchCreated, this, [this](bool success, const QString& branch, const QString& error) {
        if (success) {
            statusBar()->showMessage(tr("Branch '%1' created").arg(branch), 5000);
            QMessageBox::information(this, tr("Branch Created"),
                                     tr("Branch '%1' has been created.").arg(branch));
        } else {
            QMessageBox::warning(this, tr("Create Branch Failed"), error);
        }
    });

    connect(m_gitManager, &GitManager::branchDeleted, this, [this](bool success, const QString& branch, const QString& error) {
        if (success) {
            statusBar()->showMessage(tr("Branch '%1' deleted").arg(branch), 5000);
            QMessageBox::information(this, tr("Branch Deleted"),
                                     tr("Branch '%1' has been deleted.").arg(branch));
        } else {
            QMessageBox::warning(this, tr("Delete Branch Failed"), error);
        }
    });

    // Remote operations
    connect(m_gitManager, &GitManager::remoteRemoved, this, [this](bool success, const QString& name, const QString& error) {
        if (success) {
            statusBar()->showMessage(tr("Remote '%1' removed").arg(name), 5000);
            QMessageBox::information(this, tr("Remote Removed"),
                                     tr("Remote '%1' has been removed.").arg(name));
        } else {
            QMessageBox::warning(this, tr("Remove Remote Failed"), error);
        }
    });
}

void MainWindow::saveWindowState()
{
    Settings* settings = Application::instance()->settings();
    settings->setWindowGeometry(saveGeometry());
    settings->setWindowState(saveState());
    settings->setWindowMaximized(isMaximized());
    settings->sync();
}

void MainWindow::restoreWindowState()
{
    Settings* settings = Application::instance()->settings();

    QByteArray geometry = settings->windowGeometry();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    } else {
        resize(settings->windowSize());
        move(settings->windowPosition());
    }

    QByteArray state = settings->windowState();
    if (!state.isEmpty()) {
        restoreState(state);
    }

    if (settings->windowMaximized()) {
        showMaximized();
    }
}

void MainWindow::updateStatusBarColor(IDEState state)
{
    m_ideState = state;

    QString color;
    switch (state) {
        case IDEState::Idle:
            color = "#68217A";  // Purple
            break;
        case IDEState::ProjectLoaded:
        case IDEState::Building:
            color = "#007ACC";  // Blue
            break;
        case IDEState::Running:
        case IDEState::Debugging:
            color = "#CA5100";  // Orange
            break;
    }

    statusBar()->setStyleSheet(QString("QStatusBar { background-color: %1; color: #FFFFFF; }").arg(color));
}

void MainWindow::updateLineEndingsLabel()
{
    CodeEditor* editor = m_editorTabs->currentEditor();
    if (!editor) {
        m_lineEndingsLabel->setText(tr("CRLF"));
        return;
    }

    // Check the document for line endings
    QString text = editor->toPlainText();
    if (text.contains("\r\n")) {
        m_lineEndingsLabel->setText(tr("CRLF"));
    } else if (text.contains("\r")) {
        m_lineEndingsLabel->setText(tr("CR"));
    } else {
        m_lineEndingsLabel->setText(tr("LF"));
    }
}

void MainWindow::setCompilationEntrypoint(const QString& path)
{
    Project* project = m_projectManager->currentProject();
    if (!project) {
        QMessageBox::warning(this, tr("No Project"),
            tr("No project is currently open."));
        return;
    }

    // Convert absolute path to relative path from project directory
    QString projectDir = project->projectDir();
    QString relativePath = path;
    if (path.startsWith(projectDir)) {
        relativePath = path.mid(projectDir.length());
        if (relativePath.startsWith('/') || relativePath.startsWith('\\')) {
            relativePath = relativePath.mid(1);
        }
    }

    project->setCompilationEntryPoint(relativePath);
    project->save();

    // Update the file decorator to show entrypoint icon
    if (m_gitFileDecorator) {
        m_gitFileDecorator->setCompilationEntrypoint(relativePath);
    }

    m_buildOutputPanel->appendText(
        tr("Compilation entrypoint set to: %1\n").arg(relativePath));
}

void MainWindow::checkResumeProject()
{
    Settings* settings = Application::instance()->settings();

    // Check if we should prompt
    if (!settings->promptToResumeProject()) {
        return;
    }

    QString lastProject = settings->lastOpenedProject();
    QStringList recentProjects = settings->recentProjects();

    // Only show dialog if there are recent projects
    if (lastProject.isEmpty() && recentProjects.isEmpty()) {
        return;
    }

    // Filter out non-existent projects
    bool hasValidProjects = false;
    if (!lastProject.isEmpty() && QFileInfo::exists(lastProject)) {
        hasValidProjects = true;
    }
    for (const QString& p : recentProjects) {
        if (QFileInfo::exists(p)) {
            hasValidProjects = true;
            break;
        }
    }

    if (!hasValidProjects) {
        return;
    }

    ResumeProjectDialog dialog(lastProject, recentProjects, this);
    if (dialog.exec() == QDialog::Accepted) {
        // Handle "don't ask again"
        if (dialog.dontAskAgain()) {
            settings->setPromptToResumeProject(false);
            settings->sync();
        }

        // Open selected project
        QString selectedProject = dialog.selectedProject();
        if (!selectedProject.isEmpty()) {
            openProject(selectedProject);
        }
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // TODO: Check for unsaved changes

    saveWindowState();
    event->accept();
}

// =============================================================================
// File operations
// =============================================================================

void MainWindow::openFile(const QString& path)
{
    m_editorTabs->openFile(path);
}

void MainWindow::openProject(const QString& path)
{
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        return;
    }

    // Close all existing tabs when loading a new project
    m_editorTabs->closeAllFiles();

    m_projectManager->openProject(path);

    // Set LSP project root to the project directory
    QString projectDir = QFileInfo(path).absolutePath();
    m_lspClient->setProjectRoot(projectDir);

    // Set compilation entrypoint icon in file decorator
    Project* project = m_projectManager->currentProject();

    // Set include paths for LSP (#import resolution)
    QStringList includePaths;
    includePaths << projectDir;
    QString libraryPath = projectDir + "/Library";
    if (QDir(libraryPath).exists()) {
        includePaths << libraryPath;
        // Add each dependency's Library subfolder
        if (project) {
            for (const Dependency& dep : project->dependencies()) {
                if (!dep.localPath.isEmpty()) {
                    includePaths << dep.localPath;
                }
            }
        }
    }
    m_lspClient->setIncludePaths(includePaths);
    m_lspClient->restart();  // Restart LSP with new -I arguments

    // Set Git repository path
    if (m_gitManager) {
        m_gitManager->setRepositoryPath(projectDir);
    }
    if (project && m_gitFileDecorator) {
        m_gitFileDecorator->setCompilationEntrypoint(project->compilationEntryPoint());
    }

    // Save to recent projects and last opened
    Settings* settings = Application::instance()->settings();
    settings->addRecentProject(path);
    settings->setLastOpenedProject(path);
    settings->sync();

    // Update the recent projects menu
    updateRecentProjectsMenu();

    // Show status message
    QFileInfo info(path);
    statusBar()->showMessage(tr("Opened project: %1").arg(info.baseName()), 5000);
}

void MainWindow::newFile()
{
    m_editorTabs->newFile();
}

void MainWindow::newProject()
{
    NewProjectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString projectName = dialog.projectName();
        QString projectDir = dialog.projectLocation() + "/" + projectName;
        QString projectPath = projectDir + "/" + projectName + ".xxmlp";
        QString projectType = dialog.projectType();

        // Determine project type
        Project::Type type = (projectType == "library") ? Project::Type::Library : Project::Type::Executable;

        // Create project directory
        QDir().mkpath(projectDir);
        QDir().mkpath(projectDir + "/src");

        // Create the project file
        if (m_projectManager->createProject(projectPath, projectName, type)) {

            if (type == Project::Type::Executable) {
                // Create executable entry point file
                QString mainFile = projectDir + "/src/Main.XXML";
                QFile file(mainFile);
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream out(&file);
                    out << "// Main entry point for " << projectName << "\n";
                    out << "// This is an executable project\n\n";
                    out << "[ Namespace <" << projectName << ">\n";
                    out << "    [ Class <Main> Final Extends None\n";
                    out << "        [ Public <>\n";
                    out << "            Entrypoint <Main> Parameters () -> {\n";
                    out << "                // Your code here\n";
                    out << "                Print(\"Hello from " << projectName << "!\");\n";
                    out << "            }\n";
                    out << "        ]\n";
                    out << "    ]\n";
                    out << "]\n";
                    file.close();
                }

                // Open the main file
                openFile(mainFile);

            } else {
                // Create library example file
                QString libFile = projectDir + "/src/" + projectName + ".XXML";
                QFile file(libFile);
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    QTextStream out(&file);
                    out << "// Library: " << projectName << "\n";
                    out << "// This is a library project - no entry point\n\n";
                    out << "[ Namespace <" << projectName << ">\n";
                    out << "    [ Class <" << projectName << "> Final Extends None\n";
                    out << "        [ Private <>\n";
                    out << "            // Private members\n";
                    out << "        ]\n";
                    out << "        [ Public <>\n";
                    out << "            // Public API\n";
                    out << "            Method <Example> Parameters () -> {\n";
                    out << "                // Example method\n";
                    out << "            }\n";
                    out << "        ]\n";
                    out << "    ]\n";
                    out << "]\n";
                    file.close();
                }

                // Open the library file
                openFile(libFile);
            }

            // Set LSP project root
            m_lspClient->setProjectRoot(projectDir);

            // Set include paths for LSP (#import resolution)
            Project* newProject = m_projectManager->currentProject();
            QStringList includePaths;
            includePaths << projectDir;
            QString libraryPath = projectDir + "/Library";
            if (QDir(libraryPath).exists()) {
                includePaths << libraryPath;
                // Add each dependency's Library subfolder
                if (newProject) {
                    for (const Dependency& dep : newProject->dependencies()) {
                        if (!dep.localPath.isEmpty()) {
                            includePaths << dep.localPath;
                        }
                    }
                }
            }
            m_lspClient->setIncludePaths(includePaths);
            m_lspClient->restart();  // Restart LSP with new -I arguments

            // Set Git repository path
            if (m_gitManager) {
                m_gitManager->setRepositoryPath(projectDir);
            }

            // Update recent projects
            updateRecentProjectsMenu();

            QString typeStr = (type == Project::Type::Library) ? tr("library") : tr("executable");
            statusBar()->showMessage(tr("Created %1 project: %2").arg(typeStr, projectName), 5000);
        }
    }
}

void MainWindow::openFileDialog()
{
    QString path = QFileDialog::getOpenFileName(this,
        tr("Open File"),
        QString(),
        tr("XXML Files (*.XXML *.xxml);;All Files (*)"));

    if (!path.isEmpty()) {
        openFile(path);
    }
}

void MainWindow::openProjectDialog()
{
    QString path = QFileDialog::getOpenFileName(this,
        tr("Open Project"),
        QString(),
        tr("XXML Project Files (*.xxmlp);;All Files (*)"));

    if (!path.isEmpty()) {
        openProject(path);
    }
}

void MainWindow::saveFile()
{
    m_editorTabs->saveFile();
}

void MainWindow::saveFileAs()
{
    m_editorTabs->saveFileAs();
}

void MainWindow::saveAll()
{
    m_editorTabs->saveAllFiles();
}

void MainWindow::closeFile()
{
    m_editorTabs->closeFile();
}

// =============================================================================
// Edit operations
// =============================================================================

void MainWindow::undo()
{
    m_editorTabs->undo();
}

void MainWindow::redo()
{
    m_editorTabs->redo();
}

void MainWindow::cut()
{
    m_editorTabs->cut();
}

void MainWindow::copy()
{
    m_editorTabs->copy();
}

void MainWindow::paste()
{
    m_editorTabs->paste();
}

void MainWindow::selectAll()
{
    m_editorTabs->selectAll();
}

void MainWindow::findReplace()
{
    if (!m_findReplaceDialog) {
        m_findReplaceDialog = new FindReplaceDialog(this);

        connect(m_findReplaceDialog, &FindReplaceDialog::findNext, this, [this]() {
            CodeEditor* editor = m_editorTabs->currentEditor();
            if (!editor) return;

            QString text = m_findReplaceDialog->searchText();
            bool found = editor->findNext(text,
                                          m_findReplaceDialog->caseSensitive(),
                                          m_findReplaceDialog->wholeWord(),
                                          m_findReplaceDialog->useRegex());
            if (found) {
                statusBar()->showMessage(tr("Found: %1").arg(text), 2000);
            } else {
                statusBar()->showMessage(tr("Not found: %1").arg(text), 2000);
            }
        });

        connect(m_findReplaceDialog, &FindReplaceDialog::findPrevious, this, [this]() {
            CodeEditor* editor = m_editorTabs->currentEditor();
            if (!editor) return;

            QString text = m_findReplaceDialog->searchText();
            bool found = editor->findPrevious(text,
                                              m_findReplaceDialog->caseSensitive(),
                                              m_findReplaceDialog->wholeWord(),
                                              m_findReplaceDialog->useRegex());
            if (found) {
                statusBar()->showMessage(tr("Found: %1").arg(text), 2000);
            } else {
                statusBar()->showMessage(tr("Not found: %1").arg(text), 2000);
            }
        });

        connect(m_findReplaceDialog, &FindReplaceDialog::replace, this, [this]() {
            CodeEditor* editor = m_editorTabs->currentEditor();
            if (!editor) return;

            if (editor->replaceCurrent(m_findReplaceDialog->replaceText())) {
                // Find next occurrence after replacement
                editor->findNext(m_findReplaceDialog->searchText(),
                                m_findReplaceDialog->caseSensitive(),
                                m_findReplaceDialog->wholeWord(),
                                m_findReplaceDialog->useRegex());
                statusBar()->showMessage(tr("Replaced"), 2000);
            }
        });

        connect(m_findReplaceDialog, &FindReplaceDialog::replaceAll, this, [this]() {
            CodeEditor* editor = m_editorTabs->currentEditor();
            if (!editor) return;

            int count = editor->replaceAll(m_findReplaceDialog->searchText(),
                                           m_findReplaceDialog->replaceText(),
                                           m_findReplaceDialog->caseSensitive(),
                                           m_findReplaceDialog->wholeWord(),
                                           m_findReplaceDialog->useRegex());
            statusBar()->showMessage(tr("Replaced %1 occurrences").arg(count), 2000);
        });
    }

    // Set search text from current selection
    CodeEditor* editor = m_editorTabs->currentEditor();
    if (editor && editor->textCursor().hasSelection()) {
        m_findReplaceDialog->setSearchText(editor->textCursor().selectedText());
    }

    m_findReplaceDialog->show();
    m_findReplaceDialog->raise();
    m_findReplaceDialog->activateWindow();
}

void MainWindow::goToLine()
{
    CodeEditor* editor = m_editorTabs->currentEditor();
    if (!editor) {
        return;
    }

    GoToLineDialog dialog(this);
    dialog.setMaxLine(editor->document()->blockCount());
    dialog.setCurrentLine(editor->textCursor().blockNumber() + 1);

    if (dialog.exec() == QDialog::Accepted) {
        int line = dialog.selectedLine();
        QTextCursor cursor(editor->document()->findBlockByLineNumber(line - 1));
        editor->setTextCursor(cursor);
        editor->centerCursor();
    }
}

// =============================================================================
// Build operations
// =============================================================================

void MainWindow::buildProject()
{
    if (!m_projectManager->hasProject()) {
        QMessageBox::warning(this, tr("Build"), tr("No project is open."));
        return;
    }

    m_buildOutputPanel->clear();
    m_problemsPanel->clear();
    m_buildManager->build(m_projectManager->currentProject());
}

void MainWindow::rebuildProject()
{
    if (!m_projectManager->hasProject()) {
        QMessageBox::warning(this, tr("Rebuild"), tr("No project is open."));
        return;
    }

    m_buildOutputPanel->clear();
    m_problemsPanel->clear();
    m_buildManager->rebuild(m_projectManager->currentProject());
}

void MainWindow::cleanProject()
{
    if (!m_projectManager->hasProject()) {
        QMessageBox::warning(this, tr("Clean"), tr("No project is open."));
        return;
    }

    m_buildOutputPanel->clear();
    m_buildManager->clean(m_projectManager->currentProject());
    statusBar()->showMessage(tr("Project cleaned"), 3000);
}

void MainWindow::cancelBuild()
{
    m_buildManager->cancel();
    statusBar()->showMessage(tr("Build cancelled"), 3000);
}

void MainWindow::runProject()
{
    if (!m_projectManager->hasProject()) {
        QMessageBox::warning(this, tr("Run"), tr("No project is open."));
        return;
    }

    // Build first, then run
    if (!m_buildManager->isBuilding()) {
        // Connect to buildFinished to run after build
        QMetaObject::Connection* conn = new QMetaObject::Connection();
        *conn = connect(m_buildManager, &BuildManager::buildFinished, this, [this, conn](bool success) {
            disconnect(*conn);
            delete conn;
            if (success) {
                runWithoutBuilding();
            }
        });

        buildProject();
    }
}

void MainWindow::runWithoutBuilding()
{
    if (!m_projectManager->hasProject()) {
        QMessageBox::warning(this, tr("Run"), tr("No project is open."));
        return;
    }

    Project* project = m_projectManager->currentProject();
    BuildConfiguration* config = project->activeConfiguration();

    QString outputDir = project->projectDir() + "/" +
                        (config ? config->outputDir : project->outputDir());
    QString executable = outputDir + "/" + project->name();
#ifdef Q_OS_WIN
    if (project->type() == Project::Type::Executable) {
        executable += ".exe";
    }
#endif

    if (!QFileInfo::exists(executable)) {
        QMessageBox::warning(this, tr("Run"),
            tr("Executable not found: %1\n\nPlease build the project first.").arg(executable));
        return;
    }

    m_terminalPanel->clear();
    m_processRunner->run(executable, QStringList(), project->projectDir());
}

// =============================================================================
// View operations
// =============================================================================

void MainWindow::resetLayout()
{
    // Reset to default layout
    m_projectExplorerDock->setVisible(true);
    m_outlineDock->setVisible(true);
    m_gitChangesDock->setVisible(true);
    m_problemsDock->setVisible(true);
    m_buildOutputDock->setVisible(true);
    m_terminalDock->setVisible(true);
    m_gitHistoryDock->setVisible(true);

    // Re-arrange docks
    addDockWidget(Qt::LeftDockWidgetArea, m_projectExplorerDock);
    tabifyDockWidget(m_projectExplorerDock, m_outlineDock);
    tabifyDockWidget(m_outlineDock, m_gitChangesDock);
    m_projectExplorerDock->raise();

    addDockWidget(Qt::BottomDockWidgetArea, m_problemsDock);
    tabifyDockWidget(m_problemsDock, m_buildOutputDock);
    tabifyDockWidget(m_buildOutputDock, m_terminalDock);
    tabifyDockWidget(m_terminalDock, m_gitHistoryDock);
    m_problemsDock->raise();

    resizeDocks({m_projectExplorerDock}, {250}, Qt::Horizontal);
    resizeDocks({m_problemsDock}, {200}, Qt::Vertical);

    statusBar()->showMessage(tr("Layout reset to default"), 3000);
}

// =============================================================================
// Bookmark operations
// =============================================================================

void MainWindow::toggleBookmark()
{
    CodeEditor* editor = m_editorTabs->currentEditor();
    if (!editor || editor->filePath().isEmpty()) {
        return;
    }

    int line = editor->currentLine();
    QString lineText = editor->document()->findBlockByNumber(line - 1).text();
    m_bookmarkManager->toggleBookmark(editor->filePath(), line, lineText);
}

void MainWindow::nextBookmark()
{
    CodeEditor* editor = m_editorTabs->currentEditor();
    if (!editor) {
        return;
    }

    QString currentFile = editor->filePath();
    int currentLine = editor->currentLine();

    Bookmark next = m_bookmarkManager->nextBookmark(currentFile, currentLine);
    if (!next.filePath.isEmpty()) {
        if (next.filePath != currentFile) {
            // Open the file if it's different
            openFile(next.filePath);
        }
        CodeEditor* targetEditor = m_editorTabs->editorForFile(next.filePath);
        if (targetEditor) {
            targetEditor->goToLine(next.line);
        }
    }
}

void MainWindow::previousBookmark()
{
    CodeEditor* editor = m_editorTabs->currentEditor();
    if (!editor) {
        return;
    }

    QString currentFile = editor->filePath();
    int currentLine = editor->currentLine();

    Bookmark prev = m_bookmarkManager->previousBookmark(currentFile, currentLine);
    if (!prev.filePath.isEmpty()) {
        if (prev.filePath != currentFile) {
            // Open the file if it's different
            openFile(prev.filePath);
        }
        CodeEditor* targetEditor = m_editorTabs->editorForFile(prev.filePath);
        if (targetEditor) {
            targetEditor->goToLine(prev.line);
        }
    }
}

} // namespace XXMLStudio
