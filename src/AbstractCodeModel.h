#pragma once

#include "Code.h"
#include "Dwarf.h"
#include "FilesModel.h"
#include "TypesModel.h"

#include <qobject.h>

class AbstractCodeModel : public QObject
{
	Q_OBJECT

public:
	AbstractCodeModel(QObject* parent = nullptr);

	Dwarf* dwarf() const;
	void setDwarf(Dwarf* dwarf);

	virtual void writeDwarfEntry(Code& code, Elf32_Off offset) = 0;
	virtual void writeFile(Code& code, const QString& path) = 0;

signals:
	void modelAboutToBeReset();
	void modelReset();

protected:
	virtual void parseDwarf(Dwarf* dwarf) = 0;

	void beginResetModel();
	void endResetModel();

private:
	Dwarf* m_dwarf;
};
