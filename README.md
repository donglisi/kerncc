# kerncc
研究Linux内核的Kbuild系统让我发现一件有趣的事，Linux内核的Makefile有一个名为parpare0的规则，这个规则执行后就生成了编译绝大多数c语言源码文件的所需的头文件，同时Linux内核的Makefile生成的编译器命令行非常容易用程序去解析，于是我开发了这个项目来缩短我编译内核的时间。<br/><br/>

这个程序支持用2台机一起编译Linux内核，kerncc是编译器的包装器，kernccd是接收编译任务的守护进程。<br/><br/>

原理：<br/>
1,一台机用来执行make正常构建内核，另一台机仅仅负责生成可重定向目标文件，源代码和.config需要在两台机的相同目录各拷贝一份，通过网络传输编译器命令行以及回传编译结果。<br/><br/>

2, 正常编译的那台机通过设置内核Makefile的参数CC来指定编译器为kerncc，还需要设置KERNCC_IP环境变量来指定kernccd的ip地址，kerncc默认会将PATH中的名为gcc的文件作为编译器，设置环境变量KERNCC_CC可以指定kerncc所使用的编译器。<br/>

3, 内核的Makefile有一个名为parpare0的规则，这个规则生成一些必要的头文件，在另一台机上执行make parpare0后就可以开始接收编译任务，当.config修改后需要重新执行这一步。<br/>

4, kerncc解析传给编译器的参数，如果是编译c语言源代码源文件的命令就允许分发到另一台机去编译，否则直接exec调用本机的编译器，是否分发到远程取决于文件的大小和一个随机数，小于一定的大小就直接在本机编译（默认是小于1000，通过设置KERNCC_SIZE环境变量可以修改这个值），随机数用来进行负载均衡（每个kerncc进程都会生成一个大小在0到100之间的随机数，默认是大于50就分发到远程编译，可以通过设置环境变量KERNCC_BALANCE来更改这个值）。<br/>

5, 另一台机编译成功后会把gcc生成的.o文件（可重定向目标文件）和.d文件（用来记录有那些依赖的头文件）回传到正常编译的那台机。<br/>

6, kerncc如果遇到连接不上kernccd或在另一台机上遇到编译报错或等待编译结果的过程中kernccd被异常终止等异常情况，就会在正常编译的这台机上重新执行一下同样的命令。<br/>

<br/>
特性：<br/>
1, 因为make的过程中会生成一些.h和.c文件，这些文件只存在于正常编译的那台机上，有些依赖这些文件的c源代码文件无法在另一台机上编译，另一台机遇到编译报错的情况时会把编译器的命令行参数打印出来，可以将这些c源代码文件的路径保存到正常编译的那台机的编译输出目录下的名为files的文件中，kerncc会读取这个files文件，如果匹配到无法远程编译的文件就直接在本地编译，节省了分发到远程编译的时间。<br/><br/>

2, 支持交叉编译其他体系结构的内核，比如要为aarch64架构编译内核，只需要在make前加上KERNCC_CC=/usr/bin/aarch64-linux-gnu-gcc就可以，完整的命令行类似这样：<br/>
KERNCC_CC=/usr/bin/aarch64-linux-gnu-gcc make -j56 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- CC=kerncc O=build
