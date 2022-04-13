#include "Code.h"

Code::Code()
	: m_tokens()
{
}

void Code::clear()
{
	m_tokens.clear();
}

void Code::addToken(const CodeToken& token)
{
	m_tokens.append(token);
}

void Code::addToken(const QString& text, const QColor& color)
{
	CodeToken token;
	token.text = text;
	token.color = color;

	addToken(token);
}

QString Code::toPlainText() const
{
	QString text;

	for (const CodeToken& token : m_tokens)
	{
		text += token.text;
	}

	return text;
}

QString Code::toHtml() const
{
	QString html;

	for (const CodeToken& token : m_tokens)
	{
		QString color = token.color.name();
		QString text = token.text;

		text.replace("&", "&amp;");
		text.replace(" ", "&nbsp;");
		text.replace("<", "&lt;");
		text.replace(">", "&gt;");
		text.replace("\"", "&quot;");
		text.replace("'", "&apos;");
		text.replace("\n", "<br>");
		text.replace("\t", "&nbsp;&nbsp;&nbsp;&nbsp;");

		html += QString("<span style=\"color:%1\">%2</span>").arg(color, text);
	}

	return html;
}
