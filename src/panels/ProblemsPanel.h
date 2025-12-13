#ifndef PROBLEMSPANEL_H
#define PROBLEMSPANEL_H

#include <QWidget>
#include <QTableView>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QLabel>

namespace XXMLStudio {

/**
 * Represents a problem (error, warning, note) from the compiler or LSP.
 */
struct Problem {
    enum Severity { Error, Warning, Note };

    Severity severity = Error;
    QString file;
    int line = 0;
    int column = 0;
    QString message;
};

/**
 * Panel displaying compiler errors, warnings, and LSP diagnostics.
 */
class ProblemsPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ProblemsPanel(QWidget* parent = nullptr);
    ~ProblemsPanel();

    void clear();
    void clearProblemsForFile(const QString& file);
    void addProblem(const Problem& problem);
    void addProblem(const QString& file, int line, int column,
                    const QString& severity, const QString& message);
    void setProblems(const QList<Problem>& problems);

    int errorCount() const { return m_errorCount; }
    int warningCount() const { return m_warningCount; }

signals:
    void problemDoubleClicked(const QString& file, int line, int column);
    void problemCountChanged(int errors, int warnings);

private slots:
    void onItemDoubleClicked(const QModelIndex& index);

private:
    void setupUi();
    void updateSummary();
    QString severityIcon(Problem::Severity severity) const;

    QVBoxLayout* m_layout = nullptr;
    QLabel* m_summaryLabel = nullptr;
    QTableView* m_tableView = nullptr;
    QStandardItemModel* m_model = nullptr;

    QList<Problem> m_problems;
    int m_errorCount = 0;
    int m_warningCount = 0;
};

} // namespace XXMLStudio

#endif // PROBLEMSPANEL_H
