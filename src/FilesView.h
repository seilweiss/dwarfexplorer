#pragma once

#include <qwidget.h>

#include "FilesModel.h"
#include "TreeView.h"

#include <qlineedit.h>

class FilesView : public QWidget
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
    TreeView* m_treeView;
    QLineEdit* m_filterLineEdit;
    FilesModel* m_model;

    void updateFilter(const QModelIndex& parent = QModelIndex());

private slots:
    void currentChanged(const QModelIndex& current, const QModelIndex& previous);
    void onFilterLineEditTextChanged(const QString& text);
};
