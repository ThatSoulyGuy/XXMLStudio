#ifndef SETUPSTREAMDIALOG_H
#define SETUPSTREAMDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace XXMLStudio {

/**
 * Dialog for setting up a remote when pushing to a branch with no upstream.
 * Allows user to:
 * - Select existing remote or add a new one
 * - Enter remote URL for new remotes
 * - Set upstream for the current branch
 */
class SetUpstreamDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SetUpstreamDialog(const QString& currentBranch,
                               const QStringList& existingRemotes,
                               QWidget* parent = nullptr);
    ~SetUpstreamDialog();

    QString remoteName() const;
    QString remoteUrl() const;
    bool isNewRemote() const;

private slots:
    void onRemoteSelectionChanged(int index);
    void validateInput();

private:
    void setupUi();

    QString m_currentBranch;
    QStringList m_existingRemotes;

    QLabel* m_infoLabel = nullptr;
    QComboBox* m_remoteCombo = nullptr;
    QLineEdit* m_remoteNameEdit = nullptr;
    QLineEdit* m_remoteUrlEdit = nullptr;
    QLabel* m_remoteNameLabel = nullptr;
    QLabel* m_remoteUrlLabel = nullptr;
    QPushButton* m_okButton = nullptr;
    QPushButton* m_cancelButton = nullptr;
};

} // namespace XXMLStudio

#endif // SETUPSTREAMDIALOG_H
