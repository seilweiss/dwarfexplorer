#pragma once

#include <qtablewidget.h>

#include "Dwarf.h"

#if 0
class FunctionsModel : public QTableWidget
{
    Q_OBJECT

public:
    FunctionsModel(QObject* parent = nullptr);
    ~FunctionsModel();

    void clear();

    QList<TypesModelDefinition> typeDefinitions() const;
    void setTypeDefinitions(const QList<TypesModelDefinition>& definitions);

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

private:
    QList<TypesModelDefinition> m_typeDefinitions;
    QMap<QString, TypesModelItem> m_typeItemMap;

    void clearItems();
    void refresh();
};
#endif
