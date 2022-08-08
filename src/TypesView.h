#pragma once

#include <qwidget.h>

#include "TypesModel.h"
#include "TreeView.h"

#include <qlineedit.h>

class TypesView : public QWidget
{
    Q_OBJECT

public:
    TypesView(QWidget* parent = nullptr);

    TypesModel* model() const;
    void setModel(TypesModel* model);

signals:
    void typeDefinitionSelected(Elf32_Off dwarfOffset);
    void noneSelected();

private:
    TreeView* m_treeView;
    QLineEdit* m_filterLineEdit;
    TypesModel* m_model;

    void updateSpans();
    void updateFilter();

private slots:
    void currentChanged(const QModelIndex& current, const QModelIndex& previous);
    void onModelDwarfChanged(Dwarf* dwarf);
    void onFilterLineEditTextChanged(const QString& text);
};
