#include "TypesView.h"

#include <qheaderview.h>
#include <qboxlayout.h>
#include <qlabel.h>

TypesView::TypesView(QWidget* parent)
    : QWidget(parent)
    , m_treeView(new QTreeView)
    , m_filterLineEdit(new QLineEdit)
    , m_model(nullptr)
{
    // sorting not implemented yet
    //m_treeView->setSortingEnabled(true);

    connect(m_filterLineEdit, &QLineEdit::textChanged, this, &TypesView::onFilterLineEditTextChanged);

    QVBoxLayout* mainLayout = new QVBoxLayout;

    QHBoxLayout* topLayout = new QHBoxLayout;
    topLayout->addWidget(new QLabel(tr("Filter:")));
    topLayout->addWidget(m_filterLineEdit, 1);

    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_treeView, 1);

    setLayout(mainLayout);
}

TypesModel* TypesView::model() const
{
    return m_model;
}

void TypesView::setModel(TypesModel* model)
{
    m_treeView->setModel(model);

    m_model = model;

    connect(model, &TypesModel::dwarfChanged, this, &TypesView::onModelDwarfChanged);
    connect(m_treeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &TypesView::currentChanged);

    m_treeView->header()->setStretchLastSection(false);
    m_treeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_treeView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_treeView->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    m_filterLineEdit->clear();

    updateFilter();
}

void TypesView::updateSpans()
{
    int rowCount = m_model->rowCount(QModelIndex());

    for (int i = 0; i < rowCount; i++)
    {
        m_treeView->setFirstColumnSpanned(i, QModelIndex(), true);
    }
}

void TypesView::updateFilter()
{
    int rowCount = m_model->rowCount(QModelIndex());

    for (int i = 0; i < rowCount; i++)
    {
        QString typeName = m_model->typeName(m_model->index(i, 0));
        m_treeView->setRowHidden(i, QModelIndex(), !typeName.contains(m_filterLineEdit->text(), Qt::CaseInsensitive));
    }
}

void TypesView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
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

    if (m_model->isDefinition(current))
    {
        emit typeDefinitionSelected(m_model->dwarfOffset(current));
    }
}

void TypesView::onModelDwarfChanged(Dwarf* dwarf)
{
    updateSpans();
}

void TypesView::onFilterLineEditTextChanged(const QString& text)
{
    updateFilter();
}
