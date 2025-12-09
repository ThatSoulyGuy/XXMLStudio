#include "OutlinePanel.h"

#include <QHeaderView>

namespace XXMLStudio {

OutlinePanel::OutlinePanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

OutlinePanel::~OutlinePanel()
{
}

void OutlinePanel::setupUi()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    // Filter edit
    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText(tr("Filter symbols..."));
    m_filterEdit->setClearButtonEnabled(true);
    m_layout->addWidget(m_filterEdit);

    // Tree view
    m_treeView = new QTreeView(this);
    m_treeView->setHeaderHidden(true);
    m_treeView->setAnimated(true);
    m_treeView->setIndentation(16);
    m_layout->addWidget(m_treeView);

    // Model
    m_model = new QStandardItemModel(this);
    m_treeView->setModel(m_model);

    // Connect signals
    connect(m_treeView, &QTreeView::doubleClicked,
            this, &OutlinePanel::onItemDoubleClicked);
    connect(m_filterEdit, &QLineEdit::textChanged,
            this, &OutlinePanel::onFilterTextChanged);
}

void OutlinePanel::clear()
{
    m_model->clear();
}

void OutlinePanel::setSymbols(const QList<DocumentSymbol>& symbols)
{
    clear();

    for (const DocumentSymbol& symbol : symbols) {
        addSymbolToModel(symbol);
    }

    m_treeView->expandAll();
}

void OutlinePanel::addSymbolToModel(const DocumentSymbol& symbol, QStandardItem* parent)
{
    QString text = QString("%1 %2").arg(symbolIcon(symbol.kind), symbol.name);
    QStandardItem* item = new QStandardItem(text);

    // Store line/column in item data
    item->setData(symbol.line, Qt::UserRole);
    item->setData(symbol.column, Qt::UserRole + 1);

    if (parent) {
        parent->appendRow(item);
    } else {
        m_model->appendRow(item);
    }

    // Add children recursively
    for (const DocumentSymbol& child : symbol.children) {
        addSymbolToModel(child, item);
    }
}

void OutlinePanel::onItemDoubleClicked(const QModelIndex& index)
{
    int line = index.data(Qt::UserRole).toInt();
    int column = index.data(Qt::UserRole + 1).toInt();
    emit symbolDoubleClicked(line, column);
}

void OutlinePanel::onFilterTextChanged(const QString& text)
{
    // Simple filter: hide items that don't match
    for (int i = 0; i < m_model->rowCount(); ++i) {
        QStandardItem* item = m_model->item(i);
        bool visible = text.isEmpty() || item->text().contains(text, Qt::CaseInsensitive);
        m_treeView->setRowHidden(i, QModelIndex(), !visible);
    }
}

QString OutlinePanel::symbolIcon(DocumentSymbol::Kind kind) const
{
    switch (kind) {
        case DocumentSymbol::Class:       return QString::fromUtf8("\U0001F4E6");  // Package
        case DocumentSymbol::Interface:   return QString::fromUtf8("\U0001F517");  // Link
        case DocumentSymbol::Method:      return QString::fromUtf8("\u2699");      // Gear
        case DocumentSymbol::Function:    return QString::fromUtf8("\u0192");      // Function
        case DocumentSymbol::Property:    return QString::fromUtf8("\u25C9");      // Circle
        case DocumentSymbol::Field:       return QString::fromUtf8("\u25A0");      // Square
        case DocumentSymbol::Variable:    return QString::fromUtf8("\U0001D465");  // Math x
        case DocumentSymbol::Constant:    return QString::fromUtf8("\u03C0");      // Pi
        case DocumentSymbol::Enum:        return QString::fromUtf8("\u2630");      // Trigram
        case DocumentSymbol::EnumMember:  return QString::fromUtf8("\u2022");      // Bullet
        case DocumentSymbol::Struct:      return QString::fromUtf8("\u25A1");      // Square outline
        case DocumentSymbol::Namespace:   return QString::fromUtf8("\u2302");      // House
        case DocumentSymbol::Constructor: return QString::fromUtf8("\u2726");      // Star
        case DocumentSymbol::Event:       return QString::fromUtf8("\u26A1");      // Lightning
        case DocumentSymbol::Operator:    return QString::fromUtf8("\u00B1");      // Plus-minus
        default:                          return QString::fromUtf8("\u25CB");      // Circle
    }
}

} // namespace XXMLStudio
