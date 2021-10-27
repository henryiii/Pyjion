/*
* The MIT License (MIT)
*
* Copyright (c) Microsoft Corporation
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*
*/

#define _PEPARSE_WINDOWS_CONFLICTS
#include <pe-parse/parse.h>
#undef _PEPARSE_WINDOWS_CONFLICTS

#include <windows.h>
#include <corhdr.h>
#include <readytorun.h>
#include <holder.h>
#include <vector>

#include <iostream>
#include <limits>
#include <memory>

#include <climits>
#include <cstring>
#include "exceptions.h"

#define T_CONTEXT CONTEXT
#define T_RUNTIME_FUNCTION RUNTIME_FUNCTION
#include <daccess.h>
#define EEPOLICY_HANDLE_FATAL_ERROR(error) assert(!"EEPOLICY_HANDLE_FATAL_ERROR")
#include <nativeformatreader.h>

#ifndef PYJION_PELOADER_H
#define PYJION_PELOADER_H
#define ALIGN4BYTE(val) (((val) + 3) & ~0x3)

using namespace std;

enum class AddressType {
    PhysicalOffset,
    RelativeVirtualAddress,
    VirtualAddress
};

struct ECMA_STORAGE_HEADER {
    uint32_t      lSignature;             // "Magic" signature.
    uint16_t      iMajorVer;              // Major file version.
    uint16_t      iMinorVer;              // Minor file version.
    uint32_t      iExtraData;             // Offset to next structure of information
    uint32_t      iVersionString;         // Length of version string
    BYTE          pVersion[0];            // Version string

};
struct ECMA_STORAGE_HEADER2 {
    BYTE fFlags;// STGHDR_xxx flags.
    BYTE pad;
    uint16_t iStreams;// How many streams are there.
};

struct ECMA_STREAM_HEADER {
    uint32_t offset;     // Memory offset to start of this stream from start of the metadata root
    uint32_t size;       // Size of this stream in bytes, shall be a multiple of 4.
    char     name[32];   // Stream name
};

struct HOME_TABLE_HEADER { // Header for #~
    uint32_t Reserved1; // Reserved, always 0
    BYTE MajorVersion; // Major version of table schemata, always 1
    BYTE MinorVersion; // Minor version of table schemata, always 0
    BYTE HeapOffsetSizes; // Bit vector for heap sizes.
    BYTE Reserved2; // Reserved, always 1
    bitset<64> MaskValid; // Bit vector of present tables, let n be the number of bits that are 1.
    bitset<64> MaskSorted; // Bit vector of present tables, let n be the number of bits that are 1.
};

enum MetaDataTable
{
    MetaDataTable_Module = 0x00,
    MetaDataTable_TypeRef = 0x01,
    MetaDataTable_TypeDef = 0x02,
    MetaDataTable_FieldPtr = 0x03,
    MetaDataTable_Field = 0x04,
    MetaDataTable_MethodPtr = 0x05,
    MetaDataTable_Method = 0x06,
    MetaDataTable_ParamPtr = 0x07,
    MetaDataTable_Param = 0x08,
    MetaDataTable_InterfaceImpl = 0x09,
    MetaDataTable_MemberRef = 0x0A,
    MetaDataTable_Constant = 0x0B,
    MetaDataTable_CustomAttribute = 0x0C,
    MetaDataTable_FieldMarshal = 0x0D,
    MetaDataTable_DeclSecurity = 0x0E,
    MetaDataTable_ClassLayout = 0x0F,
    MetaDataTable_FieldLayout = 0x10,
    MetaDataTable_StandAloneSig = 0x11,
    MetaDataTable_EventMap = 0x12,
    MetaDataTable_EventPtr = 0x13,
    MetaDataTable_Event = 0x14,
    MetaDataTable_PropertyMap = 0x15,
    MetaDataTable_PropertyPtr = 0x16,
    MetaDataTable_Property = 0x17,
    MetaDataTable_MethodSemantics = 0x18,
    MetaDataTable_MethodImpl = 0x19,
    MetaDataTable_ModuleRef = 0x1A,
    MetaDataTable_TypeSpec= 0x1B,
    MetaDataTable_ImplMap= 0x1C,
    MetaDataTable_FieldRVA= 0x1D,
    MetaDataTable_ENCLog= 0x1E,
    MetaDataTable_ENCMap= 0x1F,
    MetaDataTable_Assembly= 0x20,
    MetaDataTable_AssemblyProcessor= 0x21,
    MetaDataTable_AssemblyOS= 0x22,
    MetaDataTable_AssemblyRef= 0x23,
    MetaDataTable_AssemblyRefProcessor= 0x24,
    MetaDataTable_AssemblyRefOS= 0x25,
    MetaDataTable_File= 0x26,
    MetaDataTable_ExportedType= 0x27,
    MetaDataTable_ManifestResource= 0x28,
    MetaDataTable_NestedClass= 0x29,
    MetaDataTable_GenericParam= 0x2A,
    MetaDataTable_MethodSpec= 0x2B,
    MetaDataTable_GenericParamConstraint= 0x2C,
};

#pragma pack(2)
struct ModuleTableRow {
    uint16_t Generation;
    uint16_t Name;
    uint16_t Mvid;
    uint16_t EncId;
    uint16_t EncBaseId;
};

#pragma pack(2)
struct TypeRefRow {
    uint16_t ResolutionScope;
    uint16_t Name;
    uint16_t Namespace;
};

#pragma pack(2)
struct TypeDefRow {
    uint32_t Flags;
    uint16_t Name;
    uint16_t Namespace;
    uint16_t Extends;
    uint16_t FieldList;
    uint16_t MethodList;
};

#pragma pack(2)
struct FieldRow {
    uint16_t Flags;
    uint16_t Name;
    uint16_t Signature;
};

#pragma pack(2)
struct MethodRow {
    uint32_t RVA;
    uint16_t ImplFlags;
    uint16_t Flags;
    uint16_t Name;
    uint16_t Signature;
    uint16_t ParamList;
};

#pragma pack(2)
struct ParamRow {
    uint16_t Flags;
    uint16_t Sequence;
    uint16_t Name;
};

#pragma pack(2)
struct InterfaceImplRow {
    uint16_t Class;
    uint16_t Interface;
};

static const MetaDataTable AllMetaDataTables[] = {
        MetaDataTable_Module,
        MetaDataTable_TypeRef,
        MetaDataTable_TypeDef,
        MetaDataTable_FieldPtr,
        MetaDataTable_Field,
        MetaDataTable_MethodPtr,
        MetaDataTable_Method,
        MetaDataTable_ParamPtr,
        MetaDataTable_Param,
        MetaDataTable_InterfaceImpl,
        MetaDataTable_MemberRef,
        MetaDataTable_Constant,
        MetaDataTable_CustomAttribute,
        MetaDataTable_FieldMarshal,
        MetaDataTable_DeclSecurity,
        MetaDataTable_ClassLayout,
        MetaDataTable_FieldLayout,
        MetaDataTable_StandAloneSig,
        MetaDataTable_EventMap,
        MetaDataTable_EventPtr,
        MetaDataTable_Event,
        MetaDataTable_PropertyMap,
        MetaDataTable_PropertyPtr,
        MetaDataTable_Property,
        MetaDataTable_MethodSemantics,
        MetaDataTable_MethodImpl,
        MetaDataTable_ModuleRef,
        MetaDataTable_TypeSpec,
        MetaDataTable_ImplMap,
        MetaDataTable_FieldRVA,
        MetaDataTable_ENCLog,
        MetaDataTable_ENCMap,
        MetaDataTable_Assembly,
        MetaDataTable_AssemblyProcessor,
        MetaDataTable_AssemblyOS,
        MetaDataTable_AssemblyRef,
        MetaDataTable_AssemblyRefProcessor,
        MetaDataTable_AssemblyRefOS,
        MetaDataTable_File,
        MetaDataTable_ExportedType,
        MetaDataTable_ManifestResource,
        MetaDataTable_NestedClass,
        MetaDataTable_GenericParam,
        MetaDataTable_MethodSpec,
        MetaDataTable_GenericParamConstraint };


bool convertAddress(peparse::parsed_pe* pe,
                    std::uint64_t address,
                    AddressType source_type,
                    AddressType destination_type,
                    std::uint64_t& result);

class InvalidImageException : public PyjionJitRuntimeException {
private:
    const char* reason = "InvalidImageException";

public:
    explicit InvalidImageException(const char* why) : PyjionJitRuntimeException() {
        reason = why;
    };
    const char* what() const noexcept override {
        return reason;
    }
};

#define COMPILER_ID_SIZE 50

class PEDecoder {
private:
    peparse::parsed_pe* pe;
    IMAGE_COR20_HEADER corHeader;
    READYTORUN_HEADER r2rHeader;
    char compilerIdentifier[COMPILER_ID_SIZE] = "";
    vector<READYTORUN_IMPORT_SECTION> allImportSections;
    vector<RUNTIME_FUNCTION> allRuntimeFunctions;
    NativeFormat::NativeArray m_methodDefEntryPoints;
    vector<ECMA_STREAM_HEADER> streamHeaders;
    map<MetaDataTable, uint32_t> metaDataTableSizes;
    // Metadata tables;
    vector<ModuleTableRow> moduleTable;
    vector<TypeDefRow> typeDefTable;
    vector<TypeRefRow> typeRefTable;
//    vector<FieldPtrRow> fieldPtrTable;
    vector<FieldRow> fieldTable;
//    vector<MethodPtrRow> methodPtrTable;
    vector<MethodRow> methodTable;
    vector<ParamRow> paramTable;
    vector<InterfaceImplRow> interfaceImplTable;

    std::string stringHeap;
    std::string assemblyVersion;

public:
    PEDecoder(const char* filePath);
    ~PEDecoder() {
        peparse::DestructParsedPE(pe);
    }

    IMAGE_COR20_HEADER* GetCorHeader() {
        return &corHeader;
    }

    bool IsReadyToRun() const {
        return corHeader.Flags & COMIMAGE_FLAGS_IL_LIBRARY;
    }

    READYTORUN_HEADER* GetReadyToRunHeader() {
        return &r2rHeader;
    }

    std::string GetString(uint64_t offset){ // TODO : Optimize
        uint64_t i = offset;
        std::string result;
        while (stringHeap[i] != '\0'){
            result.push_back(stringHeap[i]);
            i++;
        }
        return result;
    }

    std::string GetModuleName(){
        if (moduleTable.size() != 1)
            return "";
        return GetString(moduleTable[0].Name);
    }

    const char* GetCompilerID(){
        return compilerIdentifier;
    }

    const char* GetVersion(){
        return assemblyVersion.c_str();
    }

    vector<TypeRefRow> GetTypeRefs(){
        return typeRefTable;
    }

    vector<TypeDefRow> GetTypeDefs(){
        return typeDefTable;
    }

    vector<TypeDefRow> GetPublicClasses(){
        vector<TypeDefRow> results;
        results.reserve(typeDefTable.size());
        for (const auto &def : typeDefTable){
            if (IsTdClass(def.Flags) && IsTdPublic(def.Flags))
                results.push_back(def);
        }
        results.shrink_to_fit();
        return results;
    }
};


#endif//PYJION_PELOADER_H
