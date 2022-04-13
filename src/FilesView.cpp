#include "FilesView.h"

FilesView::FilesView(QWidget* parent)
	: QTreeView(parent)
	, m_model(nullptr)
{
}

FilesModel* FilesView::model() const
{
	return m_model;
}

void FilesView::setModel(FilesModel* model)
{
	QTreeView::setModel(model);

	m_model = model;

	connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &FilesView::currentChanged);
}

void FilesView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
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

	if (m_model->isFile(current))
	{
		emit fileSelected(m_model->path(current));
	}
	else if (m_model->isDirectory(current))
	{
		emit directorySelected(m_model->path(current));
	}
}
