#pragma once

#include <qtreeview.h>

#include "DwarfModel.h"

class DwarfView : public QTreeView
{
	Q_OBJECT

public:
	DwarfView(QWidget* parent = nullptr);

	DwarfModel* model() const;
	void setModel(DwarfModel* model);

signals:
	void entrySelected(DwarfEntry* entry);
	void attributeSelected(DwarfAttribute* attribute);
	void noneSelected();

private:
	DwarfModel* m_model;

private slots:
	void currentChanged(const QModelIndex& current, const QModelIndex& previous);
};
