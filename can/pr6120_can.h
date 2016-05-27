/* esd_pci_200.h - definitions for ESD PCI/200 board */

/* Copyright 2001 Wind River Systems, Inc. */

/* 
modification history 
--------------------
20dec01, dnb written

*/

/* 

DESCRIPTION
This file contains definitions used only in esd_pci_200.
and esd_pci_200_cfg.c 

*/
#ifndef ESD_PCI_200_H
#define ESD_PCI_200_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define ESD_PCI_200_MAX_CONTROLLERS (2)


struct ESD_PCI_200_ChannelData
{
   WNCAN_ChannelMode sja1000chnMode[SJA1000_MAX_MSG_OBJ];
};


struct ESD_PCI_200_DeviceEntry
{
    struct WNCAN_Device             canDevice[ESD_PCI_200_MAX_CONTROLLERS];
    struct ESD_PCI_200_ChannelData  chData[ESD_PCI_200_MAX_CONTROLLERS];
    struct WNCAN_Controller         canControllerArray[ESD_PCI_200_MAX_CONTROLLERS];
    struct WNCAN_Board              canBoard;
    struct TxMsg                    txMsg[ESD_PCI_200_MAX_CONTROLLERS];
    int                             bus;
    int                             dev;
    int                             func;
    BOOL                            inUse;
    BOOL                            allocated[ESD_PCI_200_MAX_CONTROLLERS];
    BOOL                            intConnect;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ESD_PCI_200_H */
