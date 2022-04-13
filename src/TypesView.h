#pragma once

#include <qtreeview.h>

#include "TypesModel.h"

class TypesView : public QTreeView
{
	Q_OBJECT

public:
	TypesView(QWidget* parent = nullptr);

	TypesModel* model() const;
	void setModel(TypesModel* model);

	void updateSpans();

signals:
	void typeDefinitionSelected(Elf32_Off dwarfOffset);
	void noneSelected();

private:
	TypesModel* m_model;

private slots:
	void currentChanged(const QModelIndex& current, const QModelIndex& previous);
};
