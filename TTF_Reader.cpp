//
//  TTF_Reader.cpp
//  
//
//  Created by supr on 2018/8/31.
//  Copyright © 2018年 supr. All rights reserved.
//

#include "TTF_Reader.hpp"
#include <stdio.h>
#include <string.h>
#include <map>
#include <istream>

using namespace std;

#define USHORT unsigned short
#define ULONG unsigned int

typedef struct _tagTT_OFFSET_TABLE{
    USHORT    uMajorVersion;
    USHORT    uMinorVersion;
    USHORT    uNumOfTables;
    USHORT    uSearchRange;
    USHORT    uEntrySelector;
    USHORT    uRangeShift;
}TT_OFFSET_TABLE;

typedef struct _tagTT_TABLE_DIRECTORY{
    char    szTag[4];            //table name
    ULONG    uCheckSum;            //Check sum
    ULONG    uOffset;            //Offset from beginning of file
    ULONG    uLength;            //length of the table in bytes
}TT_TABLE_DIRECTORY;

typedef struct _tagTT_NAME_TABLE_HEADER{
    USHORT    uFSelector;            //format selector. Always 0
    USHORT    uNRCount;            //Name Records count
    USHORT    uStorageOffset;        //Offset for strings storage, from start of the table
}TT_NAME_TABLE_HEADER;

typedef struct _tagTT_NAME_RECORD{
    USHORT    uPlatformID;
    USHORT    uEncodingID;
    USHORT    uLanguageID;
    USHORT    uNameID;
    USHORT    uStringLength;
    USHORT    uStringOffset;    //from start of storage area
}TT_NAME_RECORD;

#define MAKEWORD(a, b) (((a & 0xff) << 8) | (b >> 8))
#define MAKELONG(a, b) (((a & 0xff) << 24) | ((a & 0xff00) << 8) | ((b & 0xff0000) >> 8) | (b >> 24))

#define SWAPWORD(x)        MAKEWORD(x,x)
#define SWAPLONG(x)        MAKELONG(x,x)

#define BUFF_SIZE 256


bool getFontNameFromTTF(const char *lpszFilePath, char &szfontName, int &nFontNameEncoding)
{
    
    map<int, u16string> enName;
    map<int, u16string> cnName;
    
    FILE *f = NULL;
    int font_name_len = 0;
    f = fopen(lpszFilePath, "rb");
    if (NULL == f) {
        return NULL;
    }
    else
    {
        TT_OFFSET_TABLE ttOffsetTable;
        fread(&ttOffsetTable, sizeof(TT_OFFSET_TABLE), 1, f);
        ttOffsetTable.uNumOfTables = SWAPWORD(ttOffsetTable.uNumOfTables);
        ttOffsetTable.uMajorVersion = SWAPWORD(ttOffsetTable.uMajorVersion);
        ttOffsetTable.uMinorVersion = SWAPWORD(ttOffsetTable.uMinorVersion);
        
        //check is this is a true type font and the version is 1.0
        if(ttOffsetTable.uMajorVersion != 1 || ttOffsetTable.uMinorVersion != 0)
            return NULL;
        
        TT_TABLE_DIRECTORY tblDir;
        bool bFound = false;
        char csTemp[BUFF_SIZE] = {0};
        
        for(int i=0; i< ttOffsetTable.uNumOfTables; i++){
            fread(&tblDir, sizeof(TT_TABLE_DIRECTORY),1,f);
            strncpy(csTemp, tblDir.szTag, 4);
            if(0 == strcmp(csTemp, "name"))
            {
                bFound = true;
                tblDir.uLength = SWAPLONG(tblDir.uLength);
                tblDir.uOffset = SWAPLONG(tblDir.uOffset);
                break;
            }
        }
        
        if(bFound){
            fseek(f, tblDir.uOffset, SEEK_SET);
            
            TT_NAME_TABLE_HEADER ttNTHeader;
            fread(&ttNTHeader, sizeof(TT_NAME_TABLE_HEADER),1,f);
            ttNTHeader.uNRCount = SWAPWORD(ttNTHeader.uNRCount);
            ttNTHeader.uStorageOffset = SWAPWORD(ttNTHeader.uStorageOffset);
            TT_NAME_RECORD ttRecord;
            bFound = false;
            
            for(int i=0; i<ttNTHeader.uNRCount; i++){
                fread(&ttRecord, sizeof(TT_NAME_RECORD),1,f);
                
                ttRecord.uNameID = SWAPWORD(ttRecord.uNameID);
                ttRecord.uLanguageID = SWAPWORD(ttRecord.uLanguageID);
                ttRecord.uEncodingID = SWAPWORD(ttRecord.uEncodingID);
                ttRecord.uStringLength = SWAPWORD(ttRecord.uStringLength);
                ttRecord.uStringOffset = SWAPWORD(ttRecord.uStringOffset);
                
                long nPos = ftell(f);
                fseek(f, tblDir.uOffset + ttRecord.uStringOffset + ttNTHeader.uStorageOffset, SEEK_SET);
                char16_t * font_name = (char16_t*)malloc(BUFF_SIZE);
                memset(font_name,0, BUFF_SIZE);
                fread(font_name, ttRecord.uStringLength, 1, f);
                font_name_len = ttRecord.uStringLength;
 
                fseek(f, nPos, SEEK_SET);
                
                u16string strName(font_name);
                if (2052 == ttRecord.uLanguageID) {
                    cnName.insert(map<int, u16string>::value_type(ttRecord.uNameID,strName));
                }
                
                if (0 == ttRecord.uLanguageID) {
                    enName.insert(map<int, u16string>::value_type(ttRecord.uNameID,strName));
                }
                free(font_name);
                printf("uLanguageID:%d ---- uNameID:%d ---- uName:%s\n",ttRecord.uLanguageID,ttRecord.uNameID,strName.c_str());
                
            }
        }
        fclose(f);
    }
    
    //select
    map<int, u16string>::iterator iter;
    u16string strName;
    
    if (cnName.size()>4) {
        nFontNameEncoding = 1;
        iter = cnName.find(4);
        strName = iter->second;
        if (strName.length()>0) {
            goto copy_and_return;
        }
        else
        {
            iter = cnName.find(1);
            strName = iter->second;
            if (strName.length()>0) {
               goto copy_and_return;
            }
        }
    }
    else
    {
        nFontNameEncoding = 0;
        iter = enName.find(4);
        strName = iter->second;
        if (strName.length()>0) {
            goto copy_and_return;
        }
        else
        {
            iter = enName.find(1);
            strName = iter->second;
            if (strName.length()>0) {
               goto copy_and_return;
            }
        }
    }

copy_and_return:
    //copy data
    if (strName.length()>0) {
        memcpy(&szfontName,strName.c_str(),strName.length()*sizeof(char16_t));
    }
    
    //free
    enName.clear();
    cnName.clear();

    return true;
}


