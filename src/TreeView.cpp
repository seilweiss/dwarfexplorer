#include "TreeView.h"

#include <qevent.h>

#include "Output.h"

TreeView::TreeView(QWidget* parent)
    : QTreeView(parent)
{
}

void TreeView::mousePressEvent(QMouseEvent* event)
{
    // Always clear the selection.
    // This allows you to "reselect" an item and deselect all items by clicking a blank area
    setCurrentIndex(QModelIndex());

    QTreeView::mousePressEvent(event);
}