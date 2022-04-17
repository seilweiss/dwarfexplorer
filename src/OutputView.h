#pragma once

#include <qplaintextedit.h>

class OutputView : public QPlainTextEdit
{
    Q_OBJECT

public:
    OutputView(QWidget* parent = nullptr);

    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    QMenu* m_contextMenu;
};