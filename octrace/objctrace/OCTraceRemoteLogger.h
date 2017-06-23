//
//  OCTraceRemoteLogger.h
//  IPAPatch
//
//  Created by wadahana on 02/06/2017.
//  Copyright © 2017 . All rights reserved.
//

#ifndef OCTraceRemoteLogger_h
#define OCTraceRemoteLogger_h

#include <stdio.h>

#include "OCTraceLogger.h"


class OCTraceRemoteLogger : public OCTraceLogger {
public:
    OCTraceRemoteLogger();
    ~OCTraceRemoteLogger();
protected:
    virtual void trace(OCTraceLoggerCallee & callee);
};


#endif /* OCTraceRemoteLogger_h */
