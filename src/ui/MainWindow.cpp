#include "MainWindow.h"
#include "core/Application.h"
#include "core/Settings.h"
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
#include "lsp/LSPClient.h"
#include "lsp/LSPProtocol.h"
#include "dialogs/NewProjectDialog.h"
#include "dialogs/GoToLineDialog.h"
#include "dialogs/FindReplaceDialog.h"
#include "dialogs/SettingsDialog.h"

#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <functional>
#include <QFile>
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
    setWindowTitle("XXML Studio");
    setMinimumSize(800, 600);

    setupUi();
    restoreWindowState();
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

    createActions();
    setupMenuBar();
    setupToolBar();
    setupCentralWidget();
    setupDockWidgets();
    setupStatusBar();
    setupConnections();

    // Start LSP client with bundled server
    QString lspPath = QDir::toNativeSeparators(
        QCoreApplication::applicationDirPath() + "/toolchain/bin/xxml-lsp.exe");
    if (QFileInfo::exists(lspPath)) {
        m_lspClient->start(lspPath);
    } else {
        statusBar()->showMessage(tr("LSP server not found: %1").arg(lspPath), 5000);
    }
}

void MainWindow::createActions()
{
    // File actions
    m_newFileAction = new QAction(tr("New File"), this);
    m_newFileAction->setShortcut(QKeySequence::New);

    m_newProjectAction = new QAction(tr("New Project..."), this);
    m_newProjectAction->setShortcut(QKeySequence("Ctrl+Shift+N"));

    m_openFileAction = new QAction(tr("Open File..."), this);
    m_openFileAction->setShortcut(QKeySequence::Open);

    m_openProjectAction = new QAction(tr("Open Project..."), this);
    m_openProjectAction->setShortcut(QKeySequence("Ctrl+Shift+O"));

    m_saveAction = new QAction(tr("Save"), this);
    m_saveAction->setShortcut(QKeySequence::Save);

    m_saveAsAction = new QAction(tr("Save As..."), this);
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);

    m_saveAllAction = new QAction(tr("Save All"), this);
    m_saveAllAction->setShortcut(QKeySequence("Ctrl+Shift+S"));

    m_closeFileAction = new QAction(tr("Close"), this);
    m_closeFileAction->setShortcut(QKeySequence::Close);

    m_exitAction = new QAction(tr("Exit"), this);
    m_exitAction->setShortcut(QKeySequence::Quit);

    // Edit actions
    m_undoAction = new QAction(tr("Undo"), this);
    m_undoAction->setShortcut(QKeySequence::Undo);

    m_redoAction = new QAction(tr("Redo"), this);
    m_redoAction->setShortcut(QKeySequence::Redo);

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

    m_rebuildAction = new QAction(tr("Rebuild Project"), this);
    m_rebuildAction->setShortcut(QKeySequence("Ctrl+Shift+B"));

    m_cleanAction = new QAction(tr("Clean Project"), this);

    m_cancelBuildAction = new QAction(tr("Cancel Build"), this);
    m_cancelBuildAction->setShortcut(QKeySequence("Ctrl+Pause"));
    m_cancelBuildAction->setEnabled(false);

    // Run actions
    m_runAction = new QAction(tr("Run"), this);
    m_runAction->setShortcut(QKeySequence("F5"));

    m_runWithoutBuildAction = new QAction(tr("Run Without Building"), this);
    m_runWithoutBuildAction->setShortcut(QKeySequence("Ctrl+F5"));
}

void MainWindow::setupMenuBar()
{
    createFileMenu();
    createEditMenu();
    createViewMenu();
    createBuildMenu();
    createRunMenu();
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
    QMenu* recentMenu = fileMenu->addMenu(tr("Recent Projects"));
    recentMenu->addAction(tr("(No recent projects)"))->setEnabled(false);

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
    viewMenu->addAction(tr("Problems"), this, [this]() {
        m_problemsDock->setVisible(!m_problemsDock->isVisible());
    });
    viewMenu->addAction(tr("Build Output"), this, [this]() {
        m_buildOutputDock->setVisible(!m_buildOutputDock->isVisible());
    });
    viewMenu->addAction(tr("Terminal"), this, [this]() {
        m_terminalDock->setVisible(!m_terminalDock->isVisible());
    });

    viewMenu->addSeparator();

    QAction* resetLayoutAction = viewMenu->addAction(tr("Reset Layout"));
    connect(resetLayoutAction, &QAction::triggered, this, &MainWindow::resetLayout);
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

void MainWindow::setupToolBar()
{
    m_mainToolBar = addToolBar(tr("Main"));
    m_mainToolBar->setObjectName("MainToolBar");
    m_mainToolBar->setMovable(false);

    m_mainToolBar->addAction(m_newFileAction);
    m_mainToolBar->addAction(m_openFileAction);
    m_mainToolBar->addAction(m_saveAction);
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(m_undoAction);
    m_mainToolBar->addAction(m_redoAction);
    m_mainToolBar->addSeparator();
    m_mainToolBar->addAction(m_buildAction);
    m_mainToolBar->addAction(m_runAction);
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

    // Set initial sizes
    resizeDocks({m_projectExplorerDock}, {250}, Qt::Horizontal);
    resizeDocks({m_problemsDock}, {200}, Qt::Vertical);
}

void MainWindow::setupStatusBar()
{
    m_cursorPositionLabel = new QLabel(tr("Ln 1, Col 1"), this);
    m_cursorPositionLabel->setMinimumWidth(100);

    m_encodingLabel = new QLabel(tr("UTF-8"), this);
    m_encodingLabel->setMinimumWidth(60);

    m_lspStatusLabel = new QLabel(tr("LSP: Disconnected"), this);
    m_lspStatusLabel->setMinimumWidth(120);

    statusBar()->addPermanentWidget(m_cursorPositionLabel);
    statusBar()->addPermanentWidget(m_encodingLabel);
    statusBar()->addPermanentWidget(m_lspStatusLabel);

    statusBar()->showMessage(tr("Ready"));
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

    // Run actions
    connect(m_runAction, &QAction::triggered, this, &MainWindow::runProject);
    connect(m_runWithoutBuildAction, &QAction::triggered, this, &MainWindow::runWithoutBuilding);

    // Project manager signals
    connect(m_projectManager, &ProjectManager::projectOpened, this, [this](Project* project) {
        setWindowTitle(QString("XXML Studio - %1").arg(project->name()));
        m_projectExplorer->setRootPath(project->projectDir());
        statusBar()->showMessage(tr("Project opened: %1").arg(project->name()), 3000);
    });

    connect(m_projectManager, &ProjectManager::projectClosed, this, [this]() {
        setWindowTitle("XXML Studio");
        m_projectExplorer->setRootPath(QString());
        statusBar()->showMessage(tr("Project closed"), 3000);
    });

    connect(m_projectManager, &ProjectManager::error, this, [this](const QString& message) {
        QMessageBox::warning(this, tr("Project Error"), message);
    });

    // Project explorer signals
    connect(m_projectExplorer, &ProjectExplorer::fileDoubleClicked, this, &MainWindow::openFile);

    // Build manager signals
    connect(m_buildManager, &BuildManager::buildStarted, this, [this]() {
        m_buildAction->setEnabled(false);
        m_rebuildAction->setEnabled(false);
        m_cancelBuildAction->setEnabled(true);
        m_buildOutputDock->raise();
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
    });

    connect(m_editorTabs, &EditorTabWidget::fileOpened, this, [this](const QString& path) {
        if (m_lspClient->isReady()) {
            // Open document in LSP
            CodeEditor* editor = m_editorTabs->editorForFile(path);
            if (editor) {
                QString uri = "file:///" + path;
                uri.replace("\\", "/");
                m_lspClient->openDocument(uri, "xxml", 1, editor->toPlainText());
            }
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

    connect(m_lspClient, &LSPClient::diagnosticsReceived, this, [this](const QString& uri, const QList<LSPDiagnostic>& diagnostics) {
        // Convert URI to file path
        QString path = uri;
        if (path.startsWith("file:///")) {
            path = path.mid(8);
        }
        path.replace("/", "\\");

        // Find editor for this file
        CodeEditor* editor = m_editorTabs->editorForFile(path);
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
    m_projectManager->openProject(path);

    // Set LSP project root to the project directory
    QString projectDir = QFileInfo(path).absolutePath();
    m_lspClient->setProjectRoot(projectDir);
}

void MainWindow::newFile()
{
    m_editorTabs->newFile();
}

void MainWindow::newProject()
{
    NewProjectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString projectDir = dialog.projectLocation() + "/" + dialog.projectName();
        QString projectPath = projectDir + "/" + dialog.projectName() + ".xxmlp";

        // Create project directory
        QDir().mkpath(projectDir);
        QDir().mkpath(projectDir + "/src");

        // Create the project
        if (m_projectManager->createProject(projectPath, dialog.projectName())) {
            // Create a default Main.XXML file
            QString mainFile = projectDir + "/src/Main.XXML";
            QFile file(mainFile);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&file);
                out << "// Main entry point for " << dialog.projectName() << "\n\n";
                out << "[ Namespace <" << dialog.projectName() << ">\n";
                out << "    [ Class <Main> Final Extends None\n";
                out << "        [ Public <>\n";
                out << "            Entrypoint <Main> Parameters () -> {\n";
                out << "                // Your code here\n";
                out << "            }\n";
                out << "        ]\n";
                out << "    ]\n";
                out << "]\n";
                file.close();
            }

            // Set LSP project root
            m_lspClient->setProjectRoot(projectDir);

            statusBar()->showMessage(tr("Project created: %1").arg(dialog.projectName()), 3000);
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
    m_problemsDock->setVisible(true);
    m_buildOutputDock->setVisible(true);
    m_terminalDock->setVisible(true);

    // Re-arrange docks
    addDockWidget(Qt::LeftDockWidgetArea, m_projectExplorerDock);
    tabifyDockWidget(m_projectExplorerDock, m_outlineDock);
    m_projectExplorerDock->raise();

    addDockWidget(Qt::BottomDockWidgetArea, m_problemsDock);
    tabifyDockWidget(m_problemsDock, m_buildOutputDock);
    tabifyDockWidget(m_buildOutputDock, m_terminalDock);
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
