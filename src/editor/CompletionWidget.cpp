#include "CompletionWidget.h"
#include "CodeEditor.h"

#include <QApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QScrollBar>
#include <QScreen>
#include <QFile>
#include <QStandardPaths>
#include <QDateTime>
#include <QTextStream>

// Debug logging to file
static void logToFile(const QString& message) {
    static QString logPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/xxmlstudio_debug.log";
    QFile file(logPath);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << QDateTime::currentDateTime().toString("hh:mm:ss.zzz") << " [CompletionWidget] " << message << "\n";
        file.close();
    }
}

namespace XXMLStudio {

CompletionWidget::CompletionWidget(CodeEditor* editor)
    : QFrame(nullptr, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    , m_editor(editor)
{
    // Don't take focus from editor
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setObjectName("CompletionWidget");

    // Set up styling
    setFrameStyle(QFrame::Box | QFrame::Plain);
    setLineWidth(1);

    // Apply dark theme styling
    setStyleSheet(R"(
        QFrame#CompletionWidget {
            background-color: #252526;
            border: 1px solid #3e3e42;
            border-radius: 4px;
        }
        QListWidget {
            background-color: #252526;
            color: #e0e0e0;
            border: none;
            outline: none;
            font-family: "Consolas", "Courier New", monospace;
            font-size: 9pt;
        }
        QListWidget::item {
            padding: 1px 4px;
            border: none;
            height: 16px;
        }
        QListWidget::item:selected {
            background-color: #094771;
            color: #ffffff;
        }
        QListWidget::item:hover:!selected {
            background-color: #2a2d2e;
        }
        QScrollBar:vertical {
            background-color: #252526;
            width: 8px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background-color: #5a5a5a;
            min-height: 20px;
            border-radius: 4px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
    )");

    // Create layout
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(2, 2, 2, 2);
    m_layout->setSpacing(0);

    // Create list widget
    m_listWidget = new QListWidget(this);
    m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_listWidget->setUniformItemSizes(true);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setFocusPolicy(Qt::NoFocus);
    m_listWidget->setIconSize(QSize(16, 16));  // Small icons
    m_layout->addWidget(m_listWidget);

    // Connect signals
    connect(m_listWidget, &QListWidget::itemDoubleClicked,
            this, &CompletionWidget::onItemDoubleClicked);

    // Install event filter on editor to catch key events
    m_editor->installEventFilter(this);

    // Initially hidden
    QFrame::hide();
}

CompletionWidget::~CompletionWidget()
{
}

void CompletionWidget::showCompletions(const QList<LSPCompletionItem>& items)
{
    logToFile(QString("showCompletions called with %1 items").arg(items.size()));
    qDebug() << "CompletionWidget::showCompletions called with" << items.size() << "items";

    if (items.isEmpty()) {
        logToFile("items empty, hiding");
        qDebug() << "CompletionWidget: items empty, hiding";
        hide();
        return;
    }

    m_allItems = items;
    m_filterPrefix.clear();

    // Store trigger position
    QTextCursor cursor = m_editor->textCursor();
    m_triggerPosition = cursor.position();

    // Get the word prefix at cursor for initial filtering
    cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
    m_triggerPrefix = cursor.selectedText();
    m_filterPrefix = m_triggerPrefix;

    // Adjust trigger position to start of word
    m_triggerPosition = cursor.position();
    logToFile(QString("triggerPrefix='%1', triggerPosition=%2").arg(m_triggerPrefix).arg(m_triggerPosition));

    populateList();
    updatePosition();

    if (m_filteredItems.isEmpty()) {
        logToFile("filteredItems empty after filtering, hiding");
        hide();
        return;
    }

    logToFile(QString("Showing popup with %1 filtered items at pos (%2, %3), size (%4x%5)")
              .arg(m_filteredItems.size())
              .arg(pos().x()).arg(pos().y())
              .arg(width()).arg(height()));
    QFrame::show();
    m_listWidget->setCurrentRow(0);
    logToFile(QString("After show() - isVisible: %1").arg(QFrame::isVisible()));
}

void CompletionWidget::hide()
{
    QFrame::hide();
    m_allItems.clear();
    m_filteredItems.clear();
    m_filterPrefix.clear();
    emit dismissed();
}

bool CompletionWidget::isVisible() const
{
    return QFrame::isVisible();
}

void CompletionWidget::setFilterPrefix(const QString& prefix)
{
    if (m_filterPrefix == prefix) {
        return;
    }

    m_filterPrefix = prefix;
    int previousRow = m_listWidget->currentRow();
    QString previousLabel;
    if (previousRow >= 0 && previousRow < m_filteredItems.size()) {
        previousLabel = m_filteredItems[previousRow].label;
    }

    populateList();

    if (m_filteredItems.isEmpty()) {
        hide();
        return;
    }

    // Try to keep the same item selected
    int newRow = 0;
    if (!previousLabel.isEmpty()) {
        for (int i = 0; i < m_filteredItems.size(); ++i) {
            if (m_filteredItems[i].label == previousLabel) {
                newRow = i;
                break;
            }
        }
    }

    m_listWidget->setCurrentRow(newRow);
}

void CompletionWidget::selectNext()
{
    int currentRow = m_listWidget->currentRow();
    if (currentRow < m_listWidget->count() - 1) {
        m_listWidget->setCurrentRow(currentRow + 1);
    }
}

void CompletionWidget::selectPrevious()
{
    int currentRow = m_listWidget->currentRow();
    if (currentRow > 0) {
        m_listWidget->setCurrentRow(currentRow - 1);
    }
}

void CompletionWidget::selectFirst()
{
    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(0);
    }
}

void CompletionWidget::selectLast()
{
    if (m_listWidget->count() > 0) {
        m_listWidget->setCurrentRow(m_listWidget->count() - 1);
    }
}

LSPCompletionItem CompletionWidget::selectedItem() const
{
    int row = m_listWidget->currentRow();
    if (row >= 0 && row < m_filteredItems.size()) {
        return m_filteredItems[row];
    }
    return LSPCompletionItem{};
}

bool CompletionWidget::hasSelection() const
{
    return m_listWidget->currentRow() >= 0;
}

void CompletionWidget::applyCompletion()
{
    if (!hasSelection()) {
        hide();
        return;
    }

    LSPCompletionItem item = selectedItem();
    QString insertText = item.insertText.isEmpty() ? item.label : item.insertText;

    // Replace from trigger position to current cursor position
    QTextCursor cursor = m_editor->textCursor();
    int currentPos = cursor.position();

    // Select from trigger position to current position
    cursor.setPosition(m_triggerPosition);
    cursor.setPosition(currentPos, QTextCursor::KeepAnchor);
    cursor.insertText(insertText);

    m_editor->setTextCursor(cursor);

    emit completionApplied(insertText);
    hide();
}

void CompletionWidget::onItemDoubleClicked(QListWidgetItem* item)
{
    Q_UNUSED(item)
    applyCompletion();
}

bool CompletionWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_editor && isVisible()) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

            switch (keyEvent->key()) {
                case Qt::Key_Escape:
                    hide();
                    return true;

                case Qt::Key_Return:
                case Qt::Key_Enter:
                case Qt::Key_Tab:
                    if (hasSelection()) {
                        applyCompletion();
                        return true;
                    }
                    break;

                case Qt::Key_Up:
                    selectPrevious();
                    return true;

                case Qt::Key_Down:
                    selectNext();
                    return true;

                case Qt::Key_PageUp:
                    for (int i = 0; i < MAX_VISIBLE_ITEMS; ++i) {
                        selectPrevious();
                    }
                    return true;

                case Qt::Key_PageDown:
                    for (int i = 0; i < MAX_VISIBLE_ITEMS; ++i) {
                        selectNext();
                    }
                    return true;

                case Qt::Key_Home:
                    if (keyEvent->modifiers() & Qt::ControlModifier) {
                        selectFirst();
                        return true;
                    }
                    break;

                case Qt::Key_End:
                    if (keyEvent->modifiers() & Qt::ControlModifier) {
                        selectLast();
                        return true;
                    }
                    break;

                case Qt::Key_Left:
                case Qt::Key_Right:
                    // Let these through but update filter
                    break;

                case Qt::Key_Backspace:
                    // Let backspace through, but update filter afterward
                    break;

                default:
                    // For other keys, let them through and update filter
                    break;
            }

            // Update filter after the key is processed
            QTimer::singleShot(0, this, [this]() {
                if (!isVisible()) return;

                QTextCursor cursor = m_editor->textCursor();
                int currentPos = cursor.position();

                // If cursor moved before trigger position, dismiss
                if (currentPos < m_triggerPosition) {
                    hide();
                    return;
                }

                // Get new prefix
                cursor.setPosition(m_triggerPosition);
                cursor.setPosition(currentPos, QTextCursor::KeepAnchor);
                QString newPrefix = cursor.selectedText();

                // If prefix contains space or other non-identifier chars, dismiss
                for (const QChar& c : newPrefix) {
                    if (!c.isLetterOrNumber() && c != '_') {
                        hide();
                        return;
                    }
                }

                setFilterPrefix(newPrefix);
            });
        }
    }

    return QFrame::eventFilter(obj, event);
}

void CompletionWidget::focusOutEvent(QFocusEvent* event)
{
    Q_UNUSED(event)
    hide();
}

void CompletionWidget::updatePosition()
{
    // Get cursor rectangle in editor coordinates
    QTextCursor cursor = m_editor->textCursor();
    cursor.setPosition(m_triggerPosition);
    QRect cursorRect = m_editor->cursorRect(cursor);

    // Convert to global coordinates
    QPoint globalPos = m_editor->mapToGlobal(cursorRect.bottomLeft());

    // Calculate widget size
    int visibleItems = qMin(m_filteredItems.size(), MAX_VISIBLE_ITEMS);
    int height = visibleItems * ITEM_HEIGHT + 6;  // 6 for border/padding
    int width = 280;

    // Check screen bounds
    QScreen* screen = QApplication::screenAt(globalPos);
    if (screen) {
        QRect screenRect = screen->availableGeometry();

        // Adjust horizontal position
        if (globalPos.x() + width > screenRect.right()) {
            globalPos.setX(screenRect.right() - width);
        }
        if (globalPos.x() < screenRect.left()) {
            globalPos.setX(screenRect.left());
        }

        // Adjust vertical position - show above if not enough space below
        if (globalPos.y() + height > screenRect.bottom()) {
            globalPos.setY(m_editor->mapToGlobal(cursorRect.topLeft()).y() - height);
        }
    }

    setFixedSize(width, height);
    move(globalPos);
}

void CompletionWidget::populateList()
{
    m_listWidget->clear();
    m_filteredItems.clear();

    // Filter items by prefix
    QString lowerPrefix = m_filterPrefix.toLower();
    for (const LSPCompletionItem& item : m_allItems) {
        if (lowerPrefix.isEmpty() || item.label.toLower().startsWith(lowerPrefix)) {
            m_filteredItems.append(item);
        }
    }

    // Sort: exact prefix matches first, then alphabetically
    std::sort(m_filteredItems.begin(), m_filteredItems.end(),
        [&lowerPrefix](const LSPCompletionItem& a, const LSPCompletionItem& b) {
            bool aExact = a.label.toLower().startsWith(lowerPrefix);
            bool bExact = b.label.toLower().startsWith(lowerPrefix);
            if (aExact != bExact) return aExact;
            return a.label.toLower() < b.label.toLower();
        });

    // Populate list widget
    for (const LSPCompletionItem& item : m_filteredItems) {
        QListWidgetItem* listItem = new QListWidgetItem(m_listWidget);
        listItem->setText(item.label);
        listItem->setIcon(iconForKind(item.kind));
        if (!item.detail.isEmpty()) {
            listItem->setToolTip(item.detail);
        }
    }

    // Update size
    updatePosition();
}

QIcon CompletionWidget::iconForKind(CompletionItemKind kind) const
{
    // Return simple colored icons based on kind
    // In a full implementation, these would be actual icon files
    QString iconPath;
    switch (kind) {
        case CompletionItemKind::Method:
        case CompletionItemKind::Function:
            iconPath = ":/icons/Method.svg";
            break;
        case CompletionItemKind::Constructor:
            iconPath = ":/icons/Constructor.svg";
            break;
        case CompletionItemKind::Field:
        case CompletionItemKind::Property:
            iconPath = ":/icons/Field.svg";
            break;
        case CompletionItemKind::Variable:
            iconPath = ":/icons/Variable.svg";
            break;
        case CompletionItemKind::Class:
        case CompletionItemKind::Interface:
        case CompletionItemKind::Struct:
            iconPath = ":/icons/Class.svg";
            break;
        case CompletionItemKind::Module:
            iconPath = ":/icons/Namespace.svg";
            break;
        case CompletionItemKind::Enum:
            iconPath = ":/icons/Enum.svg";
            break;
        case CompletionItemKind::Keyword:
            iconPath = ":/icons/Keyword.svg";
            break;
        case CompletionItemKind::Snippet:
            iconPath = ":/icons/Snippet.svg";
            break;
        default:
            iconPath = ":/icons/Property.svg";
            break;
    }

    if (QFile::exists(iconPath)) {
        return QIcon(iconPath);
    }

    // Return empty icon if file doesn't exist
    return QIcon();
}

} // namespace XXMLStudio
