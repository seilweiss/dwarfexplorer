#pragma once

#include <qwidget.h>

#include "DwarfModel.h"

#include <qtreeview.h>

class DwarfView : public QWidget
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
    QTreeView* m_treeView;
    DwarfModel* m_model;

private slots:
    void currentChanged(const QModelIndex& current, const QModelIndex& previous);
};
