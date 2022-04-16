#include "CodeView.h"

#include <qboxlayout.h>
#include <qfontdatabase.h>

#include <Qsci/qscilexercpp.h>

CodeView::CodeView(QWidget* parent)
	: QWidget(parent)
	, m_model(nullptr)
	, m_editor(new QsciScintilla(this))
	, m_code()
{
	QsciLexer* lexer = new QsciLexerCPP(m_editor);

	QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
	font.setPointSize(11);

	lexer->setDefaultFont(font);

	m_editor->setLexer(lexer);
	m_editor->setWrapMode(QsciScintilla::WrapWhitespace);
	m_editor->setWrapIndentMode(QsciScintilla::WrapIndentIndented);
	m_editor->setIndentationsUseTabs(false);
	m_editor->setTabWidth(4);
	m_editor->setAutoIndent(true);
	m_editor->setMargins(1);
	m_editor->setMarginType(0, QsciScintilla::NumberMargin);
	m_editor->setMarginWidth(0, 50);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->addWidget(m_editor);
	setLayout(mainLayout);

	refresh();
}

void CodeView::clear()
{

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
	m_editor->setText(m_code);
	m_editor->setCursorPosition(0, 0);
}
