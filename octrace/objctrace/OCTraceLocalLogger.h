//
//  OCTraceLocalLogger.h
//  IPAPatch
//
//  Created by wadahana on 01/06/2017.
//  Copyright © 2017 . All rights reserved.
//

#ifndef OCTraceLocalLogger_h
#define OCTraceLocalLogger_h


#include <stdio.h>

#include "OCTraceLogger.h"


class OCTraceLocalLogger : public OCTraceLogger {
public:
    OCTraceLocalLogger();
    ~OCTraceLocalLogger();
protected:
    virtual void trace(OCTraceLoggerCallee & callee);
};

#endif /* OCTraceLocalLogger_h */
