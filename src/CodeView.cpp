#include "CodeView.h"

CodeView::CodeView(QWidget* parent)
	: QPlainTextEdit(parent)
	, m_model(nullptr)
	, m_code()
{
	//setReadOnly(true);
	setWordWrapMode(QTextOption::NoWrap);

	QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	font.setPointSize(11);

	setFont(font);

	refresh();
}

AbstractCodeModel* CodeView::model() const
{
	return m_model;
}

void CodeView::setModel(AbstractCodeModel* model)
{
	if (m_model)
	{
		m_model->disconnect(this);
	}

	m_model = model;
	m_code.clear();

	refresh();

	connect(model, &AbstractCodeModel::modelReset, this, &CodeView::refresh);
}

void CodeView::viewDwarfEntry(Elf32_Off offset)
{
	m_code.clear();

	if (m_model)
	{
		m_model->writeDwarfEntry(m_code, offset);
	}

	refresh();
}

void CodeView::viewFile(const QString& path)
{
	m_code.clear();

	if (m_model)
	{
		m_model->writeFile(m_code, path);
	}

	refresh();
}

void CodeView::refresh()
{
	clear();
	//setPlainText(m_code.toPlainText());
	appendHtml(m_code.toHtml());

	moveCursor(QTextCursor::Start);
}
