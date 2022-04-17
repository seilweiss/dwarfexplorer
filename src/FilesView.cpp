#include "FilesView.h"

#include <qboxlayout.h>
#include <qlabel.h>

FilesView::FilesView(QWidget* parent)
	: QWidget(parent)
	, m_treeView(new QTreeView)
	, m_filterLineEdit(new QLineEdit)
	, m_model(nullptr)
{
	connect(m_filterLineEdit, &QLineEdit::textChanged, this, &FilesView::onFilterLineEditTextChanged);

	QVBoxLayout* mainLayout = new QVBoxLayout;

	QHBoxLayout* topLayout = new QHBoxLayout;
	topLayout->addWidget(new QLabel(tr("Filter:")));
	topLayout->addWidget(m_filterLineEdit, 1);

	// not ready yet
	//mainLayout->addLayout(topLayout);
	mainLayout->addWidget(m_treeView, 1);

	setLayout(mainLayout);
}

FilesModel* FilesView::model() const
{
	return m_model;
}

void FilesView::setModel(FilesModel* model)
{
	m_treeView->setModel(model);

	m_model = model;

	connect(m_treeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &FilesView::currentChanged);

	m_filterLineEdit->clear();

	updateFilter();
}

void FilesView::updateFilter(const QModelIndex& parent)
{
	int rowCount = m_model->rowCount(parent);

	for (int i = 0; i < rowCount; i++)
	{
		QModelIndex child = m_model->index(i, 0, parent);
		QString path = m_model->path(child);

		m_treeView->setRowHidden(i, parent, !path.contains(m_filterLineEdit->text(), Qt::CaseInsensitive));

		updateFilter(child);
	}
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

void FilesView::onFilterLineEditTextChanged(const QString& text)
{
	updateFilter();
}
