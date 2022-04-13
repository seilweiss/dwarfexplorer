#pragma once

#include <qplaintextedit.h>

#include "AbstractCodeModel.h"

class CodeView : public QPlainTextEdit
{
	Q_OBJECT

public:
	CodeView(QWidget* parent = nullptr);

	AbstractCodeModel* model() const;
	void setModel(AbstractCodeModel* model);

	void viewDwarfEntry(Elf32_Off offset);
	void viewFile(const QString& path);

private:
	AbstractCodeModel* m_model;
	Code m_code;

	void refresh();
};
