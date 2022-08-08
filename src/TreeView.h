#pragma once

#include <qtreeview.h>

class TreeView : public QTreeView
{
    Q_OBJECT

public:
    TreeView(QWidget* parent = nullptr);

protected:
    virtual void mousePressEvent(QMouseEvent* event) override;
};