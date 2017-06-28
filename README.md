##0x01 背景

做octrace的目的，一是想做Obj-C的函数覆盖测试，另外也是想有个比较方便的工具，做APP逆向时，比较容易找到关注的Hook点。

##0x02 原理

Obj-C runtime 中函数调用都是通过objc_msgSend来实现，第一个参数是id, 第二个参数是selector，剩下的参数就是实际selector的传入参数。

>origin\_objc\_msgSend: 原始objc_msgSend函数
>
>new\_objc\_msgSend: 劫持后的objc_msgSend函数
>
>\_\_hook\_callback\_pre: 在origin\_objc\_msgSend前调用
>
>\_\_hook\_callback\_post: 在origin\_objc\_msgSend后调用


hook了objc\_msgSend后，如果仅仅要打印下class和selector，则只要保存好传参寄存器，调用\_\_hook\_callback\_pre进行打印，返回后还原传参的寄存器，再调用origin\_objc\_msgSend。
但是仅仅这样的话，无法还原出函数调用的树形关系，我们需要在origin\_objc\_msgSend前后都需要对插入自己的代码，对函数调用深度进行标记。
在call origin\_objc\_msgSend之后，再做一次函数调用，涉及到几个问题：

1. origin\_objc_msgSend 跳转/返回方式(b/bl)
2. origin\_objc_msgSend 返回值
3. \_\_hook\_callback\_post的参数

##0x03 ARM64 传参

arm64的函数调用，寄存器使用x0-x7以及q0-q7传递参数，多余的参数会使用堆栈传递；函数返回时不仅使用x0，返回NSRange时x1也会被用到，我把origin\_objc_msgSend后的x0-x7都保存起来，q0-q7传递浮点数返回值；x8可能会传递一个内存块的指针，所以也需要保存。


##0x04 堆栈现场举例

````
NSString * s = [NSString stringWithFormat:@"hahaha-%d, %d, %d, %d, %d, %d, %d, %d\n", 1,2,3,4,5,6,7,8];

````

调用[NSString stringWithFormat:] 进入new\_objc_msgSend时实际堆栈的还原。进入函数时，栈顶位置0x3c0； 0x440-0x3c0之间是堆栈传参，需搬移到0x2b0-0x230为 origin\_objc\_msgSend而准备；0x2c0-0x3b0保存当前寄存器; \_\_hook\_callback\_pre返回后需要还原，origin\_objc\_msgSend返回后，需要还原x0\x1两个寄存器，然后保存origin\_objc\_msgSend返回结果，在\_\_hook\_callback\_post返回后再还原。


````

    
0x230     sp2    ->               0000
                                  ....
                                  0000
0x2b0     fp2  ->                 fp1
                                  lr1 (local_return)
0x2c0     sp1    ->               x0
                                  ....
                                  x13
                                  q0
                                  ....
                                  q7
0x3b0     fp1 = sp0-16 ->         fp0
                sp0-8  ->         lr                    
0x3c0     sp0 ->                  0000
                                  ....
                                  0000
0x440     fp0 ->                  last fp
                                  last lr 
````

##0x05 使用说明

或者重载OCTraceLogger类，实现trace()函数。

````
    OCTraceLocalLogger * logger = new OCTraceLocalLogger();
    
    OCTraceInit(logger);
    
````

修改OCTrace.cpp 中 s\_skip\_image\_names[] 数组，这个是白名单。

s\_skip\_class\_names[] 数组是类名的黑名单。