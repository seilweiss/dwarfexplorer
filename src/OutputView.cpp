#include "OutputView.h"

OutputView::OutputView(QWidget* parent)
	: QPlainTextEdit(parent)
{
	setReadOnly(true);

	QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	font.setPointSize(10);

	setFont(font);
}
