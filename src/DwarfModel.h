#pragma once

#include <qabstractitemmodel.h>

#include "Dwarf.h"

struct DwarfModelItem
{
    enum Type
    {
        EntryItem,
        AttributeItem
    };

    Type type;
    DwarfModelItem* parentItem;
    int indexInParent;

    union
    {
        struct
        {
            DwarfEntry* entry;
            DwarfModelItem* attributeItems;
            DwarfModelItem* siblingItem;
            DwarfModelItem* childItem;
        } e;

        struct
        {
            DwarfAttribute* attribute;
        } a;
    };
};

class DwarfModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    DwarfModel(QObject* parent = nullptr);
    ~DwarfModel();

    Dwarf* dwarf() const;
    void setDwarf(Dwarf* dwarf);

    DwarfEntry* entry(const QModelIndex& index) const;
    DwarfAttribute* attribute(const QModelIndex& index) const;
    bool isEntry(const QModelIndex& index) const;
    bool isAttribute(const QModelIndex& index) const;

    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    Dwarf* m_dwarf;
    DwarfModelItem* m_items;

    void clearItems();
    void refresh();
};
