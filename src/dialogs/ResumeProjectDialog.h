#ifndef RESUMEPROJECTDIALOG_H
#define RESUMEPROJECTDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QListWidget>
#include <QVBoxLayout>

namespace XXMLStudio {

class ResumeProjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ResumeProjectDialog(const QString& lastProject,
                                  const QStringList& recentProjects,
                                  QWidget* parent = nullptr);
    ~ResumeProjectDialog();

    QString selectedProject() const { return m_selectedProject; }
    bool dontAskAgain() const { return m_dontAskAgain; }

private slots:
    void onResumeClicked();
    void onNewSessionClicked();
    void onProjectDoubleClicked(QListWidgetItem* item);
    void onSelectionChanged();

private:
    void setupUi(const QString& lastProject, const QStringList& recentProjects);

    QLabel* m_titleLabel = nullptr;
    QLabel* m_descLabel = nullptr;
    QListWidget* m_projectList = nullptr;
    QCheckBox* m_dontAskCheckbox = nullptr;
    QPushButton* m_resumeButton = nullptr;
    QPushButton* m_newSessionButton = nullptr;

    QString m_selectedProject;
    bool m_dontAskAgain = false;
};

} // namespace XXMLStudio

#endif // RESUMEPROJECTDIALOG_H
