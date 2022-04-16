#pragma once

#include "AbstractCodeModel.h"
#include "DwarfAttributes.h"

#include <qmap.h>
#include <qhash.h>

namespace Cpp
{
	enum class Keyword
	{
		Class,
		Const,
		Enum,
		Inline,
		Private,
		Protected,
		Public,
		Static,
		Struct,
		Typedef,
		Union,
		Volatile
	};

	enum class FundamentalType
	{
		Char = DW_FT_char,
		SignedChar = DW_FT_signed_char,
		UnsignedChar = DW_FT_unsigned_char,
		Short = DW_FT_short,
		SignedShort = DW_FT_signed_short,
		UnsignedShort = DW_FT_unsigned_short,
		Int = DW_FT_integer,
		SignedInt = DW_FT_signed_integer,
		UnsignedInt = DW_FT_unsigned_integer,
		Long = DW_FT_long,
		SignedLong = DW_FT_signed_long,
		UnsignedLong = DW_FT_unsigned_long,
		VoidPointer = DW_FT_pointer,
		Float = DW_FT_float,
		Double = DW_FT_dbl_prec_float,
		Void = DW_FT_void,
		Bool = DW_FT_boolean,
		LongLong = DW_FT_long_long
	};

	enum class ModifierType
	{
		Pointer,
		Reference
	};

	struct Modifier
	{
		ModifierType type;
		bool isConst;
		bool isVolatile;
	};

	struct Type
	{
		bool isFundamental;

		union
		{
			FundamentalType fundType;
			Elf32_Off userTypeOffset;
		};
		
		bool isConst;
		bool isVolatile;
		QList<Modifier> modifiers;
	};

	struct Typedef
	{
		DwarfEntry* entry;
		Type type;
		QString name;
	};

	struct Declaration
	{
		DwarfEntry* entry;
		Type type;
		QString name;
	};

	struct ClassMember : Declaration
	{
		Keyword access;
		int offset;
		bool isBitfield;
		int bitOffset;
		int bitSize;
	};

	struct ClassInheritance
	{
		DwarfEntry* entry;
		Type type;
		Keyword access;
		bool isVirtual;
	};

	struct ClassType
	{
		DwarfEntry* entry;
		Keyword keyword;
		QString name;
		int size;
		QList<ClassMember> members;
		QList<ClassInheritance> inheritances;
		QList<Typedef> typedefs;
		QList<Elf32_Off> functionOffsets;
	};

	struct EnumElement
	{
		QString name;
		int value;
	};

	struct EnumType
	{
		DwarfEntry* entry;
		QString name;
		QList<EnumElement> elements;
	};

	struct ArrayType
	{
		DwarfEntry* entry;
		Type type;
		QString name;
		QList<int> dimensions;
	};

	struct FunctionParameter : Declaration
	{
		QString location;
	};

	struct FunctionType
	{
		DwarfEntry* entry;
		Type type;
		QString name;
		QList<FunctionParameter> parameters;
	};

	struct FunctionVariable : Declaration
	{
		QString location;
	};

	struct Function : FunctionType
	{
		bool isGlobal;
		bool isInline;
		Elf32_Addr startAddress;
		Elf32_Addr endAddress;
		bool isMember;
		Elf32_Off memberTypeOffset;
		Cpp::Keyword memberAccess;
		QString mangledName;
		QList<FunctionVariable> variables;
	};

	struct Variable : Declaration
	{
		bool isGlobal;
		Elf32_Addr address;
		QString mangledName;
	};

	struct File
	{
		DwarfEntry* entry;
		QString path;
		QList<Elf32_Off> typeOffsets;
		QList<Elf32_Off> functionOffsets;
		QList<Elf32_Off> variableOffsets;
	};
}

struct CppCodeModelSettings
{
	bool printUnknownEntries;
	bool printUnknownAttributes;
	bool writeTypes;
	bool writeVariables;
	bool writeFunctionDeclarations;
	bool writeFunctionDefinitions;
	bool writeDwarfEntryOffsets;
	bool writeClassSizes;
	bool writeClassMemberOffsets;
	bool writeClassMemberBitOffsets;
	bool writeClassMemberBitSizes;
	bool writeVariableAddresses;
	bool writeVariableMangledNames;
	bool writeFunctionMangledNames;
	bool writeFunctionAddresses;
	bool writeFunctionSizes;
	bool writeFunctionVariableLocations;
	bool sortTypesAlphabetically;
	bool inlineMetrowerksUnnamedTypes;
	bool hexadecimalEnumValues;
	bool forceExplicitEnumValues;
	QHash<Cpp::FundamentalType, QString> fundamentalTypeNames;
};

class CppCodeModel : public AbstractCodeModel
{
	Q_OBJECT

public:
	CppCodeModel(QObject* parent = nullptr);

	static CppCodeModelSettings& defaultSettings();

	CppCodeModelSettings& settings();
	const CppCodeModelSettings& settings() const;

	void writeDwarfEntry(Code& code, Elf32_Off offset) override;
	void writeFile(Code& code, const QString& path) override;

protected:
	void parseDwarf(Dwarf* dwarf) override;

private:
	static CppCodeModelSettings s_defaultSettings;
	static QHash<Cpp::Keyword, QString> s_keywordToStringMap;

	CppCodeModelSettings m_settings;
	QMultiMap<QString, Elf32_Off> m_pathToOffsetMultiMap;
	QHash<Elf32_Off, DwarfEntry*> m_offsetToEntryMap;
	QHash<Elf32_Off, Cpp::File> m_offsetToFileMap;
	QHash<Elf32_Off, Cpp::ClassType> m_offsetToClassTypeMap;
	QHash<Elf32_Off, Cpp::EnumType> m_offsetToEnumTypeMap;
	QHash<Elf32_Off, Cpp::ArrayType> m_offsetToArrayTypeMap;
	QHash<Elf32_Off, Cpp::FunctionType> m_offsetToFunctionTypeMap;
	QHash<Elf32_Off, Cpp::Function> m_offsetToFunctionMap;
	QHash<Elf32_Off, Cpp::Variable> m_offsetToVariableMap;
	int m_indentLevel;

	void clear();

	void parseCompileUnit(DwarfEntry* entry);
	void parseClassType(DwarfEntry* entry, Cpp::File& file);
	void parseMember(DwarfEntry* entry, Cpp::ClassType& c);
	void parseInheritance(DwarfEntry* entry, Cpp::ClassType& c);
	void parseTypedef(DwarfEntry* entry, Cpp::ClassType& c);
	void parseEnumerationType(DwarfEntry* entry, Cpp::File& file);
	void parseArrayType(DwarfEntry* entry, Cpp::File& file);
	void parseSubroutineType(DwarfEntry* entry, Cpp::File& file);
	void parseSubroutine(DwarfEntry* entry, Cpp::File& file);
	void parseFormalParameter(DwarfEntry* entry, Cpp::FunctionType& f);
	void parseLocalVariable(DwarfEntry* entry, Cpp::Function& f);
	void parseVariable(DwarfEntry* entry, Cpp::File& f);
	void parseTypedef(DwarfEntry* entry, Cpp::Typedef& t);
	void parseType(DwarfType& dt, Cpp::Type& t);

	void warnUnknownEntry(DwarfEntry* child, DwarfEntry* parent);
	void warnUnknownAttribute(DwarfAttribute* attribute, DwarfEntry* entry);

	void writeFiles(Code& code, const QList<Elf32_Off>& fileOffsets);
	void writeClassType(Code& code, Cpp::ClassType& c, bool isInline = false);
	void writeClassMember(Code& code, Cpp::ClassMember& m);
	void writeEnumType(Code& code, Cpp::EnumType& e, bool isInline = false);
	void writeEnumElement(Code& code, Cpp::EnumElement& e, bool explicitValue);
	void writeArrayType(Code& code, Cpp::ArrayType& a, bool isInline = false);
	void writeFunctionType(Code& code, Cpp::FunctionType& f, bool isInline = false);
	void writeVariable(Code& code, Cpp::Variable& v);
	void writeFunctionDeclaration(Code& code, Cpp::Function& f, bool isInsideClass = false);
	void writeFunctionDefinition(Code& code, Cpp::Function& f);
	void writeFunctionSignature(Code& code, Cpp::Function& f, bool isDeclaration, bool isInsideClass);
	void writeFunctionVariable(Code& code, Cpp::FunctionVariable& v);
	void writeDeclaration(Code& code, Cpp::Declaration& d);
	void writeTypedef(Code& code, Cpp::Typedef& t);
	void writeTypePrefix(Code& code, Cpp::Type& t);
	void writeTypePostfix(Code& code, Cpp::Type& t);
	void writeClassTypePrefix(Code& code, Cpp::ClassType& c);
	void writeClassTypePostfix(Code& code, Cpp::ClassType& c);
	void writeEnumTypePrefix(Code& code, Cpp::EnumType& e);
	void writeEnumTypePostfix(Code& code, Cpp::EnumType& e);
	void writeArrayTypePrefix(Code& code, Cpp::ArrayType& a);
	void writeArrayTypePostfix(Code& code, Cpp::ArrayType& a);
	void writeFunctionTypePrefix(Code& code, Cpp::FunctionType& f);
	void writeFunctionTypePostfix(Code& code, Cpp::FunctionType& f);
	void writeFunctionParameters(Code& code, Cpp::FunctionType& f, bool isDeclaration);
	void writeFunctionParameter(Code& code, Cpp::FunctionParameter& p, bool isDeclaration);
	void writeFundamentalType(Code& code, Cpp::FundamentalType t);
	void writeModifier(Code& code, Cpp::Modifier& m);
	void writeConstVolatile(Code& code, bool isConst, bool isVolatile);
	void writeIdentifier(Code& code, const QString& text);
	void writeLiteral(Code& code, const QString& text);
	void writeKeyword(Code& code, Cpp::Keyword keyword);
	void writeComment(Code& code, const QString& text);
	void writeMultilineComment(Code& code, const QString& text);
	void writePunctuation(Code& code, const QString& text);
	void writeSpace(Code& code);
	void writeNewline(Code& code, bool indent = true);
	void writeIndent(Code& code);

	void increaseIndent();
	void decreaseIndent();
	void resetIndent();

	bool typeCanBeInlined(const QString& name) const;
};
