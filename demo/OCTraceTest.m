//
//  OCTraceTest.m
//  IPAPatch
//
//  Created by 吴昕 on 03/06/2017.
//  Copyright © 2017 Weibo. All rights reserved.
//

#import "OCTraceTest.h"

/* 
 octrace [13915:2256982]  |- [OCTraceTest shareInstance]
 octrace [13915:2256982]  |- - [OCTraceTest alloc]
 octrace [13915:2256982]  |- - [OCTraceTest init]
 octrace [13915:2256982]  |- [OCTraceTest test]
 octrace [13915:2256982]  |- - [OCTraceTest test1]
 octrace [13915:2256982]  |- - - [OCTraceTest test2]
 octrace [13915:2256982]  |- - - - [OCTraceTest test3]
 octrace [13915:2256982]  |- - - - [OCTraceTest test4]
 octrace [13915:2256982]  |- - - - - [OCTraceTest test5]
 octrace [13915:2256982]  |- - - [OCTraceTest test5]
 
 */
static OCTraceTest * sOCTraceTestInstance = nil;

@implementation OCTraceTest

+ (instancetype) shareInstance {
    static dispatch_once_t onceToken ;
    dispatch_once(&onceToken, ^{
        if (sOCTraceTestInstance == nil) {
            sOCTraceTestInstance = [[OCTraceTest alloc] init];
        }
    });
    return sOCTraceTestInstance;
}
- (void) test {
    [OCTraceTest test1];
}

+ (void) test1 {
    [OCTraceTest test2];
    [OCTraceTest test5];
}

+ (void) test2 {
    [OCTraceTest test3];
    [OCTraceTest test4];
}

+ (void)test3 {
    NSLog(@"ending 1");
}

+ (void) test4 {
    [OCTraceTest test5];
}

+ (void) test5 {
    NSLog(@"ending 2");
}


@end
