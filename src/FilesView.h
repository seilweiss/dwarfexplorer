#pragma once

#include <qtreeview.h>

#include "FilesModel.h"

class FilesView : public QTreeView
{
	Q_OBJECT

public:
	FilesView(QWidget* parent = nullptr);

	FilesModel* model() const;
	void setModel(FilesModel* model);

signals:
	void fileSelected(const QString& path);
	void directorySelected(const QString& path);
	void noneSelected();

private:
	FilesModel* m_model;

private slots:
	void currentChanged(const QModelIndex& current, const QModelIndex& previous);
};
