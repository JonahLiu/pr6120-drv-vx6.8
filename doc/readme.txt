多功能设备vxWorks 6.8驱动程序说明   编写日期：2016/06/01
---------------------------------

本说明包括：

* 驱动程序内容概要
* 系统需求
* 网卡驱动使用说明
* 串口驱动使用说明
* CAN驱动使用说明

驱动程序内容
=================================

文件目录：
/
 |- can								CAN驱动程序目录
 |  |- 02wnCAN.cdf					CAN设备类型驱动模块定义文件
 |  |- 03wnCAN_PR6120_CAN.cdf		CAN设备驱动模块定义文件
 |  |- canBoard.h					CAN板卡定义头文件
 |  |- pr6120_can.c					CAN设备底层驱动
 |  |- pr6120_can.h					CAN设备驱动头文件
 |  |- pr6120_can_cfg.c				CAN设备上层驱动
 |  |- sys_pr6120_can.c				CAN设备初始化
 |  |- readme.txt					CAN驱动安装说明
 |- nic								网卡驱动程序目录
 |  |- gei825xxVxbEnd.c				网卡驱动程序
 |- sio								串口驱动程序目录
 |  |- vxbNs16550Sio.c				串口驱动程序
 |- doc								文档目录
    |- readme.txt					本文件

系统需求
=================================

* VxWorks 6.8 和Windriver Workbench 3.2
* 目标系统上已安装正确的BSP并能引导系统
* 对于网卡驱动，需要在内核配置中包含INCLUDE_GEI825XX_VXB_END选项
* 对于串口驱动，需要在内核配置中包含DRV_SIO_NS16550选项
* 对于CAN驱动，需要在内核配置中包含INCLUDE_WNCAN_DEVIO, INCLUDE_WNCAN_SHOW, INCLUDE_PCI_PR6120_CAN选项

网卡驱动使用说明
=================================

网卡驱动基于VxWorks内核自带的GEI网卡驱动修改，在原驱动的基础上增加了新设备的ID和初始化操作。

操作步骤：

* 在Wind River Workbench 3.2中新建一个vxWorks Image Project
* 将gei825xxVxbEnd.c添加到工程中
* 编译生成内核镜像
* 通过FTP或复制到硬盘等方式，引导新生成的VxWorks内核运行
* 在VxWorks Shell下可通过vxBusShow命令查看网卡设备，设备名为gei
* 在VxWorks C SHELL下使用ipAttach命令开启网卡：->ipAttach 0，"gei"
* 在VxWorks CMD SHELL下使用ifconfig命令配置网卡: #ifconfig gei0 inet 192.168.0.30
* 使用ifconfig命令启动网卡：#ifconfig gei0 up
* 网卡即可正常使用

串口驱动使用说明
=================================

串口驱动基于VxWorks内核自带的NS16550驱动修改，增加了新设备的ID和初始化操作。

操作步骤：

* 在Wind River Workbench 3.2中新建一个vxWorks Image Project
* 将vxbNs16550Sio.c添加到工程中
* 编译生成内核镜像
* 通过FTP或复制到硬盘等方式，引导新生成的VxWorks内核运行
* 在VxWorks Shell下可通过vxBusShow命令查看网卡设备，设备名为ns16550
* 在VxWorks Shell下可通过devs命令查看设备文件，串口设备文件名为/tyCo/X, X为数字。
* 可使用标准文件IO和IOCTL等API进行串口编程
* 可直接在VxWorks C Shell下使用open，read, write命令进行操作

CAN驱动使用说明
=================================

CAN驱动基于VxWorks 6.8提供的WNCAN驱动框架编写，驱动程序本身只包括设备查询，初始化和底层访问操作的定义，具体的SJA1000操作和CAN协议部分以及上层API接口均使用WNCAN自带的实现。
特殊的一点是，WNCAN驱动框架默认没有包含在内核库中，需要首先使能内核编译选项中的CAN设置并编译内核源码才能开启相关API。

操作步骤：

* 在Wind River Workbench 3.2中新建一个vxWorks Source Build (Kernel Library) Project
* 打开工程的Source Build Configuration窗口，在"VxWorks Applications Configuration Options"下找到"Enable CAN"并使能
* 编译工程，如个别库编译错误可忽略，属于设置问题，我们只使用wncan库
* 按照驱动目录can目录下readme.txt文件的描述，将驱动文件复制到内核目录中，覆盖原文件
* 在Wind River Workbench 3.2中新建一个vxWorks Image Project, 在"Project Setup"中，选项"based on"选择"a source build project"，"Project"选择刚才新建的源码工程的名称
* 由于Workbench的BUG，相关配置不会在图形界面显示，需要在命令行下进行配置。右键点击工程名，选择"Open Wind River VxWorks 6.8 Development Shell"，打开命令行界面
* 输入：vxprj component add INCLUDE_WNCAN_DEVIO INCLUDE_WNCAN_SHOW INCLUDE_PR6120_CAN
* 将pr6120_can.c添加到工程中
* Rebuild Project, 生成内核镜像
* 通过FTP或复制到硬盘等方式，引导新生成的VxWorks内核运行
* 在VxWorks Shell下可通过devs命令查看设备文件，CAN设备文件名为/can/X, X为数字。
* 可使用WNCAN支持的所有API进行编程，具体参照VxWorks文档
* 为了简化操作或者移植驱动，可以考虑不重新编译内核源码，而是将所有WNCAN相关代码直接拷贝到当前工程中编译。



