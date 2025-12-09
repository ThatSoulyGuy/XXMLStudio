#include "ProblemsPanel.h"

#include <QHeaderView>

namespace XXMLStudio {

ProblemsPanel::ProblemsPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

ProblemsPanel::~ProblemsPanel()
{
}

void ProblemsPanel::setupUi()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    // Summary label
    m_summaryLabel = new QLabel(tr("No problems"), this);
    m_summaryLabel->setStyleSheet("padding: 4px; background-color: #2d2d2d;");
    m_layout->addWidget(m_summaryLabel);

    // Table view
    m_tableView = new QTableView(this);
    m_tableView->setShowGrid(false);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->verticalHeader()->hide();
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_layout->addWidget(m_tableView);

    // Model
    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels({tr(""), tr("File"), tr("Line"), tr("Message")});
    m_tableView->setModel(m_model);

    // Set column widths
    m_tableView->setColumnWidth(0, 30);   // Severity icon
    m_tableView->setColumnWidth(1, 200);  // File
    m_tableView->setColumnWidth(2, 50);   // Line

    // Connect signals
    connect(m_tableView, &QTableView::doubleClicked,
            this, &ProblemsPanel::onItemDoubleClicked);
}

void ProblemsPanel::clear()
{
    m_model->removeRows(0, m_model->rowCount());
    m_problems.clear();
    m_errorCount = 0;
    m_warningCount = 0;
    updateSummary();
}

void ProblemsPanel::addProblem(const Problem& problem)
{
    m_problems.append(problem);

    QList<QStandardItem*> row;

    // Severity icon
    QStandardItem* severityItem = new QStandardItem(severityIcon(problem.severity));
    severityItem->setTextAlignment(Qt::AlignCenter);
    row.append(severityItem);

    // File
    QStandardItem* fileItem = new QStandardItem(problem.file);
    row.append(fileItem);

    // Line
    QStandardItem* lineItem = new QStandardItem(QString::number(problem.line));
    lineItem->setTextAlignment(Qt::AlignCenter);
    row.append(lineItem);

    // Message
    QStandardItem* messageItem = new QStandardItem(problem.message);
    row.append(messageItem);

    m_model->appendRow(row);

    // Update counts
    if (problem.severity == Problem::Error) {
        ++m_errorCount;
    } else if (problem.severity == Problem::Warning) {
        ++m_warningCount;
    }

    updateSummary();
}

void ProblemsPanel::addProblem(const QString& file, int line, int column,
                               const QString& severity, const QString& message)
{
    Problem problem;
    problem.file = file;
    problem.line = line;
    problem.column = column;
    problem.message = message;

    QString sev = severity.toLower();
    if (sev == "error" || sev == "fatal") {
        problem.severity = Problem::Error;
    } else if (sev == "warning") {
        problem.severity = Problem::Warning;
    } else {
        problem.severity = Problem::Note;
    }

    addProblem(problem);
}

void ProblemsPanel::setProblems(const QList<Problem>& problems)
{
    clear();
    for (const Problem& problem : problems) {
        addProblem(problem);
    }
}

void ProblemsPanel::onItemDoubleClicked(const QModelIndex& index)
{
    int row = index.row();
    if (row >= 0 && row < m_problems.size()) {
        const Problem& problem = m_problems[row];
        emit problemDoubleClicked(problem.file, problem.line, problem.column);
    }
}

void ProblemsPanel::updateSummary()
{
    if (m_errorCount == 0 && m_warningCount == 0) {
        m_summaryLabel->setText(tr("No problems"));
    } else {
        QString text;
        if (m_errorCount > 0) {
            text += tr("%n error(s)", "", m_errorCount);
        }
        if (m_warningCount > 0) {
            if (!text.isEmpty()) text += ", ";
            text += tr("%n warning(s)", "", m_warningCount);
        }
        m_summaryLabel->setText(text);
    }

    emit problemCountChanged(m_errorCount, m_warningCount);
}

QString ProblemsPanel::severityIcon(Problem::Severity severity) const
{
    switch (severity) {
        case Problem::Error:   return QString::fromUtf8("\u274C");  // Red X
        case Problem::Warning: return QString::fromUtf8("\u26A0");  // Warning triangle
        case Problem::Note:    return QString::fromUtf8("\u2139");  // Info circle
        default: return QString();
    }
}

} // namespace XXMLStudio
