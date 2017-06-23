//
//  OCTraceUtils.h
//  IPAPatch
//
//  Created by wadahana on 02/06/2017.
//  Copyright © 2017 . All rights reserved.
//

#ifndef OCTraceUtils_h
#define OCTraceUtils_h

#include <stdio.h>
#include <string>

void string_appendv(std::string* dst, const char* format, va_list ap);

void string_appendf(std::string* dst, const char* format, ...);

std::string string_sprintf(const char* format, ...);

#endif /* OCTraceUtils_h */
