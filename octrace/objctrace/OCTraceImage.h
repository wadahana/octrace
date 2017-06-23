//
//  OCTraceImage.h
//  IPAPatch
//
//  Created by wadahana on 01/06/2017.
//  Copyright © 2017. All rights reserved.
//

#ifndef XTraceImage_h
#define XTraceImage_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>


class OCTraceImage {
public:
    OCTraceImage(){
        
        this->image_name = "";
        this->image_path = "";
        this->image_uuid = "";
        this->start_addr = NULL;
        this->end_addr = NULL;
        return;
    }
    
    OCTraceImage(const char * name,
                  const char * path,
                  const char * uuid,
                  intptr_t start_addr,
                  intptr_t end_addr) {
        this->image_name = name;
        this->image_path = path;
        this->image_uuid = uuid;
        this->start_addr = start_addr;
        this->end_addr = end_addr;
        return;
    }
    
    ~OCTraceImage() {
        return;
    }
    
public:
    std::string image_name;
    std::string image_path;
    std::string image_uuid;
    intptr_t start_addr;
    intptr_t end_addr;
};


void OCTraceImageListInit();

const OCTraceImage & OCTraceGetImageWithName(const char* name);

#endif /* OCTraceImage_h */
