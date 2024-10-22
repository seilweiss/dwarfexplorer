#pragma once

#include <qwidget.h>

#include "FunctionsModel.h"
#include "TreeView.h"

#include <qlineedit.h>
#include <qtimer.h>

class FunctionsView : public QWidget
{
    Q_OBJECT

public:
    FunctionsView(QWidget* parent = nullptr);

    FunctionsModel* model() const;
    void setModel(FunctionsModel* model);

signals:
    void functionSelected(Elf32_Off dwarfOffset);
    void noneSelected();

private:
    TreeView* m_treeView;
    QLineEdit* m_filterLineEdit;
    FunctionsModel* m_model;
    QTimer* m_filterDelay;

    void updateFilter();

private slots:
    void currentChanged(const QModelIndex& current, const QModelIndex& previous);
    void onFilterLineEditTextChanged(const QString& text);
};
