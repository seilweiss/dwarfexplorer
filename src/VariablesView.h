#pragma once

#include <qwidget.h>

#include "VariablesModel.h"

#include <qtreeview.h>
#include <qlineedit.h>

class VariablesView : public QWidget
{
	Q_OBJECT

public:
	VariablesView(QWidget* parent = nullptr);

	VariablesModel* model() const;
	void setModel(VariablesModel* model);

signals:
	void variableSelected(Elf32_Off dwarfOffset);
	void noneSelected();

private:
	QTreeView* m_treeView;
	QLineEdit* m_filterLineEdit;
	VariablesModel* m_model;

	void updateFilter();

private slots:
	void currentChanged(const QModelIndex& current, const QModelIndex& previous);
	void onFilterLineEditTextChanged(const QString& text);
};