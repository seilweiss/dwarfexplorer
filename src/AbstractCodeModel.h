#pragma once

#include "Dwarf.h"
#include "FilesModel.h"
#include "TypesModel.h"

#include <qobject.h>
#include <qmenu.h>

class AbstractCodeModel : public QObject
{
    Q_OBJECT

public:
    AbstractCodeModel(QObject* parent = nullptr);

    Dwarf* dwarf() const;
    void setDwarf(Dwarf* dwarf);

    virtual void writeDwarfEntry(QString& code, Elf32_Off offset) = 0;
    virtual void writeFile(QString& code, const QString& path) = 0;
    virtual QString dwarfEntryName(Elf32_Off offset) const = 0;
    virtual void setupSettingsMenu(QMenu* menu) = 0;

signals:
    void rewriteRequested();

protected:
    virtual void parseDwarf(Dwarf* dwarf) = 0;

    void requestRewrite();

private:
    Dwarf* m_dwarf;
};
