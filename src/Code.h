#pragma once

#include <qstring.h>
#include <qlist.h>
#include <qcolor.h>

struct CodeToken
{
	QString text;
	QColor color;
};

class Code
{
public:
	Code();

	void clear();
	void addToken(const CodeToken& token);
	void addToken(const QString& text, const QColor& color = QColorConstants::Black);

	QString toPlainText() const;
	QString toHtml() const;

private:
	QList<CodeToken> m_tokens;
};
