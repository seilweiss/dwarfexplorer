#include "FunctionsView.h"

#include <qheaderview.h>
#include <qboxlayout.h>
#include <qlabel.h>

FunctionsView::FunctionsView(QWidget* parent)
    : QWidget(parent)
    , m_treeView(new TreeView)
    , m_filterLineEdit(new QLineEdit)
    , m_model(nullptr)
    , m_filterDelay(new QTimer(this))
{
    m_treeView->setSortingEnabled(true);
    m_treeView->sortByColumn(FunctionsModel::DwarfOffsetColumn, Qt::AscendingOrder);

    connect(m_filterLineEdit, &QLineEdit::textChanged, this, &FunctionsView::onFilterLineEditTextChanged);

    m_filterDelay->setSingleShot(true);
    connect(m_filterDelay, &QTimer::timeout, this, &FunctionsView::updateFilter);

    QVBoxLayout* mainLayout = new QVBoxLayout;

    QHBoxLayout* topLayout = new QHBoxLayout;
    topLayout->addWidget(new QLabel(tr("Filter:")));
    topLayout->addWidget(m_filterLineEdit, 1);

    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_treeView, 1);

    setLayout(mainLayout);
}

FunctionsModel* FunctionsView::model() const
{
    return m_model;
}

void FunctionsView::setModel(FunctionsModel* model)
{
    m_treeView->setModel(model);

    m_model = model;

    connect(m_treeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &FunctionsView::currentChanged);

    m_treeView->header()->setStretchLastSection(false);
    m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_treeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    m_filterLineEdit->clear();

    updateFilter();
}

void FunctionsView::updateFilter()
{
    QModelIndex index;
    int rowCount = m_model->rowCount(index);
    QString filter = m_filterLineEdit->text();

    if (filter.isEmpty())
    {
        for (int i = 0; i < rowCount; i++)
        {
            m_treeView->setRowHidden(i, index, false);
        }
    }
    else
    {
        for (int i = 0; i < rowCount; i++)
        {
            QString name = m_model->name(m_model->index(i, 0));
            m_treeView->setRowHidden(i, index, !name.contains(filter, Qt::CaseInsensitive));
        }
    }
}

void FunctionsView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
{
    if (!current.isValid())
    {
        emit noneSelected();
        return;
    }

    if (!m_model)
    {
        // this shouldn't happen???
        emit noneSelected();
        return;
    }

    emit functionSelected(m_model->dwarfOffset(current));
}

void FunctionsView::onFilterLineEditTextChanged(const QString& text)
{
    m_filterDelay->start(500);
}
