#include "CppCodeModel.h"

#include "Output.h"
#include "Util.h"

#include <qdir.h>

//#define MAX_WARNINGS_ACTIVE
#define MAX_WARNINGS 100

#ifdef MAX_WARNINGS_ACTIVE
static int s_warningCount = 0;
#endif

CppCodeModelSettings CppCodeModel::s_defaultSettings
{
    true, // printUnknownEntries
    true, // printUnknownAttributes
    true, // writeClassTypes
    true, // writeEnumTypes
    true, // writeArrayTypes
    true, // writeFunctionTypes
    true, // writePointerToMemberTypes
    true, // writeVariables
    true, // writeFunctionDeclarations
    true, // writeFunctionDefinitions
    true, // writeDwarfEntryOffsets
    true, // writeClassSizes
    true, // writeClassMemberOffsets
    true, // writeClassMemberBitOffsets
    true, // writeClassMemberBitSizes
    true, // writeVariableAddresses
    true, // writeVariableMangledNames
    true, // writeFunctionMangledNames
    true, // writeFunctionAddresses
    true, // writeFunctionSizes
    true, // writeFunctionVariableLocations
    false, // sortTypesAlphabetically
    true, // inlineMetrowerksAnonymousTypes
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
    , m_offsetToPointerToMemberTypeMap()
    , m_offsetToFunctionMap()
    , m_offsetToVariableMap()
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
    m_pathToOffsetMultiMap.clear();
    m_offsetToEntryMap.clear();
    m_offsetToFileMap.clear();
    m_offsetToClassTypeMap.clear();
    m_offsetToEnumTypeMap.clear();
    m_offsetToArrayTypeMap.clear();
    m_offsetToFunctionTypeMap.clear();
    m_offsetToPointerToMemberTypeMap.clear();
    m_offsetToFunctionMap.clear();
    m_offsetToVariableMap.clear();

    requestRewrite();
}

void CppCodeModel::parseDwarf(Dwarf* dwarf)
{
#ifdef MAX_WARNINGS_ACTIVE
    s_warningCount = 0;
#endif

    clear();

    if (!dwarf)
    {
        return;
    }

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

        for (Cpp::Function& f : m_offsetToFunctionMap)
        {
            if (f.isMember)
            {
                Q_ASSERT(m_offsetToClassTypeMap.contains(f.memberTypeOffset));

                Cpp::ClassType& c = m_offsetToClassTypeMap[f.memberTypeOffset];
                c.functionOffsets.append(f.entry->offset);
            }
        }
    }

    requestRewrite();
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
        case DW_TAG_union_type:
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
        case DW_TAG_ptr_to_member_type:
            parsePointerToMemberType(child, file);
            break;
        case DW_TAG_subroutine:
        case DW_TAG_global_subroutine:
        case DW_TAG_inlined_subroutine:
            parseSubroutine(child, file);
            break;
        case DW_TAG_global_variable:
        case DW_TAG_local_variable:
            parseVariable(child, file);
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

    switch (entry->tag)
    {
    case DW_TAG_class_type:
        c.keyword = Cpp::Keyword::Class;
        break;
    case DW_TAG_structure_type:
        c.keyword = Cpp::Keyword::Struct;
        break;
    case DW_TAG_union_type:
        c.keyword = Cpp::Keyword::Union;
        break;
    }

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
        case DW_TAG_typedef:
            parseTypedef(child, c);
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
    m.isBitfield = false;
    m.bitOffset = 0;
    m.bitSize = 0;

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

            // Bitfield attributes
        case DW_AT_bit_offset:
            m.isBitfield = true;
            m.bitOffset = attr->data2;
            break;
        case DW_AT_bit_size:
            m.isBitfield = true;
            m.bitSize = attr->data4;
            break;

            // Ignored attributes
        case DW_AT_byte_size:
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

            // Ignored attributes
        case DW_AT_name:
        case DW_AT_location:
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

void CppCodeModel::parseTypedef(DwarfEntry* entry, Cpp::ClassType& c)
{
    Cpp::Typedef t;
    parseTypedef(entry, t);
    c.typedefs.append(t);
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

            // Ignored attributes
        case DW_AT_byte_size:
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
    f.isMember = false;
    f.memberTypeOffset = 0;
    f.memberAccess = Cpp::Keyword::Public;

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

            // Member attribute
        case DW_AT_member:
            f.isMember = true;
            f.memberTypeOffset = attr->ref;
            break;

            // Access attribute
        case DW_AT_public:
            f.memberAccess = Cpp::Keyword::Public;
            break;
        case DW_AT_private:
            f.memberAccess = Cpp::Keyword::Private;
            break;
        case DW_AT_protected:
            f.memberAccess = Cpp::Keyword::Protected;
            break;

            // Metrowerks mangled name attribute
        case DW_AT_MW_mangled:
            f.mangledName = attr->string;
            break;

            // Ignored attributes
        case DW_AT_MW_global_ref:
        case DW_AT_return_addr:
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

            // Ignored attributes
        case DW_AT_MW_DWARF2_location:
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
    Cpp::FunctionVariable v;

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

            // Ignored attributes
        case DW_AT_MW_DWARF2_location:
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

    f.variables.append(v);
}

void CppCodeModel::parsePointerToMemberType(DwarfEntry* entry, Cpp::File& file)
{
    Cpp::PointerToMemberType& p = m_offsetToPointerToMemberTypeMap[entry->offset];

    p.entry = entry;
    p.type.isFundamental = true;
    p.type.fundType = Cpp::FundamentalType::Int;
    p.type.isConst = false;
    p.type.isVolatile = false;
    p.containingType.isFundamental = false;
    p.containingType.userTypeOffset = 0;
    p.containingType.isConst = false;
    p.containingType.isVolatile = false;

    DwarfAttribute* typeAttribute = nullptr;

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

            // Containing type attribute
        case DW_AT_containing_type:
            p.containingType.userTypeOffset = attr->ref;
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

    file.typeOffsets.append(entry->offset);
}

void CppCodeModel::parseVariable(DwarfEntry* entry, Cpp::File& f)
{
    Cpp::Variable& v = m_offsetToVariableMap[entry->offset];

    v.entry = entry;
    v.type.isFundamental = true;
    v.type.fundType = Cpp::FundamentalType::Int;
    v.type.isConst = false;
    v.type.isVolatile = false;
    v.isGlobal = (entry->tag == DW_TAG_global_variable);
    v.address = 0;

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

            // Metrowerks mangled name attribute
        case DW_AT_MW_mangled:
            v.mangledName = attr->string;
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
            case DW_OP_ADDR:
                v.address = atom.addr;
                break;
            }
        }
    }

    f.variableOffsets.append(entry->offset);
}

void CppCodeModel::parseTypedef(DwarfEntry* entry, Cpp::Typedef& t)
{
    t.entry = entry;
    t.type.isFundamental = true;
    t.type.fundType = Cpp::FundamentalType::Int;
    t.type.isConst = false;
    t.type.isVolatile = false;

    DwarfAttribute* typeAttribute = nullptr;

    for (int i = 0; i < entry->attributeCount; i++)
    {
        DwarfAttribute* attr = &entry->attributes[i];

        switch (attr->name)
        {
            // Name attribute
        case DW_AT_name:
            t.name = attr->string;
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

        parseType(dt, t.type);
    }
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

#ifdef MAX_WARNINGS_ACTIVE
    if (s_warningCount >= MAX_WARNINGS)
    {
        return;
    }

    s_warningCount++;
#endif

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

#ifdef MAX_WARNINGS_ACTIVE
    if (s_warningCount >= MAX_WARNINGS)
    {
        return;
    }

    s_warningCount++;
#endif

    Output::write(tr("Unknown attribute %1 at offset %2 in entry %3 (%4)")
        .arg(Dwarf::attrNameToString(attribute->name))
        .arg(Util::hexToString(attribute->offset))
        .arg(Dwarf::tagToString(entry->tag))
        .arg(entry->getName()));
}

void CppCodeModel::writeDwarfEntry(QString& code, Elf32_Off offset)
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
    case DW_TAG_union_type:
        writeClassType(code, m_offsetToClassTypeMap[offset]);
        break;
    case DW_TAG_enumeration_type:
        writeEnumType(code, m_offsetToEnumTypeMap[offset]);
        break;
    case DW_TAG_array_type:
        writeArrayType(code, m_offsetToArrayTypeMap[offset]);
        break;
    case DW_TAG_subroutine_type:
        writeFunctionType(code, m_offsetToFunctionTypeMap[offset]);
        break;
    case DW_TAG_ptr_to_member_type:
        writePointerToMemberType(code, m_offsetToPointerToMemberTypeMap[offset]);
        break;
    case DW_TAG_global_variable:
        writeVariable(code, m_offsetToVariableMap[offset]);
        break;
    case DW_TAG_local_variable:
        // local variables can be in both compile units and subroutines
        // only write local variables in compile units for now
        if (m_offsetToVariableMap.contains(offset))
        {
            writeVariable(code, m_offsetToVariableMap[offset]);
        }
        break;
    case DW_TAG_subroutine:
    case DW_TAG_global_subroutine:
    case DW_TAG_inlined_subroutine:
        writeFunctionDefinition(code, m_offsetToFunctionMap[offset]);
        break;
    }
}

void CppCodeModel::writeFile(QString& code, const QString& path)
{
    if (!m_pathToOffsetMultiMap.contains(path))
    {
        return;
    }

    writeFiles(code, m_pathToOffsetMultiMap.values(path));
}

void CppCodeModel::writeFiles(QString& code, const QList<Elf32_Off>& fileOffsets)
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
            switch (entry->tag)
            {
            case DW_TAG_class_type:
            case DW_TAG_structure_type:
            case DW_TAG_union_type:
                if (!m_settings.writeClassTypes)
                {
                    continue;
                }
                break;
            case DW_TAG_enumeration_type:
                if (!m_settings.writeEnumTypes)
                {
                    continue;
                }
                break;
            case DW_TAG_array_type:
                if (!m_settings.writeArrayTypes)
                {
                    continue;
                }
            case DW_TAG_subroutine_type:
                if (!m_settings.writeFunctionTypes)
                {
                    continue;
                }
            case DW_TAG_ptr_to_member_type:
                if (!m_settings.writePointerToMemberTypes)
                {
                    continue;
                }
            }

            writeDwarfEntry(code, entry->offset);
            writeNewline(code);
            writeNewline(code);
        }
    }

    if (m_settings.writeVariables)
    {
        for (Elf32_Off fileOffset : fileOffsets)
        {
            Cpp::File& file = m_offsetToFileMap[fileOffset];

            for (Elf32_Off variableOffset : file.variableOffsets)
            {
                Cpp::Variable& v = m_offsetToVariableMap[variableOffset];

                writeVariable(code, v);
                writeNewline(code);
            }
        }

        writeNewline(code);
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

void CppCodeModel::writeClassType(QString& code, Cpp::ClassType& c, bool isInline)
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
        code += QString(" %1").arg(c.name);
    }

    if (!c.inheritances.isEmpty())
    {
        code += " : ";

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
            else if (c.keyword == Cpp::Keyword::Union && in.access != Cpp::Keyword::Public)
            {
                explicitAccess = true;
            }

            if (explicitAccess)
            {
                writeKeyword(code, in.access);
                code += " ";
            }

            writeTypePrefix(code, in.type);
            writeTypePostfix(code, in.type);

            if (i < c.inheritances.size() - 1)
            {
                code += ", ";
            }
        }
    }

    writeNewline(code);
    code += "{";

    bool empty = true;

    if (!c.typedefs.empty())
    {
        increaseIndent();

        for (Cpp::Typedef& t : c.typedefs)
        {
            writeNewline(code);
            writeTypedef(code, t);
        }

        decreaseIndent();

        writeNewline(code);
        empty = false;
    }

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
                code += ":";
            }

            writeNewline(code);
            writeClassMember(code, m);

            prevAccess = m.access;
            first = false;
        }

        decreaseIndent();

        writeNewline(code);
        empty = false;
    }

    if (!c.functionOffsets.empty())
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

            for (int offset : c.functionOffsets)
            {
                if (m_offsetToFunctionMap[offset].memberAccess != Cpp::Keyword::Public)
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

        for (int offset : c.functionOffsets)
        {
            Cpp::Function& f = m_offsetToFunctionMap[offset];

            if (f.memberAccess != prevAccess)
            {
                writeNewline(code, false);

                if (!first)
                {
                    writeNewline(code, false);
                }

                writeKeyword(code, f.memberAccess);
                code += ":";
            }

            writeNewline(code);
            writeFunctionDeclaration(code, f, true);

            prevAccess = f.memberAccess;
            first = false;
        }

        decreaseIndent();

        writeNewline(code);
        empty = false;
    }

    if (empty)
    {
        writeNewline(code);
    }

    code += "}";

    if (!isInline)
    {
        code += ";";
    }
}

void CppCodeModel::writeClassMember(QString& code, Cpp::ClassMember& m)
{
    writeDeclaration(code, m);

    if (m.isBitfield)
    {
        code += QString(" : %1").arg(m.bitSize);
    }

    code += ";";

    QStringList comment;

    if (m_settings.writeClassMemberOffsets)
    {
        comment += QString("Offset: %1").arg(Util::hexToString(m.offset));
    }

    if (m_settings.writeDwarfEntryOffsets)
    {
        comment += QString("DWARF: %1").arg(Util::hexToString(m.entry->offset));
    }

    if (m.isBitfield)
    {
        if (m_settings.writeClassMemberBitOffsets)
        {
            comment += QString("Bit Offset: %1").arg(m.bitOffset);
        }

        if (m_settings.writeClassMemberBitSizes)
        {
            comment += QString("Bit Size: %1").arg(m.bitSize);
        }
    }

    if (!comment.isEmpty())
    {
        code += " ";
        writeComment(code, comment.join(", "));
    }
}

void CppCodeModel::writeEnumType(QString& code, Cpp::EnumType& e, bool isInline)
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
        code += QString(" %1").arg(e.name);
    }

    writeNewline(code);
    code += "{";

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
                code += ",";
            }

            prevValue = el.value;
        }

        decreaseIndent();
    }

    writeNewline(code);
    code += "}";

    if (!isInline)
    {
        code += ";";
    }
}

void CppCodeModel::writeEnumElement(QString& code, Cpp::EnumElement& e, bool explicitValue)
{
    code += e.name;

    if (explicitValue || m_settings.forceExplicitEnumValues)
    {
        code += " = ";

        if (m_settings.hexadecimalEnumValues)
        {
            code += Util::hexToString(e.value);
        }
        else
        {
            code += QString("%1").arg(e.value);
        }
    }
}

void CppCodeModel::writeArrayType(QString& code, Cpp::ArrayType& a, bool isInline)
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
    code += " ";
    writeArrayTypePrefix(code, a);

    if (!isInline && !a.name.isEmpty())
    {
        code += a.name;
    }

    writeArrayTypePostfix(code, a);

    if (!isInline)
    {
        code += ";";
    }
}

void CppCodeModel::writeFunctionType(QString& code, Cpp::FunctionType& f, bool isInline)
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
    code += " ";
    writeFunctionTypePrefix(code, f);

    if (!isInline && !f.name.isEmpty())
    {
        code += f.name;
    }

    writeFunctionTypePostfix(code, f);

    if (!isInline)
    {
        code += ";";
    }
}

void CppCodeModel::writePointerToMemberType(QString& code, Cpp::PointerToMemberType& p, bool isInline)
{
    QStringList comment;

    if (m_settings.writeDwarfEntryOffsets)
    {
        comment += QString("DWARF: %1").arg(Util::hexToString(p.entry->offset));
    }

    if (!comment.isEmpty())
    {
        writeComment(code, comment.join(", "));
        writeNewline(code);
    }

    writeKeyword(code, Cpp::Keyword::Typedef);
    code += " ";
    writePointerToMemberTypePrefix(code, p);

    if (!isInline && !p.name.isEmpty())
    {
        code += p.name;
    }

    writePointerToMemberTypePostfix(code, p);

    if (!isInline)
    {
        code += ";";
    }
}

void CppCodeModel::writeVariable(QString& code, Cpp::Variable& v)
{
    if (!v.isGlobal)
    {
        writeKeyword(code, Cpp::Keyword::Static);
        code += " ";
    }

    writeDeclaration(code, v);
    code += ";";

    QStringList comment;

    if (m_settings.writeVariableMangledNames && !v.mangledName.isEmpty())
    {
        comment += v.mangledName;
    }

    if (m_settings.writeVariableAddresses)
    {
        comment += QString("Address: %1").arg(Util::hexToString(v.address));
    }

    if (!comment.isEmpty())
    {
        code += " ";
        writeComment(code, comment.join(", "));
    }
}

void CppCodeModel::writeFunctionDeclaration(QString& code, Cpp::Function& f, bool isInsideClass)
{
    writeFunctionSignature(code, f, true, isInsideClass);
    code += ";";
}

void CppCodeModel::writeFunctionDefinition(QString& code, Cpp::Function& f)
{
    if (m_settings.writeFunctionMangledNames)
    {
        writeComment(code, f.mangledName);
        writeNewline(code);
    }

    if (m_settings.writeFunctionAddresses)
    {
        writeComment(code, QString("Address: %1").arg(Util::hexToString(f.startAddress)));
        writeNewline(code);
    }

    if (m_settings.writeFunctionSizes)
    {
        writeComment(code, QString("Size: %1").arg(Util::hexToString(f.endAddress - f.startAddress)));
        writeNewline(code);
    }

    writeFunctionSignature(code, f, false, false);
    writeNewline(code);
    code += "{";

    increaseIndent();

    for (Cpp::FunctionVariable& v : f.variables)
    {
        writeNewline(code);
        writeFunctionVariable(code, v);
    }

    decreaseIndent();

    writeNewline(code);
    code += "}";
}

void CppCodeModel::writeFunctionSignature(QString& code, Cpp::Function& f, bool isDeclaration, bool isInsideClass)
{
    bool isNonStaticMemberFunction = false;
    bool isConstMemberFunction = false;

    if (f.isMember
        && !f.parameters.empty()
        && f.parameters[0].name == "this")
    {
        isNonStaticMemberFunction = true;

        if (f.parameters[0].type.isConst)
        {
            isConstMemberFunction = true;
        }
    }

    if ((!f.isMember && !f.isGlobal)
        || (f.isMember && !isNonStaticMemberFunction && isInsideClass))
    {
        writeKeyword(code, Cpp::Keyword::Static);
        code += " ";
    }

    if (f.isInline)
    {
        writeKeyword(code, Cpp::Keyword::Inline);
        code += " ";
    }

    writeTypePrefix(code, f.type);
    writeTypePostfix(code, f.type);
    code += " ";

    if (f.isMember && !isInsideClass)
    {
        Q_ASSERT(m_offsetToClassTypeMap.contains(f.memberTypeOffset));

        Cpp::ClassType& c = m_offsetToClassTypeMap[f.memberTypeOffset];

        code += QString("%1::").arg(c.name);
    }

    code += f.name;
    writeFunctionParameters(code, f, isDeclaration);

    if (isConstMemberFunction)
    {
        code += " ";
        writeKeyword(code, Cpp::Keyword::Const);
    }
}

void CppCodeModel::writeFunctionVariable(QString& code, Cpp::FunctionVariable& v)
{
    writeDeclaration(code, v);
    code += ";";

    if (m_settings.writeFunctionVariableLocations)
    {
        code += " ";
        writeComment(code, v.location);
    }
}

void CppCodeModel::writeDeclaration(QString& code, Cpp::Declaration& d)
{
    bool isFunctionType = false;

    writeTypePrefix(code, d.type, &isFunctionType);

    if (!d.name.isEmpty())
    {
        if (!isFunctionType)
        {
            code += " ";
        }

        code += d.name;
    }

    writeTypePostfix(code, d.type);
}

void CppCodeModel::writeTypedef(QString& code, Cpp::Typedef& t)
{
    bool isFunctionType = false;

    writeKeyword(code, Cpp::Keyword::Typedef);
    code += " ";
    writeTypePrefix(code, t.type, &isFunctionType);

    if (!t.name.isEmpty())
    {
        if (!isFunctionType)
        {
            code += " ";
        }

        code += t.name;
    }

    writeTypePostfix(code, t.type);
    code += ";";
}

void CppCodeModel::writeTypePrefix(QString& code, Cpp::Type& t, bool* outIsFunctionType)
{
    if (outIsFunctionType)
    {
        *outIsFunctionType = false;
    }

    if (t.isConst || t.isVolatile)
    {
        writeConstVolatile(code, t.isConst, t.isVolatile);
        code += " ";
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
            case DW_TAG_union_type:
                writeClassTypePrefix(code, m_offsetToClassTypeMap[userTypeEntry->offset]);
                break;
            case DW_TAG_enumeration_type:
                writeEnumTypePrefix(code, m_offsetToEnumTypeMap[userTypeEntry->offset]);
                break;
            case DW_TAG_array_type:
                writeArrayTypePrefix(code, m_offsetToArrayTypeMap[userTypeEntry->offset]);
                break;
            case DW_TAG_subroutine_type:
            {
                writeFunctionTypePrefix(code, m_offsetToFunctionTypeMap[userTypeEntry->offset]);

                if (outIsFunctionType)
                {
                    *outIsFunctionType = true;
                }

                break;
            }
            case DW_TAG_ptr_to_member_type:
            {
                Cpp::PointerToMemberType& p = m_offsetToPointerToMemberTypeMap[userTypeEntry->offset];
                writePointerToMemberTypePrefix(code, p);

                if (outIsFunctionType
                    && !p.type.isFundamental
                    && m_offsetToEntryMap[p.type.userTypeOffset]->tag == DW_TAG_subroutine_type)
                {
                    *outIsFunctionType = true;
                }

                break;
            }
            }
        }
        else
        {
            code += name;
        }
    }

    for (int i = 0; i < t.modifiers.size(); i++)
    {
        writeModifier(code, t.modifiers[i]);

        if (i < t.modifiers.size() - 1 && (t.modifiers[i].isConst || t.modifiers[i].isVolatile))
        {
            code += " ";
        }
    }
}

void CppCodeModel::writeTypePostfix(QString& code, Cpp::Type& t)
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
        case DW_TAG_union_type:
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
        case DW_TAG_ptr_to_member_type:
            writePointerToMemberTypePostfix(code, m_offsetToPointerToMemberTypeMap[userTypeEntry->offset]);
            break;
        };
    }
}

void CppCodeModel::writeClassTypePrefix(QString& code, Cpp::ClassType& c)
{
    writeClassType(code, c, true);
}

void CppCodeModel::writeClassTypePostfix(QString& code, Cpp::ClassType& c)
{
}

void CppCodeModel::writeEnumTypePrefix(QString& code, Cpp::EnumType& e)
{
    writeEnumType(code, e, true);
}

void CppCodeModel::writeEnumTypePostfix(QString& code, Cpp::EnumType& e)
{
}

void CppCodeModel::writeArrayTypePrefix(QString& code, Cpp::ArrayType& a)
{
    writeTypePrefix(code, a.type);
}

void CppCodeModel::writeArrayTypePostfix(QString& code, Cpp::ArrayType& a)
{
    for (int dimension : a.dimensions)
    {
        code += QString("[%1]").arg(dimension);
    }

    writeTypePostfix(code, a.type);
}

void CppCodeModel::writeFunctionTypePrefix(QString& code, Cpp::FunctionType& f)
{
    writeTypePrefix(code, f.type);
    code += "(";
}

void CppCodeModel::writeFunctionTypePostfix(QString& code, Cpp::FunctionType& f)
{
    code += ")";
    writeFunctionParameters(code, f, true);
    writeTypePostfix(code, f.type);
}

void CppCodeModel::writeFunctionParameters(QString& code, Cpp::FunctionType& f, bool isDeclaration)
{
    code += "(";

    for (int i = 0; i < f.parameters.size(); i++)
    {
        if (f.parameters[i].name == "this")
        {
            continue;
        }

        writeFunctionParameter(code, f.parameters[i], isDeclaration);

        if (i < f.parameters.size() - 1)
        {
            code += ", ";
        }
    }

    code += ")";
}

void CppCodeModel::writeFunctionParameter(QString& code, Cpp::FunctionParameter& p, bool isDeclaration)
{
    writeDeclaration(code, p);

    if (!isDeclaration
        && m_settings.writeFunctionVariableLocations
        && !p.location.isEmpty())
    {
        code += " ";
        writeMultilineComment(code, p.location);
    }
}

void CppCodeModel::writePointerToMemberTypePrefix(QString& code, Cpp::PointerToMemberType& p)
{
    bool isFunctionType = false;

    writeTypePrefix(code, p.type, &isFunctionType);

    if (!isFunctionType)
    {
        code += " ";
    }

    writeTypePrefix(code, p.containingType);
    code += "::*";
}

void CppCodeModel::writePointerToMemberTypePostfix(QString& code, Cpp::PointerToMemberType& p)
{
    writeTypePostfix(code, p.containingType);
    writeTypePostfix(code, p.type);
}

void CppCodeModel::writeFundamentalType(QString& code, Cpp::FundamentalType t)
{
    if (m_settings.fundamentalTypeNames.contains(t))
    {
        code += m_settings.fundamentalTypeNames[t];
    }
    else
    {
        code += QString("<unknown type %1>").arg(Util::hexToString((quint32)t));
    }
}

void CppCodeModel::writeModifier(QString& code, Cpp::Modifier& m)
{
    if (m.type == Cpp::ModifierType::Pointer)
    {
        code += "*";
    }
    else if (m.type == Cpp::ModifierType::Reference)
    {
        code += "&";
    }

    if (m.isConst || m.isVolatile)
    {
        code += " ";
        writeConstVolatile(code, m.isConst, m.isVolatile);
    }
}

void CppCodeModel::writeConstVolatile(QString& code, bool isConst, bool isVolatile)
{
    if (isConst && isVolatile)
    {
        writeKeyword(code, Cpp::Keyword::Const);
        code += " ";
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

void CppCodeModel::writeKeyword(QString& code, Cpp::Keyword keyword)
{
    Q_ASSERT(s_keywordToStringMap.contains(keyword));

    code += s_keywordToStringMap[keyword];
}

void CppCodeModel::writeComment(QString& code, const QString& text)
{
    code += QString("// %1").arg(text);
}

void CppCodeModel::writeMultilineComment(QString& code, const QString& text)
{
    code += QString("/* %1 */").arg(text);
}

void CppCodeModel::writeNewline(QString& code, bool indent)
{
    code += "\n";

    if (indent)
    {
        for (int i = 0; i < m_indentLevel; i++)
        {
            code += "    ";
        }
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
    if (m_settings.inlineMetrowerksAnonymousTypes && name.startsWith("@"))
    {
        return true;
    }

    return false;
}

QString CppCodeModel::dwarfEntryName(Elf32_Off offset) const
{
    Q_ASSERT(m_offsetToEntryMap.contains(offset));

    return m_offsetToEntryMap[offset]->getName();
}

void CppCodeModel::setupSettingsMenu(QMenu* menu)
{
    QAction* action;

    action = menu->addAction(tr("Write classes/structs/unions"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeClassTypes);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeClassTypes = action->isChecked();
            requestRewrite();
        });

    action = menu->addAction(tr("Write enums"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeEnumTypes);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeEnumTypes = action->isChecked();
            requestRewrite();
        });

    action = menu->addAction(tr("Write array typedefs"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeArrayTypes);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeArrayTypes = action->isChecked();
            requestRewrite();
        });

    action = menu->addAction(tr("Write function typedefs"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeFunctionTypes);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeFunctionTypes = action->isChecked();
            requestRewrite();
        });

    action = menu->addAction(tr("Write pointer-to-member typedefs"));
    action->setCheckable(true);
    action->setChecked(m_settings.writePointerToMemberTypes);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writePointerToMemberTypes = action->isChecked();
            requestRewrite();
        });

    action = menu->addAction(tr("Write variables"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeVariables);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeVariables = action->isChecked();
            requestRewrite();
        });

    action = menu->addAction(tr("Write function declarations"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeFunctionDeclarations);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeFunctionDeclarations = action->isChecked();
            requestRewrite();
        });

    action = menu->addAction(tr("Write function definitions"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeFunctionDefinitions);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeFunctionDefinitions = action->isChecked();
            requestRewrite();
        });

    action = menu->addAction(tr("Sort types alphabetically"));
    action->setCheckable(true);
    action->setChecked(m_settings.sortTypesAlphabetically);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.sortTypesAlphabetically = action->isChecked();
            requestRewrite();
        });

    action = menu->addAction(tr("Inline Metrowerks anonymous types (@class, @enum, etc.)"));
    action->setCheckable(true);
    action->setChecked(m_settings.inlineMetrowerksAnonymousTypes);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.inlineMetrowerksAnonymousTypes = action->isChecked();
            requestRewrite();
        });

    action = menu->addAction(tr("Hexadecimal enum values"));
    action->setCheckable(true);
    action->setChecked(m_settings.hexadecimalEnumValues);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.hexadecimalEnumValues = action->isChecked();
            requestRewrite();
        });

    action = menu->addAction(tr("Explicit enum values"));
    action->setCheckable(true);
    action->setChecked(m_settings.forceExplicitEnumValues);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.forceExplicitEnumValues = action->isChecked();
            requestRewrite();
        });

    /* Comments */
    QMenu* commentsMenu = menu->addMenu(tr("Comments"));
    action = commentsMenu->addAction(tr("DWARF entry offsets"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeDwarfEntryOffsets);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeDwarfEntryOffsets = action->isChecked();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Class sizes"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeClassSizes);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeClassSizes = action->isChecked();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Class member offsets"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeClassMemberOffsets);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeClassMemberOffsets = action->isChecked();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Class member bitfield offsets"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeClassMemberBitOffsets);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeClassMemberBitOffsets = action->isChecked();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Class member bitfield sizes"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeClassMemberBitSizes);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeClassMemberBitSizes = action->isChecked();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Variable addresses"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeVariableAddresses);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeVariableAddresses = action->isChecked();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Variable mangled names"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeVariableMangledNames);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeVariableMangledNames = action->isChecked();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Function mangled names"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeFunctionMangledNames);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeFunctionMangledNames = action->isChecked();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Function addresses"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeFunctionAddresses);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeFunctionAddresses = action->isChecked();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Function sizes"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeFunctionSizes);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeFunctionSizes = action->isChecked();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Function registers"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeFunctionVariableLocations);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeFunctionVariableLocations = action->isChecked();
            requestRewrite();
        });

    /* Output */
    QMenu* outputMenu = menu->addMenu(tr("Output"));
    action = outputMenu->addAction(tr("Unknown DWARF entries"));
    action->setCheckable(true);
    action->setChecked(m_settings.printUnknownEntries);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.printUnknownEntries = action->isChecked();
        });

    action = outputMenu->addAction(tr("Unknown DWARF attributes"));
    action->setCheckable(true);
    action->setChecked(m_settings.printUnknownAttributes);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.printUnknownAttributes = action->isChecked();
        });
}
