#pragma once

#include <qabstractitemmodel.h>

#include "Dwarf.h"

struct FunctionsModelItem
{
    QString name;
    Elf32_Addr address;
    Elf32_Off dwarfOffset;
    bool isPublic;
    QString fileName;
};

class FunctionsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    FunctionsModel(QObject* parent = nullptr);

    Dwarf* dwarf() const;
    void setDwarf(Dwarf* dwarf);

    QString name(const QModelIndex& index) const;
    Elf32_Addr address(const QModelIndex& index) const;
    Elf32_Off dwarfOffset(const QModelIndex& index) const;

    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    Dwarf* m_dwarf;
    QList<FunctionsModelItem> m_items;

    void clearItems();
    void refresh();
};
