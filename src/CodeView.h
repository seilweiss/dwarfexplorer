#pragma once

#include <qwidget.h>

#include "AbstractCodeModel.h"

#include <Qsci/qsciscintilla.h>

class CodeView : public QWidget
{
	Q_OBJECT

public:
	CodeView(QWidget* parent = nullptr);

	void clear();

	AbstractCodeModel* model() const;
	void setModel(AbstractCodeModel* model);

	void viewDwarfEntry(Elf32_Off offset);
	void viewFile(const QString& path);

private:
	AbstractCodeModel* m_model;
	QsciScintilla* m_editor;
	QString m_code;

	void refresh();
};
