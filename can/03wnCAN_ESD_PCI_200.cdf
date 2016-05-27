/* 02wnCAN_ESD_PCI_200.cdf -  component description file */

/* Copyright 2001-2003 Wind River Systems, Inc. */

/*
modification history
--------------------
19dec02,emcw     Added DevIO parameters
17dec01,jac      written

DESCRIPTION
This file contains descriptions for the ESD CAN PCI 200 drivers.

Multiple boards can be configured, using this component, by
specifying the parameter MAX_ESD_CAN_PCI_200_BOARDS.
*/

Component INCLUDE_ESD_CAN_PCI_200 {
        NAME            ESD PCI/200
        SYNOPSIS        ESD PCI/200 board including two sja1000 controllers
        INIT_RTN        wncan_esd_pci_200_init();
        _CHILDREN       SELECT_CAN_BOARDS
        CFG_PARAMS      MAX_ESD_CAN_PCI_200_BOARDS \
                        ESD_CAN_PCI_200_DEVIO_NAME
        CONFIGLETTES    CAN/esd_pci_200_cfg.c
        REQUIRES        INCLUDE_ESD_CAN_PCI_200_2 \
                        INCLUDE_CAN_NETWORK_INIT
        INCLUDE_WHEN    INCLUDE_ESD_CAN_PCI_200_2
        MODULES         esd_pci_200.o
}

Component INCLUDE_ESD_CAN_PCI_200_2 {
        NAME            ESD PCI/200 (2)
        SYNOPSIS        ESD PCI/200 board including two sja1000 controllers (2)
        INIT_RTN        wncan_esd_pci_200_init2();
        CONFIGLETTES    CAN/sys_esd_pci_200.c
        _CHILDREN       FOLDER_CAN_SUPPORT
        REQUIRES        INCLUDE_ESD_CAN_PCI_200
}

Parameter MAX_ESD_CAN_PCI_200_BOARDS
        {
        NAME            ESD PCI/200 MaxNumBrds
        SYNOPSIS        Maximum number of ESD PCI/200 boards that can be used
        TYPE            uint
        DEFAULT         1
        }

Parameter ESD_CAN_PCI_200_DEVIO_NAME
        {
        NAME            ESD PCI/200 Device I/O Board Name
        SYNOPSIS        Device I/O name, one for every board, no leading slash, separated by spaces
        TYPE            string
        DEFAULT         "esd_pci_200_0"
        }
