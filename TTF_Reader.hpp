//
//  TTF_Reader.hpp
//  
//
//  Created by supr on 2018/8/31.
//  Copyright © 2018年 supr. All rights reserved.
//

#ifndef TTF_Reader_hpp
#define TTF_Reader_hpp

#include <stdio.h>
bool getFontNameFromTTF(const char *lpszFilePath, char &szfontName, int &nFontNameEncoding);
#endif /* TTF_Reader_hpp */
