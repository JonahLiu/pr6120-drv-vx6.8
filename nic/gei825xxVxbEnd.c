/* gei825xxVxbEnd.c - Intel PRO/1000 VxBus END driver */

/*
 * Copyright (c) 2007-2010 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/*
modification history
--------------------
02n,15aug10,c_t  Add support for more 82576 devices, 82580 dual and quad port
                 MACs, and PCH integrated MACs. Also make sure to clear TBI
                 link reset bit when coming out of reset (WIND00187458),
                 fix 82574/82576 PHY probe problem(WIND00226662)
02o,10aug10,c_t  avoid allowing PHY SW reset or power down. (WIND00204423)
02n,02sep09,wap  Work around interrupt handling issues on simulated PRO/1000
                 devices in VMware and similar systems (WIND00182689)
02m,01sep09,wap  Correct multicast filter programming code (WIND00179790)
02l,14jul09,mdk  merge from networking branch
02k,03jun09,wap  Make sure RX DMA ring is totally drained
02j,26apr09,wap  Add support for 82574 on Apple Mac Pro and 82583, simplify
                 interrupt handling, disable/enable RX VLAN tag extraction
                 as needed in EIOCSIFCAP ioctl
02i,05mar09,wap  Make sure PAYLEN is specified correctly on big-endian
                 targets, set RX buffer size correctly when using jumbo frames
                 on advanced class devices, fix support for ICH8 devices.
01r,04dec08,dlk  Lessen 82573 RX ISR latency (WIND00146698). Also,
                 back out ISR/task synchronization changes for WIND00125287.
02h,27feb09,wap  Add support for 82576
02g,19feb09,wap  Work around EEPROM loading issue on vmware, add support for
                 ICH8, ICH9 and ICH10 devices.
02f,17nov08,dlk  If miiBusIdleErrorCheck() reports idle errors in
                 geiLinkUpdate(), mark the current link status inactive.
02e,02sep08,wap  Add code to work around intermittent autonegotiation
                 failures with some devices (82541, 82571, 82575)
                 (WIND00129165)
02d,20aug08,b_m  correct geiMiiPhyRead/geiMiiPhyWrite for tolapai.
02c,24jul08,wap  Remember to copy CSUM_IPHDR_OFFSET state to newly allocated
                 mBlk in geiEndSend(), set TDESC_RPS bit in TX descriptors
                 for 82543 and 82544 devices to avoid a TX stall under
                 high load
02b,16jul08,wap  Correct geiEndTbdClean() for advanced devices, remove
                 redundant code from geiEndAdvRxHandle(), add support
                 for Tolapai and ES2LAN devices
02a,10jul08,tor  update version
01z,03jul08,wap  Fix previous fix
01y,02jul08,dtr  Fix merge issue.
01x,26jun08,wap  Add support for 82574 and 82575 adapters, correct
                 interrupt handling code to avoid deadlock that can
                 occur when two GEI interfaces share the same
                 interrupt vector (WIND00125287)
01w,18jun08,pmr  resource documentation
01v,17jun08,wap  Set TXDCTL.22 to 1 for 82571EB/82572EI (WIND00119580)
01t,05mar08,dlk  Fix polled receive pre-invalidation error. Also, use
                 CSUM_IPHDR_OFFSET() to find offset to IP header in TX
		 checksum offload code.
01u,01may08,h_k  converted busCfgRead to vxbPciDevCfgRead.
01t,21mar08,wap  Correct 82544 PCI-X workaround test (WIND00117902), remember
                 to set GEI_CDESC_CMD_TCP bit in geiEndEncap() where
                 appropriate (WIND00118165)
01s,09jan08,dlk  Manually align descriptor memory from cacheDmaMalloc()
                 (WIND00114029)
01r,01nov07,dlk  Do not post separate jobs for RX and TX handling; call
                 receive and transmit cleanup handling routines directly
		 from geiEndIntHandle().
		 Check for done transmit descriptors by reading the DD bit,
		 unless GEI_READ_TDH is defined.
		 Do some transmit cleanup in the send routine.  When the
		 stack tries to send faster than the wire, set an occasional
		 TX descriptor to generate an immediate interrupt.
01q,25oct07,wap  Adjust interrupt coalescing timers to improve performance
01p,01oct07,wap  Increase reset timeout (WIND00105746)
01o,25sep07,tor  VXB_VERSION_3
01n,20sep07,dlk  In geiLinkUpdate(), do not call muxError() or muxTxRestart()
                 directly (WIND00100800). Also, support the EIOCGRCVJOBQ
		 ioctl.
01m,05sep07,ami  apigen related changes
01l,06aug07,wap  Enable hardware RX CRC stripping instead of doing it in
                 software (WIND00100630)
01k,30jul07,wap  Fix WIND00099929 (workarounds for 82544 errata)
01j,25jul07,wap  Fix WIND00099316 (typo, rxqueue -> rxQueue)
01i,05jul07,wap  Convert to new register access API
01h,13jun07,tor  remove VIRT_ADDR
01g,30apr07,dlk  Restrict bits in geiIntStatus to GEI_INTRS.
		 Do not set full M_BLK data lengths after endPoolTupleGet(),
		 which now does this itself.
01f,13apr07,dlk  SMP synchronization improvements.
01e,29mar07,tor  update methods
01d,15mar07,kch  Applied ipnet integratation changes - removed coreip headers 
                 that have been made obsolete.
01c,15mar07,wap  Wait longer for chip to come ready after reset or EEPROM
                 reload, avoid allocating jumbo size maps unless really
                 necessary, fix 82543 support
01b,09mar07,wap  Move vxbInstConnect() to later in driver initialization,
                 remove incorrect PCI ID for 82573_KCS device, add a couple
                 new PCI IDs, add workaround for RX stall on 2573 devices, add
                 support for fiber devices
01a,12feb07,wap  written
*/

/*
DESCRIPTION
This module implements a driver for the Intel PRO/1000 series of gigabit
ethernet controllers. The PRO/1000 family includes PCI, PCI-X, PCIe and
CSA adapters.

The PRO/1000 controllers implement all IEEE 802.3 receive and transmit 
MAC functions. They provide a Ten-Bit Interface (TBI) as specified in the IEEE 
802.3z standard for 1000Mb/s full-duplex operation with 1.25 GHz Ethernet 
transceivers (SERDES), as well as a GMII interface as specified in 
IEEE 802.3ab for 10/100/1000 BASE-T transceivers, and also an MII interface as 
specified in IEEE 802.3u for 10/100 BASE-T transceivers. 

Enhanced features available in the PRO/1000 family include TCP/IP checksum
offload for both IPv4 and IPv6, VLAN tag insertion and stripping, VLAN
tag filtering, TCP segmentation offload, interrupt coalescing, hardware
RMON statistics counters, 64-bit addressing and jumbo frames. This driver
makes use of the checksum offload, VLAN tag insertion/stripping
and jumbo frames features, as available.

Note that not all features are available on all devices. The 82543
does not support IPv4 RX checksum offload due to a hardware bug. IPv6
checksum offload is available on transmit for all adaprers, but only
available on receive with adapters newer than the 82544.

Currently, this driver supports the 82543, 82544, 82540, 82541, 82545,
82546, 82547, 82571, 82572, 82573, 82574, 82575, 82576, 82580, 82583,
Tolapai, and 631xESB/632xESB controllers with copper UTP and TBI multimode
fiber media only (some SERDES adapters have not been tested).

The 82566 and 82567 are PHYs used in conjunction with the PRO/1000 MACs
integrated into Intel ICH8/ICH9/ICH10/PCH chipsets. These devices are
functionally equivalent to other PCIe class PRO/1000 devices, except that
their station address is strored in flash instead of EEPROM, and some of
them use a default PHY address of 2 instead of 1.

The 82575 is a part of new generation of PRO/1000 devices. It supports
a new advanced DMA descriptor format for both RX and TX, four RX and
TX DMA queues, TSO, RSS, DCA and header splitting. It also supports
up to sixteen concurrent context descriptors, which can allow driver software
to pre-load TX offload configurations and avoid having to set them up
on the fly. The 82575 also supports per-queue interrupt moderation features.
This driver currently supports the TCP/IP checksum offload, VLAN
tag insertion/stripping and jumbo frame features.

The 82576 improves uppon the 82575, supporting 16 RX and TX queues, as
well as VMDq and SRIOV. The 82580 uses the same programming API as the
82576 but supports VMDq only.

BOARD LAYOUT
The PRO/1000 is available on standalone PCI, PCI-X and PCIe NICs as well
as integrated onto various system boards. All configurations are
jumperless.

EXTERNAL INTERFACE
The driver provides a vxBus external interface. The only exported
routine is the geiRegister() function, which registers the driver
with VxBus.

The PRO/1000 devices also support jumbo frames. This driver has
jumbo frame support, which is disabled by default in order to conserve
memory (jumbo frames require the use of a buffer pool with larger clusters).
Jumbo frames can be enabled on a per-interface basis using a parameter
override entry in the hwconf.c file in the BSP. For example, to enable
jumbo frame support for interface gei0, the following entry should be
added to the VXB_INST_PARAM_OVERRIDE table:

    { "gei", 0, "jumboEnable", VXB_PARAM_INT32, {(void *)1} }

INCLUDE FILES:
gei825xxVxbEnd.h geiTbiPhy.h end.h endLib.h netBufLib.h muxLib.h

SEE ALSO: vxBus, ifLib, endLib, netBufLib, miiBus

\tb "Intel(r) PCI/PCI-X Family of Gigabit Ethernet Controllers Software Developer's Manual http://download.intel.com/design/network/manuals/8254x_GBe_SDM.pdf"
\tb "Intel(r) PCIe GbE Controllers Open Source Software Developer's Manual http://download.intel.com/design/network/manuals/31608004.pdf"
\tb "Intel(r) 82544EI/82544GC Gigabit Ethernet Controller Specification Update http://download.intel.com/design/network/specupdt/82544_a4.pdf"
\tb "Intel(r) 82540EM Gigabit Ethernet Controller Specification Update http://download.intel.com/design/network/specupdt/82540em_a2.pdf"
\tb "Intel(r) 82545EM Gigabit Ethernet Controller Specification Update http://download.intel.com/design/network/specupdt/82545em.pdf"
\tb "Intel(r) 82573 Family Gigabit Ethernet Controllers Specification Update and Sighting Information http://download.intel.com/design/network/specupdt/82573.pdf"
\tb "Intel(r) 82574 GbE Controller Family Datasheet http://download.intel.com/design/network/datashts/82574.pdf"
\tb "Intel(r) 82576 Gigabit Ethernet Controller Datasheet http://download.intel.com/design/network/datashts/82576_Datasheet.pdf"
\tb "Intel(r) 82580 Quad/Dual Gigabit Ethernet LAN Controller Datasheet http://download.intel.com/design/network/datashts/321027.pdf"
\tb "Intel(r) 82583V GbE Controller Datasheet http://download.intel.com/design/network/datashts/322114.pdf"
*/
#define VENDOR_ID_PR6120_NIC	0x8086L
#define DEVICE_ID_PR6120_NIC	0xABCDL


#include <vxWorks.h>
#include <intLib.h>
#include <logLib.h>
#include <muxLib.h>
#include <netLib.h>
#include <netBufLib.h>
#include <semLib.h>
#include <sysLib.h>
#include <vxBusLib.h>
#include <wdLib.h>
#include <etherMultiLib.h>
#include <end.h>
#define END_MACROS
#include <endLib.h>
#include <vxAtomicLib.h>

#include <hwif/vxbus/vxBus.h>
#include <hwif/vxbus/hwConf.h>
#include <hwif/vxbus/vxbPciLib.h>
#ifdef GEI_VXB_DMA_BUF
#include <hwif/util/vxbDmaBufLib.h>
#else
#include <cacheLib.h>
#endif
#include <hwif/util/vxbParamSys.h>
#include <../src/hwif/h/mii/miiBus.h>
#include <../src/hwif/h/vxbus/vxbAccess.h>
#include <../src/hwif/h/hEnd/hEnd.h>

/* Need this for PCI config space register definitions */

#include <drv/pci/pciConfigLib.h>

#include <../src/hwif/h/end/gei825xxVxbEnd.h>
#include <../src/hwif/h/mii/mv88E1x11Phy.h>
#include <../src/hwif/h/mii/geiTbiPhy.h>

/* temporary */

LOCAL void geiDelay (UINT32);

IMPORT FUNCPTR _func_m2PollStatsIfPoll;
IMPORT void vxbUsDelay (int);

/* VxBus methods */

LOCAL void	geiInstInit (VXB_DEVICE_ID);
LOCAL void	geiInstInit2 (VXB_DEVICE_ID);
LOCAL void	geiInstConnect (VXB_DEVICE_ID);
LOCAL STATUS	geiInstUnlink (VXB_DEVICE_ID, void *);

/* miiBus methods */

LOCAL STATUS	geiPhyRead (VXB_DEVICE_ID, UINT8, UINT8, UINT16 *);
LOCAL STATUS    geiPhyWrite (VXB_DEVICE_ID, UINT8, UINT8, UINT16);
LOCAL STATUS    geiLinkUpdate (VXB_DEVICE_ID);

/* mux methods */

LOCAL void	geiMuxConnect (VXB_DEVICE_ID, void *);

LOCAL struct drvBusFuncs geiFuncs =
    {
    geiInstInit,	/* devInstanceInit */
    geiInstInit2,	/* devInstanceInit2 */
    geiInstConnect	/* devConnect */
    };

LOCAL struct vxbDeviceMethod geiMethods[] =
   {
   DEVMETHOD(miiRead,		geiPhyRead),
   DEVMETHOD(miiWrite,		geiPhyWrite),
   DEVMETHOD(miiMediaUpdate,	geiLinkUpdate),
   DEVMETHOD(muxDevConnect,	geiMuxConnect),
   DEVMETHOD(vxbDrvUnlink,	geiInstUnlink),
   { 0, 0 }
   };   

/*
 * List of supported device IDs.
 */

LOCAL struct vxbPciID geiPciDevIDList[] =
    {
        /* { devID, vendID } */
    	{ DEVICE_ID_PR6120_NIC, VENDOR_ID_PR6120_NIC },

        { INTEL_DEVICEID_82543GC_COPPER, INTEL_VENDORID },
        { INTEL_DEVICEID_82543GC_FIBER, INTEL_VENDORID },

        { INTEL_DEVICEID_82544EI_COPPER, INTEL_VENDORID },
        { INTEL_DEVICEID_82544GC_COPPER, INTEL_VENDORID },
        { INTEL_DEVICEID_82544GC_LOM, INTEL_VENDORID },
        { INTEL_DEVICEID_82544EI_FIBER, INTEL_VENDORID },

        { INTEL_DEVICEID_82540EM, INTEL_VENDORID },
        { INTEL_DEVICEID_82540EM2, INTEL_VENDORID },
        { INTEL_DEVICEID_82540EM_LOM, INTEL_VENDORID },
        { INTEL_DEVICEID_82540EP, INTEL_VENDORID },
        { INTEL_DEVICEID_82540EP_LP, INTEL_VENDORID },
        { INTEL_DEVICEID_82540EP_LOM, INTEL_VENDORID },

        { INTEL_DEVICEID_82541EI, INTEL_VENDORID },
        { INTEL_DEVICEID_82541ER_LOM, INTEL_VENDORID },
        { INTEL_DEVICEID_82541EI_MOBILE, INTEL_VENDORID },
        { INTEL_DEVICEID_82541GI, INTEL_VENDORID },
        { INTEL_DEVICEID_82541GI_MOBILE, INTEL_VENDORID },
        { INTEL_DEVICEID_82541ER, INTEL_VENDORID },
        { INTEL_DEVICEID_82541GI_LF, INTEL_VENDORID },

        { INTEL_DEVICEID_82545EM_COPPER, INTEL_VENDORID },
        { INTEL_DEVICEID_82545GM_COPPER, INTEL_VENDORID },
        { INTEL_DEVICEID_82545EM_FIBER, INTEL_VENDORID },
        { INTEL_DEVICEID_82545GM_FIBER, INTEL_VENDORID },
        { INTEL_DEVICEID_82545GM_SERDES, INTEL_VENDORID },

        { INTEL_DEVICEID_82546EB_COPPER, INTEL_VENDORID },
        { INTEL_DEVICEID_82546EB_FIBER, INTEL_VENDORID },
        { INTEL_DEVICEID_82546EB_QUAD_COPPER, INTEL_VENDORID },
        { INTEL_DEVICEID_82546GB_COPPER2, INTEL_VENDORID },
        { INTEL_DEVICEID_82546GB_COPPER, INTEL_VENDORID },
        { INTEL_DEVICEID_82546GB_FIBER, INTEL_VENDORID },
        { INTEL_DEVICEID_82546GB_SERDES, INTEL_VENDORID },
        { INTEL_DEVICEID_82546GB_PCIE, INTEL_VENDORID },
        { INTEL_DEVICEID_82546GB_QUAD_COPPER, INTEL_VENDORID },
        { INTEL_DEVICEID_82546GB_QUAD_COPPER_KSP3, INTEL_VENDORID },
        { INTEL_DEVICEID_82546GB_QUAD_COPPER_SRV, INTEL_VENDORID },

        { INTEL_DEVICEID_82547EI, INTEL_VENDORID },
        { INTEL_DEVICEID_82547EI_MOBILE, INTEL_VENDORID },
        { INTEL_DEVICEID_82547GI, INTEL_VENDORID },

        { INTEL_DEVICEID_80003ES2LAN_COPPER_DPT, INTEL_VENDORID },
        { INTEL_DEVICEID_80003ES2LAN_SERDES_DPT, INTEL_VENDORID },
        { INTEL_DEVICEID_80003ES2LAN_COPPER_DPT2, INTEL_VENDORID },
        { INTEL_DEVICEID_80003ES2LAN_SERDES_DPT2, INTEL_VENDORID },

        { INTEL_DEVICEID_82571EB_QUAD_COPPER, INTEL_VENDORID },
        { INTEL_DEVICEID_82571EB_QUAD_FIBER, INTEL_VENDORID },
        { INTEL_DEVICEID_82571EB_DUAL_COPPER, INTEL_VENDORID },
        { INTEL_DEVICEID_82571EB_FIBER, INTEL_VENDORID },
        { INTEL_DEVICEID_82571EB_SERDES, INTEL_VENDORID },
        { INTEL_DEVICEID_82571EB_QUAD_COPPER_LP, INTEL_VENDORID },
        { INTEL_DEVICEID_82571PT_QUAD, INTEL_VENDORID },
        { INTEL_DEVICEID_82571EB_MEZZ_DUAL, INTEL_VENDORID },
        { INTEL_DEVICEID_82571EB_MEZZ_QUAD, INTEL_VENDORID },

        { INTEL_DEVICEID_82572EI, INTEL_VENDORID },
        { INTEL_DEVICEID_82572EI_COPPER, INTEL_VENDORID },
        { INTEL_DEVICEID_82572EI_FIBER, INTEL_VENDORID },
        { INTEL_DEVICEID_82572EI_SERDES, INTEL_VENDORID },

        { INTEL_DEVICEID_82573V, INTEL_VENDORID },
        { INTEL_DEVICEID_82573E_IAMT, INTEL_VENDORID },
        { INTEL_DEVICEID_82573L, INTEL_VENDORID },
        { INTEL_DEVICEID_82573L_PL, INTEL_VENDORID },
        { INTEL_DEVICEID_82573V_PM, INTEL_VENDORID },
        { INTEL_DEVICEID_82573E_PM, INTEL_VENDORID },
        { INTEL_DEVICEID_82573L_PL2, INTEL_VENDORID },

        { INTEL_DEVICEID_82574L, INTEL_VENDORID },
        { INTEL_DEVICEID_82574L_MACPRO, INTEL_VENDORID },

        { INTEL_DEVICEID_82575EB_COPPER, INTEL_VENDORID },
        { INTEL_DEVICEID_82575EB_FIBER_SERDES, INTEL_VENDORID },
        { INTEL_DEVICEID_82575GB_QUAD_COPPER, INTEL_VENDORID },

        { INTEL_DEVICEID_TOLAPAI_REVB_ID1, INTEL_VENDORID },
        { INTEL_DEVICEID_TOLAPAI_REVC_ID1, INTEL_VENDORID },
        { INTEL_DEVICEID_TOLAPAI_REVB_ID2, INTEL_VENDORID },
        { INTEL_DEVICEID_TOLAPAI_REVC_ID2, INTEL_VENDORID },
        { INTEL_DEVICEID_TOLAPAI_REVB_ID3, INTEL_VENDORID },
        { INTEL_DEVICEID_TOLAPAI_REVC_ID3, INTEL_VENDORID },

        { INTEL_DEVICEID_ICH8_IGP_M_AMT, INTEL_VENDORID },
        { INTEL_DEVICEID_ICH8_IGP_AMT, INTEL_VENDORID },
        { INTEL_DEVICEID_ICH8_IGP_C, INTEL_VENDORID },
        { INTEL_DEVICEID_ICH8_IGP_M, INTEL_VENDORID },
        { INTEL_DEVICEID_ICH9_IGP_AMT, INTEL_VENDORID },
        { INTEL_DEVICEID_ICH9_IGP_C, INTEL_VENDORID },
        { INTEL_DEVICEID_ICH9_BM, INTEL_VENDORID },

        { INTEL_DEVICEID_82567LF, INTEL_VENDORID },
        { INTEL_DEVICEID_82567V, INTEL_VENDORID },
        { INTEL_DEVICEID_82567LM, INTEL_VENDORID },
        { INTEL_DEVICEID_82567V3, INTEL_VENDORID },
        { INTEL_DEVICEID_82567V4, INTEL_VENDORID },

        { INTEL_DEVICEID_ICH10_R_BM_LM, INTEL_VENDORID },
        { INTEL_DEVICEID_ICH10_R_BM_LF, INTEL_VENDORID },
        { INTEL_DEVICEID_ICH10_R_BM_V, INTEL_VENDORID },
        { INTEL_DEVICEID_ICH10_D_BM_LM, INTEL_VENDORID },
        { INTEL_DEVICEID_ICH10_D_BM_LF, INTEL_VENDORID },

        { INTEL_DEVICEID_PCH_M_HV_LM, INTEL_VENDORID },
        { INTEL_DEVICEID_PCH_M_HV_LC, INTEL_VENDORID },
        { INTEL_DEVICEID_PCH_D_HV_DM, INTEL_VENDORID },
        { INTEL_DEVICEID_PCH_D_HV_DC, INTEL_VENDORID },

        { INTEL_DEVICEID_82583, INTEL_VENDORID },

        { INTEL_DEVICEID_82576_COPPER, INTEL_VENDORID },
        { INTEL_DEVICEID_82576_SERDES, INTEL_VENDORID },
        { INTEL_DEVICEID_82576_QUAD_COPPER, INTEL_VENDORID },
        { INTEL_DEVICEID_82576_QUAD_SERDES, INTEL_VENDORID },
        { INTEL_DEVICEID_82576_NS, INTEL_VENDORID },
        { INTEL_DEVICEID_82576_NS_SERDES, INTEL_VENDORID },

        { INTEL_DEVICEID_82580_DUAL_COPPER, INTEL_VENDORID },
        { INTEL_DEVICEID_82580_FIBER, INTEL_VENDORID },
        { INTEL_DEVICEID_82580_BACKPLANE, INTEL_VENDORID },
        { INTEL_DEVICEID_82580_SFP, INTEL_VENDORID },
        { INTEL_DEVICEID_82580_QUAD_COPPER, INTEL_VENDORID }
    };

/* default queue parameters */
LOCAL HEND_RX_QUEUE_PARAM geiRxQueueDefault = {
    NULL,                       /* jobQueId */
    0,                          /* priority */
    0,                          /* rbdNum */
    0,                          /* rbdTupleRatio */
    0,                          /* rxBufSize */
    NULL,                       /* pBufMemBase */
    0,                          /* rxBufMemSize */
    0,                          /* rxBufMemAttributes */
    NULL,                       /* rxBufMemFreeMethod */
    NULL,                       /* pRxBdBase */
    0,                          /* rxBdMemSize */
    0,                          /* rxBdMemAttributes */
    NULL                        /* rxBdMemFreeMethod */
};

LOCAL HEND_TX_QUEUE_PARAM geiTxQueueDefault = {
    NULL,                       /* jobQueId */
    0,                          /* priority */
    0,                          /* tbdNum */
    0,                          /* allowedFrags */
    NULL,                       /* pTxBdBase */
    0,                          /* txBdMemSize */
    0,                          /* txBdMemAttributes */
    NULL                        /* txBdMemFreeMethod */
};

LOCAL VXB_PARAMETERS geiParamDefaults[] =
    {
       {"rxQueue00", VXB_PARAM_POINTER, {(void *)&geiRxQueueDefault}},
       {"txQueue00", VXB_PARAM_POINTER, {(void *)&geiTxQueueDefault}},
       {"jumboEnable", VXB_PARAM_INT32, {(void *)0}},
        {NULL, VXB_PARAM_END_OF_LIST, {NULL}}
    };

LOCAL struct vxbPciRegister geiDevPciRegistration =
    {
        {
        NULL,			/* pNext */
        VXB_DEVID_DEVICE,	/* devID */
        VXB_BUSID_PCI,		/* busID = PCI */
#ifndef VXB_VER_4_0_0
        VXBUS_VERSION_3,  	/* vxbVersion */
#else
        VXB_VER_4_0_0,  	/* vxbVersion */
#endif
        GEI_NAME,		/* drvName */
        &geiFuncs,		/* pDrvBusFuncs */
        geiMethods,		/* pMethods */
        NULL,			/* devProbe */
        geiParamDefaults	/* pParamDefaults */
        },
    NELEMENTS(geiPciDevIDList),
    geiPciDevIDList
    };

/* Driver utility functions */

LOCAL void	geiEeAddrSet (VXB_DEVICE_ID, UINT32);
LOCAL void	geiBitBangEeWordGet (VXB_DEVICE_ID, UINT32, UINT16 *);
LOCAL STATUS	geiPciEeWordGet (VXB_DEVICE_ID, UINT32, UINT16 *);
LOCAL STATUS	geiPciEEeWordGet (VXB_DEVICE_ID, UINT32, UINT16 *);
LOCAL STATUS	geiFlashWordGet (VXB_DEVICE_ID, UINT32, UINT16 *);
LOCAL void	geiEepromRead (VXB_DEVICE_ID, UINT8 *, int, int);
LOCAL STATUS	geiReset (VXB_DEVICE_ID);
LOCAL void	geiMiiSend (VXB_DEVICE_ID, UINT32, int);
LOCAL STATUS	geiBitBangPhyRead (VXB_DEVICE_ID, UINT8, UINT8, UINT16 *);
LOCAL STATUS	geiBitBangPhyWrite (VXB_DEVICE_ID, UINT8, UINT8, UINT16);
LOCAL STATUS	geiFiberPhyRead (VXB_DEVICE_ID, UINT8, UINT8, UINT16 *);
LOCAL STATUS	geiFiberPhyWrite (VXB_DEVICE_ID, UINT8, UINT8, UINT16);
#ifdef GEI_VXB_DMA_BUF
LOCAL GEI_TDESC * gei544PcixWar (GEI_DRV_CTRL *, VXB_DMA_MAP_ID, int);
#else
LOCAL GEI_TDESC * gei544PcixWar (GEI_DRV_CTRL * pDrvCtrl, M_BLK * pMblk);
#endif
LOCAL STATUS	geiKmrnRead (VXB_DEVICE_ID, UINT32, UINT16 *);
LOCAL STATUS	geiKmrnWrite (VXB_DEVICE_ID, UINT32, UINT16);
LOCAL STATUS	geiSwFlagAcquire (VXB_DEVICE_ID);
LOCAL STATUS	geiSwFlagRelease (VXB_DEVICE_ID);
LOCAL void	    geiSpdDisable (VXB_DEVICE_ID);

/* END functions */

LOCAL END_OBJ *	geiEndLoad (char *, void *);
LOCAL STATUS	geiEndUnload (END_OBJ *);
LOCAL int	geiEndIoctl (END_OBJ *, int, caddr_t);
LOCAL STATUS	geiEndMCastAddrAdd (END_OBJ *, char *);
LOCAL STATUS	geiEndMCastAddrDel (END_OBJ *, char *);
LOCAL STATUS	geiEndMCastAddrGet (END_OBJ *, MULTI_TABLE *);
LOCAL void	geiEndHashTblPopulate (GEI_DRV_CTRL *);
LOCAL STATUS	geiEndStatsDump (GEI_DRV_CTRL *);
LOCAL void	geiEndRxConfig (GEI_DRV_CTRL *);
LOCAL STATUS	geiEndStart (END_OBJ *);
LOCAL STATUS	geiEndStop (END_OBJ *);
LOCAL int	geiEndSend (END_OBJ *, M_BLK_ID);
LOCAL void	geiEndTbdClean (GEI_DRV_CTRL *);
LOCAL int	geiEndEncap (GEI_DRV_CTRL *, M_BLK_ID);
LOCAL int	geiEndAdvEncap (GEI_DRV_CTRL *, M_BLK_ID);
LOCAL STATUS	geiEndPollSend (END_OBJ *, M_BLK_ID);
LOCAL int	geiEndPollReceive (END_OBJ *, M_BLK_ID);
LOCAL int	geiEndAdvPollReceive (END_OBJ *, M_BLK_ID);
LOCAL void	geiEndInt (GEI_DRV_CTRL *);
LOCAL void	geiEndAltInt (GEI_DRV_CTRL *);
LOCAL int	geiEndAdvRxHandle (void *);
LOCAL int	geiEndRxHandle (void *);
LOCAL void	geiEndTxHandle (void *);
LOCAL void	geiEndIntHandle (void *);

LOCAL NET_FUNCS geiNetFuncs =
    {
    geiEndStart,                        /* start func. */
    geiEndStop,                         /* stop func. */
    geiEndUnload,                       /* unload func. */
    geiEndIoctl,                        /* ioctl func. */
    geiEndSend,                         /* send func. */
    geiEndMCastAddrAdd,                 /* multicast add func. */
    geiEndMCastAddrDel,                 /* multicast delete func. */
    geiEndMCastAddrGet,                 /* multicast get fun. */
    geiEndPollSend,                     /* polling send func. */
    geiEndPollReceive,                  /* polling receive func. */
    endEtherAddressForm,                /* put address info into a NET_BUFFER */
    endEtherPacketDataGet,              /* get pointer to data in NET_BUFFER */
    endEtherPacketAddrGet               /* Get packet addresses */
    };

/*****************************************************************************
*
* geiRegister - register with the VxBus subsystem
*
* This routine registers the gei driver with VxBus as a
* child of the PCI bus type.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

void geiRegister(void)
    {
    vxbDevRegister((struct vxbDevRegInfo *)&geiDevPciRegistration);
    return;
    }

/*****************************************************************************
*
* geiInstInit - VxBus instInit handler
*
* This function implements the VxBus instInit handler for an gei
* device instance. The only thing done here is to select a unit
* number for the device.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void geiInstInit
    (
    VXB_DEVICE_ID pDev
    )
    {
    vxbNextUnitGet (pDev);

    return;
    }

/*****************************************************************************
*
* geiInstInit2 - VxBus instInit2 handler
*
* This function implements the VxBus instInit2 handler for an gei
* device instance. Once we reach this stage of initialization, it's
* safe for us to allocate memory, so we can create our pDrvCtrl
* structure and do some initial hardware setup. The important
* steps we do here are to create a child miiBus instance, connect
* our ISR to our assigned interrupt vector, read the station
* address from the EEPROM, and set up our vxbDma tags and memory
* regions. We need to allocate a 64K region for the RX DMA window
* here.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void geiInstInit2
    (
    VXB_DEVICE_ID pDev
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;
    VXB_INST_PARAM_VALUE val;
    UINT8 tmp;
    int i, offset = 0;
#ifndef VXB_VER_4_0_0
    UINT32 flags;
#endif

    pDrvCtrl = malloc (sizeof(GEI_DRV_CTRL));
    bzero ((char *)pDrvCtrl, sizeof(GEI_DRV_CTRL));
    pDev->pDrvCtrl = pDrvCtrl;
    pDrvCtrl->geiDev = pDev;

    pDrvCtrl->geiDevSem = semMCreate (SEM_Q_PRIORITY|
        SEM_DELETE_SAFE|SEM_INVERSION_SAFE);

#ifndef VXB_VER_4_0_0
    /* Get the device type. */

    pDev->pAccess->busCfgRead(pDev, PCI_CFG_DEVICE_ID, 2,
        (char *)&pDrvCtrl->geiDevId, &flags);

    /* Get the revision code */

    pDev->pAccess->busCfgRead(pDev, PCI_CFG_REVISION, 1,
        (char *)&pDrvCtrl->geiRevId, &flags);
#else
    /* Get the device type. */

    VXB_PCI_BUS_CFG_READ (pDev, PCI_CFG_DEVICE_ID, 2, pDrvCtrl->geiDevId);

    /* Get the revision code */

    VXB_PCI_BUS_CFG_READ (pDev, PCI_CFG_REVISION, 1, pDrvCtrl->geiRevId);
#endif

    /* Assume PCIe device by default (will be modified as needed). */

    pDrvCtrl->geiDevType = GEI_DEVTYPE_PCIE;

    /* Assume 64 word EEPROM by default. */
  
    pDrvCtrl->geiEeWidth = 6;

    /* Also assume a PHY address of 1. May be overriden later. */

    pDrvCtrl->geiMiiPhyAddr = 1;

    /* Detect PCI-X devices. */

    if ((pDrvCtrl->geiDevId >= INTEL_DEVICEID_82543GC_FIBER &&
        pDrvCtrl->geiDevId <= INTEL_DEVICEID_82546GB_COPPER2) ||
        (pDrvCtrl->geiDevId >= INTEL_DEVICEID_82547GI &&
        pDrvCtrl->geiDevId <= INTEL_DEVICEID_82541GI_LF) ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82546GB_PCIE ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82546GB_QUAD_COPPER ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82546GB_QUAD_COPPER_SRV ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82546GB_QUAD_COPPER_KSP3)
        {
        pDrvCtrl->geiDevType = GEI_DEVTYPE_PCIX;
        pDrvCtrl->geiMiiPhyAddr = 1;
        }

    /*
     * Detect ICH/PCH devices
     *
     * Note: the ICH8 devices have PCI device IDs that fall between
     * INTEL_DEVICEID_82543GC_FIBER and INTEL_DEVICEID_82546GB_COPPER2,
     * which causes them to be falsely identified by the case above.
     * We rely on the code below to correct this so that they're
     * classified properly.
     */

    if (pDrvCtrl->geiDevId == INTEL_DEVICEID_ICH8_IGP_M_AMT ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_ICH8_IGP_AMT ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_ICH8_IGP_C ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_ICH8_IGP_M ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_ICH9_IGP_AMT ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_ICH9_IGP_C ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_ICH9_BM ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82567LF ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82567V ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82567LM ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82567V3 ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82567V4 ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_ICH10_R_BM_LM ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_ICH10_R_BM_LF ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_ICH10_R_BM_V ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_ICH10_D_BM_LM ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_ICH10_D_BM_LF ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_PCH_M_HV_LM ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_PCH_M_HV_LC ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_PCH_D_HV_DM ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_PCH_D_HV_DC)
        {
        pDrvCtrl->geiDevType = GEI_DEVTYPE_ICH;
        pDrvCtrl->geiMiiPhyAddr = 2;
        }

    /* For ICH8 and some ICH9 devices, PHY address is actually 1. */

    if (pDrvCtrl->geiDevId == INTEL_DEVICEID_ICH8_IGP_M_AMT ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_ICH8_IGP_AMT ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_ICH8_IGP_C ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_ICH8_IGP_M ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_ICH9_IGP_AMT ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_ICH9_IGP_C)
        pDrvCtrl->geiMiiPhyAddr = 1;

    /* Detect 631xESB/632xESB devices. */

    if (pDrvCtrl->geiDevId == INTEL_DEVICEID_80003ES2LAN_COPPER_DPT ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_80003ES2LAN_SERDES_DPT ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_80003ES2LAN_COPPER_DPT2 ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_80003ES2LAN_SERDES_DPT2)
        {
        pDrvCtrl->geiDevType = GEI_DEVTYPE_ES2LAN;
        pDrvCtrl->geiMiiPhyAddr = 1;
        }

    /* Detect Tolapai devices. */

    if (pDrvCtrl->geiDevId >= INTEL_DEVICEID_TOLAPAI_REVB_ID1 &&
        pDrvCtrl->geiDevId <= INTEL_DEVICEID_TOLAPAI_REVC_ID3)
        {
        pDrvCtrl->geiDevType = GEI_DEVTYPE_TOLAPAI;
        /* The tolapai uses a 256 word EEPROM, not a 64 word EEPROM. */
        pDrvCtrl->geiEeWidth = 8;
        if (pDrvCtrl->geiDevId == INTEL_DEVICEID_TOLAPAI_REVB_ID1 ||
            pDrvCtrl->geiDevId == INTEL_DEVICEID_TOLAPAI_REVC_ID1)
            {
            offset = TOLAPAI_MACADD_EEPROM_OFFSET_ID1;
            pDrvCtrl->geiMiiPhyAddr = 0;
            }
        if (pDrvCtrl->geiDevId == INTEL_DEVICEID_TOLAPAI_REVB_ID2 ||
            pDrvCtrl->geiDevId == INTEL_DEVICEID_TOLAPAI_REVC_ID2)
            {
            offset = TOLAPAI_MACADD_EEPROM_OFFSET_ID2;
            pDrvCtrl->geiMiiPhyAddr = 1;
            }
        if (pDrvCtrl->geiDevId == INTEL_DEVICEID_TOLAPAI_REVB_ID3 ||
            pDrvCtrl->geiDevId == INTEL_DEVICEID_TOLAPAI_REVC_ID3)
            {
            offset = TOLAPAI_MACADD_EEPROM_OFFSET_ID3;
            pDrvCtrl->geiMiiPhyAddr = 2;
            }
        }

    /* Detect 'advanced' devices. */

    if (pDrvCtrl->geiDevId == INTEL_DEVICEID_82575EB_COPPER ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82575EB_FIBER_SERDES ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82575GB_QUAD_COPPER ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82576_COPPER ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82576_SERDES ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82576_QUAD_COPPER ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82576_QUAD_SERDES ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82576_NS ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82576_NS_SERDES ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82580_DUAL_COPPER ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82580_FIBER ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82580_BACKPLANE ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82580_SFP ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82580_QUAD_COPPER)
        {
        pDrvCtrl->geiDevType = GEI_DEVTYPE_ADVANCED;
        pDrvCtrl->geiMiiPhyAddr = 1;
        }
    
    /* Detect PR6120 */
    
    if(pDrvCtrl->geiDevId == DEVICE_ID_PR6120_NIC)
    {
    	pDrvCtrl->geiDevType = 0;
    	pDrvCtrl->geiMiiPhyAddr = 1;
    }
     
    for (i = 0; i < VXB_MAXBARS; i++)
        {
        if (pDev->regBaseFlags[i] == VXB_REG_MEM)
            break;
        }

    pDrvCtrl->geiBar = pDev->pRegBase[i];
    vxbRegMap (pDev, i, &pDrvCtrl->geiHandle);

    /* ICH8/9/10/PCH have a second BAR for accessing the flash */

    if (pDrvCtrl->geiDevType == GEI_DEVTYPE_ICH)
        {
        pDrvCtrl->geiFlashBar = pDev->pRegBase[1];
        vxbRegMap (pDev, 1, &pDrvCtrl->geiFlashHandle);
        }

    /* Map the I/O BAR too */

    for (i = 0; i < VXB_MAXBARS; i++)
        {
        if (pDev->regBaseFlags[i] == VXB_REG_IO)
            break;
        }

    if (i != VXB_MAXBARS)
        {
        pDrvCtrl->geiIoBar = pDev->pRegBase[i];
        vxbRegMap (pDev, i, &pDrvCtrl->geiIoHandle);
        }

    if (pDrvCtrl->geiDevType == GEI_DEVTYPE_TOLAPAI)
        {
#ifndef VXB_VER_4_0_0
        pDev->pAccess->busCfgRead(pDev, PCI_SMIA_REG, 1, (char *)&tmp, &flags);
        tmp |= PCI_SMIA_ENABLE_INT0|PCI_SMIA_ENABLE_INT1;
        pDev->pAccess->busCfgWrite(pDev, PCI_SMIA_REG, 1, (char *)&tmp, &flags);
#else
        VXB_PCI_BUS_CFG_READ(pDev, PCI_SMIA_REG, 1, tmp);
        tmp |= PCI_SMIA_ENABLE_INT0|PCI_SMIA_ENABLE_INT1;
        VXB_PCI_BUS_CFG_WRITE(pDev, PCI_SMIA_REG, 1, tmp);
#endif
        }

    /*
     * Detect TBI mode
     * Note: ICH8 devices (or some of them anyway) report their
     * TBI/SERDES mode bits as set, even though they have copper
     * media. We need to filter those out.
     */

    if (CSR_READ_4(pDev, GEI_STS) & GEI_STS_TBIMODE &&
        (pDrvCtrl->geiDevId < INTEL_DEVICEID_ICH8_IGP_M_AMT ||
        pDrvCtrl->geiDevId > INTEL_DEVICEID_ICH8_IGP_AMT) &&
        pDrvCtrl->geiDevId != INTEL_DEVICEID_82567V3)
        pDrvCtrl->geiTbi = TRUE;
    
    /* Detect PR6120 */
    
    if(pDrvCtrl->geiDevId == DEVICE_ID_PR6120_NIC)
    {
    	pDrvCtrl->geiTbi = FALSE;
    }
    
    /* Reset the chip */

    geiReset (pDev);

    /* Get station address */

    geiEepromRead (pDev, pDrvCtrl->geiAddr, offset, 3);

    /*
     * Check the function ID code next. Multiport adapters
     * tend to share the same EEPROM, and hence will have
     * the same address. We adjust it by XORing the PCI
     * function ID code to the last octet. This is the
     * scheme used by Intel in its drivers.
     */

    if (pDrvCtrl->geiDevId == INTEL_DEVICEID_82580_DUAL_COPPER ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82580_QUAD_COPPER)
        pDrvCtrl->geiAddr[5] += GEI_FID(CSR_READ_4(pDev, GEI_STS));
    else
        {
        if (GEI_FID(CSR_READ_4(pDev, GEI_STS)))
            pDrvCtrl->geiAddr[5] ^= 1;
        }
    /*
     * See if the user wants jumbo frame support for this
     * interface. If the "jumboEnable" option isn't specified,
     * or it's set to 0, then jumbo support stays off,
     * otherwise we switch it on. Note: jumbo frames are not
     * supported on the 82573V and 82573E devices.
     */

    /*
     * paramDesc {
     * The jumboEnable parameter specifies whether
     * this instance should support jumbo frames.
     * The default is false. In some cases, support
     * for jumbo frames will remain off even if this
     * parameter is enabled, if the hardware is known
     * not to support jumbo frames. }
     */
    i = vxbInstParamByNameGet (pDev, "jumboEnable", VXB_PARAM_INT32, &val);

    if (i != OK || val.int32Val == 0 ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82573V ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82573E_IAMT ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82573V_PM ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82573E_PM)
        {
        i = GEI_CLSIZE;
        pDrvCtrl->geiMaxMtu = GEI_MTU;
        }
    else
        {
        i = END_JUMBO_CLSIZE;
        pDrvCtrl->geiMaxMtu = GEI_JUMBO_MTU;
        }

#ifdef GEI_VXB_DMA_BUF
    /* Get a reference to our parent tag. */

    pDrvCtrl->geiParentTag = vxbDmaBufTagParentGet (pDev, 0);

    /* Create tag for RX descriptor ring. */

    pDrvCtrl->geiRxDescTag = vxbDmaBufTagCreate (pDev,
        pDrvCtrl->geiParentTag,         /* parent */
        256 /*_CACHE_ALIGN_SIZE*/,              /* alignment */
        0,                              /* boundary */
        0xFFFFFFFF,                     /* lowaddr */
        0xFFFFFFFF,                     /* highaddr */
        NULL,                           /* filter */
        NULL,                           /* filterarg */
        sizeof(GEI_RDESC) * GEI_RX_DESC_CNT,      /* max size */
        1,                              /* nSegments */
        sizeof(GEI_RDESC) * GEI_RX_DESC_CNT,      /* max seg size */
        VXB_DMABUF_ALLOCNOW|VXB_DMABUF_NOCACHE,            /* flags */
        NULL,                           /* lockfunc */
        NULL,                           /* lockarg */
        NULL);                          /* ppDmaTag */

    pDrvCtrl->geiRxDescMem = vxbDmaBufMemAlloc (pDev,
        pDrvCtrl->geiRxDescTag, NULL, 0, &pDrvCtrl->geiRxDescMap);

    /* Create tag for TX descriptor ring. */

    pDrvCtrl->geiTxDescTag = vxbDmaBufTagCreate (pDev,
        pDrvCtrl->geiParentTag,       /* parent */
        256 /*_CACHE_ALIGN_SIZE*/,              /* alignment */
        0,                              /* boundary */
        0xFFFFFFFF,                     /* lowaddr */
        0xFFFFFFFF,                     /* highaddr */
        NULL,                           /* filter */
        NULL,                           /* filterarg */
        sizeof(GEI_TDESC) * GEI_TX_DESC_CNT,      /* max size */
        1,                              /* nSegments */
        sizeof(GEI_TDESC) * GEI_TX_DESC_CNT,      /* max seg size */
        VXB_DMABUF_ALLOCNOW|VXB_DMABUF_NOCACHE,            /* flags */
        NULL,                           /* lockfunc */
        NULL,                           /* lockarg */
        NULL);                          /* ppDmaTag */

    pDrvCtrl->geiTxDescMem = vxbDmaBufMemAlloc (pDev,
        pDrvCtrl->geiTxDescTag, NULL, 0, &pDrvCtrl->geiTxDescMap);

    /* Create tag for mBlk mappings */

    pDrvCtrl->geiMblkTag = vxbDmaBufTagCreate (pDev,
        pDrvCtrl->geiParentTag,       /* parent */
        1,                              /* alignment */
        0,                              /* boundary */
        0xFFFFFFFF,                     /* lowaddr */
        0xFFFFFFFF,                     /* highaddr */
        NULL,                           /* filter */
        NULL,                           /* filterarg */
        i,                              /* max size */
        GEI_MAXFRAG,                    /* nSegments */
        i,                              /* max seg size */
        VXB_DMABUF_ALLOCNOW,            /* flags */
        NULL,                           /* lockfunc */
        NULL,                           /* lockarg */
        NULL);                          /* ppDmaTag */

    for (i = 0; i < GEI_TX_DESC_CNT; i++)
        {
        if (vxbDmaBufMapCreate (pDev, pDrvCtrl->geiMblkTag, 0,
            &pDrvCtrl->geiTxMblkMap[i]) == NULL)
            logMsg("create Tx map %d failed\n", i, 0,0,0,0,0);
        }

    for (i = 0; i < GEI_RX_DESC_CNT; i++)
        {
        if (vxbDmaBufMapCreate (pDev, pDrvCtrl->geiMblkTag, 0,
            &pDrvCtrl->geiRxMblkMap[i]) == NULL)
            logMsg("create Rx map %d failed\n", i, 0,0,0,0,0);
        }
#else
    /*
     * Note, cacheDmaMalloc() is guaranteed to return a cache-aligned
     * buffer only when the cache is enabled (and it may not be in bootroms).
     * So we must do manual alignment.  The descriptor base address must
     * be at least 16-byte aligned, but the memory length must be a multiple 
     * of 128 for both RX and TX; so we might as well make the start of the
     * descriptor table 128-byte aligned anyway.
     *
     * We introduce geiDescBuf to hold the (possibly) unaligned
     * start address, for freeing.
     */

    if ((pDrvCtrl->geiDescBuf =
	 cacheDmaMalloc (sizeof(GEI_RDESC) * GEI_RX_DESC_CNT +
			 sizeof(GEI_TDESC) * GEI_TX_DESC_CNT + 128))
	== NULL)
	logMsg (GEI_NAME "%d: could not allocate descriptor memory\n",
		pDev->unitNumber, 0, 0, 0, 0, 0);

    pDrvCtrl->geiRxDescMem = (GEI_RDESC *)
	ROUND_UP (pDrvCtrl->geiDescBuf, 128);
    pDrvCtrl->geiTxDescMem = (GEI_TDESC *)
	(pDrvCtrl->geiRxDescMem + GEI_RX_DESC_CNT);

#endif /* GEI_VXB_DMA_BUF */

    pDrvCtrl->gei82544PcixWar = FALSE;

    /* Check for an 82544 device in PCI-X mode. */

    if ((pDrvCtrl->geiDevId >= INTEL_DEVICEID_82544EI_COPPER &&
        pDrvCtrl->geiDevId <= INTEL_DEVICEID_82544GC_LOM) &&
        CSR_READ_4(pDev, GEI_STS) & GEI_STS_PCIX_MODE)
        pDrvCtrl->gei82544PcixWar = TRUE;
    
    /* Detect PR6120 */
    
    if(pDrvCtrl->geiDevId == DEVICE_ID_PR6120_NIC)
    {
    	pDrvCtrl->gei82544PcixWar = FALSE;
    }
    
    return;
    }

/*****************************************************************************
*
* geiInstConnect -  VxBus instConnect handler
*
* This function implements the VxBus instConnect handler for an gei
* device instance.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void geiInstConnect
    (
    VXB_DEVICE_ID pDev
    )
    { 
    return;
    }

/*****************************************************************************
*
* geiInstUnlink -  VxBus unlink handler
*
* This function shuts down an gei device instance in response to an
* unlink event from VxBus. This may occur if our VxBus instance has
* been terminated, or if the gei driver has been unloaded. When an
* unlink event occurs, we must shut down and unload the END interface
* associated with this device instance and then release all the
* resources allocated during instance creation, such as vxbDma
* memory and maps, and interrupt handles. We also must destroy our
* child miiBus and PHY devices.
*
* RETURNS: OK if device was successfully destroyed, otherwise ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS geiInstUnlink
    (
    VXB_DEVICE_ID pDev,
    void * unused
    )
    { 
    GEI_DRV_CTRL * pDrvCtrl;
#ifdef GEI_VXB_DMA_BUF
    int i;
#endif

    pDrvCtrl = pDev->pDrvCtrl;

    /*
     * Stop the device and detach from the MUX.
     * Note: it's possible someone might try to delete
     * us after our vxBus instantiation has completed,
     * but before anyone has called our muxConnect method.
     * In this case, there'll be no MUX connection to
     * tear down, so we can skip this step.
     */

    if (pDrvCtrl->geiMuxDevCookie != NULL)
        {
        if (muxDevStop (pDrvCtrl->geiMuxDevCookie) != OK)
            return (ERROR);

        /* Detach from the MUX. */

        if (muxDevUnload (GEI_NAME, pDev->unitNumber) != OK)
            return (ERROR);
        }

    /* Release the memory we allocated for the DMA rings */
#ifdef GEI_VXB_DMA_BUF

    vxbDmaBufMemFree (pDrvCtrl->geiRxDescTag, pDrvCtrl->geiRxDescMem,
        pDrvCtrl->geiRxDescMap);

    vxbDmaBufMemFree (pDrvCtrl->geiTxDescTag, pDrvCtrl->geiTxDescMem,
        pDrvCtrl->geiTxDescMap);

    for (i = 0; i < GEI_RX_DESC_CNT; i++)
        vxbDmaBufMapDestroy (pDrvCtrl->geiMblkTag,
            pDrvCtrl->geiRxMblkMap[i]);

    for (i = 0; i < GEI_TX_DESC_CNT; i++)
        vxbDmaBufMapDestroy (pDrvCtrl->geiMblkTag,
            pDrvCtrl->geiTxMblkMap[i]);

    /* Destroy the tags. */

    vxbDmaBufTagDestroy (pDrvCtrl->geiRxDescTag);
    vxbDmaBufTagDestroy (pDrvCtrl->geiTxDescTag);
    vxbDmaBufTagDestroy (pDrvCtrl->geiMblkTag);
#else
    cacheDmaFree (pDrvCtrl->geiDescBuf);
#endif
    /* Disconnect the ISR. */

    vxbIntDisconnect (pDev, 0, pDrvCtrl->geiIntFunc, pDrvCtrl);

    /* Destroy our MII bus and child PHYs. */

    miiBusDelete (pDrvCtrl->geiMiiBus);

    semDelete (pDrvCtrl->geiDevSem);

    /* Destroy the adapter context. */
    free (pDrvCtrl);
    pDev->pDrvCtrl = NULL;

    /* Goodbye cruel world. */

    return (OK);
    }

/*****************************************************************************
*
* geiPhyRead - miiBus miiRead method
*
* This function implements an miiRead() method that allows PHYs
* on the miiBus to access our MII management registers. There are
* two different MDIO access mechanisms. For the 82543 adapters,
* we use bitbang I/O via software programmable data pins 2 and 3,
* which are accessible via the device control register. For the 82544
* and later adapters, an MDIO shortcut register is available instead.
*
* RETURNS: ERROR if invalid PHY addr or register is specified, else OK
*
* ERRNO: N/A
*/

LOCAL STATUS geiPhyRead
    (
    VXB_DEVICE_ID pDev,
    UINT8 phyAddr,
    UINT8 regAddr,
    UINT16 *dataVal
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;
    STATUS rval = ERROR;
    FUNCPTR miiRead;
    int i;

    pDrvCtrl = pDev->pDrvCtrl;

    if (pDrvCtrl->geiDevType == GEI_DEVTYPE_TOLAPAI)
        {
        if (pDrvCtrl->geiMiiDev == NULL ||
            phyAddr != pDrvCtrl->geiMiiPhyAddr)
            {
            *dataVal = 0xFFFF;
            return (ERROR);
            }
        miiRead = pDrvCtrl->geiMiiPhyRead;
        return (miiRead (pDrvCtrl->geiMiiDev, phyAddr, regAddr, dataVal));
        }

    if (pDrvCtrl->geiDevId == INTEL_DEVICEID_82543GC_COPPER)
        return (geiBitBangPhyRead (pDev, phyAddr, regAddr, dataVal));

    if (phyAddr != pDrvCtrl->geiMiiPhyAddr)
        {
        *dataVal = 0xFFFF;
        return (ERROR);
        }

    if (pDrvCtrl->geiTbi == TRUE)
        return (geiFiberPhyRead (pDev, phyAddr, regAddr, dataVal));

    semTake (pDrvCtrl->geiDevSem, WAIT_FOREVER);

    geiSwFlagAcquire (pDev);

    CSR_WRITE_4(pDev, GEI_MDIC, GEI_MDIO_PHYADDR(phyAddr) |
        GEI_MDIO_REGADDR(regAddr) | GEI_MDIO_READ);

    for (i = 0; i < GEI_TIMEOUT; i++)
        {
        geiDelay (500);
        if (CSR_READ_4(pDev, GEI_MDIC) & (GEI_MDIC_READY|GEI_MDIC_ERROR))
            break;
        }

    if (i == GEI_TIMEOUT || CSR_READ_4(pDev, GEI_MDIC) & GEI_MDIC_ERROR)
        *dataVal = 0xFFFF;
    else
        {
        *dataVal = CSR_READ_4(pDev, GEI_MDIC) & GEI_MDIC_DATA;
        rval = OK;
        }

    geiSwFlagRelease (pDev);

    semGive (pDrvCtrl->geiDevSem);

    return (rval);
    }

/*****************************************************************************
*
* geiPhyWrite - miiBus miiRead method
*
* This function implements an miiWrite() method that allows PHYs
* on the miiBus to access our MII management registers.
*
* RETURNS: ERROR if invalid PHY addr or register is specified, else OK
*
* ERRNO: N/A
*/

LOCAL STATUS geiPhyWrite
    (
    VXB_DEVICE_ID pDev,
    UINT8 phyAddr,
    UINT8 regAddr,
    UINT16 dataVal
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;
    VXB_DEVICE_ID pPhyDev;
    MII_DRV_CTRL * pPhy;
    MII_DRV_CTRL * pMii;
    STATUS rval = ERROR;
    FUNCPTR miiWrite;
    int i;

    pDrvCtrl = pDev->pDrvCtrl;

    if (pDrvCtrl->geiDevType == GEI_DEVTYPE_TOLAPAI)
        {
        if (pDrvCtrl->geiMiiDev == NULL ||
            pDrvCtrl->geiMiiPhyAddr != phyAddr)
            return (ERROR);
        miiWrite = pDrvCtrl->geiMiiPhyWrite;
        return (miiWrite (pDrvCtrl->geiMiiDev, phyAddr, regAddr, dataVal));
        }

    if (pDrvCtrl->geiDevId == INTEL_DEVICEID_82543GC_COPPER)
        return (geiBitBangPhyWrite (pDev, phyAddr, regAddr, dataVal));

    if (pDrvCtrl->geiTbi == TRUE)
        return (geiFiberPhyWrite (pDev, phyAddr, regAddr, dataVal));

    /*
     * If this device has an IGP PHY, avoid allowing a PHY reset
     * or power down sequence from the PHY driver. These PHYs
     * load certain internal register settings from the EEPROM
     * when a PHY reset is done via the MAC, but do *not* reload
     * those settings when a reset or power down is done via
     * the BMCR register in the PHY proper. If this internal setup
     * is not performed, the PHY will sometimes exhibit unstable
     * behavior when establishing a link.
     *
     * Note: we sneak a peek at the device ID values stashed in
     * the PHY device node to detect the PHY type, but that
     * node won't exist until after our MII bus has been fully
     * instantiated by miiBusCreate(). There is a chance we will
     * be called before that happens though, so we don't apply
     * the workaround until pDrvCtrl->geiMiiBus is properly set.
     */

    if (pDrvCtrl->geiMiiBus != NULL)
        {
        pMii = pDrvCtrl->geiMiiBus->pDrvCtrl;
        pPhyDev = pMii->miiPhyList[pDrvCtrl->geiMiiPhyAddr];
        pPhy = pPhyDev->pDrvCtrl;

        if (MII_OUI(pPhy->miiId1, pPhy->miiId2) == GEI_OUI_INTEL)
            {
            if (regAddr == MII_CTRL_REG &&
                (dataVal & (MII_CR_RESET | MII_CR_POWER_DOWN)))
                return (OK);
            }
        }

    semTake (pDrvCtrl->geiDevSem, WAIT_FOREVER);

    geiSwFlagAcquire (pDev);

    CSR_WRITE_4(pDev, GEI_MDIC, GEI_MDIO_PHYADDR(phyAddr) |
        GEI_MDIO_REGADDR(regAddr) | dataVal | GEI_MDIO_WRITE);

    for (i = 0; i < GEI_TIMEOUT; i++)
        {
        geiDelay (500);
        if (CSR_READ_4(pDev, GEI_MDIC) & GEI_MDIC_READY)
            break;
        }

    if (i == GEI_TIMEOUT || CSR_READ_4(pDev, GEI_MDIC) & GEI_MDIC_ERROR)
        rval = ERROR;
    else
        rval = OK;

    geiSwFlagRelease (pDev);

    semGive (pDrvCtrl->geiDevSem);

    return (rval);
    }

/*****************************************************************************
*
* geiSpdDisable - turn off auto power down feature
*
* Some of the Intel PRO/1000 devices have copper PHYs that include a power
* management feature that allows the PHY to automatically power down after
* a brief period when the cable is unplugged. In this state, the PHY does
* not send any signal pulses. When connected to a link partner that is
* sending signal pulses, the PHY will sense them and power up again. But
* if two Intel devices with the SPD feature enabled are connected
* back-to-back via crossover cable, and both PHYs are in the power down
* state, they won't be able to link to each other. (Both sides remain
* silent, neither side can detect the other is there.)
*
* The SPD feature can be selectively enabled by a bit in the EEPROM, and
* may not be set for all adapters, but it is for some, particularly 82571
* and 82572 cards. This routine will turn off the feature in Intel PHYs
* that support it.
*
* ERRNO: N/A
*
* RETURNS: N/A
*/

LOCAL void geiSpdDisable
    (
    VXB_DEVICE_ID pDev
    )
    {
    VXB_DEVICE_ID pPhyDev;
    GEI_DRV_CTRL * pDrvCtrl;
    MII_DRV_CTRL * pPhy;
    MII_DRV_CTRL * pMii;
    UINT16 miiVal;

    pDrvCtrl = pDev->pDrvCtrl;

    if (pDrvCtrl->geiMiiBus == NULL)
        return;

    pMii = pDrvCtrl->geiMiiBus->pDrvCtrl;
    pPhyDev = pMii->miiPhyList[pDrvCtrl->geiMiiPhyAddr];
    pPhy = pPhyDev->pDrvCtrl;

    /*
     * Intel uses the same PHY ID register values for both the
     * 82541 and 82571/82572 in order to denote that these are
     * Intel Gigabit PHYs (IGP), however the devices actually
     * have totally different register layouts. The SDP control
     * bit is in register 20 on the 82541 and 25 on the 82571.
     * We use the following logic: if the PHY is IGP, and we're
     * a 'PCIE' class device, then we must be an 82571/82572,
     * and we use register 25. If we're a 'PCIX' device, then
     * we must be an 82541, and we use register 20.
     */

    if (MII_OUI(pPhy->miiId1, pPhy->miiId2) == GEI_OUI_INTEL &&
        MII_MODEL(pPhy->miiId2) == GEI_MODEL_IGP)
        {
        switch (pDrvCtrl->geiDevType)
            {
            case GEI_DEVTYPE_PCIE:
                miiBusRead (pDrvCtrl->geiMiiBus, pDrvCtrl->geiMiiPhyAddr,
                    25, &miiVal);
                miiVal &= ~0x0001;
                miiBusWrite (pDrvCtrl->geiMiiBus, pDrvCtrl->geiMiiPhyAddr,
                    25, miiVal);
                break; 
            case GEI_DEVTYPE_PCIX:
                miiBusRead (pDrvCtrl->geiMiiBus, pDrvCtrl->geiMiiPhyAddr,
                    20, &miiVal);
                miiVal &= ~0x0020;
                miiBusWrite (pDrvCtrl->geiMiiBus, pDrvCtrl->geiMiiPhyAddr,
                    20, miiVal);
                break;
            default:
                break;
            }
        }

    return;
    }

/*****************************************************************************
*
* geiLinkUpdate - miiBus miiLinkUpdate method
*
* This function implements an miiLinkUpdate() method that allows
* miiBus to notify us of link state changes. This routine will be
* invoked by the miiMonitor task when it detects a change in link
* status. Normally, the miiMonitor task checks for link events every
* two seconds. However, we may be invoked sooner since the gei
* supports a link change interrupt. This allows us to repond to
* link events instantaneously.
*
* Once we determine the new link state, we will announce the change
* to any bound protocols via muxError(). We also update the ifSpeed
* fields in the MIB2 structures so that SNMP queries can detect the
* correct link speed.
*
* RETURNS: ERROR if obtaining the new media setting fails, else OK
*
* ERRNO: N/A
*/

LOCAL STATUS geiLinkUpdate
    (
    VXB_DEVICE_ID pDev
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;
    UINT32 oldStatus;

    if (pDev->pDrvCtrl == NULL)
        return (ERROR);

    pDrvCtrl = (GEI_DRV_CTRL *)pDev->pDrvCtrl;

    semTake (pDrvCtrl->geiDevSem, WAIT_FOREVER);

    if (pDrvCtrl->geiMiiBus == NULL)
        {
        semGive (pDrvCtrl->geiDevSem);
        return (ERROR);
        }

    oldStatus = pDrvCtrl->geiCurStatus;
    if (miiBusModeGet(pDrvCtrl->geiMiiBus,
        &pDrvCtrl->geiCurMedia, &pDrvCtrl->geiCurStatus) == ERROR)
        {
        semGive (pDrvCtrl->geiDevSem);
        return (ERROR);
        }

    CSR_CLRBIT_4(pDev, GEI_CTRL, GEI_CTRL_SPEED);

    /* Set speed to match PHY. */

    if (pDrvCtrl->geiDevId == INTEL_DEVICEID_82543GC_COPPER)
        {
        CSR_CLRBIT_4(pDev, GEI_RCTL, GEI_RCTL_SBP);
        CSR_SETBIT_4(pDev, GEI_RCTL, GEI_RCTL_SECRC);
        pDrvCtrl->geiTbiCompat = FALSE;
        }

    switch (IFM_SUBTYPE(pDrvCtrl->geiCurMedia))
        {
        case IFM_1000_T:
        case IFM_1000_SX:
            CSR_SETBIT_4(pDev, GEI_CTRL, GEI_CSPEED_1000);
            /*
             * If we're an 82543 copper card, and we're running at
             * 1000Mbps, enable the TBI compatibility workaround.
             */
            if (pDrvCtrl->geiDevId == INTEL_DEVICEID_82543GC_COPPER)
                {
                CSR_SETBIT_4(pDev, GEI_RCTL, GEI_RCTL_SBP);
                CSR_CLRBIT_4(pDev, GEI_RCTL, GEI_RCTL_SECRC);
                pDrvCtrl->geiTbiCompat = TRUE;
                }
            if (pDrvCtrl->geiDevType == GEI_DEVTYPE_ES2LAN)
                geiKmrnWrite (pDev, GEI_KMRN_HD_CTRL, GEI_KMRN_HD_CTRL_1000);
            pDrvCtrl->geiEndObj.mib2Tbl.ifSpeed = 1000000000;
            break;

        case IFM_100_TX:
            CSR_SETBIT_4(pDev, GEI_CTRL, GEI_CSPEED_100);
            if (pDrvCtrl->geiDevType == GEI_DEVTYPE_ES2LAN)
                geiKmrnWrite (pDev, GEI_KMRN_HD_CTRL, GEI_KMRN_HD_CTRL_10_100);
            pDrvCtrl->geiEndObj.mib2Tbl.ifSpeed = 100000000;
            break;

        case IFM_10_T:
            CSR_SETBIT_4(pDev, GEI_CTRL, GEI_CSPEED_10);
            if (pDrvCtrl->geiDevType == GEI_DEVTYPE_ES2LAN)
                geiKmrnWrite (pDev, GEI_KMRN_HD_CTRL, GEI_KMRN_HD_CTRL_10_100);
            pDrvCtrl->geiEndObj.mib2Tbl.ifSpeed = 10000000;
            break;

        default:
            break;
        }

    if (pDrvCtrl->geiEndObj.pMib2Tbl != NULL)
        pDrvCtrl->geiEndObj.pMib2Tbl->m2Data.mibIfTbl.ifSpeed =
            pDrvCtrl->geiEndObj.mib2Tbl.ifSpeed;

    /* Set duplex to match PHY. */

    if ((pDrvCtrl->geiCurMedia & IFM_GMASK) == IFM_FDX)
        CSR_SETBIT_4(pDev, GEI_CTRL, GEI_CTRL_FD);
    else
        CSR_CLRBIT_4(pDev, GEI_CTRL, GEI_CTRL_FD);

    if (!(pDrvCtrl->geiEndObj.flags & IFF_UP))
        {
        semGive (pDrvCtrl->geiDevSem);
        return (OK);
        }

    /* If status went from down to up, announce link up. */

    if (pDrvCtrl->geiCurStatus & IFM_ACTIVE && !(oldStatus & IFM_ACTIVE))
        {
#ifdef VXB_VER_4_0_0
        if ((pDrvCtrl->geiIntrs & GEI_LINKINTRS) &&
            miiBusIdleErrorCheck (pDrvCtrl->geiMiiBus) == ERROR)
            {
            /* Don't consider the link up yet... */
            pDrvCtrl->geiCurStatus &= ~IFM_ACTIVE;
            pDrvCtrl->geiIntrs &= ~GEI_LINKINTRS;
            semGive (pDrvCtrl->geiDevSem);
            return (OK);
            }
#endif
        jobQueueStdPost (pDrvCtrl->geiJobQueue, NET_TASK_QJOB_PRI,
            muxLinkUpNotify, &pDrvCtrl->geiEndObj,
            NULL, NULL, NULL, NULL);
        }

    /* If status went from up to down, announce link down. */

    else if (!(pDrvCtrl->geiCurStatus & IFM_ACTIVE) && oldStatus & IFM_ACTIVE)
        {
        jobQueueStdPost (pDrvCtrl->geiJobQueue, NET_TASK_QJOB_PRI,
            muxLinkDownNotify, &pDrvCtrl->geiEndObj,
            NULL, NULL, NULL, NULL);
        geiSpdDisable (pDev);
        }

    semGive (pDrvCtrl->geiDevSem);

    return (OK);
    }

/*****************************************************************************
*
* geiMuxConnect - muxConnect method handler
*
* This function handles muxConnect() events, which may be triggered
* manually or (more likely) by the bootstrap code. Most VxBus
* initialization occurs before the MUX has been fully initialized,
* so the usual muxDevLoad()/muxDevStart() sequence must be defered
* until the networking subsystem is ready. This routine will ultimately
* trigger a call to geiEndLoad() to create the END interface instance.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void geiMuxConnect
    (
    VXB_DEVICE_ID pDev,
    void * unused
    )
    {
    GEI_DRV_CTRL *pDrvCtrl;

    pDrvCtrl = pDev->pDrvCtrl;

    /* Create our MII bus. */

    if (pDrvCtrl->geiDevType == GEI_DEVTYPE_TOLAPAI)
        {
        pDrvCtrl->geiMiiDev = vxbInstByNameFind ("intelGcu", 0);
        pDrvCtrl->geiMiiPhyRead = vxbDevMethodGet (pDrvCtrl->geiMiiDev,
            (UINT32)&miiRead_desc);
        pDrvCtrl->geiMiiPhyWrite = vxbDevMethodGet (pDrvCtrl->geiMiiDev,
            (UINT32)&miiWrite_desc);
        }

    miiBusCreate (pDev, &pDrvCtrl->geiMiiBus);
    miiBusMediaListGet (pDrvCtrl->geiMiiBus, &pDrvCtrl->geiMediaList);

    /*
     * If setting the media mode fails, this is likely because the
     * PHY probe also failed. We can't operate without a child PHY
     * node (genericPhy or genTbiPhy), so if we detect this condition,
     * we abort the END intance creation, since the resulting END
     * interface won't work correctly without a PHY.
     */

    if (miiBusModeSet (pDrvCtrl->geiMiiBus,
        pDrvCtrl->geiMediaList->endMediaListDefault) != OK)
        return;

    /* Save the cookie. */

    pDrvCtrl->geiMuxDevCookie = muxDevLoad (pDev->unitNumber,
        geiEndLoad, "", TRUE, pDev);

    if (pDrvCtrl->geiMuxDevCookie != NULL)
        muxDevStart (pDrvCtrl->geiMuxDevCookie);

    if (_func_m2PollStatsIfPoll != NULL)
        endPollStatsInit (pDrvCtrl->geiMuxDevCookie,
            _func_m2PollStatsIfPoll);

    return;
    }

/*****************************************************************************
*
* geiEeAddrSet - select a word in the EEPROM
*
* This is a helper function used by geiBitBangEeWordGet(), which clocks a
* sequence of bits through to the serial EEPROM attached to the Intel
* chip which contains a read command and specifies what word we
* want to access. The format of the read command depends on the
* EEPROM: most devices have a 64 word EEPROM, but some have a 256
* word EEPROM which requires a different read command sequence.
* The 256 word variety is most often found on cardbus controllers,
* which need extra space to hold all their CIS data.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void geiEeAddrSet
    (
    VXB_DEVICE_ID pDev,
    UINT32 addr
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;
    UINT32 d;
    int i;

    pDrvCtrl = pDev->pDrvCtrl;

    d = addr | (GEI_9346_READ << pDrvCtrl->geiEeWidth);

    for (i = 1 << (pDrvCtrl->geiEeWidth + 3); i; i >>= 1)
        {
        if (d & i)
            CSR_SETBIT_4(pDev, GEI_EECD, GEI_EECD_DI);
        else
            CSR_CLRBIT_4(pDev, GEI_EECD, GEI_EECD_DI);
        geiDelay (50);
        CSR_SETBIT_4(pDev, GEI_EECD, GEI_EECD_SK);
        CSR_CLRBIT_4(pDev, GEI_EECD, GEI_EECD_SK);
        }

    return;
    }

/*****************************************************************************
*
* geiBitBangEeWordGet - read a word from the EEPROM using bitbang I/O
*
* This routine reads a single 16 bit word from a specified address
* within the EEPROM attached to the PRO/1000 controller and returns
* it to the caller. After clocking in a read command and the address
* we want to access, we read back the 16 bit datum stored there.
* This routine should only be used on the 82543 and 82544 adapters.
* Later adapters also support bitbang EEPROM access, but they
* require the use of the access request and grant bits, which are
* not used here.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void geiBitBangEeWordGet
    (
    VXB_DEVICE_ID pDev,
    UINT32 addr,
    UINT16 *dest
    )
    {
    int i;
    UINT16 word = 0;

    /*
     * Some devices require us to acquire access to the EEPROM
     * interface before we can do a read access. On devices that
     * don't support this (82543, 82544), this has no effect.
     */

    CSR_SETBIT_4(pDev, GEI_EECD, GEI_EECD_EE_REQ);

    for (i = 0; i < GEI_TIMEOUT; i++)
        if (CSR_READ_4(pDev, GEI_EECD) & GEI_EECD_EE_GNT)
            break;

    CSR_WRITE_4(pDev, GEI_EECD, GEI_EECD_EE_REQ|GEI_EECD_CS);

    geiEeAddrSet (pDev, addr);

    CSR_WRITE_4(pDev, GEI_EECD, GEI_EECD_EE_REQ|GEI_EECD_CS);

    for (i = 0x8000; i; i >>= 1)
        {
        CSR_SETBIT_4(pDev, GEI_EECD, GEI_EECD_SK);
        geiDelay (50);
        if (CSR_READ_4(pDev, GEI_EECD) & GEI_EECD_DO)
            word |= i;
        CSR_CLRBIT_4(pDev, GEI_EECD, GEI_EECD_SK);
        }

    CSR_WRITE_4(pDev, GEI_EECD, 0);

    *dest = word;
    return;
    }

/*****************************************************************************
*
* geiPciEeWordGet - read a word from the EEPROM for PCI/PCI-X devices
*
* This routine reads a single 16 bit word from a specified address
* within the EEPROM attached to the PRO/1000 controller and returns
* it to the caller. Unlike the bitbang access method, this routine
* uses the shortcut EERD register, using the PCI/PCI-X adapter layout.
* This routine should only be used on the 82540/1/5/6/7 adapters.
*
* RETURNS: ERROR if EEPROM access times out, otherwise OK
*
* ERRNO: N/A
*/

LOCAL STATUS geiPciEeWordGet
    (
    VXB_DEVICE_ID pDev,
    UINT32 addr,
    UINT16 *dest
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;
    int i;
    UINT32 tmp;

    pDrvCtrl = pDev->pDrvCtrl;

    if (pDrvCtrl->geiDevType == GEI_DEVTYPE_TOLAPAI)
        return (ERROR);

    CSR_WRITE_4(pDev, GEI_EERD, 0);
    tmp = CSR_READ_4(pDev, GEI_EERD);

    CSR_WRITE_4(pDev, GEI_EERD, GEI_EEADDR(addr) | GEI_EERD_START);

    for (i = 0; i < GEI_TIMEOUT; i++)
        {
        if (CSR_READ_4(pDev, GEI_EERD) & GEI_EERD_DONE)
            break;
        }

    if (i == GEI_TIMEOUT || CSR_READ_4(pDev, GEI_EERD) == tmp)
        {
        CSR_WRITE_4(pDev, GEI_EERD, GEI_EEADDR(0xFF) | GEI_EERD_START);
        for (i = 0; i < GEI_TIMEOUT; i++)
            CSR_READ_4(pDev, GEI_EERD);
        *dest = 0xFFFF;
        return (ERROR);
        }

    tmp = CSR_READ_4(pDev, GEI_EERD);
    *dest = GEI_EEDATA(tmp);

    return (OK);
    }

/*****************************************************************************
*
* geiPciEEeWordGet - read a word from the EEPROM for PCIe devices
*
* This routine reads a single 16 bit word from a specified address
* within the EEPROM attached to the PRO/1000 controller and returns
* it to the caller. Unlike the bitbang access method, this routine
* uses the shortcut EERD register, using the PCIe adapter layout.
* This routine should only be used on the 82571/2/3 adapters.
*
* RETURNS: ERROR if EEPROM access times out, otherwise OK
*
* ERRNO: N/A
*/

LOCAL STATUS geiPciEEeWordGet
    (
    VXB_DEVICE_ID pDev,
    UINT32 addr,
    UINT16 *dest
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;
    int i;
    UINT32 tmp;

    pDrvCtrl = pDev->pDrvCtrl;

    if (pDrvCtrl->geiDevType == GEI_DEVTYPE_TOLAPAI)
        return (ERROR);

    CSR_WRITE_4(pDev, GEI_EERD, 0);
    tmp = CSR_READ_4(pDev, GEI_EERD);

    CSR_WRITE_4(pDev, GEI_EERD, GEI_EEPCIEADDR(addr) | GEI_EERDPCIE_START);

    for (i = 0; i < GEI_TIMEOUT; i++)
        {
        if (CSR_READ_4(pDev, GEI_EERD) & GEI_EERDPCIE_DONE)
            break;
        }

    if (i == GEI_TIMEOUT || CSR_READ_4(pDev, GEI_EERD) == tmp)
        {
        CSR_WRITE_4(pDev, GEI_EERD, GEI_EEPCIEADDR(0xFF) | GEI_EERDPCIE_START);
        for (i = 0; i < GEI_TIMEOUT; i++)
            CSR_READ_4(pDev, GEI_EERD);
        *dest = 0xFFFF;
        return (ERROR);
        }

    *dest = GEI_EEPCIEDATA(CSR_READ_4(pDev, GEI_EERD));

    return (OK);
    }

/*****************************************************************************
*
* geiFlashWordGet - read a word from the flash on ICH devices
*
* This routine reads a single 16 bit word from a specified address
* within the flash attached to an ICH8/9 ethernet controller and returns
* it to the caller. ICH devices are embedded and don't have a separate
* EEPROM chip, relying on a flash device instead.
*
* RETURNS: ERROR if the access times out, otherwise OK
*
* ERRNO: N/A
*/

LOCAL STATUS geiFlashWordGet
    (
    VXB_DEVICE_ID pDev,
    UINT32 addr,
    UINT16 *dest
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;
    UINT16 sts;
    UINT32 offset, data = 0;
    int i;

    pDrvCtrl = pDev->pDrvCtrl;

    /* Clear any pending operation. */

    FL_SETBIT_2(pDev, GEI_FL_HFSSTS,
        GEI_FL_HFSSTS_FLCDONE|GEI_FL_HFSSTS_FLCERR);

    /* Calculate and set offset */

    offset = addr;
    offset <<= 1; /* Convert word offset to bytes */
    offset += (FL_READ_4(pDev, GEI_FL_GFP) & GEI_FL_GFP_BASE) *
        GEI_FL_SECTOR_SIZE;

    FL_WRITE_4(pDev, GEI_FL_FADDR, offset);

    /* Start cycle */

    FL_WRITE_2(pDev, GEI_FL_HFSCTL,
        GEI_FL_SIZE_WORD|GEI_FL_READ|GEI_FL_HFSCTL_FLCGO);

    for (i = 0; i < GEI_TIMEOUT; i++)
        {
        if (FL_READ_2(pDev, GEI_FL_HFSSTS) & GEI_FL_HFSSTS_FLCDONE)
            break;
        geiDelay (1000);
        }

    data = FL_READ_4 (pDev, GEI_FL_FDATA0);
    sts = FL_READ_2 (pDev, GEI_FL_HFSSTS);

    /* Clear status */

    FL_SETBIT_2(pDev, GEI_FL_HFSSTS,
        GEI_FL_HFSSTS_FLCDONE|GEI_FL_HFSSTS_FLCERR);

    if (i == GEI_TIMEOUT || sts & GEI_FL_HFSSTS_FLCERR)
        return (ERROR);

    *dest = data & 0xFFFF;

    return (OK);
    }

/*****************************************************************************
*
* geiEepromRead - read a sequence of words from the EEPROM
*
* This is the top-level EEPROM access function. It will read a
* sequence of words from the specified address into a supplied
* destination buffer. This is used mainly to read the station
* address.
*
* There are three available EEPROM access methods: direct 'bitbang' I/O
* via the EECD register, shortcut access via the EERD register, and
* shortcut with for large EEPROMs via EERD register. The bitbang method
* is available on all adapters, but is tricky to use with multiport cards.
* The EERD shortcut method for small EEPROMs is available on 82540/1/5/6/7
* adapters. The EERD shortcut method for large EEPROMS is available on
* the PCIe adapters such as the 82571/2/3. We attempt to use large
* shortcut EEPROM access first, followed by small shortcut access, and
* then use bitbang as a last resort.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void geiEepromRead
    (
    VXB_DEVICE_ID pDev,
    UINT8 *dest,
    int off,
    int cnt
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;
    int i;
    STATUS r;
    UINT16 word, *ptr;

    pDrvCtrl = pDev->pDrvCtrl;

    geiSwFlagAcquire (pDev);

    for (i = 0; i < cnt; i++)
        {
        if (pDrvCtrl->geiDevType == GEI_DEVTYPE_ICH)
            r = geiFlashWordGet (pDev, off + i, &word);
        else
            r = geiPciEEeWordGet (pDev, off + i, &word);
        if (r == ERROR)
            r = geiPciEeWordGet (pDev, off + i, &word);
        if (r == ERROR)
            geiBitBangEeWordGet (pDev, off + i, &word);
        ptr = (UINT16 *)(dest + (i * 2));
        *ptr = le16toh(word);
        }

    geiSwFlagRelease (pDev);

    return;
    }

/*****************************************************************************
*
* geiFiberPhyRead - miiBus miiRead method
*
* This function is provided to allow the miiBus subsystem to work with
* fiber optic cards, which don't have an MII-based PHY management
* scheme. The TXCW and RXCW registers are exposed to miiBus so that
* the geiTbiPhy driver can access them. This allows miiBus to send
* us LinkUpdate events on link changes.
*
* RETURNS: ERROR if invalid PHY addr or register is specified, else OK
*
* ERRNO: N/A
*/

LOCAL STATUS geiFiberPhyRead
    (
    VXB_DEVICE_ID pDev,
    UINT8 phyAddr,
    UINT8 regAddr,
    UINT16 *dataVal
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;
    STATUS rval = 0;
    UINT32 val;

    pDrvCtrl = pDev->pDrvCtrl;

    semTake (pDrvCtrl->geiDevSem, WAIT_FOREVER);

    switch (regAddr)
        {
        /*
         * Fake up the status register so that the miiBus
         * code thinks there's a PHY here. The only meaningful
         * info reported is the link status bit, but we throw in
         * the extended capabilities bit just so the returned
         * value is always non-zero.
         */
        case MII_STAT_REG:
            *dataVal = MII_SR_EXT_STS;
            if (CSR_READ_4(pDev, GEI_STS) & GEI_STS_LU)
                *dataVal |= MII_SR_LINK_STATUS;
            break;
        case MII_PHY_ID1_REG:
            *dataVal = GTBI_ID1;
            break;
        case MII_PHY_ID2_REG:
            *dataVal = GTBI_ID2;
            break;
        case GTBI_TXCFG:
            val = CSR_READ_4(pDev, GEI_TXCW);
            *dataVal = val & 0xFFFF;
            break;
        case GTBI_TXCTL:
            val = CSR_READ_4(pDev, GEI_TXCW);
            *dataVal = (val >> 16) & 0xFFFF;
            break;
        case GTBI_RXCFG:
            val = CSR_READ_4(pDev, GEI_RXCW);
            *dataVal = val & 0xFFFF;
            break;
        case GTBI_RXCTL:
            val = CSR_READ_4(pDev, GEI_RXCW);
            *dataVal = (val >> 16) & 0xFFFF;
            break;
        default:
            *dataVal = 0xFFFF;
            rval = ERROR;
            break;
        }

    semGive (pDrvCtrl->geiDevSem);

    return (rval);
    }

/*****************************************************************************
*
* geiFiberPhyWrite - miiBus miiRead method
*
* This function is provided to allow the miiBus subsystem to work with
* fiber optic cards, which don't have an MII-based PHY management
* scheme. The TXCW and RXCW registers are exposed to miiBus so that
* the geiTbiPhy driver can access them. This allows miiBus to send
* us LinkUpdate events on link changes.
*
* RETURNS: ERROR if invalid PHY addr or register is specified, else OK
*
* ERRNO: N/A
*/

LOCAL STATUS geiFiberPhyWrite
    (
    VXB_DEVICE_ID pDev,
    UINT8 phyAddr,
    UINT8 regAddr,
    UINT16 dataVal
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;
    UINT32 val;
    STATUS rval = OK;

    pDrvCtrl = pDev->pDrvCtrl;

    semTake (pDrvCtrl->geiDevSem, WAIT_FOREVER);

    switch (regAddr)
        {
        case GTBI_TXCFG:
            val = CSR_READ_4(pDev, GEI_TXCW);
            val &= 0xFFFF0000;
            val |= dataVal;
            CSR_WRITE_4(pDev, GEI_TXCW, val);
            break;
        case GTBI_TXCTL:
            val = CSR_READ_4(pDev, GEI_TXCW);
            val &= 0x0000FFFF;
            val |= dataVal << 16;
            CSR_WRITE_4(pDev, GEI_TXCW, val);
            break;
        case GTBI_RXCFG:
            val = CSR_READ_4(pDev, GEI_RXCW);
            val &= 0xFFFF0000;
            val |= dataVal;
            CSR_WRITE_4(pDev, GEI_RXCW, val);
            break;
        case GTBI_RXCTL:
            val = CSR_READ_4(pDev, GEI_RXCW);
            val &= 0x0000FFFF;
            val |= dataVal << 16;
            CSR_WRITE_4(pDev, GEI_RXCW, val);
            break;
        default:
            rval = ERROR;
            break;
        }

    semGive (pDrvCtrl->geiDevSem);

    return (rval);
    }

/*****************************************************************************
*
* geiMiiSend - send a series of bits through the MDIO interface
*
* This is a helper routine for the PHY read and write methods that clocks
* a series of bits from the host to the PHY via the MDIO interface.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void geiMiiSend
    (
    VXB_DEVICE_ID pDev,
    UINT32 bits,
    int len
    )
    {
    UINT32 i;

    for (i = (0x1 << (len - 1)); i; i >>= 1)
        {
        if (bits & i)
            CSR_SETBIT_4(pDev, GEI_CTRL, GEI_CTRL_MDIO);
        else
            CSR_CLRBIT_4(pDev, GEI_CTRL, GEI_CTRL_MDIO);
        CSR_SETBIT_4(pDev, GEI_CTRL, GEI_CTRL_MDC);
        CSR_CLRBIT_4(pDev, GEI_CTRL, GEI_CTRL_MDC);
        }

    return;
    }

/*****************************************************************************
*
* geiBitBangPhyRead - miiBus miiRead method
*
* This function implements an miiRead() method that allows PHYs
* on the miiBus to access MII management registers using bit-bang
* serial I/O. This is needed for 82543 adapters, which do not have
* a shortcut MDIO access mechanism.
*
* RETURNS: ERROR if invalid PHY addr or register is specified, else OK
*
* ERRNO: N/A
*/

LOCAL STATUS geiBitBangPhyRead
    (
    VXB_DEVICE_ID pDev,
    UINT8 phyAddr,
    UINT8 regAddr,
    UINT16 *dataVal
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;
    UINT16 val = 0;
    UINT32 i;
    UINT32 error;

    pDrvCtrl = pDev->pDrvCtrl;

    semTake (pDrvCtrl->geiDevSem, WAIT_FOREVER);

    /* Pull the clock pin low. */
    CSR_CLRBIT_4(pDev, GEI_CTRL, GEI_CTRL_MDC);

    /* Set clock/data direction to out */
    CSR_SETBIT_4(pDev, GEI_CTRL, GEI_CTRL_MDIO_DIR|GEI_CTRL_MDC_DIR);

    /* Toggle clock 32 times to send 32 high bits for preamble. */
    geiMiiSend (pDev, 0xFFFFFFFF, 32);

    /* Send start of frame. */
    geiMiiSend (pDev, MII_MF_ST, MII_MF_ST_LEN);

    /* Send read op command. */
    geiMiiSend (pDev, MII_MF_OP_RD, MII_MF_OP_LEN);

    /* Send PHY address. */
    geiMiiSend (pDev, phyAddr, MII_MF_ADDR_LEN);

    /* Send PHY register. */
    geiMiiSend (pDev, regAddr, MII_MF_REG_LEN);

    /* Turnaround: send a high impedance bit. */
    CSR_CLRBIT_4(pDev, GEI_CTRL, GEI_CTRL_MDIO_DIR);
    CSR_SETBIT_4(pDev, GEI_CTRL, GEI_CTRL_MDC);
    CSR_CLRBIT_4(pDev, GEI_CTRL, GEI_CTRL_MDC);

    /* Get back the error status */

    error = CSR_READ_4 (pDev, GEI_CTRL) & GEI_CTRL_MDIO;

    /* Now we read back the results. */
    for (i = 0x8000; i; i >>= 1)
        {
        CSR_SETBIT_4(pDev, GEI_CTRL, GEI_CTRL_MDC);
        CSR_CLRBIT_4(pDev, GEI_CTRL, GEI_CTRL_MDC);
        if (CSR_READ_4(pDev, GEI_CTRL) & GEI_CTRL_MDIO)
            val |= i;
        }

    /* Leave data in high impedance state. */
    CSR_CLRBIT_4 (pDev, GEI_CTRL, GEI_CTRL_MDIO_DIR|GEI_CTRL_MDC_DIR);

    semGive (pDrvCtrl->geiDevSem);

    if (error)
        {
        *dataVal = 0xFFFF;
        return (ERROR);
        }

    *dataVal = val;

    return (OK);
    }

/*****************************************************************************
*
* geiBitBangPhyWrite - miiBus miiRead method
*
* This function implements an miiWrite() method that allows PHYs
* on the miiBus to access our MII management registers.
*
* RETURNS: ERROR if invalid PHY addr or register is specified, else OK
*
* ERRNO: N/A
*/

LOCAL STATUS geiBitBangPhyWrite
    (
    VXB_DEVICE_ID pDev,
    UINT8 phyAddr,
    UINT8 regAddr,
    UINT16 dataVal
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;

    pDrvCtrl = pDev->pDrvCtrl;

    semTake (pDrvCtrl->geiDevSem, WAIT_FOREVER);

    /* Pull the clock pin low. */
    CSR_CLRBIT_4(pDev, GEI_CTRL, GEI_CTRL_MDC);

    /* Set clock/data direction to out */
    CSR_SETBIT_4(pDev, GEI_CTRL, GEI_CTRL_MDIO_DIR|GEI_CTRL_MDC_DIR);

    /* Toggle clock 32 times to send 32 high bits for preamble. */
    geiMiiSend (pDev, 0xFFFFFFFF, 32);

    /* Send start of frame. */
    geiMiiSend (pDev, MII_MF_ST, MII_MF_ST_LEN);

    /* Send read op command. */
    geiMiiSend (pDev, MII_MF_OP_WR, MII_MF_OP_LEN);

    /* Send PHY address. */
    geiMiiSend (pDev, phyAddr, MII_MF_ADDR_LEN);

    /* Send PHY register. */
    geiMiiSend (pDev, regAddr, MII_MF_REG_LEN);

    /* Turnaround: a 1 bit... */
    geiMiiSend (pDev, 1, 1);

    /* ...followed by a 0 bit. */
    geiMiiSend (pDev, 0, 1);

    /* Now send the data. */
    geiMiiSend (pDev, dataVal, MII_MF_DATA_LEN);

    /* Leave data in high impedance state. */
    CSR_CLRBIT_4 (pDev, GEI_CTRL, GEI_CTRL_MDIO_DIR|GEI_CTRL_MDC_DIR);

    semGive (pDrvCtrl->geiDevSem);

    return (OK);
    }

/*****************************************************************************
*
* geiKmrnRead - read a Kumeran (GLCI) register
*
* This routine reads a Kumeran/GLCI MAC to PHY register via the Kumeran
* control/status register.
*
* RETURNS: always OK
*
* ERRNO: N/A
*/

LOCAL STATUS geiKmrnRead
    (
    VXB_DEVICE_ID pDev,
    UINT32 addr,
    UINT16 * val
    )
    {
    CSR_WRITE_4(pDev, GEI_KUMCTLSTS, GEI_KUMCTLSTS_REN |
        GEI_KMRN_OFFSET(addr));
    geiDelay (50);
    *val = GEI_KMRN_VAL(CSR_READ_4(pDev, GEI_KUMCTLSTS));

    return (OK);
    }

/*****************************************************************************
*
* geiKmrnWrite - write a Kumeran (GLCI) register
*
* This routine writes a Kumeran/GLCI MAC to PHY register via the Kumeran
* control/status register.
*
* RETURNS: always OK
*
* ERRNO: N/A
*/

LOCAL STATUS geiKmrnWrite
    (
    VXB_DEVICE_ID pDev,
    UINT32 addr,
    UINT16 val
    )
    {
    CSR_WRITE_4(pDev, GEI_KUMCTLSTS,
        GEI_KMRN_OFFSET(addr)|GEI_KMRN_VAL(val));
    geiDelay (50);

    return (OK);
    }

/*****************************************************************************
*
* geiSwFlagAcquire - acquire software access flag
*
* This routine acquires the software semaphore flag in the SWSM register
* to indicate that the driver wants control of the EEPROM/FLASH or MDIO
* interfaces. On managed adapters, both driver software and NIC firmware
* are competing for access to these resources, so a semaphore is needed
* to avoid races.
*
* Note that this is only necessary on some adapters; on the ones where
* it is not, this routine is effectively a no-op.
*
* RETURNS: ERROR if acquiring the flag times out, otherwise OK
*
* ERRNO: N/A
*/

LOCAL STATUS geiSwFlagAcquire
    (
    VXB_DEVICE_ID pDev
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;
    int i;

    pDrvCtrl = pDev->pDrvCtrl;

    if (pDrvCtrl->geiDevType != GEI_DEVTYPE_ICH)
        return (OK);

    for (i = 0; i < 100; i++)
        {
        CSR_SETBIT_4(pDev, GEI_EXTCNF_CTRL, GEI_EXTCNF_MDIO_SW);
        if (CSR_READ_4(pDev, GEI_EXTCNF_CTRL) & GEI_EXTCNF_MDIO_SW)
            break;
        geiDelay (100);
        }

    if (i == 100)
        return (ERROR);

    return (OK);
    }

/*****************************************************************************
*
* geiSwFlagRelease - release software access flag
*
* This routine releases the software semaphore acquired by the
* geiSwFlagAcquire() routine. This informs the firmware that the driver
* software is done accessing the EEPROM/FLASH or MDIO resources.
*
* Note that this is only necessary on some adapters; on the ones where
* it is not, this routine is effectively a no-op.
*
* RETURNS: always OK
*
* ERRNO: N/A
*/

LOCAL STATUS geiSwFlagRelease
    (
    VXB_DEVICE_ID pDev
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;

    pDrvCtrl = pDev->pDrvCtrl;

    if (pDrvCtrl->geiDevType != GEI_DEVTYPE_ICH)
        return (OK);

    CSR_CLRBIT_4(pDev, GEI_EXTCNF_CTRL, GEI_EXTCNF_MDIO_SW);

    return (OK);
    }

/*****************************************************************************
*
* geiReset - reset the PRO/1000 controller
*
* This function issues a reset command to the controller and waits
* for it to complete. This routine is always used to place the
* controller into a known state prior to configuration.
*
* RETURNS: ERROR if the reset bit never clears, otherwise OK
*
* ERRNO: N/A
*/

LOCAL STATUS geiReset
    (
    VXB_DEVICE_ID pDev
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;
    UINT32 led;
    UINT16 sts;
    UINT32 ioBar;
    void * ioHandle;
    int i;

    pDrvCtrl = pDev->pDrvCtrl;

    CSR_WRITE_4(pDev, GEI_IMC, 0xFFFFFFFF);

    /*
     * Tolapai reference boards seem to use a Marvell PHY in
     * RGMII mode. We need to make sure the RGMII RX and TX delays
     * are enabled and to assert CRS on transmit.
     */

    if (pDrvCtrl->geiDevType == GEI_DEVTYPE_TOLAPAI)
        {
        geiPhyRead (pDev, pDrvCtrl->geiMiiPhyAddr, MV_CR, &sts);
        sts |= MV_CR_CRS_ON_TX;
        geiPhyWrite (pDev, pDrvCtrl->geiMiiPhyAddr, MV_CR, sts);

        geiPhyRead (pDev, pDrvCtrl->geiMiiPhyAddr, MV_ECR, &sts);
        sts |= MV_ECR_RGMII_RX_DELAY|MV_ECR_RGMII_TX_DELAY;
        geiPhyWrite (pDev, pDrvCtrl->geiMiiPhyAddr, MV_ECR, sts);
        }

    /* Must reset the PHY before resetting the MAC. */

    if (pDrvCtrl->geiDevId != INTEL_DEVICEID_82543GC_COPPER)
        {
        CSR_SETBIT_4(pDev, GEI_CTRL, GEI_CTRL_PHY_RST);
        geiDelay (10000);
        CSR_CLRBIT_4(pDev, GEI_CTRL, GEI_CTRL_PHY_RST);
        /* Wait for PHY to come ready. */
        for (i = 0; i < GEI_TIMEOUT; i++)
            {
            geiPhyRead (pDev, pDrvCtrl->geiMiiPhyAddr, 1, &sts);
            if (sts != 0xFFFF)
                break;
            }
        }

    /*
     * Workaround for erratum 4 for 82540EM and 82545EM devices.
     * The Intel 82540EM and 82545EM specification updates indicate
     * that MDI/MDIX autoselection does not work correctly in the A0
     * steppings of these devices (for copper NICs only). If left on,
     * the NIC will not reliably establish a link. These chips have a
     * Marvell 88E1011 PHY. As a workaround, we must set the MDI/MDIX
     * control bits in the PHY config register to turn auto MDI/MDIX off.
     * This means that these NICs can't do automatic pair swapping and
     * must be used with crossover cables in some scenarios. For the
     * most common configurations however, i.e. where the NIC is plugged
     * into a switch or hub with a normal patch cable, no special steps
     * are needed.
     *
     * After setting the bits in the control register, a soft reset must
     * be issued to the PHY for the change to take effect. Once set, the
     * MDI/MDIX configuration will survive future resets, so this only
     * really needs to be done once after the device is powered up. We
     * don't need to issue a reset here, since the PHY driver will do
     * that for us later anyway.
     *
     * This is fixed in the A1 stepping of these devices.
     */

    if ((pDrvCtrl->geiDevId == INTEL_DEVICEID_82545EM_COPPER ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82540EM ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82540EM_LOM) &&
        pDrvCtrl->geiRevId == 0)
        {
        geiPhyRead (pDev, pDrvCtrl->geiMiiPhyAddr, MV_CR, &sts);
        geiPhyWrite (pDev, pDrvCtrl->geiMiiPhyAddr, MV_CR, sts & ~MV_CR_MDIX);
        }

    /* For 82573 adapters, aquire MDIO ownership prior to reset. */

    if (pDrvCtrl->geiDevId == INTEL_DEVICEID_82573V ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82573E_IAMT ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82573L ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82573L_PL ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82573V_PM ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82573E_PM ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82573L_PL2)
        {
        for (i = 0; i < GEI_TIMEOUT; i++)
            {
            CSR_SETBIT_4(pDev, GEI_EXTCNF_CTRL, GEI_EXTCNF_MDIO_SW);
            if (CSR_READ_4(pDev, GEI_EXTCNF_CTRL) & GEI_EXTCNF_MDIO_SW)
                break;
            }
        }

    /*
     * The following code works around a potential bus hang on PCIe
     * adapters. If a TLP is sent during reset, it will not be ACKed
     * and the bus may hang. So disable generation of master requests
     * before issuing a reset.
     */

    if (pDrvCtrl->geiDevType == GEI_DEVTYPE_PCIE ||
        pDrvCtrl->geiDevType == GEI_DEVTYPE_ICH ||
        pDrvCtrl->geiDevType == GEI_DEVTYPE_ADVANCED)
        {
        CSR_SETBIT_4(pDev, GEI_CTRL, GEI_CTRL_GIO_MSTR_DIS);
        for (i = 0; i < GEI_TIMEOUT; i++)
            {
            if (!(CSR_READ_4(pDev, GEI_STS) & GEI_STS_GIO_MSTR_STS))
                break;
            geiDelay (100);
            }
        }

    /*
     * Apply reset workarounds. For 82545 rev 3 and 82546 rev 3
     * devices, use the shadow control register to issue the
     * reset. For other PCI-X devices, use the IO space BAR
     * to issue the reset. For all others, use the normal
     * mechanism.
     */

    if (pDrvCtrl->geiDevId == INTEL_DEVICEID_82545GM_COPPER ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82545GM_FIBER ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82545GM_SERDES ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82546GB_COPPER ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82546GB_FIBER ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82546GB_SERDES ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82546GB_PCIE ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82546GB_QUAD_COPPER ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82546GB_QUAD_COPPER_KSP3)
        CSR_SETBIT_4(pDev, GEI_CTRL_DUP, GEI_CTRL_RST);
    else if (pDrvCtrl->geiDevType == GEI_DEVTYPE_PCIX)
        {
        if (pDrvCtrl->geiIoBar == NULL)
            goto memReset;

        ioBar = (UINT32)pDrvCtrl->geiIoBar;
        ioHandle = pDrvCtrl->geiIoHandle;

        led = CSR_READ_4(pDev, GEI_CTRL);
        vxbWrite32 (ioHandle, (UINT32 *)(ioBar), GEI_CTRL);
        vxbWrite32 (ioHandle, (UINT32 *)(ioBar + 4), led | GEI_CTRL_RST);
        }
   else
       {
memReset:
       CSR_SETBIT_4(pDev, GEI_CTRL, GEI_CTRL_RST);
       }

    geiDelay (50000);

    for (i = 0; i < GEI_TIMEOUT; i++)
        {
        geiDelay (1);
        if (!(CSR_READ_4(pDev, GEI_CTRL) & GEI_CTRL_RST))
            break;
        }

    if (i == GEI_TIMEOUT)
        {
        logMsg("%s%d: reset timed out\n", (int)GEI_NAME,
            pDev->unitNumber, 0, 0, 0, 0);
        return (ERROR);
        }

    /* Force an EEPROM reload */

    CSR_SETBIT_4(pDev, GEI_CTRLEXT, GEI_CTRLX_EE_RST);

    geiDelay (50000);

    for (i = 0; i < GEI_TIMEOUT; i++)
        {
        if (!(CSR_READ_4(pDev, GEI_CTRLEXT) & GEI_CTRLX_EE_RST))
            break;
        }

    if (i == GEI_TIMEOUT)
        {
        logMsg("%s%d: EEPROM reload timed out\n", (int)GEI_NAME,
            pDev->unitNumber, 0, 0, 0, 0);
        return (ERROR);
        }

    /*
     * Once we come out of reset, interrupts may be enabled
     * again -- force them off again here to keep them masked.
     */

    CSR_WRITE_4(pDev, GEI_IMC, 0xFFFFFFFF);

    /* Set LEDs for 82541 and 82547 controllers */

    if (pDrvCtrl->geiDevId == INTEL_DEVICEID_82541EI ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82541EI_MOBILE ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82541ER_LOM ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82547EI ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82547EI_MOBILE)
        {
        led = CSR_READ_4(pDev, GEI_LEDCTL);
        led &= ~GEI_LEDCTL_LED1MODE;
        led |= GEI_LED3MODE(GEI_LEDMODE_LINK_1000) |
            GEI_LED1MODE(GEI_LEDMODE_ACTIVITY);
        CSR_WRITE_4(pDev, GEI_LEDCTL, led);
        }

    CSR_SETBIT_4(pDev, GEI_CTRL,
        GEI_CTRL_SLU|GEI_CTRL_FRCSPD|GEI_CTRL_FRCDPX);

    /*
     * On 82543 copper adapters, the PHY reset pin is
     * tied to software programmable pin 4, which is
     * accessed via the extended control register. For
     * The others, we use the PHY_RST bit in the control
     * register. Note that we must pause for at least 10ms
     * after the PHY is brought out of reset in order to
     * give it enough time to come ready.
     */

    if (pDrvCtrl->geiDevId == INTEL_DEVICEID_82543GC_COPPER)
        {
        CSR_SETBIT_4(pDev, GEI_CTRLEXT, GEI_CTRLX_SDP4_DIR);
        CSR_CLRBIT_4(pDev, GEI_CTRLEXT, GEI_CTRLX_SDP4_DATA);
        geiDelay (10000);
        CSR_SETBIT_4(pDev, GEI_CTRLEXT, GEI_CTRLX_SDP4_DATA);
        geiDelay (10000);
        /* Wait for PHY to come ready. */
        for (i = 0; i < GEI_TIMEOUT; i++)
            {
            geiPhyRead (pDev, pDrvCtrl->geiMiiPhyAddr, 1, &sts);
            if (sts != 0xFFFF)
                break;
            }
        }

    /*
     * Disable dynamic DMA clock gating for PCIe devices.
     * This is a workaround for erratum #15 described in the
     * Intel 82573 Specification Update: "The 82573E Gigabit
     * Ethernet Controller may hang if the DMA clock is stopped
     * by the D0a Dynamic DMA Clock Gating feature." Has no
     * effect on other devices.
     */

    CSR_CLRBIT_4(pDev, GEI_CTRLEXT, GEI_CTRLX_DMA_DYNGATE);

    if (pDrvCtrl->geiDevType == GEI_DEVTYPE_ES2LAN)
        {

        /*
         * Apply a workaround to prevent PHY polling from
         * triggering spurious timeouts in 10Mbps mode.
         */

        geiKmrnWrite (pDev, GEI_KMRN_PHYPOLL_TIMER, 0xFFFF);
        geiKmrnRead (pDev, GEI_KMRN_PHYPOLL_COUNT, &sts);
        sts |= 0x3F;
        geiKmrnWrite (pDev, GEI_KMRN_PHYPOLL_COUNT, sts);

        /*
         * Disable padding in Kumeran interface in the MAC and PHY
         * to avoid CRC errors.
         */

        geiKmrnRead (pDev, GEI_KMRN_INB_CTRL, &sts);
        sts |= GEI_KMRN_INB_CTRL_DIS_PADDING |
            GEI_KMRN_INB_CTRL_LINK_STS_TX_TIMEOUT;
        geiKmrnWrite (pDev, GEI_KMRN_INB_CTRL, sts);

        /* Bypass RX and TX FIFOs. */

        geiKmrnRead (pDev, GEI_KMRN_FIFO_CTRL, &sts);
        sts |= GEI_KMRN_FIFO_CTRL_RX_BYPASS|GEI_KMRN_FIFO_CTRL_TX_BYPASS;
        geiKmrnWrite (pDev, GEI_KMRN_FIFO_CTRL, sts);
        }

    /*
     * If this is a TBI/fiber device, make sure the TBI link reset
     * bit in the control register is clear. Not every fiber card
     * requires this, but it's harmless to do it on the ones that
     * don't. Without this, cards that come out of reset with this
     * bit set will fail to negotiate a link.
     */

    if (pDrvCtrl->geiTbi == TRUE)
        CSR_CLRBIT_4(pDev, GEI_CTRL, GEI_CTRL_LRST);

    return (OK);
    }

/*****************************************************************************
*
* geiEndLoad - END driver entry point
*
* This routine initializes the END interface instance associated
* with this device. In traditional END drivers, this function is
* the only public interface, and it's typically invoked by a BSP
* driver configuration stub. With VxBus, the BSP stub code is no
* longer needed, and this function is now invoked automatically
* whenever this driver's muxConnect() method is called.
*
* For older END drivers, the load string would contain various
* configuration parameters, but with VxBus this use is deprecated.
* The load string should just be an empty string. The second
* argument should be a pointer to the VxBus device instance
* associated with this device. Like older END drivers, this routine
* will still return the device name if the init string is empty,
* since this behavior is still expected by the MUX. The MUX will
* invoke this function twice: once to obtain the device name,
* and then again to create the actual END_OBJ instance.
*
* When this function is called the second time, it will initialize
* the END object, perform MIB2 setup, allocate a buffer pool, and
* initialize the supported END capabilities. We support RX checksum
* offload capabilities as well as VLAN tag insertion and stripping
* and (optionally) jumbo frames.
*
* RETURNS: An END object pointer, or NULL on error, or 0 and the name
* of the device if the <loadStr> was empty.
*
* ERRNO: N/A
*/

LOCAL END_OBJ * geiEndLoad
    (
    char * loadStr,
    void * pArg
    )
    {
    GEI_DRV_CTRL *pDrvCtrl;
    VXB_DEVICE_ID pDev;
    UINT32 icsOld;
    UINT32 icsNew;
    volatile UINT32 dummy;
    int r;

    /* Make the MUX happy. */

    if (loadStr == NULL)
        return NULL;

    if (loadStr[0] == 0)
        {
        bcopy (GEI_NAME, loadStr, sizeof(GEI_NAME));
        return NULL;
        }

    pDev = pArg;
    pDrvCtrl = pDev->pDrvCtrl;

    if (END_OBJ_INIT (&pDrvCtrl->geiEndObj, NULL, pDev->pName,
        pDev->unitNumber, &geiNetFuncs,
        "Intel PRO/1000 VxBus END Driver") == ERROR)
        {
        logMsg("%s%d: END_OBJ_INIT failed\n", (int)GEI_NAME,
            pDev->unitNumber, 0, 0, 0, 0);
        return (NULL);
        }

    endM2Init (&pDrvCtrl->geiEndObj, M2_ifType_ethernet_csmacd,
        pDrvCtrl->geiAddr, ETHER_ADDR_LEN, ETHERMTU, 100000000,
        IFF_NOTRAILERS | IFF_SIMPLEX | IFF_MULTICAST | IFF_BROADCAST);

    /* Allocate a buffer pool */

    if (pDrvCtrl->geiMaxMtu == GEI_JUMBO_MTU)
        r = endPoolJumboCreate (3 * GEI_RX_DESC_CNT, &pDrvCtrl->geiEndObj.pNetPool);
    else
        r = endPoolCreate (3 * GEI_RX_DESC_CNT, &pDrvCtrl->geiEndObj.pNetPool);

    if (r == ERROR)
        {
        logMsg("%s%d: pool creation failed\n", (int)GEI_NAME,
            pDev->unitNumber, 0, 0, 0, 0);
        return (NULL);
        }

    pDrvCtrl->geiPollBuf = endPoolTupleGet (pDrvCtrl->geiEndObj.pNetPool);

    /* Set up polling stats. */

    pDrvCtrl->geiEndStatsConf.ifPollInterval = sysClkRateGet();
    pDrvCtrl->geiEndStatsConf.ifEndObj = &pDrvCtrl->geiEndObj;
    pDrvCtrl->geiEndStatsConf.ifWatchdog = NULL;
    pDrvCtrl->geiEndStatsConf.ifValidCounters = (END_IFINUCASTPKTS_VALID |
        END_IFINMULTICASTPKTS_VALID | END_IFINBROADCASTPKTS_VALID |
        END_IFINOCTETS_VALID | END_IFINERRORS_VALID | END_IFINDISCARDS_VALID |
        END_IFOUTUCASTPKTS_VALID | END_IFOUTMULTICASTPKTS_VALID |
        END_IFOUTBROADCASTPKTS_VALID | END_IFOUTOCTETS_VALID |
        END_IFOUTERRORS_VALID);

    /* Set up capabilities. All adapters support VLAN_MTU. */

    pDrvCtrl->geiCaps.cap_available = IFCAP_VLAN_MTU;
    pDrvCtrl->geiCaps.cap_enabled = IFCAP_VLAN_MTU;

    /*
     * All adapters support the following checksum offload options:
     *
     * - RX: TCPv4, UDPv4
     * - TX: TCPv4, UDPv4, TCPv6, UDPv6, IPv4
     */

    pDrvCtrl->geiCaps.csum_flags_tx = CSUM_IP|CSUM_TCP|CSUM_UDP;
    pDrvCtrl->geiCaps.csum_flags_tx |= CSUM_TCPv6|CSUM_UDPv6;
    pDrvCtrl->geiCaps.csum_flags_rx = CSUM_UDP|CSUM_TCP;
    pDrvCtrl->geiCaps.cap_available |= IFCAP_TXCSUM|IFCAP_RXCSUM;
    pDrvCtrl->geiCaps.cap_enabled |= IFCAP_TXCSUM|IFCAP_RXCSUM;

    /*
     * All adapters except the 82543 support RX IPv4 header
     * checksum offload.
     *
     * The 82543 has a bug that prevents RX TCP/UDP and IP
     * checksumming from all being enabled at the same time.
     * For that chip, we leave IP header checksumming switched off.
     */

    if (pDrvCtrl->geiDevId > INTEL_DEVICEID_82543GC_COPPER)
        pDrvCtrl->geiCaps.csum_flags_rx |= CSUM_IP;

    /*
     * Adapters newer than the 82544 support TCPv6/UDPv6
     * checksum offload for receive as well.
     *
     * Note: while Intel documents the 82545 and later as having
     * IPv6 RX checksum offload support, it doesn't appear to work
     * on any of the PCI/PCI-X devices. Setting the 'IPv6 offload
     * enable' bit in the RXCSUM register has no effect. It does
     * work for the PCIe devices though. Setting the CSUM_TCPv6
     * and CSUM_UDPv6 bits for the RX capabilities for the PCI/PCI-X
     * devices is therefore a bit of a fib, however it doesn't
     * impact the proper operation of the driver (it just never
     * tags any RX frames as having been checksummed in hardware).
     */

    if (pDrvCtrl->geiDevId > INTEL_DEVICEID_82544GC_LOM)
        pDrvCtrl->geiCaps.csum_flags_rx |= CSUM_TCPv6|CSUM_UDPv6;

    /* All adapters support VLAN tag insertion and stripping. */

    pDrvCtrl->geiCaps.csum_flags_tx |= CSUM_VLAN;
    pDrvCtrl->geiCaps.csum_flags_rx |= CSUM_VLAN;
    pDrvCtrl->geiCaps.cap_available |= IFCAP_VLAN_HWTAGGING;
    pDrvCtrl->geiCaps.cap_enabled |= IFCAP_VLAN_HWTAGGING;
    
    /* Detect PR6120 */
    
    if(pDrvCtrl->geiDevId == DEVICE_ID_PR6120_NIC)
    {
    	/* Currently remove all capabilities */
    	pDrvCtrl->geiCaps.csum_flags_tx = 0;
    	pDrvCtrl->geiCaps.csum_flags_rx = 0;
    	pDrvCtrl->geiCaps.cap_available = 0;
    	pDrvCtrl->geiCaps.cap_enabled = 0;
    }
    
    if (pDrvCtrl->geiMaxMtu == GEI_JUMBO_MTU)
        {
        pDrvCtrl->geiCaps.cap_available |= IFCAP_JUMBO_MTU;
        pDrvCtrl->geiCaps.cap_enabled |= IFCAP_JUMBO_MTU;
        }
    
    /*
     * See if the ICS allows us to read currently
     * pending interrupts. If so, we're on real hardware,
     * and can use the normal ISR routine. If not, we're on
     * a simulated PRO/1000 which obeys the spec, but does not
     * duplicate real hardare behavior, and we have to use
     * an alternate interrupt handling scheme.
     */

    pDrvCtrl->geiIntFunc = geiEndInt;

    /* Make sure interrupts are masked off. */

    CSR_WRITE_4(pDev, GEI_IMC, 0xFFFFFFFF);

    /* Clear all events. */

    dummy = CSR_READ_4(pDev, GEI_ICR);

    /* Get the current ICS value. */

    icsOld = CSR_READ_4(pDev, GEI_ICS);

    /* Manually trigger a link change interrupt */

    CSR_WRITE_4(pDev, GEI_ICS, GEI_ICR_LSC);

    /*
     * If reading the ICS register doesn't show the LCS bit set,
     * or the register contents didn't change, switch to
     * alternate ISR.
     */

    icsNew = CSR_READ_4(pDev, GEI_ICS);

    if ((icsNew & GEI_ICR_LSC) == 0 || icsNew == icsOld)
        pDrvCtrl->geiIntFunc = geiEndAltInt;

    /* Consume/clear the event. */

    CSR_WRITE_4(pDev, GEI_ICS, 0);
    dummy = CSR_READ_4(pDev, GEI_ICR);

    /*
     * Attach our ISR. For PCI, the index value is always
     * 0, since the PCI bus controller dynamically sets
     * up interrupts for us.
     */

    vxbIntConnect (pDev, 0, pDrvCtrl->geiIntFunc, pDrvCtrl);

    return (&pDrvCtrl->geiEndObj);
    }

/*****************************************************************************
*
* geiEndUnload - unload END driver instance
*
* This routine undoes the effects of geiEndLoad(). The END object
* is destroyed, our network pool is released, the endM2 structures
* are released, and the polling stats watchdog is terminated.
*
* Note that the END interface instance can't be unloaded if the
* device is still running. The device must be stopped with muxDevStop()
* first.
*
* RETURNS: ERROR if device is still in the IFF_UP state, otherwise OK
*
* ERRNO: N/A
*/

LOCAL STATUS geiEndUnload
    (
    END_OBJ * pEnd
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;

    /* We must be stopped before we can be unloaded. */

    if (pEnd->flags & IFF_UP)
	return (ERROR);

    pDrvCtrl = (GEI_DRV_CTRL *)pEnd;

    netMblkClChainFree (pDrvCtrl->geiPollBuf);

    /* Relase our buffer pool */
    endPoolDestroy (pDrvCtrl->geiEndObj.pNetPool);

    /* terminate stats polling */
    wdDelete (pDrvCtrl->geiEndStatsConf.ifWatchdog);

    endM2Free (&pDrvCtrl->geiEndObj);

    END_OBJECT_UNLOAD (&pDrvCtrl->geiEndObj);

    return (EALREADY);  /* prevent freeing of pDrvCtrl */
    }

/*****************************************************************************
*
* geiEndHashTblPopulate - populate the multicast hash filter
*
* This function programs the PRO/1000's controller's multicast hash
* filter to receive frames sent to the multicast groups specified
* in the multicast address list attached to the END object. If
* the interface is in IFF_ALLMULTI mode, the filter will be
* programmed to receive all multicast packets by setting the
* 'multicast promiscuous' bit in the RX control register.
*
* The PRO/1000 has two RX filters: a 16-entry CAM filter which can
* be used to perform perfect filtering, and a 4096-bit hash filter.
* We need the first entry in the CAM filter for the station address.
* This leaves 15 entries free for multicasts, so the first 15 multicast
* groups that we join will be programmed into the CAM filter, and any
* additional groups will be programmed into the hash filter.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void geiEndHashTblPopulate
    (
    GEI_DRV_CTRL * pDrvCtrl
    )
    {
    VXB_DEVICE_ID pDev;
    UINT32 addr[2];
    UINT32 h, tmp = 0;
    int reg, bit;
    ETHER_MULTI * mCastNode = NULL;
    int i;

    pDev = pDrvCtrl->geiDev;

    /* First, clear out the original filter. */

    CSR_CLRBIT_4(pDev, GEI_RCTL, GEI_RCTL_MPE);

    for (i = 1; i < GEI_PAR_CNT; i++)
        {
	CSR_WRITE_4(pDev, GEI_PARL0 + (i * sizeof(UINT64)), 0);
        CSR_WRITE_4(pDev, GEI_PARH0 + (i * sizeof(UINT64)), 0);
        }

    for (i = 0; i < GEI_MAR_CNT; i++)
	CSR_WRITE_4(pDev, GEI_MAR0 + (i * sizeof(UINT32)), 0);

    if (pDrvCtrl->geiEndObj.flags & (IFF_ALLMULTI|IFF_PROMISC))
        {
        /* set allmulticast mode */
        CSR_SETBIT_4(pDev, GEI_RCTL, GEI_RCTL_MPE);
        return;
        }

    /* Now repopulate it. */

    i = 1;

    for (mCastNode =
        (ETHER_MULTI *) lstFirst (&pDrvCtrl->geiEndObj.multiList);
         mCastNode != NULL;
         mCastNode = (ETHER_MULTI *) lstNext (&mCastNode->node))
        {
        if (i < GEI_PAR_CNT)
            {
            bcopy (mCastNode->addr, (char *)addr, ETHER_ADDR_LEN);
            CSR_WRITE_4(pDev, GEI_PARL0 + (i * sizeof(UINT64)),
                htole32(addr[0]));
            CSR_WRITE_4(pDev, GEI_PARH0 + (i * sizeof(UINT64)),
                (htole32(addr[1]) & 0xFFFF) | GEI_PARH_AV);
            i++;
            }
        else
            {
            h = (mCastNode->addr[4] >> 4) | (mCastNode->addr[5] << 4);
            reg = (h >> 5) & 0x7F;
            bit = h & 0x1F;
            /*
             * For the 82544 controller, writing to an odd numbered
             * multicast hash table register may corrupt the even numbered
             * one immediately preceeding it. We therefore save it before
             * updating the filter and restore it after.
             */
            if (reg & 0x1)
                tmp = CSR_READ_4(pDev, GEI_MAR0 + ((reg-1) * sizeof(UINT32)));
            CSR_SETBIT_4(pDev, GEI_MAR0 + (reg * sizeof(UINT32)), 1 << bit);
            if (reg & 0x1)
                CSR_WRITE_4(pDev, GEI_MAR0 + ((reg-1) * sizeof(UINT32)), tmp);
            }
        }

    return;
    }

/*****************************************************************************
*
* geiEndMCastAddrAdd - add a multicast address for the device
*
* This routine adds a multicast address to whatever the driver
* is already listening for.  It then resets the address filter.
*
* RETURNS: OK or ERROR.
*
* ERRNO: N/A
*/

LOCAL STATUS geiEndMCastAddrAdd
    (
    END_OBJ * pEnd,
    char * pAddr
    )
    {
    int retVal;
    GEI_DRV_CTRL * pDrvCtrl;

    pDrvCtrl = (GEI_DRV_CTRL *)pEnd;

    semTake (pDrvCtrl->geiDevSem, WAIT_FOREVER);

    if (!(pDrvCtrl->geiEndObj.flags & IFF_UP))
        {
        semGive (pDrvCtrl->geiDevSem);
        return (OK);
        }

    retVal = etherMultiAdd (&pEnd->multiList, pAddr);

    if (retVal == ENETRESET)
        {
        pEnd->nMulti++;
        geiEndHashTblPopulate ((GEI_DRV_CTRL *)pEnd);
        }

    semGive (pDrvCtrl->geiDevSem);
    return (OK);
    }

/*****************************************************************************
*
* geiEndMCastAddrDel - delete a multicast address for the device
*
* This routine removes a multicast address from whatever the driver
* is listening for.  It then resets the address filter.
*
* RETURNS: OK or ERROR.
*
* ERRNO: N/A
*/

LOCAL STATUS geiEndMCastAddrDel
    (
    END_OBJ * pEnd,
    char * pAddr
    )
    {
    int retVal;
    GEI_DRV_CTRL * pDrvCtrl;

    pDrvCtrl = (GEI_DRV_CTRL *)pEnd;

    semTake (pDrvCtrl->geiDevSem, WAIT_FOREVER);

    if (!(pDrvCtrl->geiEndObj.flags & IFF_UP))
        {
        semGive (pDrvCtrl->geiDevSem);
        return (OK);
        }

    retVal = etherMultiDel (&pEnd->multiList, pAddr);

    if (retVal == ENETRESET)
        {
        pEnd->nMulti--;
        geiEndHashTblPopulate ((GEI_DRV_CTRL *)pEnd);
        }

    semGive (pDrvCtrl->geiDevSem);

    return (OK);
    }

/*****************************************************************************
*
* geiEndMCastAddrGet - get the multicast address list for the device
*
* This routine gets the multicast list of whatever the driver
* is already listening for.
*
* RETURNS: OK or ERROR.
*
* ERRNO: N/A
*/

LOCAL STATUS geiEndMCastAddrGet
    (
    END_OBJ * pEnd,
    MULTI_TABLE * pTable
    )
    {
    STATUS rval;
    GEI_DRV_CTRL * pDrvCtrl;

    pDrvCtrl = (GEI_DRV_CTRL *)pEnd;

    semTake (pDrvCtrl->geiDevSem, WAIT_FOREVER);

    if (!(pDrvCtrl->geiEndObj.flags & IFF_UP))
        {
        semGive (pDrvCtrl->geiDevSem);
        return (OK);
        }

    rval = etherMultiGet (&pEnd->multiList, pTable);

    semGive (pDrvCtrl->geiDevSem);

    return (rval);
    }

/*****************************************************************************
*
* geiEndStatsDump - return polled statistics counts
*
* This routine is automatically invoked periodically by the polled
* statistics watchdog.
*
* RETURNS: always OK
*
* ERRNO: N/A
*/

LOCAL STATUS geiEndStatsDump
    (
    GEI_DRV_CTRL * pDrvCtrl
    )
    {
    VXB_DEVICE_ID pDev;
    UINT32 tmp;
    END_IFCOUNTERS *    pEndStatsCounters;

    semTake (pDrvCtrl->geiDevSem, WAIT_FOREVER);

    if (!(pDrvCtrl->geiEndObj.flags & IFF_UP))
        {
        semGive (pDrvCtrl->geiDevSem);
        return (ERROR);
        }

    pDev = pDrvCtrl->geiDev;
    pEndStatsCounters = &pDrvCtrl->geiEndStatsCounters;

    /*
     * Get number of RX'ed octets
     * Note: the octet counts are 64-bit quantities saved in two
     * 32-bit registers. Reading the high word clears the count,
     * so we have to read the low word first.
     */

    tmp = CSR_READ_4(pDev, GEI_GORCL);
    pEndStatsCounters->ifInOctets = tmp;
    tmp = CSR_READ_4(pDev, GEI_GORCH);
    pEndStatsCounters->ifInOctets |= (UINT64)tmp << 32;

    /* Get number of TX'ed octets */

    tmp = CSR_READ_4(pDev, GEI_GOTCL);
    pEndStatsCounters->ifOutOctets = tmp;
    tmp = CSR_READ_4(pDev, GEI_GOTCH);
    pEndStatsCounters->ifOutOctets |= (UINT64)tmp << 32;

    /* Get RX'ed unicasts, broadcasts, multicasts */

    tmp = CSR_READ_4(pDev, GEI_GPRC);
    pEndStatsCounters->ifInUcastPkts = tmp;
    tmp = CSR_READ_4(pDev, GEI_BPRC);
    pEndStatsCounters->ifInBroadcastPkts = tmp;
    tmp = CSR_READ_4(pDev, GEI_MPRC);
    pEndStatsCounters->ifInMulticastPkts = tmp;
    pEndStatsCounters->ifInUcastPkts -= (pEndStatsCounters->ifInMulticastPkts +
        pEndStatsCounters->ifInBroadcastPkts);

    /* Get TX'ed unicasts, broadcasts, multicasts */

    tmp = CSR_READ_4(pDev, GEI_GPTC);
    pEndStatsCounters->ifOutUcastPkts = tmp;
    tmp = CSR_READ_4(pDev, GEI_BPTC);
    pEndStatsCounters->ifOutBroadcastPkts = tmp;
    tmp = CSR_READ_4(pDev, GEI_MPTC);
    pEndStatsCounters->ifOutMulticastPkts = tmp;
    pEndStatsCounters->ifOutUcastPkts -=
        (pEndStatsCounters->ifOutMulticastPkts +
        pEndStatsCounters->ifOutBroadcastPkts);

    semGive (pDrvCtrl->geiDevSem);

    return (OK);
    }

/*****************************************************************************
*
* geiEndIoctl - the driver I/O control routine
*
* This function processes ioctl requests supplied via the muxIoctl()
* routine. In addition to the normal boilerplate END ioctls, this
* driver supports the IFMEDIA ioctls, END capabilities ioctls, and
* polled stats ioctls.
*
* RETURNS: A command specific response, usually OK or ERROR.
*
* ERRNO: N/A
*/

LOCAL int geiEndIoctl
    (
    END_OBJ * pEnd,
    int cmd,
    caddr_t data
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;
    END_MEDIALIST * mediaList;
    END_CAPABILITIES * hwCaps;
    END_MEDIA * pMedia;
    END_RCVJOBQ_INFO * qinfo;
    UINT32 nQs;
    VXB_DEVICE_ID pDev;
    INT32 value;
    int error = OK;

    pDrvCtrl = (GEI_DRV_CTRL *)pEnd;
    pDev = pDrvCtrl->geiDev;

    if (cmd != EIOCPOLLSTART && cmd != EIOCPOLLSTOP)
        semTake (pDrvCtrl->geiDevSem, WAIT_FOREVER);

    switch (cmd)
        {
        case EIOCSADDR:
            if (data == NULL)
                error = EINVAL;
            else
                {
                bcopy ((char *)data, (char *)pDrvCtrl->geiAddr,
                    ETHER_ADDR_LEN);
                bcopy ((char *)data,
                    (char *)pEnd->mib2Tbl.ifPhysAddress.phyAddress,
                    ETHER_ADDR_LEN);
                if (pEnd->pMib2Tbl != NULL)
                    bcopy ((char *)data,
                        (char *)pEnd->pMib2Tbl->m2Data.mibIfTbl.ifPhysAddress.phyAddress,
                        ETHER_ADDR_LEN);
                }
            geiEndRxConfig (pDrvCtrl);
            break;

        case EIOCGADDR:
            if (data == NULL)
                error = EINVAL;
            else
                bcopy ((char *)pDrvCtrl->geiAddr, (char *)data,
                    ETHER_ADDR_LEN);
            break;
        case EIOCSFLAGS:

            value = (INT32) data;
            if (value < 0)
                {
                value = -value;
                value--;
                END_FLAGS_CLR (pEnd, value);
                }
            else
                END_FLAGS_SET (pEnd, value);

            geiEndRxConfig (pDrvCtrl);

            break;

        case EIOCGFLAGS:
            if (data == NULL)
                error = EINVAL;
            else
                *(long *)data = END_FLAGS_GET(pEnd);

            break;

        case EIOCMULTIADD:
            error = geiEndMCastAddrAdd (pEnd, (char *) data);
            break;

        case EIOCMULTIDEL:
            error = geiEndMCastAddrDel (pEnd, (char *) data);
            break;

        case EIOCMULTIGET:
            error = geiEndMCastAddrGet (pEnd, (MULTI_TABLE *) data);
            break;

        case EIOCPOLLSTART:
            pDrvCtrl->geiPolling = TRUE;
            pDrvCtrl->geiIntMask = CSR_READ_4(pDev, GEI_IMS);
            CSR_WRITE_4(pDev, GEI_IMC, 0xFFFFFFFF);

            /*
             * We may have been asked to enter polled mode while
             * there are transmissions pending. This is a problem,
             * because the polled transmit routine expects that
             * the TX ring will be empty when it's called. In
             * order to guarantee this, we have to drain the TX
             * ring here. We could also just plain reset and
             * reinitialize the transmitter, but this is faster.
             */

            while (CSR_READ_4(pDev, GEI_TDH) != CSR_READ_4(pDev, GEI_TDT))
               ;

            while (pDrvCtrl->geiTxFree < GEI_TX_DESC_CNT)
                {
#ifdef GEI_VXB_DMA_BUF
                VXB_DMA_MAP_ID pMap;
#endif
                M_BLK_ID pMblk;
 
                pMblk = pDrvCtrl->geiTxMblk[pDrvCtrl->geiTxCons];
 
                if (pMblk != NULL)
                    {
#ifdef GEI_VXB_DMA_BUF
                    pMap = pDrvCtrl->geiTxMblkMap[pDrvCtrl->geiTxCons];
                    vxbDmaBufMapUnload (pDrvCtrl->geiMblkTag, pMap);
#endif
                    endPoolTupleFree (pMblk);
                    pDrvCtrl->geiTxMblk[pDrvCtrl->geiTxCons] = NULL;
                    }
 
                pDrvCtrl->geiTxFree++;
                GEI_INC_DESC (pDrvCtrl->geiTxCons, GEI_TX_DESC_CNT);
                }

	    pDrvCtrl->geiTxSinceMark = 0;

            break;

        case EIOCPOLLSTOP:
            pDrvCtrl->geiPolling = FALSE;
            CSR_WRITE_4(pDev, GEI_IMS, pDrvCtrl->geiIntMask);

            break;

        case EIOCGMIB2233:
        case EIOCGMIB2:
            error = endM2Ioctl (&pDrvCtrl->geiEndObj, cmd, data);
            break;

        case EIOCGPOLLCONF:
            if (data == NULL)
                error = EINVAL;
            else
                *((END_IFDRVCONF **)data) = &pDrvCtrl->geiEndStatsConf;
            break;

        case EIOCGPOLLSTATS:
            if (data == NULL)
                error = EINVAL;
            else
                {
                error = geiEndStatsDump(pDrvCtrl);
                if (error == OK)
                    *((END_IFCOUNTERS **)data) = &pDrvCtrl->geiEndStatsCounters;
                }
            break;

        case EIOCGMEDIALIST:
            if (data == NULL)
                {
                error = EINVAL;
                break;
                }
            if (pDrvCtrl->geiMediaList->endMediaListLen == 0)
                {
                error = ENOTSUP;
                break;
                }

            mediaList = (END_MEDIALIST *)data;
            if (mediaList->endMediaListLen <
                pDrvCtrl->geiMediaList->endMediaListLen)
                {
                mediaList->endMediaListLen =
                    pDrvCtrl->geiMediaList->endMediaListLen;
                error = ENOSPC;
                break;
                }

            bcopy((char *)pDrvCtrl->geiMediaList, (char *)mediaList,
                  sizeof(END_MEDIALIST) + (sizeof(UINT32) *
                  pDrvCtrl->geiMediaList->endMediaListLen));
            break;

        case EIOCGIFMEDIA:
            if (data == NULL)
                error = EINVAL;
            else
                {
                pMedia = (END_MEDIA *)data;
                pMedia->endMediaActive = pDrvCtrl->geiCurMedia;
                pMedia->endMediaStatus = pDrvCtrl->geiCurStatus;
                }
            break;

        case EIOCSIFMEDIA:
            if (data == NULL)
                error = EINVAL;
            else
                {
                pMedia = (END_MEDIA *)data;
                miiBusModeSet (pDrvCtrl->geiMiiBus, pMedia->endMediaActive);
                geiSpdDisable (pDrvCtrl->geiDev);
                geiLinkUpdate (pDrvCtrl->geiDev);
                error = OK;
                }
            break;

        case EIOCGIFCAP:
            hwCaps = (END_CAPABILITIES *)data;
            if (hwCaps == NULL)
                {
                error = EINVAL;
                break;
                }
            hwCaps->csum_flags_tx = pDrvCtrl->geiCaps.csum_flags_tx;
            hwCaps->csum_flags_rx = pDrvCtrl->geiCaps.csum_flags_rx;
            hwCaps->cap_available = pDrvCtrl->geiCaps.cap_available;
            hwCaps->cap_enabled = pDrvCtrl->geiCaps.cap_enabled;
            break;

        case EIOCSIFCAP:
            hwCaps = (END_CAPABILITIES *)data;
            if (hwCaps == NULL)
                {
                error = EINVAL;
                break;
                }

            /*
             * We must enable or disable RX VLAN tag extraxtion
             * as specified, since leaving it on all the time will
             * always cause the chip to strip the VLAN tag field
             * from the frame.
             */

            pDrvCtrl->geiCaps.cap_enabled = hwCaps->cap_enabled;
            if (pDrvCtrl->geiCaps.cap_enabled & IFCAP_VLAN_HWTAGGING)
                CSR_SETBIT_4(pDev, GEI_CTRL, GEI_CTRL_VME);
            else
                CSR_CLRBIT_4(pDev, GEI_CTRL, GEI_CTRL_VME);
            break;

        case EIOCGIFMTU:
            if (data == NULL)
                error = EINVAL;
            else
                *(INT32 *)data = pEnd->mib2Tbl.ifMtu;
            break;

        case EIOCSIFMTU:
            value = (INT32)data;
            if (value <= 0 || value > pDrvCtrl->geiMaxMtu)
                error = EINVAL;
            else
                {
                pEnd->mib2Tbl.ifMtu = value;
                if (pEnd->pMib2Tbl != NULL)
                    pEnd->pMib2Tbl->m2Data.mibIfTbl.ifMtu = value;
                }
            break;

	case EIOCGRCVJOBQ:
            if (data == NULL)
		{
                error = EINVAL;
		break;
		}

	    qinfo = (END_RCVJOBQ_INFO *)data;
	    nQs = qinfo->numRcvJobQs;
	    qinfo->numRcvJobQs = 1;
	    if (nQs < 1)
		error = ENOSPC;
	    else
		qinfo->qIds[0] = pDrvCtrl->geiJobQueue;
	    break;

        default:
            error = EINVAL;
            break;
        }

    if (cmd != EIOCPOLLSTART && cmd != EIOCPOLLSTOP)
        semGive (pDrvCtrl->geiDevSem);

    return (error);
    }

/*****************************************************************************
*
* geiEndRxConfig - configure the PRO/1000 RX filter
*
* This is a helper routine used by geiEndIoctl() and geiEndStart() to
* configure the controller's RX filter. The unicast address filter is
* programmed with the currently configured MAC address, and the RX
* configuration is set to allow unicast and broadcast frames to be
* received. If the interface is in IFF_PROMISC mode, the UPE
* bit is set, which allows all packets to be received.
*
* The geiEndHashTblPopulate() routine is also called to update the
* multicast filter.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void geiEndRxConfig
    (
    GEI_DRV_CTRL * pDrvCtrl
    )
    {
    UINT32 addr[2];
    VXB_DEVICE_ID pDev;

    pDev = pDrvCtrl->geiDev;

    /* Copy the address to a buffer we know is 32-bit aligned. */

    bcopy ((char *)pDrvCtrl->geiAddr, (char *)addr, ETHER_ADDR_LEN);

    /*
     * Init our MAC address.
     */

    CSR_WRITE_4(pDev, GEI_PARL0, htole32(addr[0]));
    CSR_WRITE_4(pDev, GEI_PARH0, (htole32(addr[1]) & 0xFFFF) | GEI_PARH_AV);

    /* Program the RX filter to receive unicasts and broadcasts */

    CSR_SETBIT_4(pDev, GEI_RCTL, GEI_RCTL_BAM);

    /* Enable promisc mode, if specified. */

    if (pDrvCtrl->geiEndObj.flags & IFF_PROMISC)
        CSR_SETBIT_4(pDev, GEI_RCTL, GEI_RCTL_UPE);
    else
        CSR_CLRBIT_4(pDev, GEI_RCTL, GEI_RCTL_UPE);

    /* Program the multicast filter. */

    geiEndHashTblPopulate (pDrvCtrl);

    return;
    }

/*****************************************************************************
*
* geiEndStart - start the device
*
* This function resets the device to put it into a known state and
* then configures it for RX and TX operation. The RX and TX configuration
* registers are initialized, and the address of the RX DMA window is
* loaded into the device. Interrupts are then enabled, and the initial
* link state is configured.
*
* Note that this routine also checks to see if an alternate jobQueue
* has been specified via the vxbParam subsystem. This allows the driver
* to divert its work to an alternate processing task, such as may be
* done with TIPC. This means that the jobQueue can be changed while
* the system is running, but the device must be stopped and restarted
* for the change to take effect.
*
* RETURNS: ERROR if device initialization failed, otherwise OK
*
* ERRNO: N/A
*/

LOCAL STATUS geiEndStart
    (
    END_OBJ * pEnd
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;
    VXB_DEVICE_ID pDev;
    VXB_INST_PARAM_VALUE val;
    HEND_RX_QUEUE_PARAM * pRxQueue;
    GEI_RDESC * pDesc;
    M_BLK_ID pMblk;
#ifdef GEI_VXB_DMA_BUF
    VXB_DMA_MAP_ID pMap;
#else
    UINT32 addr;
#endif
    int i;

    pDrvCtrl = (GEI_DRV_CTRL *)pEnd;

    semTake (pDrvCtrl->geiDevSem, WAIT_FOREVER);

    if (pEnd->flags & IFF_UP)
        {
        semGive (pDrvCtrl->geiDevSem);
        return (OK);
        }

    END_TX_SEM_TAKE (pEnd, WAIT_FOREVER); 

    pDev = pDrvCtrl->geiDev;

    /* Initialize job queues */

    pDrvCtrl->geiJobQueue = netJobQueueId;

    /* Override the job queue ID if the user supplied an alternate one. */

    /*
     * paramDesc {
     * The rxQueue00 parameter specifies a pointer to a
     * HEND_RX_QUEUE_PARAM structure, which contains,
     * among other things, an ID for the job queue
     * to be used for this instance. }
     */
    if (vxbInstParamByNameGet (pDev, "rxQueue00",
        VXB_PARAM_POINTER, &val) == OK)
        {
        pRxQueue = (HEND_RX_QUEUE_PARAM *) val.pValue;
        if (pRxQueue->jobQueId != NULL)
            pDrvCtrl->geiJobQueue = pRxQueue->jobQueId;
        }

    QJOB_SET_PRI(&pDrvCtrl->geiIntJob, NET_TASK_QJOB_PRI);
    pDrvCtrl->geiIntJob.func = geiEndIntHandle;

    vxAtomicSet (&pDrvCtrl->geiIntPending, 0);

    /* Reset the controller to a known state */

    geiReset (pDev);

    /* Set up the RX ring. */

    bzero ((char *)pDrvCtrl->geiRxDescMem,
        sizeof(GEI_RDESC) * GEI_RX_DESC_CNT);

    for (i = 0; i < GEI_RX_DESC_CNT; i++)
        {
        pMblk = endPoolTupleGet (pDrvCtrl->geiEndObj.pNetPool);
        if (pMblk == NULL)
            {
            END_TX_SEM_GIVE (pEnd);
            semGive (pDrvCtrl->geiDevSem);
            return (ERROR);
            }

#ifndef GEI_VXB_DMA_BUF
	CACHE_USER_INVALIDATE (pMblk->m_data, pMblk->m_len);
#endif
	/* Note, endPoolTupleIdGet() has properly set the tuple data lengths */

        pMblk->m_next = NULL;
        GEI_ADJ (pMblk);
        pDrvCtrl->geiRxMblk[i] = pMblk;

        pDesc = &pDrvCtrl->geiRxDescMem[i];
#ifdef GEI_VXB_DMA_BUF
        pMap = pDrvCtrl->geiRxMblkMap[i];

        vxbDmaBufMapMblkLoad (pDev, pDrvCtrl->geiMblkTag, pMap, pMblk, 0);

        /*
         * Currently, pointers in VxWorks are always 32 bits, even on
         * 64-bit architectures. This means that for the time being,
         * we can save a few cycles by only setting the addrlo field
         * in DMA descriptors and leaving the addrhi field set to 0.
         * When pointers become 64 bits wide, this will need to be
         * changed.
         */

#ifdef GEI_64
        pDesc->gei_addrlo = htole32(GEI_ADDR_LO(pMap->fragList[0].frag));
        pDesc->gei_addrhi = htole32(GEI_ADDR_HI(pMap->fragList[0].frag));
#else
        pDesc->gei_addrlo = htole32((UINT32)pMap->fragList[0].frag);
#endif
#else /* ! defined (GEI_VXB_DMA_BUF) */
#ifdef GEI_64
#error "GEI_64" is not supported without GEI_VXB_DMA_BUF
#endif
	{
	addr = (UINT32)pMblk->m_data;
	if (cacheUserFuncs.virtToPhysRtn)
	    addr = cacheUserFuncs.virtToPhysRtn (addr);
	pDesc->gei_addrlo = htole32(addr);
	}
#endif /* GEI_VXB_DMA_BUF */
        }

    /* Set up the TX ring. */

    bzero ((char *)pDrvCtrl->geiTxDescMem,
        sizeof(GEI_TDESC) * GEI_TX_DESC_CNT);

#ifdef GEI_VXB_DMA_BUF
    /* Load the maps for the RX and TX DMA ring. */

    vxbDmaBufMapLoad (pDev, pDrvCtrl->geiRxDescTag,
        pDrvCtrl->geiRxDescMap, pDrvCtrl->geiRxDescMem,
        sizeof(GEI_RDESC) * GEI_RX_DESC_CNT, 0);

    vxbDmaBufMapLoad (pDev, pDrvCtrl->geiTxDescTag,
        pDrvCtrl->geiTxDescMap, pDrvCtrl->geiTxDescMem,
        sizeof(GEI_TDESC) * GEI_TX_DESC_CNT, 0);
#endif
    /* Initialize state */

    pDrvCtrl->geiRxIdx = 0;
    pDrvCtrl->geiMoreRx = 0;
    pDrvCtrl->geiTxStall = FALSE;
    pDrvCtrl->geiTxProd = 0;
    pDrvCtrl->geiTxCons = 0;
    pDrvCtrl->geiTxFree = GEI_TX_DESC_CNT;
    pDrvCtrl->geiTxSinceMark = 0;
    pDrvCtrl->geiTbiCompat = FALSE;
    pDrvCtrl->geiLastCtx = 0xFFFF;
#ifdef CSUM_IPHDR_OFFSET
    pDrvCtrl->geiLastOffsets = -1;
#else
    pDrvCtrl->geiLastIpLen = -1;
#endif
    pDrvCtrl->geiLastVlan = 0;

    /*
     * For 82543 cards, set the DMA priority to fair rather than
     * favoring RX. Not valid for 82544 or later chips.
     */
     
    if (pDrvCtrl->geiDevId <= INTEL_DEVICEID_82543GC_COPPER)
        CSR_SETBIT_4(pDev, GEI_CTRL, GEI_CTRL_PRIOR);

    /* Enable VLAN tag stripping on receive */

    if (pDrvCtrl->geiCaps.cap_enabled & IFCAP_VLAN_HWTAGGING)
        CSR_SETBIT_4(pDev, GEI_CTRL, GEI_CTRL_VME);

    /*
     * Initialize the RX control word.
     * Note: advanced class devices don't support the BSEX
     * bit, so if we want jumbo frame support, we need to
     * do some extra configuration in the SRRCTL register
     * later.
     */

    if (pDrvCtrl->geiMaxMtu == GEI_JUMBO_MTU &&
        pDrvCtrl->geiDevType != GEI_DEVTYPE_ADVANCED)
        CSR_WRITE_4(pDev, GEI_RCTL, GEI_RCTL_LPE|GEI_BSIZE_16384|GEI_MO_47_36);
    else
        CSR_WRITE_4(pDev, GEI_RCTL, GEI_BSIZE_2048|GEI_MO_47_36);

    /* Enable hardware RX CRC stripping */

    CSR_SETBIT_4(pDev, GEI_RCTL, GEI_RCTL_SECRC);

    /* Disable delivery of bad frames. */

    CSR_CLRBIT_4(pDev, GEI_RCTL, GEI_RCTL_SBP);

    /*
     * Activate RX checksum offload. TCP/UDP checksum offload is
     * always on. IPv4 header checksums are on for everything except
     * the 82543 (which has a bug that prevents TCP/UDP and IP header
     * checksum offload from being enabled at the same time). IPv6
     * support is enabled for all adapters newer than the 82544.
     */

    CSR_WRITE_4(pDev, GEI_RXCSUM, GEI_RXCSUM_TUOFLD);

    if (pDrvCtrl->geiDevId > INTEL_DEVICEID_82543GC_COPPER)
        CSR_SETBIT_4(pDev, GEI_RXCSUM, GEI_RXCSUM_IPOFLD);

    if (pDrvCtrl->geiDevId > INTEL_DEVICEID_82544GC_LOM)
        CSR_SETBIT_4(pDev, GEI_RXCSUM, GEI_RXCSUM_IPV6OFL);

    /*
     * Do special setup for 82575 and later devices with
     * 'advanced' descriptor format (82576, 82580).
     */
 
    if (pDrvCtrl->geiDevType == GEI_DEVTYPE_ADVANCED)
        {
        CSR_CLRBIT_4(pDev, GEI_RXCSUM, GEI_RXCSUM_IPV6OFL);
        CSR_SETBIT_4(pDev, GEI_RXCSUM, GEI_RXCSUM_CRCOFL);
        CSR_CLRBIT_4(pDev, GEI_SRRCTL, GEI_SRRCTL_DESCTYPE);
        CSR_SETBIT_4(pDev, GEI_SRRCTL, GEI_DESCTYPE_ADV_ONE);
        /*
         * If jumbo frames are enabled, make sure to increase
         * the packet buffer size, otherwise the chip will
         * try to fragment jumbo frames across multiple
         * descriptors, which we don't handle.
         */
        CSR_CLRBIT_4(pDev, GEI_SRRCTL, GEI_SRRCTL_BSIZEPKT);
        if (pDrvCtrl->geiMaxMtu == GEI_JUMBO_MTU)
            {
            CSR_SETBIT_4(pDev, GEI_RCTL, GEI_RCTL_LPE);
            CSR_SETBIT_4(pDev, GEI_SRRCTL, 10);
            }
        else
            CSR_SETBIT_4(pDev, GEI_SRRCTL, 2);
        }

    /* Initialize RX descriptor ring size */

    CSR_WRITE_4(pDev, GEI_RDLEN, GEI_RX_DESC_CNT * sizeof(GEI_RDESC));

    /* Set the base address of the RX ring */
#ifdef GEI_VXB_DMA_BUF
    CSR_WRITE_4(pDev, GEI_RDBAH,
        GEI_ADDR_HI(pDrvCtrl->geiRxDescMap->fragList[0].frag));
    CSR_WRITE_4(pDev, GEI_RDBAL,
        GEI_ADDR_LO(pDrvCtrl->geiRxDescMap->fragList[0].frag));
#else
    CSR_WRITE_4(pDev, GEI_RDBAH, 0);
    addr = (UINT32)pDrvCtrl->geiRxDescMem;
    if (cacheUserFuncs.virtToPhysRtn)
	addr = cacheUserFuncs.virtToPhysRtn (addr);
    CSR_WRITE_4(pDev, GEI_RDBAL, addr);
#endif

    /*
     * Workaround occasional RX stalls on 82573 devices. Normally,
     * Intel recommends leaving RDTR set to 0, however for these
     * adapters, it needs to be set to a non-zero value, otherwise
     * the receiver may stall briefly under load, resulting in choppy
     * traffic patterns.  We set RADV to 1, the smallest value, to limit
     * the interrupt delay to 1us, and set RDTR a bit larger.  See
     * WIND00146698 for one way to reproduce the issue that occurs
     * when RDTR is left at zero.
     */

    if (pDrvCtrl->geiDevId == INTEL_DEVICEID_82573V ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82573E_IAMT ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82573L ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82573L_PL ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82573V_PM ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82573E_PM ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82573L_PL2)
	{
        CSR_WRITE_4(pDev, GEI_RDTR, 2);
        CSR_WRITE_4(pDev, GEI_RADV, 1);
	}

    /* Initialize the TX control word. */

    CSR_SETBIT_4(pDev, GEI_TCTL, GEI_TCTL_PSP);
    CSR_CLRBIT_4(pDev, GEI_TCTL, GEI_TCTL_CT);
    CSR_SETBIT_4(pDev, GEI_TCTL, GEI_CT(GEI_COLLTHRESH));
    if (pDrvCtrl->geiDevType == GEI_DEVTYPE_ADVANCED)
        {
        CSR_CLRBIT_4(pDev, GEI_TCTL_EXT, GEI_TCTLEXT_COLD);
        CSR_SETBIT_4(pDev, GEI_TCTL_EXT, GEI_COLD_EXT(GEI_COLLDIST_HDX));
        }
    else
        {
        CSR_CLRBIT_4(pDev, GEI_TCTL, GEI_TCTL_COLD);
        CSR_SETBIT_4(pDev, GEI_TCTL, GEI_COLD(GEI_COLLDIST_HDX));
        }

    /* Initialize TX descriptor ring size */

    CSR_WRITE_4(pDev, GEI_TDLEN, GEI_TX_DESC_CNT * sizeof(GEI_TDESC));

    /* Set producer and consumer indexes */

    CSR_WRITE_4(pDev, GEI_TDH, 0);
    CSR_WRITE_4(pDev, GEI_TDT, 0);

    if (pDrvCtrl->geiDevType == GEI_DEVTYPE_ES2LAN)
        CSR_SETBIT_4(pDev, GEI_TCTL, GEI_TCTL_MULR|GEI_TCTL_RTLC);

    /*
     * Leave TXDCTL, TIDV and TADV alone for 82575.
     * The default TXDCTL settings should be fine, and TIDV
     * and TADV aren't supported on the 82575 devices.
     */

    if (pDrvCtrl->geiDevType == GEI_DEVTYPE_ADVANCED)
        goto noTxDctl;

    /* Set TX descriptor thresholds */

    if (pDrvCtrl->geiDevId > INTEL_DEVICEID_82543GC_COPPER)
        CSR_WRITE_4(pDev, GEI_TXDCTL, GEI_TXDCTL_GRAN | GEI_WTHRESH(1));

    /*
     * Per Intel documentation, only set the GEI_TXDCTL_COUNT_DESC
     * bit (TXDCTL.22) for the 82571EB/82572EI and 631xESB/632xESB.
     */

    if (pDrvCtrl->geiDevId == INTEL_DEVICEID_82571EB_QUAD_COPPER ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82571EB_QUAD_FIBER ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82571EB_DUAL_COPPER ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82571EB_FIBER ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82571EB_SERDES ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82571EB_QUAD_COPPER_LP ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82571EB_MEZZ_DUAL ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82571EB_MEZZ_QUAD ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82571PT_QUAD ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82572EI_COPPER ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82572EI_FIBER ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82572EI_SERDES ||
        pDrvCtrl->geiDevId == INTEL_DEVICEID_82572EI ||
        pDrvCtrl->geiDevType == GEI_DEVTYPE_ES2LAN)
        CSR_SETBIT_4(pDev, GEI_TXDCTL, GEI_TXDCTL_COUNT_DESC);

    /*
     * Workaround for 82544EI/82544GC erratum #20. The 82544EI/82544GC
     * specification update states that setting the TXDCTL WTHRESH
     * to a non-zero value may result in corrupted descriptor writebacks.
     */

    if (pDrvCtrl->geiDevId >= INTEL_DEVICEID_82544EI_COPPER &&
        pDrvCtrl->geiDevId <= INTEL_DEVICEID_82544GC_LOM)
        CSR_WRITE_4(pDev, GEI_TXDCTL, 0);

    /*
     * We use a fairly large TIDV and no TADV, as we now do
     * some TX clean-up in the send routine, and also set
     * occasional TX descriptors for immediate interrupt upon
     * completion (RS set but IDE clear) when the stack
     * is sending faster than the wire and the TX ring
     * fills up.
     */
    CSR_WRITE_4(pDev, GEI_TIDV, 0x200);

    if (pDrvCtrl->geiDevId > INTEL_DEVICEID_82544GC_LOM)
        CSR_WRITE_4(pDev, GEI_TADV, 0);

noTxDctl:

    /*
     * Explicitly program the TIPG register. This seems to be required for
     * the 82544GC chip, even though reading the register shows that it
     * contains the expected default IPG values. If we don't do a write
     * to this register, there will sometimes be TX failures at 1000Mbps
     * with some switches.
     */

    if (pDrvCtrl->geiTbi == TRUE)
        CSR_WRITE_4(pDev, GEI_TIPG, GEI_IPGT(GEI_IPGT_DFLT_FIBER)|
            GEI_IPGR1(GEI_IPGR1_DFLT)|GEI_IPGR2(GEI_IPGR2_DFLT));
    else if (pDrvCtrl->geiDevType == GEI_DEVTYPE_ES2LAN)
        CSR_WRITE_4(pDev, GEI_TIPG, GEI_IPGT(GEI_IPGT_DFLT_COPPER)|
            GEI_IPGR1(GEI_IPGR1_DFLT)|GEI_IPGR2(GEI_IPGR2_ES2LAN_DFLT));
    else
        CSR_WRITE_4(pDev, GEI_TIPG, GEI_IPGT(GEI_IPGT_DFLT_COPPER)|
            GEI_IPGR1(GEI_IPGR1_DFLT)|GEI_IPGR2(GEI_IPGR2_DFLT));

    /* Set the base address of the TX ring */

#ifdef GEI_VXB_DMA_BUF
    CSR_WRITE_4(pDev, GEI_TDBAH,
        GEI_ADDR_HI(pDrvCtrl->geiTxDescMap->fragList[0].frag));
    CSR_WRITE_4(pDev, GEI_TDBAL,
        GEI_ADDR_LO(pDrvCtrl->geiTxDescMap->fragList[0].frag));
#else
    CSR_WRITE_4(pDev, GEI_TDBAH, 0);
    addr = (UINT32)pDrvCtrl->geiTxDescMem;
    if (cacheUserFuncs.virtToPhysRtn)
	addr = cacheUserFuncs.virtToPhysRtn (addr);
    CSR_WRITE_4(pDev, GEI_TDBAL, addr);
#endif
    /* Program the RX filter. */

    geiEndRxConfig (pDrvCtrl);

    /*
     * Set interrupt throttling timer.
     * This has an effect on both RX and TX interrupt processing. TX
     * interrupt processing can be additionally affected by the TIDV
     * and TADV timers. For RX interrupts, we don't want to introduce
     * too large of a delay, since that can impact TCP and forwarding
     * performance.
     *
     * We now clear ITR, disabling interrupt throttling (except for
     * the 82575, which only supports ITR as an interrupt moderation
     * mechanism -- note: the documentation for the 82575 does not
     * mention support for the ITR register, but the Intel supplied
     * sample code uses it, and it does work).
     */

    if (pDrvCtrl->geiDevType == GEI_DEVTYPE_ADVANCED)
        CSR_WRITE_4(pDev, GEI_ITR, 200);
    else if (pDrvCtrl->geiDevId > INTEL_DEVICEID_82544GC_LOM)
        CSR_WRITE_4(pDev, GEI_ITR, 0);

    /* Enable interrupts */

    vxbIntEnable (pDev, 0, pDrvCtrl->geiIntFunc, pDrvCtrl);
    pDrvCtrl->geiIntrs = GEI_INTRS;
    CSR_WRITE_4(pDev, GEI_IMS, pDrvCtrl->geiIntrs);

    /*
     * Enable receiver and transmitter
     * For advanced class devices, we also have to enable
     * the queue we want to use. For thr 82575 and 82576,
     * this is optional as queue 0 is enabled by default
     * when the device comes out of reset (for backwards
     * compatibility). For the 82580, this is mandatory,
     * as all queues are off by default.
     */

    if (pDrvCtrl->geiDevType == GEI_DEVTYPE_ADVANCED)
        {
        CSR_SETBIT_4(pDev, GEI_RXDCTL, GEI_RXDCTL_ENABLE);
        CSR_SETBIT_4(pDev, GEI_TXDCTL, GEI_TXDCTL_ENABLE);
        }

    CSR_SETBIT_4(pDev, GEI_RCTL, GEI_RCTL_EN);
    CSR_SETBIT_4(pDev, GEI_TCTL, GEI_TCTL_EN);

    /* Set producer and consumer indexes */

    CSR_WRITE_4(pDev, GEI_RDH, 0);
    CSR_WRITE_4(pDev, GEI_RDT, GEI_RX_DESC_CNT - 1);

    /* Make sure we're able to manually set speed and duplex. */
 
    CSR_SETBIT_4(pDev, GEI_CTRL,
        GEI_CTRL_SLU|GEI_CTRL_FRCSPD|GEI_CTRL_FRCDPX);

    /* Set initial link state */

    pDrvCtrl->geiCurMedia = IFM_ETHER|IFM_NONE;
    pDrvCtrl->geiCurStatus = IFM_AVALID;

    miiBusModeSet (pDrvCtrl->geiMiiBus,
        pDrvCtrl->geiMediaList->endMediaListDefault);

    geiSpdDisable (pDev);

    END_FLAGS_SET (pEnd, (IFF_UP | IFF_RUNNING));

    END_TX_SEM_GIVE (pEnd);
    semGive (pDrvCtrl->geiDevSem);

    return (OK);
    }

/*****************************************************************************
*
* geiEndStop - stop the device
*
* This function undoes the effects of geiEndStart(). The device is shut
* down and all resources are released. Note that the shutdown process
* pauses to wait for all pending RX, TX and link event jobs that may have
* been initiated by the interrupt handler to complete. This is done
* to prevent tNetTask from accessing any data that might be released by
* this routine.
*
* RETURNS: ERROR if device shutdown failed, otherwise OK
*
* ERRNO: N/A
*/

LOCAL STATUS geiEndStop
    (
    END_OBJ * pEnd
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;
    VXB_DEVICE_ID pDev;
    int i;

    pDrvCtrl = (GEI_DRV_CTRL *)pEnd;

    semTake (pDrvCtrl->geiDevSem, WAIT_FOREVER);
    if (!(pEnd->flags & IFF_UP))
        {
        semGive (pDrvCtrl->geiDevSem);
        return (OK);
        }

    pDev = pDrvCtrl->geiDev;

    /* Disable interrupts */
    /*vxbIntDisable (pDev, 0, pDrvCtrl->geiIntFunc, pDrvCtrl);*/
    pDrvCtrl->geiIntrs = 0;
    CSR_WRITE_4(pDev, GEI_IMC, 0xFFFFFFFF);

    /*
     * Wait for all jobs to drain.
     * Note: this must be done before we disable the receiver
     * and transmitter below. If someone tries to reboot us via
     * WDB, this routine may be invoked while the RX handler is
     * still running in tNetTask. If we disable the chip while
     * that function is running, it'll start reading inconsistent
     * status from the chip. We have to wait for that job to
     * terminate first, then we can disable the receiver and
     * transmitter.
     */

    for (i = 0; i < GEI_TIMEOUT; i++)
        {
        if (vxAtomicGet (&pDrvCtrl->geiIntPending) == FALSE)
            break;
        taskDelay(1);
        }

    if (i == GEI_TIMEOUT)
        logMsg("%s%d: timed out waiting for job to complete\n",
            (int)GEI_NAME, pDev->unitNumber, 0, 0, 0, 0);

    END_TX_SEM_TAKE (pEnd, WAIT_FOREVER); 

    END_FLAGS_CLR (pEnd, (IFF_UP | IFF_RUNNING));

    /* Disable RX and TX. */

    geiReset (pDev);

    /* Release resources */

#ifdef GEI_VXB_DMA_BUF
    vxbDmaBufMapUnload (pDrvCtrl->geiRxDescTag, pDrvCtrl->geiRxDescMap);
    vxbDmaBufMapUnload (pDrvCtrl->geiTxDescTag, pDrvCtrl->geiTxDescMap);
#endif

    for (i = 0; i < GEI_RX_DESC_CNT; i++)
        {
        if (pDrvCtrl->geiRxMblk[i] != NULL)
            {
            netMblkClChainFree (pDrvCtrl->geiRxMblk[i]);
            pDrvCtrl->geiRxMblk[i] = NULL;
#ifdef GEI_VXB_DMA_BUF
            vxbDmaBufMapUnload (pDrvCtrl->geiMblkTag,
                pDrvCtrl->geiRxMblkMap[i]);
#endif
            }
        }

    /*
     * Flush the recycle cache to shake loose any of our
     * mBlks that may be stored there.
     */

    endMcacheFlush ();

    for (i = 0; i < GEI_TX_DESC_CNT; i++)
        {
        if (pDrvCtrl->geiTxMblk[i] != NULL)
            {
            netMblkClChainFree (pDrvCtrl->geiTxMblk[i]);
            pDrvCtrl->geiTxMblk[i] = NULL;
#ifdef GEI_VXB_DMA_BUF
            vxbDmaBufMapUnload (pDrvCtrl->geiMblkTag,
                pDrvCtrl->geiTxMblkMap[i]);
#endif
            }
        }

    END_TX_SEM_GIVE (pEnd); 
    semGive (pDrvCtrl->geiDevSem);

    return (OK);
    }

/*****************************************************************************
*
* geiEndIntHandle - task level interrupt handler
*
* This routine is scheduled to run in tNetTask by the interrupt service
* routine whenever a chip interrupt occurs. This function will check
* what interrupt events are pending and schedule additional jobs to
* service them. Once there are no more events waiting to be serviced
* (i.e. the chip has gone idle), interrupts will be unmasked so that
* the ISR can fire again.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void geiEndIntHandle
    (
    void * pArg
    )
    {
    QJOB *pJob;
    GEI_DRV_CTRL *pDrvCtrl;
    VXB_DEVICE_ID pDev;
    UINT32 status;
    int  loopCtr;

    pJob = pArg;
    pDrvCtrl = member_to_object (pJob, GEI_DRV_CTRL, geiIntJob);
    pDev = pDrvCtrl->geiDev;

    /* Read and acknowledge interrupts here. */ 

    status = CSR_READ_4(pDev, GEI_ICR) | pDrvCtrl->geiMoreRx;
    pDrvCtrl->geiMoreRx = 0;
 
    loopCtr = 1;
    if (status & GEI_RXINTRS)
	{
        if (pDrvCtrl->geiDevType == GEI_DEVTYPE_ADVANCED)
	    loopCtr = geiEndAdvRxHandle (pDrvCtrl);
        else
	    loopCtr = geiEndRxHandle (pDrvCtrl);
        if (loopCtr == 0)
            pDrvCtrl->geiMoreRx = GEI_RXINTRS;
	}


    if (status & GEI_TXINTRS)
	geiEndTxHandle (pDrvCtrl);

    /* May as well just do this one directly. */

    if ((status & GEI_LINKINTRS) && (pDrvCtrl->geiIntrs & GEI_LINKINTRS))
        geiLinkUpdate (pDev);

    /* Check for additional interrupt events */

    if (loopCtr == 0 || CSR_READ_4(pDev, GEI_ICS) & GEI_INTRS)
        {
        jobQueuePost (pDrvCtrl->geiJobQueue, &pDrvCtrl->geiIntJob);
        return;
        }

    vxAtomicSet (&pDrvCtrl->geiIntPending, FALSE);

    /* Unmask interrupts here */

    CSR_WRITE_4(pDev, GEI_IMS, pDrvCtrl->geiIntrs);

    return;
    }

/*****************************************************************************
*
* geiEndInt - handle device interrupts
*
* This function is invoked whenever the PRO/1000's interrupt line is asserted.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void geiEndInt
    (
    GEI_DRV_CTRL * pDrvCtrl
    )
    {
    VXB_DEVICE_ID pDev;

    pDev = pDrvCtrl->geiDev;

    /*
     * With our interrupt handling scheme, we want to check for
     * pending events here in the ISR, but we don't necessarily
     * want to clear them yet. This means we don't want to read
     * the ICR register here, since that will read and implicitly
     * acknowledge/clear pending events in one step. Reading the
     * ICS instead will allow us to read pending events without
     * this auto-ack behavior.
     */

    if (!(CSR_READ_4(pDev, GEI_ICS) & GEI_INTRS))
        return;

    if (!CSR_READ_4(pDev, GEI_IMS))
        return;

    vxAtomicSet (&pDrvCtrl->geiIntPending, TRUE);
    CSR_WRITE_4(pDev, GEI_IMC, GEI_INTRS);
    jobQueuePost (pDrvCtrl->geiJobQueue, &pDrvCtrl->geiIntJob);

    return;
    }

/*****************************************************************************
*
* geiEndAltInt - handle device interrupts
*
* This function is invoked whenever the PRO/1000's interrupt line is asserted.
* This is an alternate interrupt handler which is used when VxWorks is run
* in VMware or other simulated environment that does not correctly duplicate
* the actual of the behavior of the ICS register on real hardware.
*
* The Intel PRO/1000 has an ICR register, which is typically used to read
* and acknowledge interrupt events. Reading the ICR register shows which
* interrupt sources need servicing, and implicitly acknolwedges the events
* as well. The intent is to save driver software from having to do two
* interrupt accesses to read and cancel interrupts.
*
* Unfortunately, in VxWorks, this optimization has a drawback. We typically
* read the interrupt sources twice: once in the ISR (this routine) to see
* if events are actually pending, so we can decide whether or not to
* schedule a task-level handler, and again in the task-level handler code
* proper. But since reading the ICR register clears the interrupt event
* bits, reading ICR in the ISR routine forces us to preserve the ICR
* contents in software so that the task level handler can act on them.
* This is tricky to do correctly, and complicates the interrupt handling
* code.
*
* It turns out there's an alternate method we can use: the ICS register
* mirrors the contents of ICR register, and reading it will return the
* currently pending interrupt events without the auto-clear behavior of
* the ICR register itself.
*
* The problem is that all the Intel PRO/1000 manuals document the ICS
* register as write-only, stating that it can be used by software to
* manually trigger interrupts by forcing various event bits to be set. In
* spite of what the documentation says, reading the ICS register does
* return the set of currently pending interrupt events on all known
* PRO/1000 devices.
*
* Implementors of simulated PRO/1000 devices in virtualization software
* such as VMware have chosesn to implement ICS as specified in the Intel
* documentation rather than how it actually works in the hardware. The
* consequence is that this driver fails when used on these simulated
* PRO/1000 devices.
*
* To work around this, we provide an alternate ISR routine that avoids
* using the ICS register, using the IMS register only. We know that when
* all interrupts are masked off, a read of the IMS register will
* return 0. So we use the following logic:
*
* - If GEI_IMS is non-zero, then we were invoked because of an
*   interrupt event and the task level handler is not currently
*   running, so we mask off interrupts and dispatch it.
* - If GEI_IMS is zero (or none of the events we're interested
*   in are currently enabled), then we know the task level handler
*   is already running, and we were invoked because of an interrupt
*   triggered by another device that's sharing our interrupt line.
*   So we do nothing.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void geiEndAltInt
    (
    GEI_DRV_CTRL * pDrvCtrl
    )
    {
    VXB_DEVICE_ID pDev;

    pDev = pDrvCtrl->geiDev;

    if (CSR_READ_4(pDev, GEI_IMS) & GEI_INTRS)
        {
        /* Mask interrupts here */
        CSR_WRITE_4(pDev, GEI_IMC, GEI_INTRS);
        vxAtomicSet (&pDrvCtrl->geiIntPending, TRUE);
        jobQueuePost (pDrvCtrl->geiJobQueue, &pDrvCtrl->geiIntJob);
        }

    return;
    }

/******************************************************************************
*
* geiEndAdvRxHandle - process received frames with advanced descriptors
*
* This function is similar to geiEndRxHandle(), except it's designed
* to handle 'advanced' RX descriptors as used in the 82575 chip. These
* descriptors are the same size as those used by earlier chips, but have
* a different layout.
*
* Advanced format RX descriptors have both a 'read' and 'write' format.
* The read format is used when the driver provides a fresh descriptor
* with empty buffers for the controller to populate. The write format
* is written back by the chip and contains the packet RX status, packet
* info, and error bits. If header splitting is enabled, the read format
* can contain two buffer points: one of the frame payload, and one for
* just the header.
*
* Because the controller modifies the entire descriptor when it performs
* a writeback, it's important that we remember to properly reinitialize
* all of the descriptor fields when returning the descriptor to the chip
* after we're processed it.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL int geiEndAdvRxHandle
    (
    void * pArg
    )
    {
    GEI_DRV_CTRL *pDrvCtrl;
    VXB_DEVICE_ID pDev;
    M_BLK_ID pMblk;
    M_BLK_ID pNewMblk;
    GEI_ADV_RDESC * pDesc;
#ifdef GEI_VXB_DMA_BUF
    VXB_DMA_MAP_ID pMap;
#endif
    int loopCounter = GEI_MAX_RX;

    pDrvCtrl = pArg;
    pDev = pDrvCtrl->geiDev;

    pDesc = (GEI_ADV_RDESC *)&pDrvCtrl->geiRxDescMem[pDrvCtrl->geiRxIdx];

    while (loopCounter)
        {

        if (!(pDesc->write.gei_errsts & htole32(GEI_ADV_RDESC_STS_DD)))
            break;

#ifdef GEI_VXB_DMA_BUF
        pMap = pDrvCtrl->geiRxMblkMap[pDrvCtrl->geiRxIdx];
#endif
        pMblk = pDrvCtrl->geiRxMblk[pDrvCtrl->geiRxIdx];

        /*
         * Ignore checksum errors here. They'll be handled by the
         * checksum offload code below, or by the stack.
         */

        if (pDesc->write.gei_errsts & htole32(GEI_ADV_RDESC_ERRSUM))
            {
            logMsg("%s%d: bad packet, stsadv: %x (%p %d)\n", (int)GEI_NAME,
                pDev->unitNumber, le32toh(pDesc->write.gei_errsts), (int)pDesc,
                pDrvCtrl->geiRxIdx, 0);
            goto skip;
            }

        pNewMblk = endPoolTupleGet (pDrvCtrl->geiEndObj.pNetPool);

        if (pNewMblk == NULL)
            {
            logMsg("%s%d: out of mBlks at %d\n", (int)GEI_NAME,
                pDev->unitNumber, pDrvCtrl->geiRxIdx,0,0,0);
            pDrvCtrl->geiLastError.errCode = END_ERR_NO_BUF;
            muxError (&pDrvCtrl->geiEndObj, &pDrvCtrl->geiLastError);
skip:
#ifdef GEI_VXB_DMA_BUF
            pDesc->read.gei_addrlo =
                htole32(GEI_ADDR_LO(pMap->fragList[0].frag));
            pDesc->read.gei_addrhi =
                htole32(GEI_ADDR_HI(pMap->fragList[0].frag));
#else  /* GEI_VXB_DMA_BUF */
	    {
	    UINT32 addr = (UINT32)pMblk->m_data;
	    if (cacheUserFuncs.virtToPhysRtn)
	        addr = cacheUserFuncs.virtToPhysRtn (addr);
	    pDesc->read.gei_addrlo = htole32(addr);
	    pDesc->read.gei_addrhi = 0;
	    }
#endif /* GEI_VXB_DMA_BUF */
            pDesc->write.gei_errsts = 0;
	    pDesc->read.gei_hdrhi = 0;
            CSR_WRITE_4(pDev, GEI_RDT, pDrvCtrl->geiRxIdx);
            GEI_INC_DESC(pDrvCtrl->geiRxIdx, GEI_RX_DESC_CNT);
            loopCounter--;
            pDesc = (GEI_ADV_RDESC *)&pDrvCtrl->geiRxDescMem[pDrvCtrl->geiRxIdx];
            continue;
            }

        /* Sync the packet buffer and unload the map. */

#ifdef GEI_VXB_DMA_BUF
        vxbDmaBufSync (pDev, pDrvCtrl->geiMblkTag,
            pMap, VXB_DMABUFSYNC_PREREAD);
        vxbDmaBufMapUnload (pDrvCtrl->geiMblkTag, pMap);
#else
	/* pre-invalidate the replacement buffer */
	CACHE_USER_INVALIDATE (pNewMblk->m_data, pNewMblk->m_len);
#endif
        /* Swap the mBlks. */

        pDrvCtrl->geiRxMblk[pDrvCtrl->geiRxIdx] = pNewMblk;
        pNewMblk->m_next = NULL;
	/* Note, the data space lengths in pNewMblk were set by
	   endPoolTupleGet(). */
        GEI_ADJ (pNewMblk);

#ifdef GEI_VXB_DMA_BUF
        vxbDmaBufMapMblkLoad (pDev, pDrvCtrl->geiMblkTag, pMap, pNewMblk, 0);

        pDesc->read.gei_addrlo = htole32(GEI_ADDR_LO(pMap->fragList[0].frag));
        pDesc->read.gei_addrhi = htole32(GEI_ADDR_HI(pMap->fragList[0].frag));
#else  /* GEI_VXB_DMA_BUF */
	{
	UINT32 addr = (UINT32)pNewMblk->m_data;
	if (cacheUserFuncs.virtToPhysRtn)
	    addr = cacheUserFuncs.virtToPhysRtn (addr);
	pDesc->read.gei_addrlo = htole32(addr);
	pDesc->read.gei_addrhi = 0;
	}
#endif /* GEI_VXB_DMA_BUF */

        pMblk->m_len = pMblk->m_pkthdr.len = le16toh(pDesc->write.gei_len);
        pMblk->m_flags = M_PKTHDR|M_EXT;

        /* Handle checksum offload. */

        if (pDrvCtrl->geiCaps.cap_enabled & IFCAP_RXCSUM)
            {
            if (pDesc->write.gei_errsts & htole32(GEI_ADV_RDESC_STS_IPCS))
                pMblk->m_pkthdr.csum_flags |= CSUM_IP_CHECKED;
            if (!(pDesc->write.gei_errsts & htole32(GEI_ADV_RDESC_ERR_IPE)))
                pMblk->m_pkthdr.csum_flags |= CSUM_IP_VALID;
            if (pDesc->write.gei_errsts & htole32(GEI_ADV_RDESC_STS_TCPCS) &&
                !(pDesc->write.gei_errsts & htole32(GEI_ADV_RDESC_ERR_TCPE)))
                {
                pMblk->m_pkthdr.csum_flags |= CSUM_DATA_VALID|CSUM_PSEUDO_HDR;
                pMblk->m_pkthdr.csum_data = 0xffff;
                }
            }

        /* Handle VLAN tags */

        if (pDrvCtrl->geiCaps.cap_enabled & IFCAP_VLAN_HWTAGGING)
           {
           if (pDesc->write.gei_errsts & htole32(GEI_ADV_RDESC_STS_VLAN))
               {
               pMblk->m_pkthdr.csum_flags |= CSUM_VLAN;
               pMblk->m_pkthdr.vlan = le16toh(pDesc->write.gei_vlan);
               }
           }

        /* Reset this descriptor's status fields. */

        pDesc->write.gei_errsts = 0;
        pDesc->read.gei_hdrhi = 0;

        /* Advance to the next descriptor */

        CSR_WRITE_4(pDev, GEI_RDT, pDrvCtrl->geiRxIdx);
        GEI_INC_DESC(pDrvCtrl->geiRxIdx, GEI_RX_DESC_CNT);
        loopCounter--;

        END_RCV_RTN_CALL (&pDrvCtrl->geiEndObj, pMblk);

        pDesc = (GEI_ADV_RDESC *)&pDrvCtrl->geiRxDescMem[pDrvCtrl->geiRxIdx];
        }

    return loopCounter;
    }

/******************************************************************************
*
* geiEndRxHandle - process received frames
*
* This function is scheduled by the ISR to run in the context of tNetTask
* whenever an RX interrupt is received. It processes packets from the
* RX window and encapsulates them into mBlk tuples which are handed up
* to the MUX.
*
* There may be several packets waiting in the window to be processed.
* We take care not to process too many packets in a single run through
* this function so as not to monopolize tNetTask and starve out other
* jobs waiting in the jobQueue. If we detect that there's still more
* packets waiting to be processed, we queue ourselves up for another
* round of processing.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL int geiEndRxHandle
    (
    void * pArg
    )
    {
    GEI_DRV_CTRL *pDrvCtrl;
    VXB_DEVICE_ID pDev;
    M_BLK_ID pMblk;
    M_BLK_ID pNewMblk;
    GEI_RDESC * pDesc;
#ifdef GEI_VXB_DMA_BUF
    VXB_DMA_MAP_ID pMap;
#endif
    int loopCounter = GEI_MAX_RX;

    pDrvCtrl = pArg;
    pDev = pDrvCtrl->geiDev;

    pDesc = &pDrvCtrl->geiRxDescMem[pDrvCtrl->geiRxIdx];
#ifdef GEI_VXB_DMA_BUF
    pMap = pDrvCtrl->geiRxMblkMap[pDrvCtrl->geiRxIdx];
#endif
    while (loopCounter)
        {

        if (!(pDesc->gei_sts & GEI_RDESC_STS_DD))
            break;

        /*
         * Workaround for 82543 TBI compatibility bug. With 82543
         * copper adapters running at 1000Mbps, the chip will
         * sometimes report false CRC errors. If we detect one,
         * we check for the presence of the carrier extension byte
         * at the end. If we find this byte, then the frame is
         * actually good: we decrement the frame length by one
         * byte and accept the frame.
         */

        if (pDrvCtrl->geiTbiCompat && pDesc->gei_err == GEI_RDESC_ERR_CE)
            {
            UINT8 last;
            UINT16 rxLen;
            pMblk = pDrvCtrl->geiRxMblk[pDrvCtrl->geiRxIdx];
#ifdef GEI_VXB_DMA_BUF
            vxbDmaBufSync (pDev, pDrvCtrl->geiMblkTag,
                pMap, VXB_DMABUFSYNC_PREREAD);
#endif
            rxLen = le16toh(pDesc->gei_len);
#ifndef GEI_VXB_DMA_BUF
	    CACHE_USER_INVALIDATE (pMblk->m_data + (rxLen - 1), 1);
#endif
            last = pMblk->m_data[rxLen - 1];
            if (last == GEI_CARREXT)
                {
                rxLen -= 1;
                pDesc->gei_len = htole16(rxLen);
                pDesc->gei_err = 0;
                }
            }

        /*
         * Ignore checksum errors here. They'll be handled by the
         * checksum offload code below, or by the stack.
         */

        if (pDesc->gei_err & ~(GEI_RDESC_ERR_IPE|GEI_RDESC_ERR_TCPE))
            {
            logMsg("%s%d: bad packet, sts: %x (%p %d)\n", (int)GEI_NAME,
                pDev->unitNumber, pDesc->gei_err, (int)pDesc,
                pDrvCtrl->geiRxIdx, 0);
            goto skip;
            }

        pNewMblk = endPoolTupleGet (pDrvCtrl->geiEndObj.pNetPool);

        if (pNewMblk == NULL)
            {
            logMsg("%s%d: out of mBlks at %d\n", (int)GEI_NAME,
                pDev->unitNumber, pDrvCtrl->geiRxIdx,0,0,0);
            pDrvCtrl->geiLastError.errCode = END_ERR_NO_BUF;
            muxError (&pDrvCtrl->geiEndObj, &pDrvCtrl->geiLastError);
skip:
	    /* descriptor address field doesn't need to change */
            pDesc->gei_sts = 0;
            pDesc->gei_err = 0;
            CSR_WRITE_4(pDev, GEI_RDT, pDrvCtrl->geiRxIdx);
            GEI_INC_DESC(pDrvCtrl->geiRxIdx, GEI_RX_DESC_CNT);
            loopCounter--;
            pDesc = &pDrvCtrl->geiRxDescMem[pDrvCtrl->geiRxIdx];
#ifdef GEI_VXB_DMA_BUF
            pMap = pDrvCtrl->geiRxMblkMap[pDrvCtrl->geiRxIdx];
#endif
            continue;
            }

        /* Sync the packet buffer and unload the map. */

#ifdef GEI_VXB_DMA_BUF
        vxbDmaBufSync (pDev, pDrvCtrl->geiMblkTag,
            pMap, VXB_DMABUFSYNC_PREREAD);
        vxbDmaBufMapUnload (pDrvCtrl->geiMblkTag, pMap);
#else
	/* pre-invalidate the replacement buffer */
	CACHE_USER_INVALIDATE (pNewMblk->m_data, pNewMblk->m_len);
#endif
        /* Swap the mBlks. */

        pMblk = pDrvCtrl->geiRxMblk[pDrvCtrl->geiRxIdx];
        pDrvCtrl->geiRxMblk[pDrvCtrl->geiRxIdx] = pNewMblk;
        pNewMblk->m_next = NULL;
	/* Note, the data space lengths in pNewMblk were set by
	   endPoolTupleGet(). */
        GEI_ADJ (pNewMblk);

#ifdef GEI_VXB_DMA_BUF
        vxbDmaBufMapMblkLoad (pDev, pDrvCtrl->geiMblkTag, pMap, pNewMblk, 0);

#ifdef GEI_64
        pDesc->gei_addrlo = htole32(GEI_ADDR_LO(pMap->fragList[0].frag));
        pDesc->gei_addrhi = htole32(GEI_ADDR_HI(pMap->fragList[0].frag));
#else
        pDesc->gei_addrlo = htole32((UINT32)pMap->fragList[0].frag);
#endif
#else  /* GEI_VXB_DMA_BUF */
	{
	UINT32 addr = (UINT32)pNewMblk->m_data;
	if (cacheUserFuncs.virtToPhysRtn)
	    addr = cacheUserFuncs.virtToPhysRtn (addr);
	pDesc->gei_addrlo = htole32(addr);
	}
#endif /* GEI_VXB_DMA_BUF */


        pMblk->m_len = pMblk->m_pkthdr.len = le16toh(pDesc->gei_len);
        if (pDrvCtrl->geiTbiCompat)
            pMblk->m_len -= ETHER_CRC_LEN;
        pMblk->m_pkthdr.len = pMblk->m_len;
        pMblk->m_flags = M_PKTHDR|M_EXT;

        /* Handle checksum offload. */

        if (pDrvCtrl->geiCaps.cap_enabled & IFCAP_RXCSUM)
            {
            if (pDesc->gei_sts & GEI_RDESC_STS_IPCS)
                pMblk->m_pkthdr.csum_flags |= CSUM_IP_CHECKED;
            if (!(pDesc->gei_err & GEI_RDESC_ERR_IPE))
                pMblk->m_pkthdr.csum_flags |= CSUM_IP_VALID;
            if (pDesc->gei_sts & GEI_RDESC_STS_TCPCS &&
                !(pDesc->gei_err & GEI_RDESC_ERR_TCPE))
                {
                pMblk->m_pkthdr.csum_flags |= CSUM_DATA_VALID|CSUM_PSEUDO_HDR;
                pMblk->m_pkthdr.csum_data = 0xffff;
                }
            }

        /* Handle VLAN tags */

        if (pDrvCtrl->geiCaps.cap_enabled & IFCAP_VLAN_HWTAGGING)
           {
           if (pDesc->gei_sts & GEI_RDESC_STS_VP)
               {
               pMblk->m_pkthdr.csum_flags |= CSUM_VLAN;
               pMblk->m_pkthdr.vlan = le16toh(pDesc->gei_special);
               }
           }

        /* Reset this descriptor's status fields. */

        pDesc->gei_sts = 0;
        pDesc->gei_err = 0;

        /* Advance to the next descriptor */

        CSR_WRITE_4(pDev, GEI_RDT, pDrvCtrl->geiRxIdx);
        GEI_INC_DESC(pDrvCtrl->geiRxIdx, GEI_RX_DESC_CNT);
        loopCounter--;

        END_RCV_RTN_CALL (&pDrvCtrl->geiEndObj, pMblk);

        pDesc = &pDrvCtrl->geiRxDescMem[pDrvCtrl->geiRxIdx];
#ifdef GEI_VXB_DMA_BUF
        pMap = pDrvCtrl->geiRxMblkMap[pDrvCtrl->geiRxIdx];
#endif
        }

    return loopCounter;
    }

/******************************************************************************
*
* geiEndTbdClean - clean the TX ring
*
* This function is called from both geiEndSend() or geiEndTxHandle() to
* reclaim TX descriptors, and to free packets and DMA maps associated
* with completed transmits.  This routine must be called with the END
* TX semaphore already held.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void geiEndTbdClean
    (
    GEI_DRV_CTRL * pDrvCtrl
    )
    {
    VXB_DEVICE_ID pDev;
#ifdef GEI_VXB_DMA_BUF
    VXB_DMA_MAP_ID pMap;
#endif
#if !defined (GEI_64) || !defined (GEI_READ_TDH)
    GEI_TDESC * pDesc;
#endif
    M_BLK_ID pMblk;
#ifdef GEI_READ_TDH
    UINT32 curCons;
#endif

    pDev = pDrvCtrl->geiDev;

#ifdef GEI_READ_TDH
    curCons = CSR_READ_4(pDev, GEI_TDH);
#endif

    while (pDrvCtrl->geiTxFree < GEI_TX_DESC_CNT)
        {

#ifdef GEI_READ_TDH
        if (pDrvCtrl->geiTxCons == curCons && pDrvCtrl->geiTxFree)
            break;
#endif

	pDesc = &pDrvCtrl->geiTxDescMem[pDrvCtrl->geiTxCons];

#ifndef GEI_READ_TDH
        /*
         * With 'advanced' class adapters, completed context
         * descriptors never have the DD bit set.
         */

        if ((pDesc->gei_cmd & htole32(GEI_ADV_TDESC_CMD_DTYP)) ==
            htole32(GEI_ADV_TDESC_DTYP_CTX))
            goto skip;

        if ((pDesc->gei_sts & GEI_TDESC_STS_DD) == 0)
            break;
#endif

        pMblk = pDrvCtrl->geiTxMblk[pDrvCtrl->geiTxCons];

#ifdef GEI_VXB_DMA_BUF
        pMap = pDrvCtrl->geiTxMblkMap[pDrvCtrl->geiTxCons];
#endif
        if (pMblk != NULL)
            {
#ifdef GEI_VXB_DMA_BUF
            vxbDmaBufMapUnload (pDrvCtrl->geiMblkTag, pMap);
#endif
            endPoolTupleFree (pMblk);
            pDrvCtrl->geiTxMblk[pDrvCtrl->geiTxCons] = NULL;
            }
#ifndef GEI_READ_TDH
skip:
#endif
        pDrvCtrl->geiTxFree++;
        GEI_INC_DESC (pDrvCtrl->geiTxCons, GEI_TX_DESC_CNT);

#ifndef GEI_64
        /*
         * We forcibly clear out the addrhi field in the descriptor.
         * All other fields are always initialized each time a
         * descriptor is consumed, but the addrhi field is only
         * used for TCP/IP context setup descriptors. After a
         * context descriptor is completed, we need to reset the
         * addrhi field in case the descriptor is used again later
         * for a normal TX descriptor, otherwise we will end up
         * telling the chip to DMA from an invalid address.
         */

        pDesc->gei_addrhi = 0;
#endif

        }

    /*
     * If the number of still-in-use TX descriptors is less than
     * the number of descriptors used since the last mark, then
     * the last marked descriptor was cleaned, and we reset the
     * last marked count to maintain the invariant
     * pDrvCtrl->geiTxSinceMark <= GEI_TX_DESC_CNT - pDrvCtrl->geiTxFree
     */
    if (pDrvCtrl->geiTxSinceMark >
	GEI_TX_DESC_CNT - pDrvCtrl->geiTxFree)
	pDrvCtrl->geiTxSinceMark = GEI_TX_DESC_CNT - pDrvCtrl->geiTxFree;

    return;
    }

/******************************************************************************
*
* geiEndTxHandle - process TX completion events
*
* This function is scheduled by the ISR to run in the context of tNetTask
* whenever an TX interrupt is received. It runs through all the TX
* descriptoes that have been completed, as indicated by the TX head
* index register, and for each fully completed frame, the associated
* TX mBlk is released.
*
* If the transmitter has stalled, this routine will also call muxTxRestart()
* to drain any packets that may be waiting in the protocol send queues,
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

LOCAL void geiEndTxHandle
    (
    void * pArg
    )
    {
    GEI_DRV_CTRL *pDrvCtrl;
    UINT32 origFree;
    pDrvCtrl = pArg;

    END_TX_SEM_TAKE (&pDrvCtrl->geiEndObj, WAIT_FOREVER);
    origFree = pDrvCtrl->geiTxFree;
    geiEndTbdClean (pDrvCtrl);
    if (pDrvCtrl->geiTxStall && origFree < pDrvCtrl->geiTxFree)
	{
	pDrvCtrl->geiTxStall = FALSE;
	origFree = ~0UL; /* flag that we should TX restart */
	}
    END_TX_SEM_GIVE (&pDrvCtrl->geiEndObj); 

    if (origFree == (UINT32)~0)
        muxTxRestart (pDrvCtrl);

    return;
    }

/******************************************************************************
*
* gei544PcixWar - workaround for erratum 24 for 82544 devices
*
* The Intel 82544EI/GC Specification Update documents a possible system
* lockup condition that can occur when the controller is used on a PCI-X
* bus: the lockup occurs if the chip attempts to fetch a buffer that
* terminates with even doubleword alignment (0x0 - 0x3, 0x8 - 0xb).
*
* To work around this problem, we have to test the alignment of transmit
* buffers and if we find one that meets the problem criteria, we can
* prevent the lockup by splitting up the DMA so that the last 4 bytes
* are transfered in a separate descriptor.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/

#ifdef GEI_VXB_DMA_BUF
LOCAL GEI_TDESC * gei544PcixWar
    (
    GEI_DRV_CTRL * pDrvCtrl,
    VXB_DMA_MAP_ID pMap,
    int i
    )
    {
    GEI_TDESC * pDesc;

    pDesc = &pDrvCtrl->geiTxDescMem[pDrvCtrl->geiTxProd];
  
    if (pMap->fragList[i].fragLen > 4 &&
        !(((UINT32)pMap->fragList[i].frag + pMap->fragList[i].fragLen - 1) & 4))
        {
        /* Reduce previous buffer length by 4 */
        pMap->fragList[i].fragLen -= 4;
        pDesc->gei_cmd &= ~htole32(GEI_TDESC_CMD_LEN);
        pDesc->gei_cmd |= htole32(pMap->fragList[i].fragLen);

        /* Set up DMA of residual 4 bytes in another descriptor */
        pDrvCtrl->geiTxFree--;
        GEI_INC_DESC(pDrvCtrl->geiTxProd, GEI_TX_DESC_CNT);
        pDesc = &pDrvCtrl->geiTxDescMem[pDrvCtrl->geiTxProd];
#ifdef GEI_64
        pDesc->gei_addrlo = htole32(GEI_ADDR_LO(pMap->fragList[i].frag) +
            pMap->fragList[i].fragLen);
        pDesc->gei_addrhi = htole32(GEI_ADDR_HI(pMap->fragList[i].frag));
#else
        pDesc->gei_addrlo = htole32((UINT32)pMap->fragList[i].frag +
            pMap->fragList[i].fragLen);
#endif
        pDesc->gei_cmd = htole32(GEI_TDESC_CMD_DEXT|GEI_TDESC_DTYP_DSC|4);
        pDesc->gei_popts = 0;
        }

    return (pDesc);
    }

#else /* ! defined (GEI_VXB_DMA_BUF) */

LOCAL GEI_TDESC * gei544PcixWar
    (
    GEI_DRV_CTRL * pDrvCtrl,
    M_BLK * pMblk
    )
    {
    GEI_TDESC * pDesc;
    int len;
    UINT32 addr;

    pDesc = &pDrvCtrl->geiTxDescMem[pDrvCtrl->geiTxProd];

    len = pMblk->m_len;

    if (len > 4 && !(((UINT32)pMblk->m_data + len - 1) & 4))
        {
        /* Reduce previous buffer length by 4 */
        len -= 4;
        pDesc->gei_cmd &= ~htole32(GEI_TDESC_CMD_LEN);
        pDesc->gei_cmd |= htole32(len);

        /* Set up DMA of residual 4 bytes in another descriptor */
        pDrvCtrl->geiTxFree--;
        GEI_INC_DESC(pDrvCtrl->geiTxProd, GEI_TX_DESC_CNT);
        pDesc = &pDrvCtrl->geiTxDescMem[pDrvCtrl->geiTxProd];

	addr = (UINT32)pMblk->m_data + len;
	if (cacheUserFuncs.virtToPhysRtn)
	    addr = cacheUserFuncs.virtToPhysRtn (addr);
	pDesc->gei_addrlo = htole32(addr);
        pDesc->gei_cmd = htole32(GEI_TDESC_CMD_DEXT|GEI_TDESC_DTYP_DSC|4);
        pDesc->gei_popts = 0;
        }

    return (pDesc);
    }
#endif /* GEI_VXB_DMA_BUF */

/******************************************************************************
*
* geiEndAdvEncap - encapsulate an outbound packet using advanced descriptors
*
* This function is similar to the genEndEncap() routine, except it uses
* advanced format descriptors as found in the 82575 controller. The advanced
* TX data and TX context descriptors are used in much the same way with
* the 82575 as with earlier devices, however the layout is completely
* different.
*
* There is one key operational different related to VLAN tag insertion.
* With all previous devices, VLAN tags are specified using the TX data
* descriptor for every outbound frame. With the advanced descriptor
* format, the VLAN field has been moved to the context descriptor instead.
* This allows simplified VLAN tag insertion somewhat since the chip can
* re-use the same VLAN tag across multiple frames.
*
* RETURNS: ENOSPC if there are too many fragments in the packet, EAGAIN
* if the DMA ring is full, otherwise OK.
*
* ERRNO: N/A
*/

LOCAL int geiEndAdvEncap
    (
    GEI_DRV_CTRL * pDrvCtrl,
    M_BLK_ID pMblk
    )
    {
    VXB_DEVICE_ID pDev;
    GEI_ADV_TDESC * pDesc = NULL;
    GEI_ADV_CDESC * pCtx;
    UINT32 firstIdx, lastIdx = 0;
#ifndef GEI_VXB_DMA_BUF
    M_BLK * pTmp;
    UINT32 nFrags;
    UINT32 addr;
#else
    int i;
    VXB_DMA_MAP_ID pMap;
#endif
    UINT32 origFree;

    pDev = pDrvCtrl->geiDev;
    firstIdx = pDrvCtrl->geiTxProd;
    origFree = pDrvCtrl->geiTxFree;

#ifdef GEI_VXB_DMA_BUF
    pMap = pDrvCtrl->geiTxMblkMap[pDrvCtrl->geiTxProd];
#endif

    /* Set up a TCP/IP offload context descriptor if needed. */

    if (pMblk->m_pkthdr.csum_flags & (CSUM_VLAN|CSUM_IP|CSUM_UDP|
        CSUM_TCP|CSUM_UDPv6|CSUM_TCPv6))
        {
#ifdef CSUM_IPHDR_OFFSET
	int offsets = CSUM_IPHDR_OFFSET(pMblk) +
	    (CSUM_IP_HDRLEN (pMblk) << 8);
        if (pMblk->m_pkthdr.csum_flags != pDrvCtrl->geiLastCtx ||
            offsets != pDrvCtrl->geiLastOffsets ||
#else
       if (pMblk->m_pkthdr.csum_flags != pDrvCtrl->geiLastCtx ||
            CSUM_IP_HDRLEN (pMblk) != pDrvCtrl->geiLastIpLen ||
#endif
            ((pMblk->m_pkthdr.csum_flags & CSUM_VLAN) &&
            pMblk->m_pkthdr.vlan != pDrvCtrl->geiLastVlan))
            {
            pDrvCtrl->geiLastCtx = pMblk->m_pkthdr.csum_flags;
#ifdef CSUM_IPHDR_OFFSET
            pDrvCtrl->geiLastOffsets = offsets;
#else
            pDrvCtrl->geiLastIpLen = CSUM_IP_HDRLEN (pMblk);
#endif
            pDrvCtrl->geiLastVlan = pMblk->m_pkthdr.vlan;
            pCtx=(GEI_ADV_CDESC *)&pDrvCtrl->geiTxDescMem[pDrvCtrl->geiTxProd];
            pCtx->gei_cmd = htole32(GEI_ADV_TDESC_CMD_DEXT|
                GEI_ADV_TDESC_DTYP_CTX);
            if (pMblk->m_pkthdr.csum_flags & (CSUM_TCP|CSUM_TCPv6))
                pCtx->gei_cmd |= htole32(GEI_ADV_CDESC_L4T_TCP);
            if (pMblk->m_pkthdr.csum_flags & (CSUM_IP|CSUM_TCP|CSUM_UDP))
                pCtx->gei_cmd |= htole32(GEI_ADV_CDESC_CMD_IPV4);
            pCtx->gei_macip = htole16(GEI_ADV_MACLEN(SIZEOF_ETHERHEADER));
            pCtx->gei_macip |= htole16(GEI_ADV_IPLEN(CSUM_IP_HDRLEN(pMblk)));
            if (pMblk->m_pkthdr.csum_flags & CSUM_VLAN)
                pCtx->gei_vlan = htole16(pMblk->m_pkthdr.vlan);
            pDrvCtrl->geiTxFree--;
            GEI_INC_DESC(pDrvCtrl->geiTxProd, GEI_TX_DESC_CNT);
            }
        }

    /*
     * Load the DMA map to build the segment list.
     * This will fail if there are too many segments.
     */
#ifdef GEI_VXB_DMA_BUF
    if (vxbDmaBufMapMblkLoad (pDev, pDrvCtrl->geiMblkTag,
        pMap, pMblk, 0) != OK || (pMap->nFrags > pDrvCtrl->geiTxFree))
        {
        vxbDmaBufMapUnload (pDrvCtrl->geiMblkTag, pMap);
        return (ENOSPC); /* XXX what about the context descriptor? */
        }

    for (i = 0; i < pMap->nFrags; i++)
        {
        pDesc = (GEI_ADV_TDESC *)&pDrvCtrl->geiTxDescMem[pDrvCtrl->geiTxProd];
#ifdef GEI_64
        pDesc->gei_addrlo = htole32(GEI_ADDR_LO(pMap->fragList[i].frag));
        pDesc->gei_addrhi = htole32(GEI_ADDR_HI(pMap->fragList[i].frag));
#else
        pDesc->gei_addrlo = htole32((UINT32)pMap->fragList[i].frag);
#endif
        pDesc->gei_cmd = htole32(GEI_ADV_TDESC_CMD_DEXT|GEI_ADV_TDESC_DTYP_DSC|
            GEI_ADV_TDESC_CMD_RS);
        pDesc->gei_cmd |= htole32(pMap->fragList[i].fragLen);
	pDesc->gei_sts = 0;

        if (i == 0)
            {
            if (pMblk->m_pkthdr.csum_flags & CSUM_IP)
                pDesc->gei_sts |= htole32(GEI_ADV_POPT_IXSM);
            if (pMblk->m_pkthdr.csum_flags & (CSUM_UDP|CSUM_TCP|
                CSUM_TCPv6|CSUM_UDPv6))
                pDesc->gei_sts |= htole32(GEI_ADV_POPT_TXSM);
            if (pMblk->m_pkthdr.csum_flags & CSUM_VLAN)
                pDesc->gei_cmd |= htole32(GEI_ADV_TDESC_CMD_VLE);
            /*
             * Note: this field is optional for the 82575,
             * but mandatory for the 82576.
             */
            pDesc->gei_sts |=
                htole32(GEI_ADV_PAYLEN(pMblk->m_pkthdr.len));
            pDesc->gei_cmd |= htole32(GEI_ADV_TDESC_CMD_IFCS);
            }
        pDrvCtrl->geiTxFree--;
        lastIdx = pDrvCtrl->geiTxProd;
        GEI_INC_DESC(pDrvCtrl->geiTxProd, GEI_TX_DESC_CNT);
        }
#else /* ! defined (GEI_VXB_DMA_BUF) */

    nFrags = 0;
    for (pTmp = pMblk; pTmp != NULL; pTmp = pTmp->m_next)
	{
	if (pTmp->m_len != 0)
	    ++nFrags;
	}

    if (nFrags > pDrvCtrl->geiTxFree)
	return ENOSPC; /* XXX what about the context descriptor? */

    nFrags = 0;
    for (pTmp = pMblk; pTmp != NULL; pTmp = pTmp->m_next)
        {
	UINT32 cmd;

	if ((cmd = pTmp->m_len) == 0)
	    continue;
        pDesc = (GEI_ADV_TDESC *)&pDrvCtrl->geiTxDescMem[pDrvCtrl->geiTxProd];

	addr = (UINT32)pTmp->m_data;
	CACHE_USER_FLUSH (addr, cmd);
	if (cacheUserFuncs.virtToPhysRtn)
	    addr = cacheUserFuncs.virtToPhysRtn (addr);
	pDesc->gei_addrlo = htole32(addr);

        cmd |= GEI_ADV_TDESC_CMD_DEXT|GEI_ADV_TDESC_DTYP_DSC|
            GEI_ADV_TDESC_CMD_RS;

        pDesc->gei_cmd = htole32(cmd);
	pDesc->gei_sts = 0;
        if (nFrags++ == 0)
            {
            if (pMblk->m_pkthdr.csum_flags & CSUM_IP)
                pDesc->gei_sts |= htole32(GEI_ADV_POPT_IXSM);
            if (pMblk->m_pkthdr.csum_flags & (CSUM_UDP|CSUM_TCP|
                CSUM_TCPv6|CSUM_UDPv6))
                pDesc->gei_sts |= htole32(GEI_ADV_POPT_TXSM);
            if (pMblk->m_pkthdr.csum_flags & CSUM_VLAN)
                pDesc->gei_cmd |= htole32(GEI_ADV_TDESC_CMD_VLE);
            /*
             * Note: this field is optional for the 82575,
             * but mandatory for the 82576.
             */
            pDesc->gei_sts |=
                htole32(GEI_ADV_PAYLEN(pMblk->m_pkthdr.len));
            pDesc->gei_cmd |= htole32(GEI_ADV_TDESC_CMD_IFCS);
            }
        pDrvCtrl->geiTxFree--;
        lastIdx = pDrvCtrl->geiTxProd;
        GEI_INC_DESC(pDrvCtrl->geiTxProd, GEI_TX_DESC_CNT);
        }

#endif /* GEI_VXB_DMA_BUF */

    pDesc->gei_cmd |= htole32(GEI_ADV_TDESC_CMD_EOP|GEI_ADV_TDESC_CMD_RS);

    /* Save the mBlk for later. */

    pDrvCtrl->geiTxMblk[lastIdx] = pMblk;

#ifdef GEI_VXB_DMA_BUF
    /*
     * Ensure that the map for this transmission
     * is placed at the array index of the last descriptor
     * in this chain.  (Swap last and first dmamaps.)
     */

    pDrvCtrl->geiTxMblkMap[firstIdx] = pDrvCtrl->geiTxMblkMap[lastIdx];
    pDrvCtrl->geiTxMblkMap[lastIdx] = pMap;

    /* Sync the buffer. */

    vxbDmaBufSync (pDev, pDrvCtrl->geiMblkTag, pMap, VXB_DMABUFSYNC_POSTWRITE);
#endif
    return (OK);
    }

/******************************************************************************
*
* geiEndEncap - encapsulate an outbound packet in the TX DMA ring
*
* This function sets up a descriptor for a packet transmit operation.
* With the PRO/1000 ethrnet controller, the TX DMA ring consists of
* descriptors that each describe a single packet fragment. We consume
* as many descriptors as there are mBlks in the outgoing packet, unless
* the chain is too long. The length is limited by the number of DMA
* segments we want to allow in a given DMA map. If there are too many
* segments, this routine will fail, and the caller must coalesce the
* data into fewer buffers and try again.
*
* This routine will also fail if there aren't enough free descriptors
* available in the ring, in which case the caller must defer the
* transmission until more descriptors are completed by the chip.
*
* RETURNS: ENOSPC if there are too many fragments in the packet, EAGAIN
* if the DMA ring is full, otherwise OK.
*
* ERRNO: N/A
*/

LOCAL int geiEndEncap
    (
    GEI_DRV_CTRL * pDrvCtrl,
    M_BLK_ID pMblk
    )
    {
    VXB_DEVICE_ID pDev;
    GEI_TDESC * pDesc = NULL;
    GEI_CDESC * pCtx;
    UINT32 firstIdx, lastIdx = 0;
#ifndef GEI_VXB_DMA_BUF
    M_BLK * pTmp;
    UINT32 nFrags;
    UINT32 addr;
#else
    int i;
    VXB_DMA_MAP_ID pMap;
#endif
    UINT32 origFree;

    pDev = pDrvCtrl->geiDev;
    firstIdx = pDrvCtrl->geiTxProd;
    origFree = pDrvCtrl->geiTxFree;

#ifdef GEI_VXB_DMA_BUF
    pMap = pDrvCtrl->geiTxMblkMap[pDrvCtrl->geiTxProd];
#endif

    /* Set up a TCP/IP offload context descriptor if needed. */

    if (pMblk->m_pkthdr.csum_flags & (CSUM_IP|CSUM_UDP|
        CSUM_TCP|CSUM_UDPv6|CSUM_TCPv6))
        {
#ifdef CSUM_IPHDR_OFFSET
	int offsets = CSUM_IPHDR_OFFSET(pMblk) +
	    (CSUM_IP_HDRLEN (pMblk) << 8);
        if (pMblk->m_pkthdr.csum_flags != pDrvCtrl->geiLastCtx ||
            offsets != pDrvCtrl->geiLastOffsets)
#else
        if (pMblk->m_pkthdr.csum_flags != pDrvCtrl->geiLastCtx ||
            CSUM_IP_HDRLEN (pMblk) != pDrvCtrl->geiLastIpLen)
#endif
            {
	    pDrvCtrl->geiLastCtx = pMblk->m_pkthdr.csum_flags;
#ifdef CSUM_IPHDR_OFFSET
            pDrvCtrl->geiLastOffsets = offsets;
#else
            pDrvCtrl->geiLastIpLen = CSUM_IP_HDRLEN (pMblk);
#endif
            pCtx = (GEI_CDESC *)&pDrvCtrl->geiTxDescMem[pDrvCtrl->geiTxProd];
#ifdef GEI_READ_TDH
            pCtx->gei_cmd = htole32(GEI_TDESC_CMD_DEXT|GEI_TDESC_DTYP_CTX);
#else
            pCtx->gei_cmd = htole32(GEI_TDESC_CMD_DEXT|GEI_TDESC_DTYP_CTX|
				    GEI_TDESC_CMD_RS);
#endif
            if (pMblk->m_pkthdr.csum_flags & (CSUM_TCP|CSUM_TCPv6))
                pCtx->gei_cmd |= htole32(GEI_CDESC_CMD_TCP);
#ifdef CSUM_IPHDR_OFFSET
            pCtx->gei_ipcss = CSUM_IPHDR_OFFSET(pMblk);
#else
            pCtx->gei_ipcss = SIZEOF_ETHERHEADER;
#endif
            pCtx->gei_ipcso = pCtx->gei_ipcss + GEI_IP_CSUM_OFFSET;
            pCtx->gei_ipcse = pCtx->gei_ipcss + CSUM_IP_HDRLEN(pMblk) - 1;
            pCtx->gei_ipcse = htole16(pCtx->gei_ipcse);
            pCtx->gei_tucss = pCtx->gei_ipcss + CSUM_IP_HDRLEN(pMblk);
            pCtx->gei_tucso = pCtx->gei_tucss + CSUM_XPORT_CSUM_OFF(pMblk);
            pCtx->gei_tucse = pCtx->gei_hdrlen = pCtx->gei_mss = 0;
#ifndef GEI_READ_TDH
	    pCtx->gei_sts = 0;
#endif
            pDrvCtrl->geiTxFree--;
            GEI_INC_DESC(pDrvCtrl->geiTxProd, GEI_TX_DESC_CNT);
            }
        }

    /*
     * Load the DMA map to build the segment list.
     * This will fail if there are too many segments.
     */
#ifdef GEI_VXB_DMA_BUF
    if (vxbDmaBufMapMblkLoad (pDev, pDrvCtrl->geiMblkTag,
        pMap, pMblk, 0) != OK || 
        (pMap->nFrags * (pDrvCtrl->gei82544PcixWar == TRUE ? 2 : 1)) >
        pDrvCtrl->geiTxFree)
        {
        vxbDmaBufMapUnload (pDrvCtrl->geiMblkTag, pMap);
        return (ENOSPC); /* XXX what about the context descriptor? */
        }

    for (i = 0; i < pMap->nFrags; i++)
        {
        pDesc = &pDrvCtrl->geiTxDescMem[pDrvCtrl->geiTxProd];
#ifdef GEI_64
        pDesc->gei_addrlo = htole32(GEI_ADDR_LO(pMap->fragList[i].frag));
        pDesc->gei_addrhi = htole32(GEI_ADDR_HI(pMap->fragList[i].frag));
#else
        pDesc->gei_addrlo = htole32((UINT32)pMap->fragList[i].frag);
#endif
#ifdef GEI_READ_TDH
        pDesc->gei_cmd = htole32(GEI_TDESC_CMD_DEXT|GEI_TDESC_DTYP_DSC);
#else
        pDesc->gei_cmd = htole32(GEI_TDESC_CMD_DEXT|GEI_TDESC_DTYP_DSC|
				 GEI_TDESC_CMD_RS);
#endif
        pDesc->gei_cmd |= htole32(pMap->fragList[i].fragLen);
#ifndef GEI_READ_TDH
	pDesc->gei_sts = 0;
#endif
        pDesc->gei_popts = 0;
        if (i == 0)
            {
            if (pMblk->m_pkthdr.csum_flags & CSUM_IP)
                pDesc->gei_popts |= GEI_TDESC_OPT_IXSM;
            if (pMblk->m_pkthdr.csum_flags & (CSUM_UDP|CSUM_TCP|
                CSUM_TCPv6|CSUM_UDPv6))
                pDesc->gei_popts |= GEI_TDESC_OPT_TXSM;
            if (pMblk->m_pkthdr.csum_flags & CSUM_VLAN)
                {
                pDesc->gei_cmd |= htole32(GEI_TDESC_CMD_VLE);
                pDesc->gei_special = htole16(pMblk->m_pkthdr.vlan);
                }
	    pDesc->gei_cmd |= htole32(GEI_TDESC_CMD_IFCS);
            }
        if (pDrvCtrl->gei82544PcixWar == TRUE)
            pDesc = gei544PcixWar (pDrvCtrl, pMap, i);
        pDrvCtrl->geiTxFree--;
        lastIdx = pDrvCtrl->geiTxProd;
        GEI_INC_DESC(pDrvCtrl->geiTxProd, GEI_TX_DESC_CNT);
        }
#else /* ! defined (GEI_VXB_DMA_BUF) */

    nFrags = 0;
    for (pTmp = pMblk; pTmp != NULL; pTmp = pTmp->m_next)
	{
	if (pTmp->m_len != 0)
	    ++nFrags;
	}

    if (nFrags * (pDrvCtrl->gei82544PcixWar ? 2 : 1) > pDrvCtrl->geiTxFree)
	return ENOSPC; /* XXX what about the context descriptor? */

    nFrags = 0;
    for (pTmp = pMblk; pTmp != NULL; pTmp = pTmp->m_next)
        {
	UINT32 cmd;

	if ((cmd = pTmp->m_len) == 0)
	    continue;
        pDesc = &pDrvCtrl->geiTxDescMem[pDrvCtrl->geiTxProd];

	addr = (UINT32)pTmp->m_data;
	CACHE_USER_FLUSH (addr, cmd);
	if (cacheUserFuncs.virtToPhysRtn)
	    addr = cacheUserFuncs.virtToPhysRtn (addr);
	pDesc->gei_addrlo = htole32(addr);

#ifdef GEI_READ_TDH
        cmd |= GEI_TDESC_CMD_DEXT|GEI_TDESC_DTYP_DSC;
#else
        cmd |= GEI_TDESC_CMD_DEXT|GEI_TDESC_DTYP_DSC| GEI_TDESC_CMD_RS;
#endif
        pDesc->gei_cmd = htole32(cmd);
#ifndef GEI_READ_TDH
	pDesc->gei_sts = 0;
#endif
        pDesc->gei_popts = 0;
        if (nFrags++ == 0)
            {
            if (pMblk->m_pkthdr.csum_flags & CSUM_IP)
                pDesc->gei_popts |= GEI_TDESC_OPT_IXSM;
            if (pMblk->m_pkthdr.csum_flags & (CSUM_UDP|CSUM_TCP|
                CSUM_TCPv6|CSUM_UDPv6))
                pDesc->gei_popts |= GEI_TDESC_OPT_TXSM;
            if (pMblk->m_pkthdr.csum_flags & CSUM_VLAN)
                {
                pDesc->gei_cmd |= htole32(GEI_TDESC_CMD_VLE);
                pDesc->gei_special = htole16(pMblk->m_pkthdr.vlan);
                }
	        pDesc->gei_cmd |= htole32(GEI_TDESC_CMD_IFCS);
            }
        if (pDrvCtrl->gei82544PcixWar)
            pDesc = gei544PcixWar (pDrvCtrl, pTmp);
        pDrvCtrl->geiTxFree--;
        lastIdx = pDrvCtrl->geiTxProd;
        GEI_INC_DESC(pDrvCtrl->geiTxProd, GEI_TX_DESC_CNT);
        }

#endif /* GEI_VXB_DMA_BUF */

    /*
     * We need to set the RPS bit for the 82543 and 82544
     * adapters, otherwise the interrupt deferral scheme
     * we use here might cause the NIC to stall when
     * transmitting at a high frame rate. Per Intel
     * documentation, this bit is only supported for the
     * 82544 and 82543, and should be set to 0 for all
     * other devices.
     */

    if (pDrvCtrl->geiDevId <= INTEL_DEVICEID_82544GC_LOM)
        pDesc->gei_cmd |= htole32(GEI_TDESC_CMD_RPS);

    if (pDrvCtrl->geiTxSinceMark >= GEI_TX_MARK_THRESH)
	{
	/* Mark this descriptor to generate an interrupt without delay
	   when done. */	   
	pDesc->gei_cmd |= htole32(GEI_TDESC_CMD_EOP|GEI_TDESC_CMD_RS);
	pDrvCtrl->geiTxSinceMark = 0;
	}
    else
	{
	pDesc->gei_cmd |=
	    htole32(GEI_TDESC_CMD_EOP|GEI_TDESC_CMD_IDE|GEI_TDESC_CMD_RS);
	/* increase geiTxSinceMark by the number of descriptors used. */
	pDrvCtrl->geiTxSinceMark += (origFree - pDrvCtrl->geiTxFree);
	}

    /* Save the mBlk for later. */

    pDrvCtrl->geiTxMblk[lastIdx] = pMblk;

#ifdef GEI_VXB_DMA_BUF
    /*
     * Insure that the map for this transmission
     * is placed at the array index of the last descriptor
     * in this chain.  (Swap last and first dmamaps.)
     */

    pDrvCtrl->geiTxMblkMap[firstIdx] = pDrvCtrl->geiTxMblkMap[lastIdx];
    pDrvCtrl->geiTxMblkMap[lastIdx] = pMap;

    /* Sync the buffer. */

    vxbDmaBufSync (pDev, pDrvCtrl->geiMblkTag, pMap, VXB_DMABUFSYNC_POSTWRITE);
#endif
    return (OK);
    }

/******************************************************************************
*
* geiEndSend - transmit a packet
*
* This function transmits the packet specified in <pMblk>.
*
* RETURNS: OK, ERROR, or END_ERR_BLOCK.
*
* ERRNO: N/A
*/

LOCAL int geiEndSend
    (
    END_OBJ * pEnd,
    M_BLK_ID pMblk
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;
    VXB_DEVICE_ID pDev;
    M_BLK_ID pTmp;
    int rval;

    pDrvCtrl = (GEI_DRV_CTRL *)pEnd;

    pDev = pDrvCtrl->geiDev;

    END_TX_SEM_TAKE (pEnd, WAIT_FOREVER);

    if (!pDrvCtrl->geiTxFree || !(pDrvCtrl->geiCurStatus & IFM_ACTIVE))
        goto blocked;

    /* Attempt early cleanup */

    if (pDrvCtrl->geiTxFree < GEI_TX_DESC_CNT - GEI_TX_CLEAN_THRESH)
        geiEndTbdClean (pDrvCtrl);

    /*
     * First, try to do an in-place transmission, using
     * gather-write DMA.
     */

    if (pDrvCtrl->geiDevType == GEI_DEVTYPE_ADVANCED)
        rval = geiEndAdvEncap (pDrvCtrl, pMblk);
    else
        rval = geiEndEncap (pDrvCtrl, pMblk);

    /*
     * If geiEndEncap() returns ENOSPC, it means it ran out
     * of TX descriptors and couldn't encapsulate the whole
     * packet fragment chain. In that case, we need to
     * coalesce everything into a single buffer and try
     * again. If any other error is returned, then something
     * went wrong, and we have to abort the transmission
     * entirely.
     */

    if (rval == ENOSPC)
        {
        if ((pTmp = endPoolTupleGet (pDrvCtrl->geiEndObj.pNetPool)) == NULL)
            goto blocked;
        pTmp->m_len = pTmp->m_pkthdr.len =
            netMblkToBufCopy (pMblk, mtod(pTmp, char *), NULL);
        pTmp->m_flags = pMblk->m_flags;
        pTmp->m_pkthdr.csum_flags = pMblk->m_pkthdr.csum_flags;
        pTmp->m_pkthdr.csum_data = pMblk->m_pkthdr.csum_data;
        pTmp->m_pkthdr.vlan = pMblk->m_pkthdr.vlan;
        CSUM_IP_HDRLEN(pTmp) = CSUM_IP_HDRLEN(pMblk);
#ifdef CSUM_IPHDR_OFFSET
        CSUM_IPHDR_OFFSET(pTmp) = CSUM_IPHDR_OFFSET(pMblk);
#endif

        /* Try transmission again, should succeed this time. */

        if (pDrvCtrl->geiDevType == GEI_DEVTYPE_ADVANCED)
            rval = geiEndAdvEncap (pDrvCtrl, pTmp);
        else
            rval = geiEndEncap (pDrvCtrl, pTmp);

        if (rval == OK)
            endPoolTupleFree (pMblk);
        else
            endPoolTupleFree (pTmp);
        }

    /* Issue transmit command */

    CSR_WRITE_4(pDev, GEI_TDT, pDrvCtrl->geiTxProd);

    if (rval != OK)
        goto blocked;

    END_TX_SEM_GIVE (pEnd);
    return (OK);

blocked:

    pDrvCtrl->geiTxStall = TRUE;
    END_TX_SEM_GIVE (pEnd);

    return (END_ERR_BLOCK);
    }

/******************************************************************************
*
* geiEndPollSend - polled mode transmit routine
*
* This function is similar to the geiEndSend() routine shown above, except
* it performs transmissions synchronously with interrupts disabled. After
* the transmission is initiated, the routine will poll the state of the
* TX status register associated with the current slot until transmission
* completed. If transmission times out, this routine will return ERROR.
*
* RETURNS: OK, EAGAIN, or ERROR
*
* ERRNO: N/A
*/

LOCAL STATUS geiEndPollSend
    (
    END_OBJ * pEnd,
    M_BLK_ID pMblk
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;
    VXB_DEVICE_ID pDev;
#ifdef GEI_VXB_DMA_BUF
    VXB_DMA_MAP_ID pMap;
#endif
    GEI_TDESC * pDesc;
    M_BLK_ID pTmp;
    int len, i;

    pDrvCtrl = (GEI_DRV_CTRL *)pEnd;

    if (pDrvCtrl->geiPolling == FALSE)
        return (ERROR);

    pDev = pDrvCtrl->geiDev;
    pTmp = pDrvCtrl->geiPollBuf;

    len = netMblkToBufCopy (pMblk, mtod(pTmp, char *), NULL);
    pTmp->m_len = pTmp->m_pkthdr.len = len;
    pTmp->m_flags = pMblk->m_flags;
    pTmp->m_pkthdr.csum_flags = pMblk->m_pkthdr.csum_flags;
    pTmp->m_pkthdr.csum_data = pMblk->m_pkthdr.csum_data;
    pTmp->m_pkthdr.vlan = pMblk->m_pkthdr.vlan;
    CSUM_IP_HDRLEN(pTmp) = CSUM_IP_HDRLEN(pMblk);
 
    /* Encapsulate buffer */

    if (pDrvCtrl->geiDevType == GEI_DEVTYPE_ADVANCED)
        i = geiEndAdvEncap (pDrvCtrl, pTmp);
    else
        i = geiEndEncap (pDrvCtrl, pTmp);

    if (i != OK)
        return (EAGAIN);

    /* Issue transmit command */

    CSR_WRITE_4(pDev, GEI_TDT, pDrvCtrl->geiTxProd);

    /* Wait for transmission to complete */

    for (i = 0; i < GEI_TIMEOUT; i++)
        {
        if (CSR_READ_4(pDev, GEI_TDH) == CSR_READ_4(pDev, GEI_TDT))
            break;
        }

    CSR_WRITE_4(pDev, GEI_ICR, GEI_TXINTRS);

    /* Remember to unload the map once transmit completes. */
 
    pDesc = &pDrvCtrl->geiTxDescMem[pDrvCtrl->geiTxCons];
    FOREVER
        {
        pDesc = &pDrvCtrl->geiTxDescMem[pDrvCtrl->geiTxCons];
        pDesc->gei_addrhi = 0;
        pTmp = pDrvCtrl->geiTxMblk[pDrvCtrl->geiTxCons];
        if (pDrvCtrl->geiTxMblk[pDrvCtrl->geiTxCons] != NULL)
            {
#ifdef GEI_VXB_DMA_BUF
            pMap = pDrvCtrl->geiTxMblkMap[pDrvCtrl->geiTxCons];
            vxbDmaBufMapUnload (pDrvCtrl->geiMblkTag, pMap);
#endif
            pDrvCtrl->geiTxMblk[pDrvCtrl->geiTxCons] = NULL;
            }
        pDrvCtrl->geiTxFree++;
        GEI_INC_DESC(pDrvCtrl->geiTxCons, GEI_TX_DESC_CNT);
        if (pDesc->gei_cmd & htole32(GEI_TDESC_CMD_EOP))
            break;
        }

    if (i == GEI_TIMEOUT)
        return (ERROR);

    return (OK);
    }

/******************************************************************************
*
* geiEndAdvPollReceive - advanced polled mode receive routine
*
* This function handles polled mode reception of frames on controllers
* with advanced format RX descriptors, such as the 82575.
*
* If no packet is available, this routine will return EAGAIN. If the
* supplied mBlk is too small to contain the received frame, the routine
* will return ERROR.
*
* RETURNS: OK, EAGAIN, or ERROR
*
* ERRNO: N/A
*/

LOCAL int geiEndAdvPollReceive
    (
    END_OBJ * pEnd,
    M_BLK_ID pMblk
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;
    VXB_DEVICE_ID pDev;
#ifdef GEI_VXB_DMA_BUF
    VXB_DMA_MAP_ID pMap;
#endif
    GEI_ADV_RDESC * pDesc;
    M_BLK_ID pPkt;
    int rval = EAGAIN;
    UINT16 rxLen = 0;

    pDrvCtrl = (GEI_DRV_CTRL *)pEnd;
    if (pDrvCtrl->geiPolling == FALSE)
        return (ERROR);

    if (!(pMblk->m_flags & M_EXT))
        return (ERROR);

    pDev = pDrvCtrl->geiDev;

    pDesc = (GEI_ADV_RDESC *)&pDrvCtrl->geiRxDescMem[pDrvCtrl->geiRxIdx];

    if (!(pDesc->write.gei_errsts & htole32(GEI_ADV_RDESC_STS_DD)))
        return (EAGAIN);

    CSR_WRITE_4(pDev, GEI_ICR, GEI_RXINTRS);

    pPkt = pDrvCtrl->geiRxMblk[pDrvCtrl->geiRxIdx];
#ifdef GEI_VXB_DMA_BUF
    pMap = pDrvCtrl->geiRxMblkMap[pDrvCtrl->geiRxIdx];
#endif

    rxLen = le16toh(pDesc->write.gei_len);

#ifdef GEI_VXB_DMA_BUF
    vxbDmaBufSync (pDev, pDrvCtrl->geiMblkTag, pMap, VXB_DMABUFSYNC_PREREAD);
#endif

    /*
     * Ignore checksum errors here. They'll be handled by the
     * checksum offload code below, or by the stack.
     */

    if (!(pDesc->write.gei_errsts & htole32(GEI_ADV_RDESC_ERRSUM)))
        {
        /* Handle checksum offload. */
 
        if (pDrvCtrl->geiCaps.cap_enabled & IFCAP_RXCSUM)
            {
            if (pDesc->write.gei_errsts & htole32(GEI_ADV_RDESC_STS_IPCS))
                pMblk->m_pkthdr.csum_flags |= CSUM_IP_CHECKED;
            if (!(pDesc->write.gei_errsts & htole32(GEI_ADV_RDESC_ERR_IPE)))
                pMblk->m_pkthdr.csum_flags |= CSUM_IP_VALID;
            if (pDesc->write.gei_errsts & htole32(GEI_ADV_RDESC_STS_TCPCS) &&
                !(pDesc->write.gei_errsts & htole32(GEI_ADV_RDESC_ERR_TCPE)))
                {
                pMblk->m_pkthdr.csum_flags |= CSUM_DATA_VALID|CSUM_PSEUDO_HDR;
                pMblk->m_pkthdr.csum_data = 0xffff;
                }
            }

        /* Handle VLAN tags */

        if (pDrvCtrl->geiCaps.cap_enabled & IFCAP_VLAN_HWTAGGING)
           {
           if (pDesc->write.gei_errsts & htole32(GEI_ADV_RDESC_STS_VLAN))
               {
               pMblk->m_pkthdr.csum_flags |= CSUM_VLAN;
               pMblk->m_pkthdr.vlan = le16toh(pDesc->write.gei_vlan);
               }
           }

        pMblk->m_flags |= M_PKTHDR;
        pMblk->m_len = pMblk->m_pkthdr.len = rxLen;
        pMblk->m_data += 2;
        bcopy (mtod(pPkt, char *), mtod(pMblk, char *), rxLen);
        rval = OK;
        }

#ifndef GEI_VXB_DMA_BUF
    /*
     * Pre-invalidate the re-used buffer. Note that pPkt->m_len
     * was set by endPoolTupleGet(), and hasn't been changed,
     * but pPkt->m_data was adjusted up by 2 bytes.
     */
    CACHE_DMA_INVALIDATE (pPkt->m_data - 2, pPkt->m_len);
#endif    

    /* Reset the descriptor */

#ifdef GEI_VXB_DMA_BUF
    pDesc->read.gei_addrlo = htole32(GEI_ADDR_LO(pMap->fragList[0].frag));
    pDesc->read.gei_addrhi = htole32(GEI_ADDR_HI(pMap->fragList[0].frag));
#else  /* GEI_VXB_DMA_BUF */
    {
    UINT32 addr = (UINT32)pPkt->m_data;
    if (cacheUserFuncs.virtToPhysRtn)
        addr = cacheUserFuncs.virtToPhysRtn (addr);
    pDesc->read.gei_addrlo = htole32(addr);
    pDesc->read.gei_addrhi = 0;
    }
#endif /* GEI_VXB_DMA_BUF */
    pDesc->write.gei_errsts = 0;
    pDesc->read.gei_hdrhi = 0;

    CSR_WRITE_4(pDev, GEI_RDT, pDrvCtrl->geiRxIdx);
    GEI_INC_DESC(pDrvCtrl->geiRxIdx, GEI_RX_DESC_CNT);

    return (rval);
    }

/******************************************************************************
*
* geiEndPollReceive - polled mode receive routine
*
* This function receive a packet in polled mode, with interrupts disabled.
* It's similar in operation to the geiEndRxHandle() routine, except it
* doesn't process more than one packet at a time and does not load out
* buffers. Instead, the caller supplied an mBlk tuple into which this
* function will place the received packet.
*
* If no packet is available, this routine will return EAGAIN. If the
* supplied mBlk is too small to contain the received frame, the routine
* will return ERROR.
*
* RETURNS: OK, EAGAIN, or ERROR
*
* ERRNO: N/A
*/

LOCAL int geiEndPollReceive
    (
    END_OBJ * pEnd,
    M_BLK_ID pMblk
    )
    {
    GEI_DRV_CTRL * pDrvCtrl;
    VXB_DEVICE_ID pDev;
#ifdef GEI_VXB_DMA_BUF
    VXB_DMA_MAP_ID pMap;
#endif
    GEI_RDESC * pDesc;
    M_BLK_ID pPkt;
    int rval = EAGAIN;
    UINT16 rxLen = 0;

    pDrvCtrl = (GEI_DRV_CTRL *)pEnd;

    if (pDrvCtrl->geiDevType == GEI_DEVTYPE_ADVANCED)
        return (geiEndAdvPollReceive (pEnd, pMblk));

    if (pDrvCtrl->geiPolling == FALSE)
        return (ERROR);

    if (!(pMblk->m_flags & M_EXT))
        return (ERROR);

    pDev = pDrvCtrl->geiDev;

    pDesc = &pDrvCtrl->geiRxDescMem[pDrvCtrl->geiRxIdx];

    if (!(pDesc->gei_sts & GEI_RDESC_STS_DD))
        return (EAGAIN);

    CSR_WRITE_4(pDev, GEI_ICR, GEI_RXINTRS);

    pPkt = pDrvCtrl->geiRxMblk[pDrvCtrl->geiRxIdx];
#ifdef GEI_VXB_DMA_BUF
    pMap = pDrvCtrl->geiRxMblkMap[pDrvCtrl->geiRxIdx];
#endif

    rxLen = le16toh(pDesc->gei_len);

#ifdef GEI_VXB_DMA_BUF
    vxbDmaBufSync (pDev, pDrvCtrl->geiMblkTag, pMap, VXB_DMABUFSYNC_PREREAD);
#endif

    /* Handle 82543 TBI compatibility workaround. */

    if (pDrvCtrl->geiTbiCompat)
	{
	if (pDesc->gei_err == GEI_RDESC_ERR_CE)
	    {
	    UINT8 last;
	    last = pPkt->m_data[rxLen - 1];
	    if (last == GEI_CARREXT)
		{
		rxLen -= 1;
		pDesc->gei_err = 0;
		}
	    }
	rxLen -= ETHER_CRC_LEN;
	}

    /*
     * Ignore checksum errors here. They'll be handled by the
     * checksum offload code below, or by the stack.
     */

    if (!(pDesc->gei_err & ~(GEI_RDESC_ERR_IPE|GEI_RDESC_ERR_TCPE)))
        {
        /* Handle checksum offload. */
 
        if (pDrvCtrl->geiCaps.cap_enabled & IFCAP_RXCSUM)
            {
            if (pDesc->gei_sts & GEI_RDESC_STS_IPCS)
                pMblk->m_pkthdr.csum_flags |= CSUM_IP_CHECKED;
            if (!(pDesc->gei_err & GEI_RDESC_ERR_IPE))
                pMblk->m_pkthdr.csum_flags |= CSUM_IP_VALID;
            if (pDesc->gei_sts & GEI_RDESC_STS_TCPCS &&
                !(pDesc->gei_err & GEI_RDESC_ERR_TCPE))
                {
                pMblk->m_pkthdr.csum_flags |= CSUM_DATA_VALID|CSUM_PSEUDO_HDR;
                pMblk->m_pkthdr.csum_data = 0xffff;
                }
            }

        /* Handle VLAN tags */

        if (pDrvCtrl->geiCaps.cap_enabled & IFCAP_VLAN_HWTAGGING)
           {
           if (pDesc->gei_sts & GEI_RDESC_STS_VP)
               {
               pMblk->m_pkthdr.csum_flags |= CSUM_VLAN;
               pMblk->m_pkthdr.vlan = le16toh(pDesc->gei_special);
               }
           }

        pMblk->m_flags |= M_PKTHDR;
        pMblk->m_len = pMblk->m_pkthdr.len = rxLen;
        pMblk->m_data += 2;
        bcopy (mtod(pPkt, char *), mtod(pMblk, char *), rxLen);
        rval = OK;
        }

#ifndef GEI_VXB_DMA_BUF
    /*
     * Pre-invalidate the re-used buffer. Note that pPkt->m_len
     * was set by endPoolTupleGet(), and hasn't been changed,
     * but pPkt->m_data was adjusted up by 2 bytes.
     */
    CACHE_DMA_INVALIDATE (pPkt->m_data - 2, pPkt->m_len);
#endif    

    /* Reset the descriptor */

    pDesc->gei_sts = 0;
    pDesc->gei_err = 0;
    CSR_WRITE_4(pDev, GEI_RDT, pDrvCtrl->geiRxIdx);
    GEI_INC_DESC(pDrvCtrl->geiRxIdx, GEI_RX_DESC_CNT);

    return (rval);
    }

LOCAL void geiDelay
    (
    UINT32 usec
    )
    {
    vxbUsDelay (usec);
    return;
    }
