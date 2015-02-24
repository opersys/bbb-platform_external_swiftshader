//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

//
// Symbol table for parsing.  Most functionaliy and main ideas
// are documented in the header file.
//

#if defined(_MSC_VER)
#pragma warning(disable: 4718)
#endif

#include "SymbolTable.h"

#include <stdio.h>
#include <algorithm>

#if defined(_MSC_VER)
#define snprintf _snprintf
#endif

int TSymbolTableLevel::uniqueId = 0;

TType::TType(const TPublicType &p) :
    type(p.type), precision(p.precision), qualifier(p.qualifier), size(p.size), matrix(p.matrix), array(p.array), arraySize(p.arraySize),
    maxArraySize(0), arrayInformationType(0), structure(0), structureSize(0), deepestStructNesting(0), fieldName(0), mangled(0), typeName(0)
{
    if (p.userDef)
    {
        structure = p.userDef->getStruct();
        typeName = NewPoolTString(p.userDef->getTypeName().c_str());
        computeDeepestStructNesting();
    }
}

//
// Recursively generate mangled names.
//
void TType::buildMangledName(TString& mangledName)
{
    if (isMatrix())
        mangledName += 'm';
    else if (isVector())
        mangledName += 'v';

    switch (type) {
    case EbtFloat:              mangledName += 'f';      break;
    case EbtInt:                mangledName += 'i';      break;
    case EbtUInt:               mangledName += 'u';      break;
    case EbtBool:               mangledName += 'b';      break;
    case EbtSampler2D:          mangledName += "s2";     break;
    case EbtSamplerCube:        mangledName += "sC";     break;
    case EbtSamplerExternalOES: mangledName += "sE";     break;
	case EbtSampler3D:          mangledName += "s3";     break;
	case EbtStruct:
        mangledName += "struct-";
        if (typeName)
            mangledName += *typeName;
        {// support MSVC++6.0
            for (unsigned int i = 0; i < structure->size(); ++i) {
                mangledName += '-';
                (*structure)[i].type->buildMangledName(mangledName);
            }
        }
    default:
        break;
    }

    mangledName += static_cast<char>('0' + getNominalSize());
    if (isArray()) {
        char buf[20];
        snprintf(buf, sizeof(buf), "%d", arraySize);
        mangledName += '[';
        mangledName += buf;
        mangledName += ']';
    }
}

int TType::getStructSize() const
{
    if (!getStruct()) {
        assert(false && "Not a struct");
        return 0;
    }

    if (structureSize == 0)
        for (TTypeList::const_iterator tl = getStruct()->begin(); tl != getStruct()->end(); tl++)
            structureSize += ((*tl).type)->getObjectSize();

    return structureSize;
}

void TType::computeDeepestStructNesting()
{
    if (!structure)
    {
        return;
    }

    int maxNesting = 0;
    for (TTypeList::const_iterator tl = structure->begin(); tl != structure->end(); tl++)
    {
        maxNesting = std::max(maxNesting, ((*tl).type)->getDeepestStructNesting());
    }

    deepestStructNesting = 1 + maxNesting;
}

bool TType::isStructureContainingArrays() const
{
    if (!structure)
    {
        return false;
    }

    for (TTypeList::const_iterator member = structure->begin(); member != structure->end(); member++)
    {
        if (member->type->isArray() ||
            member->type->isStructureContainingArrays())
        {
            return true;
        }
    }

    return false;
}

//
// Functions have buried pointers to delete.
//
TFunction::~TFunction()
{
    for (TParamList::iterator i = parameters.begin(); i != parameters.end(); ++i)
        delete (*i).type;
}

//
// Symbol table levels are a map of pointers to symbols that have to be deleted.
//
TSymbolTableLevel::~TSymbolTableLevel()
{
    for (tLevel::iterator it = level.begin(); it != level.end(); ++it)
        delete (*it).second;
}

//
// Change all function entries in the table with the non-mangled name
// to be related to the provided built-in operation.  This is a low
// performance operation, and only intended for symbol tables that
// live across a large number of compiles.
//
void TSymbolTableLevel::relateToOperator(const char* name, TOperator op)
{
    tLevel::iterator it;
    for (it = level.begin(); it != level.end(); ++it) {
        if ((*it).second->isFunction()) {
            TFunction* function = static_cast<TFunction*>((*it).second);
            if (function->getName() == name)
                function->relateToOperator(op);
        }
    }
}

//
// Change all function entries in the table with the non-mangled name
// to be related to the provided built-in extension. This is a low
// performance operation, and only intended for symbol tables that
// live across a large number of compiles.
//
void TSymbolTableLevel::relateToExtension(const char* name, const TString& ext)
{
    for (tLevel::iterator it = level.begin(); it != level.end(); ++it) {
        if (it->second->isFunction()) {
            TFunction* function = static_cast<TFunction*>(it->second);
            if (function->getName() == name)
                function->relateToExtension(ext);
        }
    }
}

TSymbol *TSymbolTable::find(const TString &name, int shaderVersion, bool *builtIn, bool *sameScope) const
{
    int level = currentLevel();
    TSymbol *symbol = nullptr;

    do
    {
        symbol = table[level]->find(name);
        --level;
    }
    while(!symbol && level >= 0);

    level++;

    if(builtIn)
    {
        *builtIn = (level == 0);
    }

    if(sameScope)
    {
        *sameScope = (level == currentLevel());
    }

    return symbol;
}

TSymbol *TSymbolTable::findBuiltIn(const TString &name, int shaderVersion) const
{
    return table[0]->find(name);
}

TSymbol::TSymbol(const TSymbol& copyOf)
{
    name = NewPoolTString(copyOf.name->c_str());
}
