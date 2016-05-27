/* 03wnCAN_PR6120_CAN.cdf -  component description file */

/*
modification history
--------------------
2016/05/27 Jonah Liu Created on base of ESD_PCI_200

DESCRIPTION
This file contains descriptions for the PR6120 CAN drivers.

Multiple boards can be configured, using this component, by
specifying the parameter MAX_PR6120_CAN_BOARDS.

*/

Component INCLUDE_PR6120_CAN {
        NAME            PR6120 CAN
        SYNOPSIS        PR6120 CAN board including two sja1000 controllers
        INIT_RTN        wncan_pr6120_can_init();
        _CHILDREN       SELECT_CAN_BOARDS
        CFG_PARAMS      MAX_PR6120_CAN_BOARDS \
                        PR6120_CAN_DEVIO_NAME
        CONFIGLETTES    CAN/pr6120_can_cfg.c
        REQUIRES        INCLUDE_PR6120_CAN_2 \
                        INCLUDE_CAN_NETWORK_INIT
        INCLUDE_WHEN    INCLUDE_PR6120_CAN_2
        MODULES         esd_pci_200.o
}

Component INCLUDE_PR6120_CAN_2 {
        NAME            PR6120 CAN (2)
        SYNOPSIS        PR6120 CAN board including two sja1000 controllers (2)
        INIT_RTN        wncan_pr6120_can_init2();
        CONFIGLETTES    CAN/sys_pr6120_can.c
        _CHILDREN       FOLDER_CAN_SUPPORT
        REQUIRES        INCLUDE_PR6120_CAN
}

Parameter MAX_PR6120_CAN_BOARDS
        {
        NAME            PR6120 CAN MaxNumBrds
        SYNOPSIS        Maximum number of PR6120 CAN boards that can be used
        TYPE            uint
        DEFAULT         1
        }

Parameter PR6120_CAN_DEVIO_NAME
        {
        NAME            PR6120 CAN Device I/O Board Name
        SYNOPSIS        Device I/O name, one for every board, no leading slash, separated by spaces
        TYPE            string
        DEFAULT         "/can"
        }
