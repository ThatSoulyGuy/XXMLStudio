#ifndef NEWPROJECTDIALOG_H
#define NEWPROJECTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>

namespace XXMLStudio {

/**
 * Dialog for creating a new XXML project.
 */
class NewProjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewProjectDialog(QWidget* parent = nullptr);
    ~NewProjectDialog();

    QString projectName() const;
    QString projectLocation() const;
    QString projectType() const;

private slots:
    void browseLocation();
    void validateInput();

private:
    void setupUi();

    QLineEdit* m_nameEdit = nullptr;
    QLineEdit* m_locationEdit = nullptr;
    QComboBox* m_typeCombo = nullptr;
    QPushButton* m_browseButton = nullptr;
    QPushButton* m_createButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
};

} // namespace XXMLStudio

#endif // NEWPROJECTDIALOG_H
