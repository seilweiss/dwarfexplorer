#include "VariablesView.h"

#include <qheaderview.h>
#include <qboxlayout.h>
#include <qlabel.h>

VariablesView::VariablesView(QWidget* parent)
	: QWidget(parent)
	, m_treeView(new QTreeView)
	, m_filterLineEdit(new QLineEdit)
	, m_model(nullptr)
{
	connect(m_filterLineEdit, &QLineEdit::textChanged, this, &VariablesView::onFilterLineEditTextChanged);

	QVBoxLayout* mainLayout = new QVBoxLayout;

	QHBoxLayout* topLayout = new QHBoxLayout;
	topLayout->addWidget(new QLabel(tr("Filter:")));
	topLayout->addWidget(m_filterLineEdit, 1);

	mainLayout->addLayout(topLayout);
	mainLayout->addWidget(m_treeView, 1);

	setLayout(mainLayout);
}

VariablesModel* VariablesView::model() const
{
	return m_model;
}

void VariablesView::setModel(VariablesModel* model)
{
	m_treeView->setModel(model);

	m_model = model;

	connect(m_treeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &VariablesView::currentChanged);

	m_treeView->header()->setStretchLastSection(false);
	m_treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
	m_treeView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	m_treeView->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	m_treeView->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
	m_treeView->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);

	m_filterLineEdit->clear();

	updateFilter();
}

void VariablesView::updateFilter()
{
	int rowCount = m_model->rowCount(QModelIndex());

	for (int i = 0; i < rowCount; i++)
	{
		QString name = m_model->name(m_model->index(i, 0));
		m_treeView->setRowHidden(i, QModelIndex(), !name.contains(m_filterLineEdit->text(), Qt::CaseInsensitive));
	}
}

void VariablesView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
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

	emit variableSelected(m_model->dwarfOffset(current));
}

void VariablesView::onFilterLineEditTextChanged(const QString& text)
{
	updateFilter();
}
