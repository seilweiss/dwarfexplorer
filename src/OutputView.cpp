#include "OutputView.h"

#include <qmenu.h>

OutputView::OutputView(QWidget* parent)
    : QPlainTextEdit(parent)
    , m_contextMenu(createStandardContextMenu())
{
    setReadOnly(true);

    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSize(10);

    setFont(font);

    QAction* clearAction = new QAction(tr("Clear"));
    connect(clearAction, &QAction::triggered, this, &QPlainTextEdit::clear);

    QAction* sep = m_contextMenu->insertSeparator(m_contextMenu->actions().first());
    m_contextMenu->insertAction(sep, clearAction);
}

void OutputView::contextMenuEvent(QContextMenuEvent* event)
{
    m_contextMenu->exec(event->globalPos());
}
