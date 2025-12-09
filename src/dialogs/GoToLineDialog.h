#ifndef GOTOLINEDIALOG_H
#define GOTOLINEDIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>

namespace XXMLStudio {

/**
 * Dialog for navigating to a specific line number.
 */
class GoToLineDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GoToLineDialog(QWidget* parent = nullptr);
    ~GoToLineDialog();

    void setMaxLine(int max);
    void setCurrentLine(int line);
    int selectedLine() const;

private:
    void setupUi();

    QSpinBox* m_lineSpinBox = nullptr;
    QLabel* m_infoLabel = nullptr;
    QPushButton* m_goButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
};

} // namespace XXMLStudio

#endif // GOTOLINEDIALOG_H
