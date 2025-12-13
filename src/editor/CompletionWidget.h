#ifndef COMPLETIONWIDGET_H
#define COMPLETIONWIDGET_H

#include <QListWidget>
#include <QFrame>
#include <QVBoxLayout>
#include <QTimer>
#include "../lsp/LSPProtocol.h"

namespace XXMLStudio {

class CodeEditor;

/**
 * Popup widget for displaying autocomplete suggestions.
 * Shows completion items from the LSP server in a filterable list.
 */
class CompletionWidget : public QFrame
{
    Q_OBJECT

public:
    explicit CompletionWidget(CodeEditor* editor);
    ~CompletionWidget();

    // Show completions at current cursor position
    void showCompletions(const QList<LSPCompletionItem>& items);
    void hide();
    bool isVisible() const;

    // Filter completions by prefix
    void setFilterPrefix(const QString& prefix);

    // Navigation
    void selectNext();
    void selectPrevious();
    void selectFirst();
    void selectLast();

    // Get selected item
    LSPCompletionItem selectedItem() const;
    bool hasSelection() const;

    // Apply selected completion
    void applyCompletion();

signals:
    void completionApplied(const QString& text);
    void dismissed();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;

private slots:
    void onItemDoubleClicked(QListWidgetItem* item);

private:
    void updatePosition();
    void populateList();
    QIcon iconForKind(CompletionItemKind kind) const;

    CodeEditor* m_editor;
    QListWidget* m_listWidget;
    QVBoxLayout* m_layout;

    QList<LSPCompletionItem> m_allItems;
    QList<LSPCompletionItem> m_filteredItems;
    QString m_filterPrefix;

    // Position where completion was triggered
    int m_triggerPosition = 0;
    QString m_triggerPrefix;

    static constexpr int MAX_VISIBLE_ITEMS = 8;
    static constexpr int ITEM_HEIGHT = 18;
};

} // namespace XXMLStudio

#endif // COMPLETIONWIDGET_H
