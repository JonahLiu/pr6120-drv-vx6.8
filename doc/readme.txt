�๦���豸vxWorks 6.8��������˵��   ��д���ڣ�2016/06/01
---------------------------------

��˵��������

* �����������ݸ�Ҫ
* ϵͳ����
* ��������ʹ��˵��
* ��������ʹ��˵��
* CAN����ʹ��˵��

������������
=================================

�ļ�Ŀ¼��
/
 |- can								CAN��������Ŀ¼
 |  |- 02wnCAN.cdf					CAN�豸��������ģ�鶨���ļ�
 |  |- 03wnCAN_PR6120_CAN.cdf		CAN�豸����ģ�鶨���ļ�
 |  |- canBoard.h					CAN�忨����ͷ�ļ�
 |  |- pr6120_can.c					CAN�豸�ײ�����
 |  |- pr6120_can.h					CAN�豸����ͷ�ļ�
 |  |- pr6120_can_cfg.c				CAN�豸�ϲ�����
 |  |- sys_pr6120_can.c				CAN�豸��ʼ��
 |  |- readme.txt					CAN������װ˵��
 |- nic								������������Ŀ¼
 |  |- gei825xxVxbEnd.c				������������
 |- sio								������������Ŀ¼
 |  |- vxbNs16550Sio.c				������������
 |- doc								�ĵ�Ŀ¼
    |- readme.txt					���ļ�

ϵͳ����
=================================

* VxWorks 6.8 ��Windriver Workbench 3.2
* Ŀ��ϵͳ���Ѱ�װ��ȷ��BSP��������ϵͳ
* ����������������Ҫ���ں������а���INCLUDE_GEI825XX_VXB_ENDѡ��
* ���ڴ�����������Ҫ���ں������а���DRV_SIO_NS16550ѡ��
* ����CAN��������Ҫ���ں������а���INCLUDE_WNCAN_DEVIO, INCLUDE_WNCAN_SHOW, INCLUDE_PCI_PR6120_CANѡ��

��������ʹ��˵��
=================================

������������VxWorks�ں��Դ���GEI���������޸ģ���ԭ�����Ļ��������������豸��ID�ͳ�ʼ��������

�������裺

* ��Wind River Workbench 3.2���½�һ��vxWorks Image Project
* ��gei825xxVxbEnd.c��ӵ�������
* ���������ں˾���
* ͨ��FTP���Ƶ�Ӳ�̵ȷ�ʽ�����������ɵ�VxWorks�ں�����
* ��VxWorks Shell�¿�ͨ��vxBusShow����鿴�����豸���豸��Ϊgei
* ��VxWorks C SHELL��ʹ��ipAttach�����������->ipAttach 0��"gei"
* ��VxWorks CMD SHELL��ʹ��ifconfig������������: #ifconfig gei0 inet 192.168.0.30
* ʹ��ifconfig��������������#ifconfig gei0 up
* ������������ʹ��

��������ʹ��˵��
=================================

������������VxWorks�ں��Դ���NS16550�����޸ģ����������豸��ID�ͳ�ʼ��������

�������裺

* ��Wind River Workbench 3.2���½�һ��vxWorks Image Project
* ��vxbNs16550Sio.c��ӵ�������
* ���������ں˾���
* ͨ��FTP���Ƶ�Ӳ�̵ȷ�ʽ�����������ɵ�VxWorks�ں�����
* ��VxWorks Shell�¿�ͨ��vxBusShow����鿴�����豸���豸��Ϊns16550
* ��VxWorks Shell�¿�ͨ��devs����鿴�豸�ļ��������豸�ļ���Ϊ/tyCo/X, XΪ���֡�
* ��ʹ�ñ�׼�ļ�IO��IOCTL��API���д��ڱ��
* ��ֱ����VxWorks C Shell��ʹ��open��read, write������в���

CAN����ʹ��˵��
=================================

CAN��������VxWorks 6.8�ṩ��WNCAN������ܱ�д������������ֻ�����豸��ѯ����ʼ���͵ײ���ʲ����Ķ��壬�����SJA1000������CANЭ�鲿���Լ��ϲ�API�ӿھ�ʹ��WNCAN�Դ���ʵ�֡�
�����һ���ǣ�WNCAN�������Ĭ��û�а������ں˿��У���Ҫ����ʹ���ں˱���ѡ���е�CAN���ò������ں�Դ����ܿ������API��

�������裺

* ��Wind River Workbench 3.2���½�һ��vxWorks Source Build (Kernel Library) Project
* �򿪹��̵�Source Build Configuration���ڣ���"VxWorks Applications Configuration Options"���ҵ�"Enable CAN"��ʹ��
* ���빤�̣�������������ɺ��ԣ������������⣬����ֻʹ��wncan��
* ��������Ŀ¼canĿ¼��readme.txt�ļ����������������ļ����Ƶ��ں�Ŀ¼�У�����ԭ�ļ�
* ��Wind River Workbench 3.2���½�һ��vxWorks Image Project, ��"Project Setup"�У�ѡ��"based on"ѡ��"a source build project"��"Project"ѡ��ղ��½���Դ�빤�̵�����
* ����Workbench��BUG��������ò�����ͼ�ν�����ʾ����Ҫ���������½������á��Ҽ������������ѡ��"Open Wind River VxWorks 6.8 Development Shell"���������н���
* ���룺vxprj component add INCLUDE_WNCAN_DEVIO INCLUDE_WNCAN_SHOW INCLUDE_PR6120_CAN
* ��pr6120_can.c��ӵ�������
* Rebuild Project, �����ں˾���
* ͨ��FTP���Ƶ�Ӳ�̵ȷ�ʽ�����������ɵ�VxWorks�ں�����
* ��VxWorks Shell�¿�ͨ��devs����鿴�豸�ļ���CAN�豸�ļ���Ϊ/can/X, XΪ���֡�
* ��ʹ��WNCAN֧�ֵ�����API���б�̣��������VxWorks�ĵ�
* Ϊ�˼򻯲���������ֲ���������Կ��ǲ����±����ں�Դ�룬���ǽ�����WNCAN��ش���ֱ�ӿ�������ǰ�����б��롣



