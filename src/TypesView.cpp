#include "TypesView.h"

#include <qheaderview.h>

TypesView::TypesView(QWidget* parent)
	: QTreeView(parent)
	, m_model(nullptr)
{
}

TypesModel* TypesView::model() const
{
	return m_model;
}

void TypesView::setModel(TypesModel* model)
{
	QTreeView::setModel(model);

	m_model = model;

	connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &TypesView::currentChanged);
	
	header()->setStretchLastSection(false);
	header()->setSectionResizeMode(1, QHeaderView::Stretch);
}

void TypesView::updateSpans()
{
	int rowCount = model()->rowCount(QModelIndex());

	for (int i = 0; i < rowCount; i++)
	{
		setFirstColumnSpanned(i, QModelIndex(), true);
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
