#ifndef FINDREPLACEDIALOG_H
#define FINDREPLACEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>

namespace XXMLStudio {

/**
 * Dialog for find and replace functionality.
 */
class FindReplaceDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindReplaceDialog(QWidget* parent = nullptr);
    ~FindReplaceDialog();

    QString searchText() const;
    QString replaceText() const;
    bool caseSensitive() const;
    bool wholeWord() const;
    bool useRegex() const;

    void setSearchText(const QString& text);

signals:
    void findNext();
    void findPrevious();
    void replace();
    void replaceAll();

private:
    void setupUi();

    QLineEdit* m_searchEdit = nullptr;
    QLineEdit* m_replaceEdit = nullptr;
    QCheckBox* m_caseSensitiveCheck = nullptr;
    QCheckBox* m_wholeWordCheck = nullptr;
    QCheckBox* m_regexCheck = nullptr;
    QPushButton* m_findNextButton = nullptr;
    QPushButton* m_findPrevButton = nullptr;
    QPushButton* m_replaceButton = nullptr;
    QPushButton* m_replaceAllButton = nullptr;
    QPushButton* m_closeButton = nullptr;
    QLabel* m_statusLabel = nullptr;
};

} // namespace XXMLStudio

#endif // FINDREPLACEDIALOG_H
