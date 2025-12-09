#ifndef OUTLINEPANEL_H
#define OUTLINEPANEL_H

#include <QWidget>
#include <QTreeView>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QLineEdit>

namespace XXMLStudio {

/**
 * Represents a symbol in the document (class, method, property, etc.)
 */
struct DocumentSymbol {
    enum Kind {
        File, Module, Namespace, Package, Class, Method, Property,
        Field, Constructor, Enum, Interface, Function, Variable,
        Constant, String, Number, Boolean, Array, Object, Key,
        Null, EnumMember, Struct, Event, Operator, TypeParameter
    };

    QString name;
    Kind kind = Class;
    int line = 0;
    int column = 0;
    int endLine = 0;
    int endColumn = 0;
    QList<DocumentSymbol> children;
};

/**
 * Panel displaying document outline (symbols) from LSP.
 */
class OutlinePanel : public QWidget
{
    Q_OBJECT

public:
    explicit OutlinePanel(QWidget* parent = nullptr);
    ~OutlinePanel();

    void clear();
    void setSymbols(const QList<DocumentSymbol>& symbols);

signals:
    void symbolDoubleClicked(int line, int column);

private slots:
    void onItemDoubleClicked(const QModelIndex& index);
    void onFilterTextChanged(const QString& text);

private:
    void setupUi();
    void addSymbolToModel(const DocumentSymbol& symbol, QStandardItem* parent = nullptr);
    QString symbolIcon(DocumentSymbol::Kind kind) const;

    QVBoxLayout* m_layout = nullptr;
    QLineEdit* m_filterEdit = nullptr;
    QTreeView* m_treeView = nullptr;
    QStandardItemModel* m_model = nullptr;
};

} // namespace XXMLStudio

#endif // OUTLINEPANEL_H
