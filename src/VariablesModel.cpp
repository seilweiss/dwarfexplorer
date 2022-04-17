#include "VariablesModel.h"

#include "Util.h"
#include "DwarfAttributes.h"

#include <qdir.h>

VariablesModel::VariablesModel(QObject* parent)
	: QAbstractItemModel(parent)
	, m_dwarf(nullptr)
	, m_items()
{
}

Dwarf* VariablesModel::dwarf() const
{
	return m_dwarf;
}

void VariablesModel::setDwarf(Dwarf* dwarf)
{
	m_dwarf = dwarf;

	beginResetModel();
	refresh();
	endResetModel();
}

void VariablesModel::clearItems()
{
	m_items.clear();
}

void VariablesModel::refresh()
{
	clearItems();

	if (!m_dwarf)
	{
		return;
	}

	if (m_dwarf->entryCount == 0)
	{
		return;
	}

	for (DwarfEntry* entry = &m_dwarf->entries[0]; entry != nullptr; entry = entry->sibling)
	{
		if (entry->tag == DW_TAG_compile_unit)
		{
			QString compileUnitPath = entry->getName();
			QString fileName;

			if (!compileUnitPath.isEmpty())
			{
				QStringList parts = QDir::toNativeSeparators(compileUnitPath).split(QDir::separator(), Qt::SkipEmptyParts);

				if (!parts.isEmpty())
				{
					fileName = parts.back();
				}
			}

			for (DwarfEntry* child = entry->firstChild; child != nullptr; child = child->sibling)
			{
				bool isVariableEntry = true;
				bool isPublic = false;

				switch (child->tag)
				{
				case DW_TAG_global_variable:
					isPublic = true;
					break;
				case DW_TAG_local_variable:
					isPublic = false;
					break;
				default:
					isVariableEntry = false;
					break;
				}

				if (isVariableEntry)
				{
					VariablesModelItem item;
					item.dwarfOffset = child->offset;
					item.isPublic = isPublic;
					item.address = 0;
					item.fileName = fileName;

					DwarfAttribute* locationAttribute = nullptr;

					for (int i = 0; i < child->attributeCount; i++)
					{
						DwarfAttribute* attr = &child->attributes[i];

						switch (attr->name)
						{
						case DW_AT_name:
							item.name = attr->string;
							break;
						case DW_AT_MW_mangled:
							item.name = attr->string;
							break;
						case DW_AT_location:
							locationAttribute = attr;
							break;
						}
					}

					if (locationAttribute)
					{
						DwarfLocation location;
						location.read(dwarf(), locationAttribute);

						for (const DwarfLocationAtom& atom : location.atoms)
						{
							switch (atom.op)
							{
							case DW_OP_ADDR:
								item.address = atom.addr;
								break;
							}
						}
					}

					m_items.append(item);
				}
			}
		}
	}
}

QModelIndex VariablesModel::index(int row, int column, const QModelIndex& parent) const
{
	if (!hasIndex(row, column, parent))
	{
		return QModelIndex();
	}

	if (!parent.isValid())
	{
		return createIndex(row, column, &m_items[row]);
	}

	return QModelIndex();
}

QModelIndex VariablesModel::parent(const QModelIndex& child) const
{
	return QModelIndex();
}

int VariablesModel::rowCount(const QModelIndex& parent) const
{
	if (!parent.isValid())
	{
		return m_items.size();
	}

	return 0;
}

int VariablesModel::columnCount(const QModelIndex& parent) const
{
	return ColumnCount;
}

QVariant VariablesModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid())
	{
		return QVariant();
	}

	if (role != Qt::DisplayRole)
	{
		return QVariant();
	}

	const VariablesModelItem* item = (VariablesModelItem*)index.internalPointer();

	switch (index.column())
	{
	case NameColumn:
		return item->name;
	case FileColumn:
		return item->fileName;
	case AddressColumn:
		return QString("0x%1").arg(item->address, 8, 16, QLatin1Char('0'));
	case PublicColumn:
		return item->isPublic ? "P" : QString();
	case DwarfOffsetColumn:
		return Util::hexToString(item->dwarfOffset);
	}

	return QVariant();
}

Qt::ItemFlags VariablesModel::flags(const QModelIndex& index) const
{
	if (!index.isValid())
	{
		return Qt::NoItemFlags;
	}

	return QAbstractItemModel::flags(index);
}

QVariant VariablesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole)
	{
		return QVariant();
	}

	if (orientation != Qt::Horizontal)
	{
		return QVariant();
	}

	switch (section)
	{
	case NameColumn:
		return tr("Name");
	case FileColumn:
		return tr("File");
	case AddressColumn:
		return tr("Address");
	case PublicColumn:
		return tr("Public");
	case DwarfOffsetColumn:
		return tr("DWARF Offset");
	}

	return QVariant();
}

void VariablesModel::sort(int column, Qt::SortOrder order)
{
	beginResetModel();

	if (order == Qt::AscendingOrder)
	{
		switch (column)
		{
		case NameColumn:
			std::sort(m_items.begin(), m_items.end(), [](const auto& a, const auto& b) { return a.name < b.name; });
			break;
		case FileColumn:
			std::sort(m_items.begin(), m_items.end(), [](const auto& a, const auto& b) { return a.fileName < b.fileName; });
			break;
		case AddressColumn:
			std::sort(m_items.begin(), m_items.end(), [](const auto& a, const auto& b) { return a.address < b.address; });
			break;
		case PublicColumn:
			std::sort(m_items.begin(), m_items.end(), [](const auto& a, const auto&) { return !a.isPublic; });
			break;
		case DwarfOffsetColumn:
			std::sort(m_items.begin(), m_items.end(), [](const auto& a, const auto& b) { return a.dwarfOffset < b.dwarfOffset; });
			break;
		}
	}
	else
	{
		switch (column)
		{
		case NameColumn:
			std::sort(m_items.begin(), m_items.end(), [](const auto& a, const auto& b) { return a.name > b.name; });
			break;
		case FileColumn:
			std::sort(m_items.begin(), m_items.end(), [](const auto& a, const auto& b) { return a.fileName > b.fileName; });
			break;
		case AddressColumn:
			std::sort(m_items.begin(), m_items.end(), [](const auto& a, const auto& b) { return a.address > b.address; });
			break;
		case PublicColumn:
			std::sort(m_items.begin(), m_items.end(), [](const auto& a, const auto&) { return a.isPublic; });
			break;
		case DwarfOffsetColumn:
			std::sort(m_items.begin(), m_items.end(), [](const auto& a, const auto& b) { return a.dwarfOffset > b.dwarfOffset; });
			break;
		}
	}

	endResetModel();
}

QString VariablesModel::name(const QModelIndex& index) const
{
	if (!index.isValid())
	{
		return QString();
	}

	const VariablesModelItem* item = (VariablesModelItem*)index.internalPointer();

	return item->name;
}

Elf32_Addr VariablesModel::address(const QModelIndex& index) const
{
	if (!index.isValid())
	{
		return 0;
	}

	const VariablesModelItem* item = (VariablesModelItem*)index.internalPointer();

	return item->address;
}

Elf32_Off VariablesModel::dwarfOffset(const QModelIndex& index) const
{
	if (!index.isValid())
	{
		return 0;
	}

	const VariablesModelItem* item = (VariablesModelItem*)index.internalPointer();

	return item->dwarfOffset;
}
