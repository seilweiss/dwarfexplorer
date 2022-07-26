#include "CppCodeModel.h"

#include "Output.h"
#include "Util.h"
#include "CppFundamentalTypeNamesDialog.h"
#include "Disassemblers.h"

#include <qdir.h>
#include <qsettings.h>

//#define MAX_WARNINGS_ACTIVE
#define MAX_WARNINGS 100

#ifdef MAX_WARNINGS_ACTIVE
static int s_warningCount = 0;
#endif

static const QMap<QString, QString> operatorDemangleMap =
{
    { "__as", "operator=" },
    { "__nw", "operator new" },
    { "__dl", "operator delete" },
    { "__nwa", "operator new[]" },
    { "__dla", "operator delete[]" },
    { "__pl", "operator+" },
    { "__mi", "operator-" },
    { "__ml", "operator*" },
    { "__dv", "operator/" },
    { "__md", "operator%" },
    { "__er", "operator^" },
    { "__ad", "operator&" },
    { "__or", "operator|" },
    { "__co", "operator~" },
    { "__nt", "operator!" },
    { "__lt", "operator<" },
    { "__gt", "operator>" },
    { "__apl", "operator+=" },
    { "__ami", "operator-=" },
    { "__amu", "operator*=" },
    { "__adv", "operator/=" },
    { "__amd", "operator%=" },
    { "__aer", "operator^=" },
    { "__aad", "operator&=" },
    { "__aor", "operator|=" },
    { "__ls", "operator<<" },
    { "__rs", "operator>>" },
    { "__als", "operator<<=" },
    { "__ars", "operator>>=" },
    { "__eq", "operator==" },
    { "__ne", "operator!=" },
    { "__le", "operator<=" },
    { "__ge", "operator>=" },
    { "__aa", "operator&&" },
    { "__oo", "operator||" },
    { "__pp", "operator++" },
    { "__mm", "operator--" },
    { "__cm", "operator," },
    { "__rm", "operator->*" },
    { "__rf", "operator*" },
    { "__cl", "operator()" },
    { "__vc", "operator[]" },
};

CppCodeModelSettings CppCodeModel::s_defaultSettings
{
    true, // warnUnknownEntries
    true, // warnUnknownAttributes
    true, // warnUnknownLineNumberFunctions
    true, // warnUnknownLocationConfigurations
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
    true, // writeFunctionDisassembly
    true, // writeFunctionLineNumbers
    false, // sortTypesAlphabetically
    true, // sortFunctionsByLineNumber
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
    , m_settings()
    , m_pathToOffsetMultiMap()
    , m_offsetToEntryMap()
    , m_offsetToSourceStatementTableMap()
    , m_offsetToFileMap()
    , m_offsetToClassTypeMap()
    , m_offsetToEnumTypeMap()
    , m_offsetToArrayTypeMap()
    , m_offsetToFunctionTypeMap()
    , m_offsetToPointerToMemberTypeMap()
    , m_offsetToFunctionMap()
    , m_offsetToVariableMap()
    , m_indentLevel(0)
    , m_firstSourceStatementTableParsed(false)
{
    loadSettings();
    saveSettings();
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

void CppCodeModel::loadSettings()
{
    QSettings settings;
    m_settings.warnUnknownEntries = settings.value("cppcodemodel/warnUnknownEntries", s_defaultSettings.warnUnknownEntries).toBool();
    m_settings.warnUnknownAttributes = settings.value("cppcodemodel/warnUnknownAttributes", s_defaultSettings.warnUnknownAttributes).toBool();
    m_settings.warnUnknownLineNumberFunctions = settings.value("cppcodemodel/warnUnknownLineNumberFunctions", s_defaultSettings.warnUnknownLineNumberFunctions).toBool();
    m_settings.warnUnknownLocationConfigurations = settings.value("cppcodemodel/warnUnknownLocationConfigurations", s_defaultSettings.warnUnknownLocationConfigurations).toBool();
    m_settings.writeClassTypes = settings.value("cppcodemodel/writeClassTypes", s_defaultSettings.writeClassTypes).toBool();
    m_settings.writeEnumTypes = settings.value("cppcodemodel/writeEnumTypes", s_defaultSettings.writeEnumTypes).toBool();
    m_settings.writeArrayTypes = settings.value("cppcodemodel/writeArrayTypes", s_defaultSettings.writeArrayTypes).toBool();
    m_settings.writeFunctionTypes = settings.value("cppcodemodel/writeFunctionTypes", s_defaultSettings.writeFunctionTypes).toBool();
    m_settings.writePointerToMemberTypes = settings.value("cppcodemodel/writePointerToMemberTypes", s_defaultSettings.writePointerToMemberTypes).toBool();
    m_settings.writeVariables = settings.value("cppcodemodel/writeVariables", s_defaultSettings.writeVariables).toBool();
    m_settings.writeFunctionDeclarations = settings.value("cppcodemodel/writeFunctionDeclarations", s_defaultSettings.writeFunctionDeclarations).toBool();
    m_settings.writeFunctionDefinitions = settings.value("cppcodemodel/writeFunctionDefinitions", s_defaultSettings.writeFunctionDefinitions).toBool();
    m_settings.writeDwarfEntryOffsets = settings.value("cppcodemodel/writeDwarfEntryOffsets", s_defaultSettings.writeDwarfEntryOffsets).toBool();
    m_settings.writeClassSizes = settings.value("cppcodemodel/writeClassSizes", s_defaultSettings.writeClassSizes).toBool();
    m_settings.writeClassMemberOffsets = settings.value("cppcodemodel/writeClassMemberOffsets", s_defaultSettings.writeClassMemberOffsets).toBool();
    m_settings.writeClassMemberBitOffsets = settings.value("cppcodemodel/writeClassMemberBitOffsets", s_defaultSettings.writeClassMemberBitOffsets).toBool();
    m_settings.writeClassMemberBitSizes = settings.value("cppcodemodel/writeClassMemberBitSizes", s_defaultSettings.writeClassMemberBitSizes).toBool();
    m_settings.writeVariableAddresses = settings.value("cppcodemodel/writeVariableAddresses", s_defaultSettings.writeVariableAddresses).toBool();
    m_settings.writeVariableMangledNames = settings.value("cppcodemodel/writeVariableMangledNames", s_defaultSettings.writeVariableMangledNames).toBool();
    m_settings.writeFunctionMangledNames = settings.value("cppcodemodel/writeFunctionMangledNames", s_defaultSettings.writeFunctionMangledNames).toBool();
    m_settings.writeFunctionAddresses = settings.value("cppcodemodel/writeFunctionAddresses", s_defaultSettings.writeFunctionAddresses).toBool();
    m_settings.writeFunctionSizes = settings.value("cppcodemodel/writeFunctionSizes", s_defaultSettings.writeFunctionSizes).toBool();
    m_settings.writeFunctionVariableLocations = settings.value("cppcodemodel/writeFunctionVariableLocations", s_defaultSettings.writeFunctionVariableLocations).toBool();
    m_settings.writeFunctionDisassembly = settings.value("cppcodemodel/writeFunctionDisassembly", s_defaultSettings.writeFunctionDisassembly).toBool();
    m_settings.writeFunctionLineNumbers = settings.value("cppcodemodel/writeFunctionLineNumbers", s_defaultSettings.writeFunctionLineNumbers).toBool();
    m_settings.sortTypesAlphabetically = settings.value("cppcodemodel/sortTypesAlphabetically", s_defaultSettings.sortTypesAlphabetically).toBool();
    m_settings.sortFunctionsByLineNumber = settings.value("cppcodemodel/sortFunctionsByLineNumber", s_defaultSettings.sortFunctionsByLineNumber).toBool();
    m_settings.inlineMetrowerksAnonymousTypes = settings.value("cppcodemodel/inlineMetrowerksAnonymousTypes", s_defaultSettings.inlineMetrowerksAnonymousTypes).toBool();
    m_settings.hexadecimalEnumValues = settings.value("cppcodemodel/hexadecimalEnumValues", s_defaultSettings.hexadecimalEnumValues).toBool();
    m_settings.forceExplicitEnumValues = settings.value("cppcodemodel/forceExplicitEnumValues", s_defaultSettings.forceExplicitEnumValues).toBool();
    m_settings.fundamentalTypeNames[Cpp::FundamentalType::Char] = settings.value("cppcodemodel/fundamentalTypeNames/Char", s_defaultSettings.fundamentalTypeNames[Cpp::FundamentalType::Char]).toString();
    m_settings.fundamentalTypeNames[Cpp::FundamentalType::SignedChar] = settings.value("cppcodemodel/fundamentalTypeNames/SignedChar", s_defaultSettings.fundamentalTypeNames[Cpp::FundamentalType::SignedChar]).toString();
    m_settings.fundamentalTypeNames[Cpp::FundamentalType::UnsignedChar] = settings.value("cppcodemodel/fundamentalTypeNames/UnsignedChar", s_defaultSettings.fundamentalTypeNames[Cpp::FundamentalType::UnsignedChar]).toString();
    m_settings.fundamentalTypeNames[Cpp::FundamentalType::Short] = settings.value("cppcodemodel/fundamentalTypeNames/Short", s_defaultSettings.fundamentalTypeNames[Cpp::FundamentalType::Short]).toString();
    m_settings.fundamentalTypeNames[Cpp::FundamentalType::SignedShort] = settings.value("cppcodemodel/fundamentalTypeNames/SignedShort", s_defaultSettings.fundamentalTypeNames[Cpp::FundamentalType::SignedShort]).toString();
    m_settings.fundamentalTypeNames[Cpp::FundamentalType::UnsignedShort] = settings.value("cppcodemodel/fundamentalTypeNames/UnsignedShort", s_defaultSettings.fundamentalTypeNames[Cpp::FundamentalType::UnsignedShort]).toString();
    m_settings.fundamentalTypeNames[Cpp::FundamentalType::Int] = settings.value("cppcodemodel/fundamentalTypeNames/Int", s_defaultSettings.fundamentalTypeNames[Cpp::FundamentalType::Int]).toString();
    m_settings.fundamentalTypeNames[Cpp::FundamentalType::SignedInt] = settings.value("cppcodemodel/fundamentalTypeNames/SignedInt", s_defaultSettings.fundamentalTypeNames[Cpp::FundamentalType::SignedInt]).toString();
    m_settings.fundamentalTypeNames[Cpp::FundamentalType::UnsignedInt] = settings.value("cppcodemodel/fundamentalTypeNames/UnsignedInt", s_defaultSettings.fundamentalTypeNames[Cpp::FundamentalType::UnsignedInt]).toString();
    m_settings.fundamentalTypeNames[Cpp::FundamentalType::Long] = settings.value("cppcodemodel/fundamentalTypeNames/Long", s_defaultSettings.fundamentalTypeNames[Cpp::FundamentalType::Long]).toString();
    m_settings.fundamentalTypeNames[Cpp::FundamentalType::SignedLong] = settings.value("cppcodemodel/fundamentalTypeNames/SignedLong", s_defaultSettings.fundamentalTypeNames[Cpp::FundamentalType::SignedLong]).toString();
    m_settings.fundamentalTypeNames[Cpp::FundamentalType::UnsignedLong] = settings.value("cppcodemodel/fundamentalTypeNames/UnsignedLong", s_defaultSettings.fundamentalTypeNames[Cpp::FundamentalType::UnsignedLong]).toString();
    m_settings.fundamentalTypeNames[Cpp::FundamentalType::VoidPointer] = settings.value("cppcodemodel/fundamentalTypeNames/VoidPointer", s_defaultSettings.fundamentalTypeNames[Cpp::FundamentalType::VoidPointer]).toString();
    m_settings.fundamentalTypeNames[Cpp::FundamentalType::Float] = settings.value("cppcodemodel/fundamentalTypeNames/Float", s_defaultSettings.fundamentalTypeNames[Cpp::FundamentalType::Float]).toString();
    m_settings.fundamentalTypeNames[Cpp::FundamentalType::Double] = settings.value("cppcodemodel/fundamentalTypeNames/Double", s_defaultSettings.fundamentalTypeNames[Cpp::FundamentalType::Double]).toString();
    m_settings.fundamentalTypeNames[Cpp::FundamentalType::Void] = settings.value("cppcodemodel/fundamentalTypeNames/Void", s_defaultSettings.fundamentalTypeNames[Cpp::FundamentalType::Void]).toString();
    m_settings.fundamentalTypeNames[Cpp::FundamentalType::Bool] = settings.value("cppcodemodel/fundamentalTypeNames/Bool", s_defaultSettings.fundamentalTypeNames[Cpp::FundamentalType::Bool]).toString();
    m_settings.fundamentalTypeNames[Cpp::FundamentalType::LongLong] = settings.value("cppcodemodel/fundamentalTypeNames/LongLong", s_defaultSettings.fundamentalTypeNames[Cpp::FundamentalType::LongLong]).toString();
}

void CppCodeModel::saveSettings()
{
    QSettings settings;
    settings.setValue("cppcodemodel/warnUnknownEntries", m_settings.warnUnknownEntries);
    settings.setValue("cppcodemodel/warnUnknownAttributes", m_settings.warnUnknownAttributes);
    settings.setValue("cppcodemodel/warnUnknownLineNumberFunctions", m_settings.warnUnknownLineNumberFunctions);
    settings.setValue("cppcodemodel/warnUnknownLocationConfigurations", m_settings.warnUnknownLocationConfigurations);
    settings.setValue("cppcodemodel/writeClassTypes", m_settings.writeClassTypes);
    settings.setValue("cppcodemodel/writeEnumTypes", m_settings.writeEnumTypes);
    settings.setValue("cppcodemodel/writeArrayTypes", m_settings.writeArrayTypes);
    settings.setValue("cppcodemodel/writeFunctionTypes", m_settings.writeFunctionTypes);
    settings.setValue("cppcodemodel/writePointerToMemberTypes", m_settings.writePointerToMemberTypes);
    settings.setValue("cppcodemodel/writeVariables", m_settings.writeVariables);
    settings.setValue("cppcodemodel/writeFunctionDeclarations", m_settings.writeFunctionDeclarations);
    settings.setValue("cppcodemodel/writeFunctionDefinitions", m_settings.writeFunctionDefinitions);
    settings.setValue("cppcodemodel/writeDwarfEntryOffsets", m_settings.writeDwarfEntryOffsets);
    settings.setValue("cppcodemodel/writeClassSizes", m_settings.writeClassSizes);
    settings.setValue("cppcodemodel/writeClassMemberOffsets", m_settings.writeClassMemberOffsets);
    settings.setValue("cppcodemodel/writeClassMemberBitOffsets", m_settings.writeClassMemberBitOffsets);
    settings.setValue("cppcodemodel/writeClassMemberBitSizes", m_settings.writeClassMemberBitSizes);
    settings.setValue("cppcodemodel/writeVariableAddresses", m_settings.writeVariableAddresses);
    settings.setValue("cppcodemodel/writeVariableMangledNames", m_settings.writeVariableMangledNames);
    settings.setValue("cppcodemodel/writeFunctionMangledNames", m_settings.writeFunctionMangledNames);
    settings.setValue("cppcodemodel/writeFunctionAddresses", m_settings.writeFunctionAddresses);
    settings.setValue("cppcodemodel/writeFunctionSizes", m_settings.writeFunctionSizes);
    settings.setValue("cppcodemodel/writeFunctionVariableLocations", m_settings.writeFunctionVariableLocations);
    settings.setValue("cppcodemodel/writeFunctionDisassembly", m_settings.writeFunctionDisassembly);
    settings.setValue("cppcodemodel/writeFunctionLineNumbers", m_settings.writeFunctionLineNumbers);
    settings.setValue("cppcodemodel/sortTypesAlphabetically", m_settings.sortTypesAlphabetically);
    settings.setValue("cppcodemodel/sortFunctionsByLineNumber", m_settings.sortFunctionsByLineNumber);
    settings.setValue("cppcodemodel/inlineMetrowerksAnonymousTypes", m_settings.inlineMetrowerksAnonymousTypes);
    settings.setValue("cppcodemodel/hexadecimalEnumValues", m_settings.hexadecimalEnumValues);
    settings.setValue("cppcodemodel/forceExplicitEnumValues", m_settings.forceExplicitEnumValues);
    settings.setValue("cppcodemodel/fundamentalTypeNames/Char", m_settings.fundamentalTypeNames[Cpp::FundamentalType::Char]);
    settings.setValue("cppcodemodel/fundamentalTypeNames/SignedChar", m_settings.fundamentalTypeNames[Cpp::FundamentalType::SignedChar]);
    settings.setValue("cppcodemodel/fundamentalTypeNames/UnsignedChar", m_settings.fundamentalTypeNames[Cpp::FundamentalType::UnsignedChar]);
    settings.setValue("cppcodemodel/fundamentalTypeNames/Short", m_settings.fundamentalTypeNames[Cpp::FundamentalType::Short]);
    settings.setValue("cppcodemodel/fundamentalTypeNames/SignedShort", m_settings.fundamentalTypeNames[Cpp::FundamentalType::SignedShort]);
    settings.setValue("cppcodemodel/fundamentalTypeNames/UnsignedShort", m_settings.fundamentalTypeNames[Cpp::FundamentalType::UnsignedShort]);
    settings.setValue("cppcodemodel/fundamentalTypeNames/Int", m_settings.fundamentalTypeNames[Cpp::FundamentalType::Int]);
    settings.setValue("cppcodemodel/fundamentalTypeNames/SignedInt", m_settings.fundamentalTypeNames[Cpp::FundamentalType::SignedInt]);
    settings.setValue("cppcodemodel/fundamentalTypeNames/UnsignedInt", m_settings.fundamentalTypeNames[Cpp::FundamentalType::UnsignedInt]);
    settings.setValue("cppcodemodel/fundamentalTypeNames/Long", m_settings.fundamentalTypeNames[Cpp::FundamentalType::Long]);
    settings.setValue("cppcodemodel/fundamentalTypeNames/SignedLong", m_settings.fundamentalTypeNames[Cpp::FundamentalType::SignedLong]);
    settings.setValue("cppcodemodel/fundamentalTypeNames/UnsignedLong", m_settings.fundamentalTypeNames[Cpp::FundamentalType::UnsignedLong]);
    settings.setValue("cppcodemodel/fundamentalTypeNames/VoidPointer", m_settings.fundamentalTypeNames[Cpp::FundamentalType::VoidPointer]);
    settings.setValue("cppcodemodel/fundamentalTypeNames/Float", m_settings.fundamentalTypeNames[Cpp::FundamentalType::Float]);
    settings.setValue("cppcodemodel/fundamentalTypeNames/Double", m_settings.fundamentalTypeNames[Cpp::FundamentalType::Double]);
    settings.setValue("cppcodemodel/fundamentalTypeNames/Void", m_settings.fundamentalTypeNames[Cpp::FundamentalType::Void]);
    settings.setValue("cppcodemodel/fundamentalTypeNames/Bool", m_settings.fundamentalTypeNames[Cpp::FundamentalType::Bool]);
    settings.setValue("cppcodemodel/fundamentalTypeNames/LongLong", m_settings.fundamentalTypeNames[Cpp::FundamentalType::LongLong]);
}

void CppCodeModel::clear()
{
    m_pathToOffsetMultiMap.clear();
    m_offsetToEntryMap.clear();
    m_offsetToSourceStatementTableMap.clear();
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

    if (dwarf->sourceStatementTableCount > 0)
    {
        for (int i = 0; i < dwarf->sourceStatementTableCount; i++)
        {
            DwarfSourceStatementTable* table = &dwarf->sourceStatementTables[i];

            m_offsetToSourceStatementTableMap[table->offset] = table;
        }
    }

    m_firstSourceStatementTableParsed = false;

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

    DwarfAttribute* statementListAttribute = nullptr;

    for (int i = 0; i < entry->attributeCount; i++)
    {
        DwarfAttribute* attr = &entry->attributes[i];

        switch (attr->name)
        {
            // Name attribute
        case DW_AT_name:
            file.path = attr->string;
            break;

            // Statement list attribute
        case DW_AT_stmt_list:
            statementListAttribute = attr;
            break;

            // Ignored attributes
        case DW_AT_producer:
        case DW_AT_language: // Can't ignore this in the future if we want to support more languages.
        case DW_AT_low_pc:
        case DW_AT_high_pc:
            break;

            // Unknown attribute
        default:
            warnUnknownAttribute(attr, entry);
            break;
        }
    }

    if (statementListAttribute)
    {
        // Hack to prevent unknown line number function warnings.
        // Some compile units have an AT_stmt_list offset of 0 if they don't have any line numbers,
        // despite 0 being a valid offset (it's the offset of the first table in the .line section).
        // This causes a bunch of warnings because the Cpp::File will contain all of the line numbers
        // from the first table when it really shouldn't contain any.
        // To get around this, we only parse the table at offset 0 once, under the assumption
        // that it corresponds to the correct compile unit (though this might not always be true...)
        bool canParse = true;
        
        if (statementListAttribute->data4 == 0)
        {
            if (m_firstSourceStatementTableParsed)
            {
                canParse = false;
            }
            else
            {
                m_firstSourceStatementTableParsed = true;
            }
        }

        if (canParse)
        {
            Q_ASSERT(m_offsetToSourceStatementTableMap.contains(statementListAttribute->data4));

            parseSourceStatementTable(m_offsetToSourceStatementTableMap[statementListAttribute->data4], file);
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

            // Ignored attributes
        case DW_AT_ordering:
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
        case DW_AT_MW_restore_SP:
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
        case DW_TAG_lexical_block:
            parseLexicalBlock(child, f);
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

        p.location = locationToString(location);

        if (p.location.isEmpty())
        {
            warnUnknownLocationConfiguration(locationAttribute, entry);
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

            // Metrowerks mangled name attribute
        case DW_AT_MW_mangled:
            v.mangledName = attr->string;
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

        v.location = locationToString(location);

        if (v.location.isEmpty())
        {
            warnUnknownLocationConfiguration(locationAttribute, entry);
        }
    }

    f.variables.append(v);
}

void CppCodeModel::parseLexicalBlock(DwarfEntry* entry, Cpp::Function& f)
{
    for (DwarfEntry* child = entry->firstChild; child != nullptr; child = child->sibling)
    {
        switch (child->tag)
        {
        case DW_TAG_local_variable:
            parseLocalVariable(child, f);
            break;
        default:
            warnUnknownEntry(child, entry);
            break;
        }
    }
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

void CppCodeModel::parseSourceStatementTable(DwarfSourceStatementTable* table, Cpp::File& file)
{
    for (int i = 0; i < table->entryCount; i++)
    {
        DwarfSourceStatementEntry* entry = &table->entries[i];

        if (i == table->entryCount - 1 && entry->lineNumber == 0)
        {
            continue;
        }

        Cpp::Function* f = nullptr;

        for (Elf32_Off offset : file.functionOffsets)
        {
            Cpp::Function& testFunction = m_offsetToFunctionMap[offset];

            if (entry->address >= testFunction.startAddress
                && entry->address < testFunction.endAddress)
            {
                f = &testFunction;
                break;
            }
        }

        if (!f)
        {
            warnUnknownLineNumberFunction(entry, file);
            continue;
        }

        Cpp::FunctionLineNumber l;
        l.line = entry->lineNumber;
        l.character = entry->lineCharacter;
        l.address = entry->address;
        l.isWholeLine = (entry->lineCharacter == DW_SOURCE_NO_POS);

        f->lineNumbers.append(l);
    }
}

void CppCodeModel::warnUnknownEntry(DwarfEntry* child, DwarfEntry* parent)
{
    if (!m_settings.warnUnknownEntries)
    {
        return;
    }

    // Ignore null entries
    if (child->isNull())
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

    Output::write(tr("Warning: Unknown child entry %1 (%2) at offset %3 in parent entry %4 (%5)")
        .arg(Dwarf::tagToString(child->tag))
        .arg(child->getName())
        .arg(Util::hexToString(child->offset))
        .arg(parent ? Dwarf::tagToString(parent->tag) : tr("(root)"))
        .arg(parent ? parent->getName() : QString()));
}

void CppCodeModel::warnUnknownAttribute(DwarfAttribute* attribute, DwarfEntry* entry)
{
    if (!m_settings.warnUnknownAttributes)
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

    Output::write(tr("Warning: Unknown attribute %1 at offset %2 in entry %3 (%4)")
        .arg(Dwarf::attrNameToString(attribute->name))
        .arg(Util::hexToString(attribute->offset))
        .arg(Dwarf::tagToString(entry->tag))
        .arg(entry->getName()));
}

void CppCodeModel::warnUnknownLineNumberFunction(DwarfSourceStatementEntry* entry, Cpp::File& file)
{
    if (!m_settings.warnUnknownLineNumberFunctions)
    {
        return;
    }

#ifdef MAX_WARNINGS_ACTIVE
    if (s_warningCount >= MAX_WARNINGS)
    {
        return;
    }

    s_warningCount++;
#endif

    Output::write(tr("Warning: Line number address %1 is not in any function in file %2")
        .arg(Util::hexToString(entry->address))
        .arg(file.path));
}

void CppCodeModel::warnUnknownLocationConfiguration(DwarfAttribute* attribute, DwarfEntry* entry)
{
    if (!m_settings.warnUnknownLocationConfigurations)
    {
        return;
    }

#ifdef MAX_WARNINGS_ACTIVE
    if (s_warningCount >= MAX_WARNINGS)
    {
        return;
    }

    s_warningCount++;
#endif

    Output::write(tr("Warning: Unknown location configuration at offset %1 in entry %2 (%3)")
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
        writeFunctionDeclaration(code, m_offsetToFunctionMap[offset]);
        writeNewline(code);
        writeNewline(code);
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

    if (m_settings.writeFunctionDeclarations
        || m_settings.writeFunctionDefinitions)
    {
        QList<Cpp::Function*> functions;

        for (Elf32_Off fileOffset : fileOffsets)
        {
            Cpp::File& file = m_offsetToFileMap[fileOffset];

            for (Elf32_Off functionOffset : file.functionOffsets)
            {
                functions.append(&m_offsetToFunctionMap[functionOffset]);
            }
        }

        if (m_settings.sortFunctionsByLineNumber)
        {
            std::sort(functions.begin(), functions.end(),
                [](Cpp::Function* a, Cpp::Function* b)
                {
                    if (a->lineNumbers.isEmpty() || b->lineNumbers.isEmpty())
                    {
                        return false;
                    }

                    return a->lineNumbers[0].line < b->lineNumbers[0].line;
                });
        }

        if (m_settings.writeFunctionDeclarations)
        {
            for (Cpp::Function* f : functions)
            {
                writeFunctionDeclaration(code, *f);
                writeNewline(code);
            }

            writeNewline(code);
        }

        if (m_settings.writeFunctionDefinitions)
        {
            for (Cpp::Function* f : functions)
            {
                writeFunctionDefinition(code, *f);
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

    if (m_settings.writeDwarfEntryOffsets)
    {
        writeComment(code, QString("DWARF: %1").arg(Util::hexToString(f.entry->offset)));
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

    if (m_settings.writeFunctionDisassembly)
    {
        Disassembly disasm;

        if (Disassemblers::disassemble(disasm, dwarf()->elf, f.startAddress, f.endAddress))
        {
            writeNewline(code);

            int leftSize = 0;
            int rightSize = 0;

            for (int line = 0; line < disasm.lineCount(); line++)
            {
                leftSize = qMax(leftSize, disasm.leftText(line).size());
                rightSize = qMax(rightSize, disasm.rightText(line).size());
            }

            leftSize += 4;
            rightSize += 1;

            if (!f.lineNumbers.isEmpty() && m_settings.writeFunctionLineNumbers)
            {
                int lineNumberIndex = 0;

                for (int line = 0; line < disasm.lineCount(); line++)
                {
                    writeNewline(code);
                    writeDisassemblyLineComment(code, disasm.leftText(line), disasm.rightText(line), leftSize, rightSize);

                    if (lineNumberIndex < f.lineNumbers.size()
                        && f.lineNumbers[lineNumberIndex].address == disasm.address(line))
                    {
                        writeFunctionLineNumberComment(code, f.lineNumbers[lineNumberIndex]);
                        lineNumberIndex++;
                    }
                }
            }
            else
            {
                for (int line = 0; line < disasm.lineCount(); line++)
                {
                    writeNewline(code);
                    writeDisassemblyLineComment(code, disasm.leftText(line), disasm.rightText(line), leftSize, rightSize);
                }
            }

            writeNewline(code);
        }
    }
    else if (!f.lineNumbers.isEmpty() && m_settings.writeFunctionLineNumbers)
    {
        writeNewline(code);

        for (Cpp::FunctionLineNumber& l : f.lineNumbers)
        {
            writeNewline(code);
            writeFunctionLineNumberComment(code, l);
        }
    }

    decreaseIndent();

    writeNewline(code);
    code += "}";
}

void CppCodeModel::writeDisassemblyLineComment(QString& code, const QString& leftText, const QString& rightText, int leftSize, int rightSize)
{
    code += QString("// %1%2").arg(leftText, -leftSize, ' ').arg(rightText, -rightSize, ' ');
}

void CppCodeModel::writeFunctionLineNumberComment(QString& code, Cpp::FunctionLineNumber& l)
{
    QStringList comment;

    comment += QString("Line: %1").arg(l.line);

    if (!l.isWholeLine)
    {
        comment += QString("Character: %1").arg(l.character);
    }

    comment += QString("Address: %1").arg(Util::hexToString(l.address));

    writeComment(code, comment.join(", "));
}

void CppCodeModel::writeFunctionSignature(QString& code, Cpp::Function& f, bool isDeclaration, bool isInsideClass)
{
    bool isNonStaticMemberFunction = false;
    bool isConstMemberFunction = false;
    bool isConstructorOrDestructor = false;

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

    if (f.name == "__ct" || f.name == "__dt")
    {
        isConstructorOrDestructor = true;
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

    if (!isConstructorOrDestructor)
    {
        writeTypePrefix(code, f.type);
        writeTypePostfix(code, f.type);
        code += " ";
    }

    QString name = f.name;

    if (f.isMember)
    {
        Q_ASSERT(m_offsetToClassTypeMap.contains(f.memberTypeOffset));

        Cpp::ClassType& c = m_offsetToClassTypeMap[f.memberTypeOffset];

        if (!isInsideClass)
        {
            code += QString("%1::").arg(c.name);
        }
        
        if (isConstructorOrDestructor)
        {
            if (f.name == "__ct")
            {
                name = c.name;
            }
            else if (f.name == "__dt")
            {
                name = QString("~%1").arg(c.name);
            }
        }
    }

    if (!isConstructorOrDestructor
        && f.name.startsWith("__")
        && operatorDemangleMap.contains(f.name))
    {
        name = operatorDemangleMap[f.name];
    }

    code += name;
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

    QStringList comment;

    if (m_settings.writeVariableMangledNames && !v.mangledName.isEmpty())
    {
        comment += v.mangledName;
    }

    if (m_settings.writeFunctionVariableLocations)
    {
        comment += v.location;
    }

    if (!comment.isEmpty())
    {
        code += " ";
        writeComment(code, comment.join(", "));
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

QString CppCodeModel::locationAtomToString(DwarfLocationAtom& atom)
{
    switch (atom.op)
    {
    case DW_OP_REG:
    case DW_OP_BASEREG:
        /* PowerPC (Gekko) */
        if (atom.number < 32)
        {
            return QString("r%1").arg(atom.number);
        }
        else if (atom.number < 64)
        {
            return QString("f%1").arg(atom.number - 32);
        }
        break;
    case DW_OP_ADDR:
        return Util::hexToString(atom.addr);
    case DW_OP_CONST:
        return Util::hexToString(atom.number);
    }

    return QString();
}

QString CppCodeModel::locationToString(DwarfLocation& location)
{
    switch (location.atoms.size())
    {
    case 1:
        return locationAtomToString(location.atoms[0]);
    case 2:
        if (location.atoms[0].op == DW_OP_CONST
            && location.atoms[1].op == DW_OP_ADD)
        {
            return locationAtomToString(location.atoms[0]);
        }
    case 3:
        if (location.atoms[0].op == DW_OP_BASEREG
            && location.atoms[1].op == DW_OP_CONST
            && location.atoms[2].op == DW_OP_ADD)
        {
            return QString("%1(%2)")
                .arg(locationAtomToString(location.atoms[1]))
                .arg(locationAtomToString(location.atoms[0]));
        }
    }

    return QString();
}

QString CppCodeModel::dwarfEntryName(Elf32_Off offset) const
{
    Q_ASSERT(m_offsetToEntryMap.contains(offset));

    return m_offsetToEntryMap[offset]->getName();
}

void CppCodeModel::setupSettingsMenu(QMenu* menu)
{
    QAction* action;

    action = menu->addAction(tr("Edit fundamental type names..."));
    connect(action, &QAction::triggered, this, [=]
        {
            CppFundamentalTypeNamesDialog dialog;
            dialog.setNames(m_settings.fundamentalTypeNames);
            dialog.setDefaultNames(s_defaultSettings.fundamentalTypeNames);

            if (dialog.exec() == QDialog::Accepted)
            {
                m_settings.fundamentalTypeNames = dialog.names();
                saveSettings();
                requestRewrite();
            }
        });

    action = menu->addAction(tr("Write classes/structs/unions"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeClassTypes);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeClassTypes = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = menu->addAction(tr("Write enums"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeEnumTypes);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeEnumTypes = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = menu->addAction(tr("Write array typedefs"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeArrayTypes);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeArrayTypes = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = menu->addAction(tr("Write function typedefs"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeFunctionTypes);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeFunctionTypes = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = menu->addAction(tr("Write pointer-to-member typedefs"));
    action->setCheckable(true);
    action->setChecked(m_settings.writePointerToMemberTypes);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writePointerToMemberTypes = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = menu->addAction(tr("Write variables"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeVariables);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeVariables = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = menu->addAction(tr("Write function declarations"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeFunctionDeclarations);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeFunctionDeclarations = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = menu->addAction(tr("Write function definitions"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeFunctionDefinitions);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeFunctionDefinitions = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = menu->addAction(tr("Sort types alphabetically"));
    action->setCheckable(true);
    action->setChecked(m_settings.sortTypesAlphabetically);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.sortTypesAlphabetically = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = menu->addAction(tr("Sort functions by line number"));
    action->setCheckable(true);
    action->setChecked(m_settings.sortFunctionsByLineNumber);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.sortFunctionsByLineNumber = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = menu->addAction(tr("Inline Metrowerks anonymous types (@class, @enum, etc.)"));
    action->setCheckable(true);
    action->setChecked(m_settings.inlineMetrowerksAnonymousTypes);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.inlineMetrowerksAnonymousTypes = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = menu->addAction(tr("Hexadecimal enum values"));
    action->setCheckable(true);
    action->setChecked(m_settings.hexadecimalEnumValues);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.hexadecimalEnumValues = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = menu->addAction(tr("Explicit enum values"));
    action->setCheckable(true);
    action->setChecked(m_settings.forceExplicitEnumValues);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.forceExplicitEnumValues = action->isChecked();
            saveSettings();
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
            saveSettings();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Class sizes"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeClassSizes);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeClassSizes = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Class member offsets"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeClassMemberOffsets);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeClassMemberOffsets = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Class member bitfield offsets"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeClassMemberBitOffsets);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeClassMemberBitOffsets = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Class member bitfield sizes"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeClassMemberBitSizes);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeClassMemberBitSizes = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Variable addresses"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeVariableAddresses);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeVariableAddresses = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Variable mangled names"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeVariableMangledNames);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeVariableMangledNames = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Function mangled names"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeFunctionMangledNames);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeFunctionMangledNames = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Function addresses"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeFunctionAddresses);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeFunctionAddresses = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Function sizes"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeFunctionSizes);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeFunctionSizes = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Function registers"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeFunctionVariableLocations);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeFunctionVariableLocations = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Function disassembly"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeFunctionDisassembly);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeFunctionDisassembly = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    action = commentsMenu->addAction(tr("Function line numbers"));
    action->setCheckable(true);
    action->setChecked(m_settings.writeFunctionLineNumbers);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.writeFunctionLineNumbers = action->isChecked();
            saveSettings();
            requestRewrite();
        });

    /* Warnings */
    QMenu* warningsMenu = menu->addMenu(tr("Warnings"));
    action = warningsMenu->addAction(tr("Unknown DWARF entries"));
    action->setCheckable(true);
    action->setChecked(m_settings.warnUnknownEntries);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.warnUnknownEntries = action->isChecked();
            saveSettings();
        });

    action = warningsMenu->addAction(tr("Unknown DWARF attributes"));
    action->setCheckable(true);
    action->setChecked(m_settings.warnUnknownAttributes);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.warnUnknownAttributes = action->isChecked();
            saveSettings();
        });

    action = warningsMenu->addAction(tr("Unknown line number functions"));
    action->setCheckable(true);
    action->setChecked(m_settings.warnUnknownLineNumberFunctions);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.warnUnknownLineNumberFunctions = action->isChecked();
            saveSettings();
        });

    action = warningsMenu->addAction(tr("Unknown location configurations"));
    action->setCheckable(true);
    action->setChecked(m_settings.warnUnknownLocationConfigurations);
    connect(action, &QAction::triggered, this, [=]
        {
            m_settings.warnUnknownLocationConfigurations = action->isChecked();
            saveSettings();
        });
}
