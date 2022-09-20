# kerncc
2台机一起编译Linux内核，kerncc是编译器的包装器，kernccd是接收编译任务的守护进程

<br/>
原理：<br/>
1, 一台机用来执行make正常构建内核，另一台机仅仅负责生成可重定向目标文件，源代码和.config需要在两台机的相同目录各拷贝一份，通过网络传输编译器命令行以及回传编译结果。<br/>

2, 正常编译的那台机通过传递make的参数CC来指定编译器为kerncc，kerncc默认会将PATH中的名为gcc的文件作为编译器。<br/>

3, 内核的Makefile有一个名为parpare0的规则，这个规则生成一些必要的头文件，在另一台机上执行make parpare0后就可以开始接收编译任务。<br/>

4, kerncc解析传给编译器的参数，如果是编译c语言源代码源文件的命令就允许分发到另一台机去编译，否则直接execve调用本机的编译器，是否分发到远程取决于文件的大小和一个随机数，小于一定的大小就直接在本机编译，用随机数来进行负载均衡。<br/>

5, 另一台机编译成功后会把gcc生成的.o文件（可重定向目标文件）和.d文件（用来记录有那些依赖的头文件）回传到正常编译的那台机。<br/>

6, 如果在另一台机上遇到编译报错，就会在正常编译的这台机上重新执行一下同样的命令。<br/>

<br/>
特性：<br/>
1, 在另一台机上之所以会出现编译报错的情况，是因为make的过程中会生成一些.h和.c文件，这些文件只存在于正常编译的那台机上，<br/>
可以将编译失败的文件统计出来存到正常编译的那台机的编译输出目录的名为files的文件中，kernccd会打印出编译失败的文件，很好统计。<br/>
2, kernccd支持同时接受编译不同版本，不同配置的内核的任务，只需要在make前手动在另一台机用相同的源代码和.config执行make prepare0。<br/>
3, 
