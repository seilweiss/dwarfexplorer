#include "DwarfView.h"

#include <qboxlayout.h>
#include <qheaderview.h>

DwarfView::DwarfView(QWidget* parent)
	: QWidget(parent)
	, m_treeView(new QTreeView)
	, m_model(nullptr)
{
	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addWidget(m_treeView);
	setLayout(mainLayout);
}

DwarfModel* DwarfView::model() const
{
	return m_model;
}

void DwarfView::setModel(DwarfModel* model)
{
	m_treeView->setModel(model);

	m_model = model;

	connect(m_treeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &DwarfView::currentChanged);

	m_treeView->header()->setStretchLastSection(false);
	m_treeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	m_treeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	m_treeView->header()->setSectionResizeMode(2, QHeaderView::Stretch);
}

void DwarfView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
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

	if (m_model->isEntry(current))
	{
		emit entrySelected(m_model->entry(current));
	}
	else if (m_model->isAttribute(current))
	{
		emit attributeSelected(m_model->attribute(current));
	}
}
