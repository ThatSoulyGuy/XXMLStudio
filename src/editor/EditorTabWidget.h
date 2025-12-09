#ifndef EDITORTABWIDGET_H
#define EDITORTABWIDGET_H

#include <QTabWidget>
#include <QMap>

namespace XXMLStudio {

class CodeEditor;

/**
 * Tabbed container for code editors.
 * Manages multiple open files with close buttons and modified indicators.
 */
class EditorTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit EditorTabWidget(QWidget* parent = nullptr);
    ~EditorTabWidget();

    // File operations
    CodeEditor* openFile(const QString& path);
    CodeEditor* newFile();
    bool saveFile(int index = -1);
    bool saveFileAs(int index = -1);
    bool saveAllFiles();
    bool closeFile(int index = -1);
    bool closeAllFiles();

    // Editor access
    CodeEditor* currentEditor() const;
    CodeEditor* editorAt(int index) const;
    CodeEditor* editorForFile(const QString& path) const;
    int indexOfFile(const QString& path) const;

    // State queries
    bool hasUnsavedChanges() const;
    QString currentFilePath() const;

public slots:
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void selectAll();

signals:
    void currentEditorChanged(CodeEditor* editor);
    void fileOpened(const QString& path);
    void fileSaved(const QString& path);
    void fileClosed(const QString& path);
    void modificationChanged(bool modified);
    void cursorPositionChanged(int line, int column);

private slots:
    void onTabCloseRequested(int index);
    void onCurrentChanged(int index);
    void onEditorModificationChanged(bool modified);
    void onEditorCursorPositionChanged();

private:
    void setupUi();
    void updateTabTitle(int index);
    QString generateUntitledName();

    QMap<QString, CodeEditor*> m_fileEditors;  // path -> editor
    int m_untitledCounter = 0;
};

} // namespace XXMLStudio

#endif // EDITORTABWIDGET_H
