#include "CppCodeModel.h"

#include "Output.h"
#include "Util.h"

#include <qdir.h>

CppCodeModelSettings CppCodeModel::s_defaultSettings
{
	false, // printUnknownEntries
	false, // printUnknownAttributes
	true, // writeTypes
	true, // writeFunctionDeclarations
	true, // writeFunctionDefinitions
	false, // writeDwarfEntryOffsets
	true, // writeClassSizes
	true, // writeClassMemberOffsets
	true, // writeFunctionAddresses
	true, // writeFunctionSizes
	false, // writeFunctionVariableLocations
	true, // sortTypesAlphabetically
	true, // inlineMetrowerksUnnamedTypes
	false, // hexadecimalEnumValues
	false, // forceExplicitEnumValues

	// fundamentalTypeNames
	{
		{ Cpp::FundamentalType::Char, "char" },
		{ Cpp::FundamentalType::SignedChar, "signed char" },
		{ Cpp::FundamentalType::UnsignedChar, "unsigned char" },
		{ Cpp::FundamentalType::Short, "short" },
		{ Cpp::FundamentalType::SignedShort, "signed short" },
		{ Cpp::FundamentalType::UnsignedShort, "unsigned short" },
		{ Cpp::FundamentalType::Int, "int" },
		{ Cpp::FundamentalType::SignedInt, "signed int" },
		{ Cpp::FundamentalType::UnsignedInt, "unsigned int" },
		{ Cpp::FundamentalType::Long, "long" },
		{ Cpp::FundamentalType::SignedLong, "signed long" },
		{ Cpp::FundamentalType::UnsignedLong, "unsigned long" },
		{ Cpp::FundamentalType::VoidPointer, "void*" },
		{ Cpp::FundamentalType::Float, "float" },
		{ Cpp::FundamentalType::Double, "double" },
		{ Cpp::FundamentalType::Void, "void" },
		{ Cpp::FundamentalType::Bool, "bool" },
		{ Cpp::FundamentalType::LongLong, "long long" },
	},
};

QHash<Cpp::Keyword, QString> CppCodeModel::s_keywordToStringMap =
{
	{ Cpp::Keyword::Class, "class" },
	{ Cpp::Keyword::Const, "const" },
	{ Cpp::Keyword::Enum, "enum" },
	{ Cpp::Keyword::Inline, "inline" },
	{ Cpp::Keyword::Private, "private" },
	{ Cpp::Keyword::Protected, "protected" },
	{ Cpp::Keyword::Public, "public" },
	{ Cpp::Keyword::Static, "static" },
	{ Cpp::Keyword::Struct, "struct" },
	{ Cpp::Keyword::Typedef, "typedef" },
	{ Cpp::Keyword::Union, "union" },
	{ Cpp::Keyword::Volatile, "volatile" },
};

CppCodeModel::CppCodeModel(QObject* parent)
	: AbstractCodeModel(parent)
	, m_pathToOffsetMultiMap()
	, m_offsetToEntryMap()
	, m_offsetToFileMap()
	, m_offsetToClassTypeMap()
	, m_offsetToEnumTypeMap()
	, m_offsetToArrayTypeMap()
	, m_offsetToFunctionTypeMap()
	, m_offsetToFunctionMap()
	, m_indentLevel(0)
	, m_settings(s_defaultSettings)
{
}

CppCodeModelSettings& CppCodeModel::defaultSettings()
{
	return s_defaultSettings;
}

CppCodeModelSettings& CppCodeModel::settings()
{
	return m_settings;
}

const CppCodeModelSettings& CppCodeModel::settings() const
{
	return m_settings;
}

void CppCodeModel::clear()
{
	beginResetModel();
	
	m_pathToOffsetMultiMap.clear();
	m_offsetToEntryMap.clear();
	m_offsetToFileMap.clear();
	m_offsetToClassTypeMap.clear();
	m_offsetToEnumTypeMap.clear();
	m_offsetToArrayTypeMap.clear();
	m_offsetToFunctionTypeMap.clear();
	m_offsetToFunctionMap.clear();

	endResetModel();
}

void CppCodeModel::parseDwarf(Dwarf* dwarf)
{
	clear();

	if (!dwarf)
	{
		return;
	}

	beginResetModel();

	if (dwarf->entryCount > 0)
	{
		for (int i = 0; i < dwarf->entryCount; i++)
		{
			DwarfEntry* entry = &dwarf->entries[i];

			m_offsetToEntryMap[entry->offset] = entry;
		}

		for (DwarfEntry* entry = &dwarf->entries[0]; entry != nullptr; entry = entry->sibling)
		{
			switch (entry->tag)
			{
			case DW_TAG_compile_unit:
				parseCompileUnit(entry);
				break;
			default:
				warnUnknownEntry(entry, nullptr);
				break;
			}
		}
	}

	endResetModel();
}

void CppCodeModel::parseCompileUnit(DwarfEntry* entry)
{
	Cpp::File& file = m_offsetToFileMap[entry->offset];

	file.entry = entry;
	file.path = entry->getName();

	for (DwarfEntry* child = entry->firstChild; child != nullptr; child = child->sibling)
	{
		switch (child->tag)
		{
		case DW_TAG_class_type:
		case DW_TAG_structure_type:
			parseClassType(child, file);
			break;
		case DW_TAG_enumeration_type:
			parseEnumerationType(child, file);
			break;
		case DW_TAG_array_type:
			parseArrayType(child, file);
			break;
		case DW_TAG_subroutine_type:
			parseSubroutineType(child, file);
			break;
		case DW_TAG_subroutine:
		case DW_TAG_global_subroutine:
		case DW_TAG_inlined_subroutine:
			parseSubroutine(child, file);
			break;
		default:
			warnUnknownEntry(child, entry);
			break;
		}
	}

	m_pathToOffsetMultiMap.insert(file.path, entry->offset);
}

void CppCodeModel::parseClassType(DwarfEntry* entry, Cpp::File& file)
{
	Cpp::ClassType& c = m_offsetToClassTypeMap[entry->offset];

	c.entry = entry;
	c.keyword = (entry->tag == DW_TAG_class_type) ? Cpp::Keyword::Class : Cpp::Keyword::Struct;
	c.name = entry->getName();
	c.size = 0;

	if (DwarfAttribute* a = entry->findAttribute(DW_AT_byte_size))
	{
		c.size = a->data4;
	}

	for (DwarfEntry* child = entry->firstChild; child != nullptr; child = child->sibling)
	{
		switch (child->tag)
		{
		case DW_TAG_member:
			parseMember(child, c);
			break;
		case DW_TAG_inheritance:
			parseInheritance(child, c);
			break;
		default:
			warnUnknownEntry(child, entry);
			break;
		}
	}

	file.typeOffsets.append(entry->offset);
}

void CppCodeModel::parseMember(DwarfEntry* entry, Cpp::ClassType& c)
{
	Cpp::ClassMember m;

	m.type.isFundamental = true;
	m.type.fundType = Cpp::FundamentalType::Int;
	m.type.isConst = false;
	m.type.isVolatile = false;
	m.entry = entry;
	m.access = Cpp::Keyword::Public;
	m.offset = 0;

	DwarfAttribute* typeAttribute = nullptr;
	DwarfAttribute* locationAttribute = nullptr;

	for (int i = 0; i < entry->attributeCount; i++)
	{
		DwarfAttribute* attr = &entry->attributes[i];

		switch (attr->name)
		{
		// Name attribute
		case DW_AT_name:
			m.name = attr->string;
			break;

		// Access attribute
		case DW_AT_public:
			m.access = Cpp::Keyword::Public;
			break;
		case DW_AT_private:
			m.access = Cpp::Keyword::Private;
			break;
		case DW_AT_protected:
			m.access = Cpp::Keyword::Protected;
			break;

		// Type attribute
		case DW_AT_fund_type:
		case DW_AT_user_def_type:
		case DW_AT_mod_fund_type:
		case DW_AT_mod_u_d_type:
			typeAttribute = attr;
			break;

		// Location attribute
		case DW_AT_location:
			locationAttribute = attr;
			break;

		// Unknown attribute
		default:
			warnUnknownAttribute(attr, entry);
			break;
		}
	}

	if (typeAttribute)
	{
		DwarfType dt;
		dt.read(dwarf(), typeAttribute);

		parseType(dt, m.type);
	}

	m.offset = 0;

	if (locationAttribute)
	{
		DwarfLocation location;
		location.read(dwarf(), locationAttribute);

		for (const DwarfLocationAtom& atom : location.atoms)
		{
			if (atom.op == DW_OP_CONST)
			{
				m.offset = atom.number;
				break;
			}
		}
	}

	c.members.append(m);
}

void CppCodeModel::parseInheritance(DwarfEntry* entry, Cpp::ClassType& c)
{
	Cpp::ClassInheritance in;

	in.entry = entry;
	in.access = (c.keyword == Cpp::Keyword::Class) ? Cpp::Keyword::Private : Cpp::Keyword::Public;
	in.isVirtual = false;

	DwarfAttribute* typeAttribute = nullptr;

	for (int i = 0; i < entry->attributeCount; i++)
	{
		DwarfAttribute* attr = &entry->attributes[i];

		switch (attr->name)
		{
		// Access attribute
		case DW_AT_public:
			in.access = Cpp::Keyword::Public;
			break;
		case DW_AT_private:
			in.access = Cpp::Keyword::Private;
			break;
		case DW_AT_protected:
			in.access = Cpp::Keyword::Protected;
			break;

		// Type attribute
		case DW_AT_fund_type:
		case DW_AT_user_def_type:
		case DW_AT_mod_fund_type:
		case DW_AT_mod_u_d_type:
			typeAttribute = attr;
			break;

		// Virtual attribute
		case DW_AT_virtual:
			in.isVirtual = true;
			break;

		// Unknown attribute
		default:
			warnUnknownAttribute(attr, entry);
			break;
		}
	}

	if (typeAttribute)
	{
		DwarfType dt;
		dt.read(dwarf(), typeAttribute);

		parseType(dt, in.type);
	}

	c.inheritances.append(in);
}

void CppCodeModel::parseEnumerationType(DwarfEntry* entry, Cpp::File& file)
{
	Cpp::EnumType& e = m_offsetToEnumTypeMap[entry->offset];

	e.entry = entry;

	DwarfAttribute* elementListAttribute = nullptr;

	for (int i = 0; i < entry->attributeCount; i++)
	{
		DwarfAttribute* attr = &entry->attributes[i];

		switch (attr->name)
		{
		// Name attribute
		case DW_AT_name:
			e.name = attr->string;
			break;

		// Element list attribute
		case DW_AT_element_list:
			elementListAttribute = attr;
			break;

		// Unknown attribute
		default:
			warnUnknownAttribute(attr, entry);
			break;
		}
	}

	if (elementListAttribute)
	{
		DwarfElementList list;
		list.read(dwarf(), elementListAttribute);

		for (DwarfElementListItem& item : list.items)
		{
			Cpp::EnumElement element;
			element.name = item.name;
			element.value = item.value;

			e.elements.append(element);
		}
	}

	file.typeOffsets.append(entry->offset);
}

void CppCodeModel::parseArrayType(DwarfEntry* entry, Cpp::File& file)
{
	Cpp::ArrayType& a = m_offsetToArrayTypeMap[entry->offset];

	a.entry = entry;
	a.type.isFundamental = true;
	a.type.fundType = Cpp::FundamentalType::Int;
	a.type.isConst = false;
	a.type.isVolatile = false;

	DwarfAttribute* subscrDataAttribute = nullptr;

	for (int i = 0; i < entry->attributeCount; i++)
	{
		DwarfAttribute* attr = &entry->attributes[i];

		switch (attr->name)
		{
		// Name attribute
		case DW_AT_name:
			a.name = attr->string;
			break;

		// Subscript data attribute
		case DW_AT_subscr_data:
			subscrDataAttribute = attr;
			break;

		// Unknown attribute
		default:
			warnUnknownAttribute(attr, entry);
			break;
		}
	}

	if (subscrDataAttribute)
	{
		DwarfSubscriptData data;
		data.read(dwarf(), subscrDataAttribute);

		parseType(data.elementType, a.type);

		for (DwarfSubscriptDataItem& item : data.items)
		{
			int dimension = 1;

			if (item.lowBound.isConstant)
			{
				dimension -= item.lowBound.constant;
			}

			if (item.highBound.isConstant)
			{
				dimension += item.highBound.constant;
			}

			a.dimensions.prepend(dimension);
		}
	}

	file.typeOffsets.append(entry->offset);
}

void CppCodeModel::parseSubroutineType(DwarfEntry* entry, Cpp::File& file)
{
	Cpp::FunctionType& f = m_offsetToFunctionTypeMap[entry->offset];

	f.entry = entry;
	f.type.isFundamental = true;
	f.type.fundType = Cpp::FundamentalType::Void;
	f.type.isConst = false;
	f.type.isVolatile = false;

	DwarfAttribute* typeAttribute = nullptr;

	for (int i = 0; i < entry->attributeCount; i++)
	{
		DwarfAttribute* attr = &entry->attributes[i];

		switch (attr->name)
		{
		// Name attribute
		case DW_AT_name:
			f.name = attr->string;
			break;

		// Type attribute
		case DW_AT_fund_type:
		case DW_AT_user_def_type:
		case DW_AT_mod_fund_type:
		case DW_AT_mod_u_d_type:
			typeAttribute = attr;
			break;

		// Unknown attribute
		default:
			warnUnknownAttribute(attr, entry);
			break;
		}
	}

	if (typeAttribute)
	{
		DwarfType dt;
		dt.read(dwarf(), typeAttribute);

		parseType(dt, f.type);
	}

	for (DwarfEntry* child = entry->firstChild; child != nullptr; child = child->sibling)
	{
		switch (child->tag)
		{
		case DW_TAG_formal_parameter:
			parseFormalParameter(child, f);
			break;
		default:
			warnUnknownEntry(child, entry);
			break;
		}
	}

	file.typeOffsets.append(entry->offset);
}

void CppCodeModel::parseSubroutine(DwarfEntry* entry, Cpp::File& file)
{
	Cpp::Function& f = m_offsetToFunctionMap[entry->offset];

	f.entry = entry;
	f.type.isFundamental = true;
	f.type.fundType = Cpp::FundamentalType::Void;
	f.type.isConst = false;
	f.type.isVolatile = false;
	f.isGlobal = (entry->tag == DW_TAG_global_subroutine);
	f.isInline = (entry->tag == DW_TAG_inlined_subroutine);
	f.startAddress = 0;
	f.endAddress = 0;

	DwarfAttribute* typeAttribute = nullptr;

	for (int i = 0; i < entry->attributeCount; i++)
	{
		DwarfAttribute* attr = &entry->attributes[i];

		switch (attr->name)
		{
		// Name attribute
		case DW_AT_name:
			f.name = attr->string;
			break;

		// Type attribute
		case DW_AT_fund_type:
		case DW_AT_user_def_type:
		case DW_AT_mod_fund_type:
		case DW_AT_mod_u_d_type:
			typeAttribute = attr;
			break;

		// PC attributes
		case DW_AT_low_pc:
			f.startAddress = attr->addr;
			break;
		case DW_AT_high_pc:
			f.endAddress = attr->addr;
			break;

		// Inline attribute
		case DW_AT_inline:
			f.isInline = true;
			break;

		// Unknown attribute
		default:
			warnUnknownAttribute(attr, entry);
			break;
		}
	}

	if (typeAttribute)
	{
		DwarfType dt;
		dt.read(dwarf(), typeAttribute);

		parseType(dt, f.type);
	}

	for (DwarfEntry* child = entry->firstChild; child != nullptr; child = child->sibling)
	{
		switch (child->tag)
		{
		case DW_TAG_formal_parameter:
			parseFormalParameter(child, f);
			break;
		case DW_TAG_local_variable:
			parseLocalVariable(child, f);
			break;
		default:
			warnUnknownEntry(child, entry);
			break;
		}
	}

	file.functionOffsets.append(entry->offset);
}

void CppCodeModel::parseFormalParameter(DwarfEntry* entry, Cpp::FunctionType& f)
{
	Cpp::FunctionParameter p;

	p.entry = entry;
	p.type.isFundamental = false;
	p.type.fundType = Cpp::FundamentalType::Int;
	p.type.isConst = false;
	p.type.isVolatile = false;

	DwarfAttribute* typeAttribute = nullptr;
	DwarfAttribute* locationAttribute = nullptr;

	for (int i = 0; i < entry->attributeCount; i++)
	{
		DwarfAttribute* attr = &entry->attributes[i];

		switch (attr->name)
		{
		// Name attribute
		case DW_AT_name:
			p.name = attr->string;
			break;

		// Type attribute
		case DW_AT_fund_type:
		case DW_AT_user_def_type:
		case DW_AT_mod_fund_type:
		case DW_AT_mod_u_d_type:
			typeAttribute = attr;
			break;

		// Location attribute
		case DW_AT_location:
			locationAttribute = attr;
			break;
		
		// Unknown attribute
		default:
			warnUnknownAttribute(attr, entry);
			break;
		}
	}

	if (typeAttribute)
	{
		DwarfType dt;
		dt.read(dwarf(), typeAttribute);

		parseType(dt, p.type);
	}

	if (locationAttribute)
	{
		DwarfLocation location;
		location.read(dwarf(), locationAttribute);

		for (const DwarfLocationAtom& atom : location.atoms)
		{
			switch (atom.op)
			{
			case DW_OP_REG:
				p.location = QString("r%1").arg(atom.number);
				break;
			}
		}
	}

	f.parameters.append(p);
}

void CppCodeModel::parseLocalVariable(DwarfEntry* entry, Cpp::Function& f)
{
	Cpp::LocalVariable v;

	v.entry = entry;
	v.type.isFundamental = true;
	v.type.fundType = Cpp::FundamentalType::Int;
	v.type.isConst = false;
	v.type.isVolatile = false;

	DwarfAttribute* typeAttribute = nullptr;
	DwarfAttribute* locationAttribute = nullptr;

	for (int i = 0; i < entry->attributeCount; i++)
	{
		DwarfAttribute* attr = &entry->attributes[i];

		switch (attr->name)
		{
		// Name attribute
		case DW_AT_name:
			v.name = attr->string;
			break;

		// Type attribute
		case DW_AT_fund_type:
		case DW_AT_user_def_type:
		case DW_AT_mod_fund_type:
		case DW_AT_mod_u_d_type:
			typeAttribute = attr;
			break;

		// Location attribute
		case DW_AT_location:
			locationAttribute = attr;
			break;

		// Unknown attribute
		default:
			warnUnknownAttribute(attr, entry);
			break;
		}
	}

	if (typeAttribute)
	{
		DwarfType dt;
		dt.read(dwarf(), typeAttribute);

		parseType(dt, v.type);
	}

	if (locationAttribute)
	{
		DwarfLocation location;
		location.read(dwarf(), locationAttribute);

		for (const DwarfLocationAtom& atom : location.atoms)
		{
			switch (atom.op)
			{
			case DW_OP_REG:
				v.location = QString("r%1").arg(atom.number);
				break;
			}
		}
	}

	f.localVariables.append(v);
}

void CppCodeModel::parseType(DwarfType& dt, Cpp::Type& t)
{
	t.isConst = false;
	t.isVolatile = false;
	t.isFundamental = dt.isFundamental;

	if (dt.isFundamental)
	{
		t.fundType = (Cpp::FundamentalType)dt.fundType;
	}
	else
	{
		t.userTypeOffset = dt.udTypeOffset;
	}

	if (!dt.modifiers.empty())
	{
		Cpp::Modifier modifier;
		modifier.isConst = false;
		modifier.isVolatile = false;

		for (int i = 0; i < dt.modifiers.size(); i++)
		{
			char mod = dt.modifiers[i];

			if (mod == DW_MOD_pointer_to)
			{
				modifier.type = Cpp::ModifierType::Pointer;

				t.modifiers.prepend(modifier);

				// reset modifier
				modifier.isConst = false;
				modifier.isVolatile = false;
			}
			else if (mod == DW_MOD_reference_to)
			{
				modifier.type = Cpp::ModifierType::Reference;

				t.modifiers.prepend(modifier);

				// reset modifier
				modifier.isConst = false;
				modifier.isVolatile = false;
			}
			else if (mod == DW_MOD_const && i < dt.modifiers.size() - 1)
			{
				modifier.isConst = true;
			}
			else if (mod == DW_MOD_volatile && i < dt.modifiers.size() - 1)
			{
				modifier.isVolatile = true;
			}
		}

		if (dt.modifiers.back() == DW_MOD_const)
		{
			t.isConst = true;
		}
		else if (dt.modifiers.back() == DW_MOD_volatile)
		{
			t.isVolatile = true;
		}
	}
}

void CppCodeModel::warnUnknownEntry(DwarfEntry* child, DwarfEntry* parent)
{
	if (!m_settings.printUnknownEntries)
	{
		return;
	}

	// Filter out some common entries
	switch (child->tag)
	{
	case DW_TAG_padding:
		return;
	}

	Output::write(tr("Unknown child entry %1 (%2) at offset %3 in parent entry %4 (%5)")
		.arg(Dwarf::tagToString(child->tag))
		.arg(child->getName())
		.arg(Util::hexToString(child->offset))
		.arg(parent ? Dwarf::tagToString(parent->tag) : tr("(root)"))
		.arg(parent ? parent->getName() : QString()));
}

void CppCodeModel::warnUnknownAttribute(DwarfAttribute* attribute, DwarfEntry* entry)
{
	if (!m_settings.printUnknownAttributes)
	{
		return;
	}

	// Filter out some common attributes
	switch (attribute->name)
	{
	case DW_AT_sibling:
		return;
	}

	Output::write(tr("Unknown attribute %1 at offset %2 in entry %3 (%4)")
		.arg(Dwarf::attrNameToString(attribute->name))
		.arg(Util::hexToString(attribute->offset))
		.arg(Dwarf::tagToString(entry->tag))
		.arg(entry->getName()));
}

void CppCodeModel::writeDwarfEntry(Code& code, Elf32_Off offset)
{
	resetIndent();

	Q_ASSERT(m_offsetToEntryMap.contains(offset));

	DwarfEntry* entry = m_offsetToEntryMap[offset];

	switch (entry->tag)
	{
	case DW_TAG_compile_unit:
		writeFiles(code, { offset });
		break;
	case DW_TAG_class_type:
	case DW_TAG_structure_type:
		writeClassType(code, m_offsetToClassTypeMap[offset]);
		writePunctuation(code, ";");
		break;
	case DW_TAG_enumeration_type:
		writeEnumType(code, m_offsetToEnumTypeMap[offset]);
		writePunctuation(code, ";");
		break;
	case DW_TAG_array_type:
		writeArrayType(code, m_offsetToArrayTypeMap[offset]);
		writePunctuation(code, ";");
		break;
	case DW_TAG_subroutine_type:
		writeFunctionType(code, m_offsetToFunctionTypeMap[offset]);
		writePunctuation(code, ";");
		break;
	case DW_TAG_subroutine:
	case DW_TAG_global_subroutine:
		writeFunctionDefinition(code, m_offsetToFunctionMap[offset]);
		break;
	}
}

void CppCodeModel::writeFile(Code& code, const QString& path)
{
	if (!m_pathToOffsetMultiMap.contains(path))
	{
		return;
	}

	writeFiles(code, m_pathToOffsetMultiMap.values(path));
}

void CppCodeModel::writeFiles(Code& code, const QList<Elf32_Off>& fileOffsets)
{
	if (fileOffsets.isEmpty())
	{
		return;
	}

#ifdef QT_DEBUG
	for (Elf32_Off fileOffset : fileOffsets)
	{
		Q_ASSERT(m_offsetToFileMap.contains(fileOffset));
	}
#endif

	if (m_settings.writeDwarfEntryOffsets)
	{
		for (Elf32_Off fileOffset : fileOffsets)
		{
			Cpp::File& file = m_offsetToFileMap[fileOffset];

			writeComment(code, QString("DWARF: %1").arg(Util::hexToString(file.entry->offset)));
			writeNewline(code);
			writeNewline(code);
		}
	}

	if (m_settings.writeTypes)
	{
		for (Elf32_Off fileOffset : fileOffsets)
		{
			Cpp::File& file = m_offsetToFileMap[fileOffset];
			QList<DwarfEntry*> entries;

			for (Elf32_Off typeOffset : file.typeOffsets)
			{
				DwarfEntry* entry = m_offsetToEntryMap[typeOffset];

				if (!typeCanBeInlined(entry->getName()))
				{
					entries.append(entry);
				}
			}

			if (m_settings.sortTypesAlphabetically)
			{
				std::sort(entries.begin(), entries.end(),
					[](DwarfEntry* a, DwarfEntry* b)
					{
						return stricmp(a->getName(), b->getName()) < 0;
					});
			}

			for (DwarfEntry* entry : entries)
			{
				writeDwarfEntry(code, entry->offset);
				writeNewline(code);
				writeNewline(code);
			}
		}
	}

	if (m_settings.writeFunctionDeclarations)
	{
		for (Elf32_Off fileOffset : fileOffsets)
		{
			Cpp::File& file = m_offsetToFileMap[fileOffset];

			for (Elf32_Off functionOffset : file.functionOffsets)
			{
				Cpp::Function& f = m_offsetToFunctionMap[functionOffset];

				writeFunctionDeclaration(code, f);
				writePunctuation(code, ";");
				writeNewline(code);
			}
		}

		writeNewline(code);
	}

	if (m_settings.writeFunctionDefinitions)
	{
		for (Elf32_Off fileOffset : fileOffsets)
		{
			Cpp::File& file = m_offsetToFileMap[fileOffset];

			for (Elf32_Off functionOffset : file.functionOffsets)
			{
				Cpp::Function& f = m_offsetToFunctionMap[functionOffset];

				writeFunctionDefinition(code, f);
				writeNewline(code);
				writeNewline(code);
			}
		}
	}
}

void CppCodeModel::writeClassType(Code& code, Cpp::ClassType& c, bool isInline)
{
	QStringList comment;

	if (m_settings.writeClassSizes)
	{
		comment += QString("Size: %1").arg(Util::hexToString(c.size));
	}

	if (m_settings.writeDwarfEntryOffsets)
	{
		comment += QString("DWARF: %1").arg(Util::hexToString(c.entry->offset));
	}

	if (!comment.isEmpty())
	{
		writeComment(code, comment.join(", "));
		writeNewline(code);
	}
	
	writeKeyword(code, c.keyword);

	if (!isInline && !c.name.isEmpty())
	{
		writeSpace(code);
		writeIdentifier(code, c.name);
	}

	if (!c.inheritances.isEmpty())
	{
		writeSpace(code);
		writePunctuation(code, ":");
		writeSpace(code);

		for (int i = 0; i < c.inheritances.size(); i++)
		{
			Cpp::ClassInheritance& in = c.inheritances[i];
			bool explicitAccess = false;

			if (c.keyword == Cpp::Keyword::Class && in.access != Cpp::Keyword::Private)
			{
				explicitAccess = true;
			}
			else if (c.keyword == Cpp::Keyword::Struct && in.access != Cpp::Keyword::Public)
			{
				explicitAccess = true;
			}

			if (explicitAccess)
			{
				writeKeyword(code, in.access);
				writeSpace(code);
			}

			writeTypePrefix(code, in.type);
			writeTypePostfix(code, in.type);

			if (i < c.inheritances.size() - 1)
			{
				writePunctuation(code, ",");
				writeSpace(code);
			}
		}
	}
	
	writeNewline(code);
	writePunctuation(code, "{");

	if (!c.members.empty())
	{
		Cpp::Keyword prevAccess;

		switch (c.keyword)
		{
		case Cpp::Keyword::Class:
		{
			prevAccess = (Cpp::Keyword)-1;
			break;
		}
		case Cpp::Keyword::Struct:
		{
			prevAccess = Cpp::Keyword::Public;

			for (Cpp::ClassMember& m : c.members)
			{
				if (m.access != Cpp::Keyword::Public)
				{
					prevAccess = (Cpp::Keyword)-1;
					break;
				}
			}

			break;
		}
		case Cpp::Keyword::Union:
		{
			prevAccess = Cpp::Keyword::Public;
			break;
		}
		default:
		{
			prevAccess = (Cpp::Keyword)-1;
			break;
		}
		}

		increaseIndent();

		bool first = true;

		for (Cpp::ClassMember& m : c.members)
		{
			if (m.access != prevAccess)
			{
				writeNewline(code, false);

				if (!first)
				{
					writeNewline(code, false);
				}

				writeKeyword(code, m.access);
				writePunctuation(code, ":");
			}

			writeNewline(code);
			writeClassMember(code, m);

			prevAccess = m.access;
			first = false;
		}

		decreaseIndent();
	}

	writeNewline(code);
	writePunctuation(code, "}");
}

void CppCodeModel::writeClassMember(Code& code, Cpp::ClassMember& m)
{
	writeDeclaration(code, m);
	writePunctuation(code, ";");

	QStringList comment;

	if (m_settings.writeClassMemberOffsets)
	{
		comment += QString("Offset: %1").arg(Util::hexToString(m.offset));
	}

	if (m_settings.writeDwarfEntryOffsets)
	{
		comment += QString("DWARF: %1").arg(Util::hexToString(m.entry->offset));
	}

	if (!comment.isEmpty())
	{
		writeSpace(code);
		writeComment(code, comment.join(", "));
	}
}

void CppCodeModel::writeEnumType(Code& code, Cpp::EnumType& e, bool isInline)
{
	QStringList comment;

	if (m_settings.writeDwarfEntryOffsets)
	{
		comment += QString("DWARF: %1").arg(Util::hexToString(e.entry->offset));
	}

	if (!comment.isEmpty())
	{
		writeComment(code, comment.join(", "));
		writeNewline(code);
	}

	writeKeyword(code, Cpp::Keyword::Enum);

	if (!isInline && !e.name.isEmpty())
	{
		writeSpace(code);
		writeIdentifier(code, e.name);
	}

	writeNewline(code);
	writePunctuation(code, "{");

	if (!e.elements.empty())
	{
		int prevValue = -1;

		increaseIndent();

		for (int i = 0; i < e.elements.size(); i++)
		{
			Cpp::EnumElement& el = e.elements[i];
			bool explicitValue = (el.value != prevValue + 1);

			writeNewline(code);
			writeEnumElement(code, el, explicitValue);

			if (i < e.elements.size() - 1)
			{
				writePunctuation(code, ",");
			}

			prevValue = el.value;
		}

		decreaseIndent();
	}

	writeNewline(code);
	writePunctuation(code, "}");
}

void CppCodeModel::writeEnumElement(Code& code, Cpp::EnumElement& e, bool explicitValue)
{
	writeIdentifier(code, e.name);

	if (explicitValue || m_settings.forceExplicitEnumValues)
	{
		writeSpace(code);
		writePunctuation(code, "=");
		writeSpace(code);

		if (m_settings.hexadecimalEnumValues)
		{
			writeLiteral(code, Util::hexToString(e.value));
		}
		else
		{
			writeLiteral(code, QString("%1").arg(e.value));
		}
	}
}

void CppCodeModel::writeArrayType(Code& code, Cpp::ArrayType& a, bool isInline)
{
	QStringList comment;

	if (m_settings.writeDwarfEntryOffsets)
	{
		comment += QString("DWARF: %1").arg(Util::hexToString(a.entry->offset));
	}

	if (!comment.isEmpty())
	{
		writeComment(code, comment.join(", "));
		writeNewline(code);
	}

	writeKeyword(code, Cpp::Keyword::Typedef);
	writeSpace(code);
	writeArrayTypePrefix(code, a);

	if (!isInline && !a.name.isEmpty())
	{
		writeIdentifier(code, a.name);
	}

	writeArrayTypePostfix(code, a);
}

void CppCodeModel::writeFunctionType(Code& code, Cpp::FunctionType& f, bool isInline)
{
	QStringList comment;

	if (m_settings.writeDwarfEntryOffsets)
	{
		comment += QString("DWARF: %1").arg(Util::hexToString(f.entry->offset));
	}

	if (!comment.isEmpty())
	{
		writeComment(code, comment.join(", "));
		writeNewline(code);
	}

	writeKeyword(code, Cpp::Keyword::Typedef);
	writeSpace(code);
	writeFunctionTypePrefix(code, f);

	if (!isInline && !f.name.isEmpty())
	{
		writeIdentifier(code, f.name);
	}

	writeFunctionTypePostfix(code, f);
}

void CppCodeModel::writeFunctionDeclaration(Code& code, Cpp::Function& f)
{
	if (!f.isGlobal)
	{
		writeKeyword(code, Cpp::Keyword::Static);
		writeSpace(code);
	}

	if (f.isInline)
	{
		writeKeyword(code, Cpp::Keyword::Inline);
		writeSpace(code);
	}

	writeTypePrefix(code, f.type);
	writeTypePostfix(code, f.type);

	if (!f.name.isEmpty())
	{
		writeSpace(code);
		writeIdentifier(code, f.name);
	}

	writeFunctionParameters(code, f);
}

void CppCodeModel::writeFunctionDefinition(Code& code, Cpp::Function& f)
{
	QStringList comment;

	if (m_settings.writeFunctionAddresses)
	{
		comment += QString("Address: %1").arg(Util::hexToString(f.startAddress));
	}

	if (m_settings.writeFunctionSizes)
	{
		comment += QString("Size: %1").arg(Util::hexToString(f.endAddress - f.startAddress));
	}

	if (!comment.isEmpty())
	{
		writeComment(code, comment.join(", "));
		writeNewline(code);
	}

	writeFunctionDeclaration(code, f);
	writeNewline(code);
	writePunctuation(code, "{");

	increaseIndent();

	for (Cpp::LocalVariable& v : f.localVariables)
	{
		writeNewline(code);
		writeLocalVariable(code, v);
	}

	decreaseIndent();

	writeNewline(code);
	writePunctuation(code, "}");
}

void CppCodeModel::writeLocalVariable(Code& code, Cpp::LocalVariable& v)
{
	writeDeclaration(code, v);
	writePunctuation(code, ";");

	if (m_settings.writeFunctionVariableLocations)
	{
		writeSpace(code);
		writeMultilineComment(code, v.location);
	}
}

void CppCodeModel::writeDeclaration(Code& code, Cpp::Declaration& d)
{
	writeTypePrefix(code, d.type);

	if (!d.name.isEmpty())
	{
		writeSpace(code);
		writeIdentifier(code, d.name);
	}

	writeTypePostfix(code, d.type);
}

void CppCodeModel::writeTypePrefix(Code& code, Cpp::Type& t)
{
	if (t.isConst || t.isVolatile)
	{
		writeConstVolatile(code, t.isConst, t.isVolatile);
		writeSpace(code);
	}

	if (t.isFundamental)
	{
		writeFundamentalType(code, t.fundType);
	}
	else
	{
		Q_ASSERT(m_offsetToEntryMap.contains(t.userTypeOffset));

		DwarfEntry* userTypeEntry = m_offsetToEntryMap[t.userTypeOffset];
		QString name = userTypeEntry->getName();

		if (typeCanBeInlined(name))
		{
			switch (userTypeEntry->tag)
			{
			case DW_TAG_class_type:
			case DW_TAG_structure_type:
				writeClassTypePrefix(code, m_offsetToClassTypeMap[userTypeEntry->offset]);
				break;
			case DW_TAG_enumeration_type:
				writeEnumTypePrefix(code, m_offsetToEnumTypeMap[userTypeEntry->offset]);
				break;
			case DW_TAG_array_type:
				writeArrayTypePrefix(code, m_offsetToArrayTypeMap[userTypeEntry->offset]);
				break;
			case DW_TAG_subroutine_type:
				writeFunctionTypePrefix(code, m_offsetToFunctionTypeMap[userTypeEntry->offset]);
				break;
			}
		}
		else
		{
			writeIdentifier(code, name);
		}
	}

	for (int i = 0; i < t.modifiers.size(); i++)
	{
		writeModifier(code, t.modifiers[i]);

		if (i < t.modifiers.size() - 1 && (t.modifiers[i].isConst || t.modifiers[i].isVolatile))
		{
			writeSpace(code);
		}
	}
}

void CppCodeModel::writeTypePostfix(Code& code, Cpp::Type& t)
{
	if (t.isFundamental)
	{
		return;
	}

	Q_ASSERT(m_offsetToEntryMap.contains(t.userTypeOffset));

	DwarfEntry* userTypeEntry = m_offsetToEntryMap[t.userTypeOffset];
	QString name = userTypeEntry->getName();

	if (typeCanBeInlined(name))
	{
		switch (userTypeEntry->tag)
		{
		case DW_TAG_class_type:
		case DW_TAG_structure_type:
			writeClassTypePostfix(code, m_offsetToClassTypeMap[userTypeEntry->offset]);
			break;
		case DW_TAG_enumeration_type:
			writeEnumTypePostfix(code, m_offsetToEnumTypeMap[userTypeEntry->offset]);
			break;
		case DW_TAG_array_type:
			writeArrayTypePostfix(code, m_offsetToArrayTypeMap[userTypeEntry->offset]);
			break;
		case DW_TAG_subroutine_type:
			writeFunctionTypePostfix(code, m_offsetToFunctionTypeMap[userTypeEntry->offset]);
			break;
		};
	}
}

void CppCodeModel::writeClassTypePrefix(Code& code, Cpp::ClassType& c)
{
	writeClassType(code, c, true);
}

void CppCodeModel::writeClassTypePostfix(Code& code, Cpp::ClassType& c)
{
}

void CppCodeModel::writeEnumTypePrefix(Code& code, Cpp::EnumType& e)
{
	writeEnumType(code, e, true);
}

void CppCodeModel::writeEnumTypePostfix(Code& code, Cpp::EnumType& e)
{
}

void CppCodeModel::writeArrayTypePrefix(Code& code, Cpp::ArrayType& a)
{
	writeTypePrefix(code, a.type);
}

void CppCodeModel::writeArrayTypePostfix(Code& code, Cpp::ArrayType& a)
{
	for (int dimension : a.dimensions)
	{
		writePunctuation(code, "[");
		writeLiteral(code, QString("%1").arg(dimension));
		writePunctuation(code, "]");
	}

	writeTypePostfix(code, a.type);
}

void CppCodeModel::writeFunctionTypePrefix(Code& code, Cpp::FunctionType& f)
{
	writeTypePrefix(code, f.type);
	writePunctuation(code, "(");
}

void CppCodeModel::writeFunctionTypePostfix(Code& code, Cpp::FunctionType& f)
{
	writePunctuation(code, ")");
	writeFunctionParameters(code, f);
	writeTypePostfix(code, f.type);
}

void CppCodeModel::writeFunctionParameters(Code& code, Cpp::FunctionType& f)
{
	writePunctuation(code, "(");

	for (int i = 0; i < f.parameters.size(); i++)
	{
		writeFunctionParameter(code, f.parameters[i]);

		if (i < f.parameters.size() - 1)
		{
			writePunctuation(code, ",");
			writeSpace(code);
		}
	}

	writePunctuation(code, ")");
}

void CppCodeModel::writeFunctionParameter(Code& code, Cpp::FunctionParameter& p)
{
	writeDeclaration(code, p);

	if (m_settings.writeFunctionVariableLocations)
	{
		writeSpace(code);
		writeMultilineComment(code, p.location);
	}
}

void CppCodeModel::writeFundamentalType(Code& code, Cpp::FundamentalType t)
{
	QString text;

	if (m_settings.fundamentalTypeNames.contains(t))
	{
		text = m_settings.fundamentalTypeNames[t];
	}
	else
	{
		text = QString("<unknown type %1>").arg(Util::hexToString((quint32)t));
	}

	code.addToken(text, QColorConstants::Blue);
}

void CppCodeModel::writeModifier(Code& code, Cpp::Modifier& m)
{
	if (m.type == Cpp::ModifierType::Pointer)
	{
		writePunctuation(code, "*");
	}
	else if (m.type == Cpp::ModifierType::Reference)
	{
		writePunctuation(code, "&");
	}

	if (m.isConst || m.isVolatile)
	{
		writeSpace(code);
		writeConstVolatile(code, m.isConst, m.isVolatile);
	}
}

void CppCodeModel::writeConstVolatile(Code& code, bool isConst, bool isVolatile)
{
	if (isConst && isVolatile)
	{
		writeKeyword(code, Cpp::Keyword::Const);
		writeSpace(code);
		writeKeyword(code, Cpp::Keyword::Volatile);
	}
	else if (isConst)
	{
		writeKeyword(code, Cpp::Keyword::Const);
	}
	else if (isVolatile)
	{
		writeKeyword(code, Cpp::Keyword::Volatile);
	}
}

void CppCodeModel::writeIdentifier(Code& code, const QString& text)
{
	code.addToken(text, QColorConstants::Black);
}

void CppCodeModel::writeLiteral(Code& code, const QString& text)
{
	code.addToken(text, QColorConstants::Black);
}

void CppCodeModel::writeKeyword(Code& code, Cpp::Keyword keyword)
{
	Q_ASSERT(s_keywordToStringMap.contains(keyword));

	code.addToken(s_keywordToStringMap[keyword], QColorConstants::Blue);
}

void CppCodeModel::writeComment(Code& code, const QString& text)
{
	code.addToken(QString("// %1").arg(text), QColorConstants::Gray);
}

void CppCodeModel::writeMultilineComment(Code& code, const QString& text)
{
	code.addToken(QString("/* %1 */").arg(text), QColorConstants::Gray);
}

void CppCodeModel::writePunctuation(Code& code, const QString& text)
{
	code.addToken(text, QColorConstants::Black);
}

void CppCodeModel::writeSpace(Code& code)
{
	code.addToken(" ");
}

void CppCodeModel::writeNewline(Code& code, bool indent)
{
	code.addToken("\n");

	if (indent)
	{
		writeIndent(code);
	}
}

void CppCodeModel::writeIndent(Code& code)
{
	for (int i = 0; i < m_indentLevel; i++)
	{
		code.addToken("    ");
	}
}

void CppCodeModel::increaseIndent()
{
	m_indentLevel++;
}

void CppCodeModel::decreaseIndent()
{
	m_indentLevel--;
}

void CppCodeModel::resetIndent()
{
	m_indentLevel = 0;
}

bool CppCodeModel::typeCanBeInlined(const QString& name) const
{
	if (name.isEmpty())
	{
		return true;
	}

	// Metrowerks generates names for unnamed things (types, initializers, float constants, etc.).
	// They always start with '@', which is invalid in C/C++.
	// So it should be safe to assume all types starting with '@' were inlined in the original source.
	if (m_settings.inlineMetrowerksUnnamedTypes && name.startsWith("@"))
	{
		return true;
	}

	return false;
}
