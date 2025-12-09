#include "EditorTabWidget.h"
#include "CodeEditor.h"
#include "core/Application.h"
#include "core/Settings.h"

#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>

namespace XXMLStudio {

EditorTabWidget::EditorTabWidget(QWidget* parent)
    : QTabWidget(parent)
{
    setupUi();
}

EditorTabWidget::~EditorTabWidget()
{
}

void EditorTabWidget::setupUi()
{
    setTabsClosable(true);
    setMovable(true);
    setDocumentMode(true);

    connect(this, &QTabWidget::tabCloseRequested,
            this, &EditorTabWidget::onTabCloseRequested);
    connect(this, &QTabWidget::currentChanged,
            this, &EditorTabWidget::onCurrentChanged);
}

CodeEditor* EditorTabWidget::openFile(const QString& path)
{
    // Check if already open
    if (m_fileEditors.contains(path)) {
        CodeEditor* editor = m_fileEditors[path];
        setCurrentWidget(editor);
        return editor;
    }

    // Read file content
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"),
            tr("Cannot open file %1:\n%2").arg(path, file.errorString()));
        return nullptr;
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    // Create editor
    CodeEditor* editor = new CodeEditor(this);
    editor->setPlainText(content);
    editor->setFilePath(path);

    // Apply current syntax theme from settings
    Settings* settings = Application::instance()->settings();
    editor->setSyntaxTheme(static_cast<SyntaxTheme>(settings->syntaxTheme()));

    // Connect signals
    connect(editor, &CodeEditor::modificationChanged,
            this, &EditorTabWidget::onEditorModificationChanged);
    connect(editor, &CodeEditor::cursorPositionChanged,
            this, &EditorTabWidget::onEditorCursorPositionChanged);

    // Add tab
    QFileInfo fileInfo(path);
    int index = addTab(editor, fileInfo.fileName());
    setCurrentIndex(index);

    m_fileEditors[path] = editor;
    emit fileOpened(path);

    return editor;
}

CodeEditor* EditorTabWidget::newFile()
{
    CodeEditor* editor = new CodeEditor(this);
    QString name = generateUntitledName();
    editor->setFilePath(QString());  // No file path yet

    // Apply current syntax theme from settings
    Settings* settings = Application::instance()->settings();
    editor->setSyntaxTheme(static_cast<SyntaxTheme>(settings->syntaxTheme()));

    connect(editor, &CodeEditor::modificationChanged,
            this, &EditorTabWidget::onEditorModificationChanged);
    connect(editor, &CodeEditor::cursorPositionChanged,
            this, &EditorTabWidget::onEditorCursorPositionChanged);

    int index = addTab(editor, name);
    setCurrentIndex(index);

    return editor;
}

bool EditorTabWidget::saveFile(int index)
{
    if (index < 0) index = currentIndex();
    if (index < 0) return false;

    CodeEditor* editor = editorAt(index);
    if (!editor) return false;

    QString path = editor->filePath();
    if (path.isEmpty()) {
        return saveFileAs(index);
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Error"),
            tr("Cannot save file %1:\n%2").arg(path, file.errorString()));
        return false;
    }

    QTextStream out(&file);
    out << editor->toPlainText();
    file.close();

    editor->document()->setModified(false);
    updateTabTitle(index);
    emit fileSaved(path);

    return true;
}

bool EditorTabWidget::saveFileAs(int index)
{
    if (index < 0) index = currentIndex();
    if (index < 0) return false;

    CodeEditor* editor = editorAt(index);
    if (!editor) return false;

    QString path = QFileDialog::getSaveFileName(this,
        tr("Save File As"),
        editor->filePath().isEmpty() ? QString() : editor->filePath(),
        tr("XXML Files (*.xxml *.XXML);;All Files (*)"));

    if (path.isEmpty()) return false;

    // Update path mapping
    QString oldPath = editor->filePath();
    if (!oldPath.isEmpty()) {
        m_fileEditors.remove(oldPath);
    }

    editor->setFilePath(path);
    m_fileEditors[path] = editor;

    return saveFile(index);
}

bool EditorTabWidget::saveAllFiles()
{
    bool allSaved = true;
    for (int i = 0; i < count(); ++i) {
        CodeEditor* editor = editorAt(i);
        if (editor && editor->document()->isModified()) {
            if (!saveFile(i)) {
                allSaved = false;
            }
        }
    }
    return allSaved;
}

bool EditorTabWidget::closeFile(int index)
{
    if (index < 0) index = currentIndex();
    if (index < 0) return true;

    CodeEditor* editor = editorAt(index);
    if (!editor) return true;

    // Check for unsaved changes
    if (editor->document()->isModified()) {
        QMessageBox::StandardButton result = QMessageBox::question(this,
            tr("Unsaved Changes"),
            tr("The file has unsaved changes. Save before closing?"),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save);

        if (result == QMessageBox::Cancel) {
            return false;
        }
        if (result == QMessageBox::Save) {
            if (!saveFile(index)) {
                return false;
            }
        }
    }

    QString path = editor->filePath();
    if (!path.isEmpty()) {
        m_fileEditors.remove(path);
    }

    removeTab(index);
    editor->deleteLater();

    if (!path.isEmpty()) {
        emit fileClosed(path);
    }

    return true;
}

bool EditorTabWidget::closeAllFiles()
{
    while (count() > 0) {
        if (!closeFile(0)) {
            return false;
        }
    }
    return true;
}

CodeEditor* EditorTabWidget::currentEditor() const
{
    return qobject_cast<CodeEditor*>(currentWidget());
}

CodeEditor* EditorTabWidget::editorAt(int index) const
{
    return qobject_cast<CodeEditor*>(widget(index));
}

CodeEditor* EditorTabWidget::editorForFile(const QString& path) const
{
    return m_fileEditors.value(path, nullptr);
}

int EditorTabWidget::indexOfFile(const QString& path) const
{
    CodeEditor* editor = editorForFile(path);
    return editor ? indexOf(editor) : -1;
}

bool EditorTabWidget::hasUnsavedChanges() const
{
    for (int i = 0; i < count(); ++i) {
        CodeEditor* editor = editorAt(i);
        if (editor && editor->document()->isModified()) {
            return true;
        }
    }
    return false;
}

QString EditorTabWidget::currentFilePath() const
{
    CodeEditor* editor = currentEditor();
    return editor ? editor->filePath() : QString();
}

void EditorTabWidget::undo()
{
    if (CodeEditor* editor = currentEditor()) {
        editor->undo();
    }
}

void EditorTabWidget::redo()
{
    if (CodeEditor* editor = currentEditor()) {
        editor->redo();
    }
}

void EditorTabWidget::cut()
{
    if (CodeEditor* editor = currentEditor()) {
        editor->cut();
    }
}

void EditorTabWidget::copy()
{
    if (CodeEditor* editor = currentEditor()) {
        editor->copy();
    }
}

void EditorTabWidget::paste()
{
    if (CodeEditor* editor = currentEditor()) {
        editor->paste();
    }
}

void EditorTabWidget::selectAll()
{
    if (CodeEditor* editor = currentEditor()) {
        editor->selectAll();
    }
}

void EditorTabWidget::onTabCloseRequested(int index)
{
    closeFile(index);
}

void EditorTabWidget::onCurrentChanged(int index)
{
    CodeEditor* editor = editorAt(index);
    emit currentEditorChanged(editor);

    if (editor) {
        emit modificationChanged(editor->document()->isModified());
        onEditorCursorPositionChanged();
    }
}

void EditorTabWidget::onEditorModificationChanged(bool modified)
{
    CodeEditor* editor = qobject_cast<CodeEditor*>(sender());
    if (!editor) return;

    int index = indexOf(editor);
    if (index >= 0) {
        updateTabTitle(index);
    }

    if (editor == currentEditor()) {
        emit modificationChanged(modified);
    }
}

void EditorTabWidget::onEditorCursorPositionChanged()
{
    CodeEditor* editor = currentEditor();
    if (!editor) return;

    QTextCursor cursor = editor->textCursor();
    int line = cursor.blockNumber() + 1;
    int column = cursor.columnNumber() + 1;
    emit cursorPositionChanged(line, column);
}

void EditorTabWidget::updateTabTitle(int index)
{
    CodeEditor* editor = editorAt(index);
    if (!editor) return;

    QString title;
    QString path = editor->filePath();

    if (path.isEmpty()) {
        title = tabText(index);
        if (title.startsWith('*')) {
            title = title.mid(1);
        }
    } else {
        QFileInfo fileInfo(path);
        title = fileInfo.fileName();
    }

    if (editor->document()->isModified()) {
        title = "*" + title;
    }

    setTabText(index, title);
}

QString EditorTabWidget::generateUntitledName()
{
    return tr("Untitled-%1").arg(++m_untitledCounter);
}

} // namespace XXMLStudio
