#include "DwarfView.h"

DwarfView::DwarfView(QWidget* parent)
	: QTreeView(parent)
	, m_model(nullptr)
{
}

DwarfModel* DwarfView::model() const
{
	return m_model;
}

void DwarfView::setModel(DwarfModel* model)
{
	QTreeView::setModel(model);

	m_model = model;

	connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &DwarfView::currentChanged);
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
