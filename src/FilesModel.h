#pragma once

#include <qabstractitemmodel.h>

#include "Dwarf.h"

struct FilesModelItem
{
    enum Type
    {
        DirectoryItem,
        FileItem
    };

    Type type;
    FilesModelItem* parentItem;
    int indexInParent;
    QString text;
    QString path;
    QMap<QString, FilesModelItem> subitems;
};

class FilesModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Column
    {
        NameColumn,
        ColumnCount
    };

    FilesModel(QObject* parent = nullptr);

    Dwarf* dwarf() const;
    void setDwarf(Dwarf* dwarf);

    QString path(const QModelIndex& index) const;
    bool isDirectory(const QModelIndex& index) const;
    bool isFile(const QModelIndex& index) const;

    QVariant data(const QModelIndex& index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    Dwarf* m_dwarf;
    FilesModelItem m_rootItem;

    void clearItems();
    void refresh();
};
