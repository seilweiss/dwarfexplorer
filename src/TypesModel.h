#pragma once

#include <qabstractitemmodel.h>

#include "Dwarf.h"

#include <qmap.h>

struct TypesModelItem
{
    enum Type
    {
        TypeItem,
        DefinitionItem
    };

    Type type;
    TypesModelItem* parentItem;
    int indexInParent;
    QString typeKeyword;
    QString typeNameOrCompileUnit;
    int typeSize;
    Elf32_Off typeDwarfOffset;
    QVector<TypesModelItem> subitems;
};

class TypesModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    TypesModel(QObject* parent = nullptr);

    Dwarf* dwarf() const;
    void setDwarf(Dwarf* dwarf);

    QString typeName(const QModelIndex& index) const;
    QString compileUnit(const QModelIndex& index) const;
    Elf32_Off dwarfOffset(const QModelIndex& index) const;
    bool isType(const QModelIndex& index) const;
    bool isDefinition(const QModelIndex& index) const;

    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    Dwarf* m_dwarf;
    QMap<QString, TypesModelItem> m_typeItemMap;

    void clearItems();
    void refresh();
};
