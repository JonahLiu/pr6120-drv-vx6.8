/* 02wnCAN.cdf - WNCAN component description file */

/*
 * Copyright (c) 2001-2002, 2004-2005, 2007 Wind River Systems, Inc.
 *
 * The right to copy, distribute or otherwise make use of this software
 * may be licensed only pursuant to the terms of an applicable Wind River
 * license agreement.
 */

/*
modification history
--------------------
2016/05/27  Jonah Liu   Add support for PR6120 CAN
01a,04Dec07,d_z  add support for tolapai CAN
12may05,lsg	 Replace C++ style comments with C style. Former caused error on 		 Linux.
26feb04,bjn      Added mpc5200 support.
19dec02,emcw     Added DevIO component
14dec01,dnb      written

DESCRIPTION
This file contains descriptions for the WNCAN drivers.
This file provides common configuration of the drivers.
Each board-specific driver will have its own file.

*/

Folder FOLDER_CAN_NETWORK {
        NAME            CAN network devices
        SYNOPSIS        Folder containing CAN board drivers
        _CHILDREN       FOLDER_NETWORK
        CHILDREN        INCLUDE_WNCAN            \
                        INCLUDE_CAN_NETWORK_INIT \
                        INCLUDE_WNCAN_SHOW       \
                        INCLUDE_WNCAN_DEVIO      \
                        SELECT_CAN_BOARDS        \
                        FOLDER_CAN_SUPPORT
}

Folder FOLDER_CAN_SUPPORT {
        NAME            CAN support components (private)
        SYNOPSIS        Folder containing dependent components
       _CHILDREN        FOLDER_CAN_NETWORK
}


Component INCLUDE_WNCAN_SHOW {
        NAME            CAN show routines
        SYNOPSIS        Include support for CAN show routines
       _CHILDREN        FOLDER_CAN_NETWORK
        CONFIGLETTES    CAN/wnCAN_show.c
        REQUIRES        INCLUDE_CAN_NETWORK_INIT
}


Component INCLUDE_WNCAN_DEVIO {
        NAME            CAN device I/O interface
        SYNOPSIS        Include support for CAN device I/O interface
       _CHILDREN        FOLDER_CAN_NETWORK
        INIT_RTN        wncDevIODrvInstall();
/*      No configlette */
        REQUIRES        INCLUDE_CAN_NETWORK_INIT \
                        SELECT_CAN_BOARDS        \
                        INCLUDE_SELECT           \
                        INCLUDE_IO_SYSTEM        
        MODULES         wncanDevIO.o
}


Selection SELECT_CAN_BOARDS {
        NAME            CAN boards
        SYNOPSIS        Folder containing CAN board drivers
        _CHILDREN       FOLDER_CAN_NETWORK
        COUNT           1-
}


Component INCLUDE_CAN_NETWORK_INIT {
        NAME            CAN core initialization
        SYNOPSIS        Core WindNetCAN init routine
       _CHILDREN        FOLDER_CAN_NETWORK
        INIT_RTN        wncan_core_init();
        MODULES         can_api.o canBoard.o canController.o \
                        canFixedLL.o wnCAN.o
}


/*
* Initialization Groups
*/

/*
 * Group0: used to initialize WindNetCAN core.
 * This guarantees initialization before all other WNC configlettes
*/
InitGroup usrWindNetCAN_Group0
{
        SYNOPSIS        WindNetCAN Core Initialization
        INIT_RTN        usrWindNetCAN_G0 ();
        _INIT_ORDER     usrRoot
        INIT_ORDER      INCLUDE_CAN_NETWORK_INIT

        /* trick to initialize before G1 */
        INIT_BEFORE     INCLUDE_MEM_MGR_BASIC
}

/*
 * Group1: used to initialize the device very early in the init order
 * as the device may be dependent on bus initialization.
*/
InitGroup usrWindNetCAN_Group1
{
        SYNOPSIS        WindNetCAN Initialization Group (PCI BUS related)
        INIT_RTN        usrWindNetCAN_G1 ();
        _INIT_ORDER     usrRoot
        INIT_ORDER      INCLUDE_ESD_CAN_PCI_200 \
                        INCLUDE_ADLINK_7841 \
                        INCLUDE_PR6120_CAN

        /* force an initialization very early within usrroot */
        INIT_BEFORE     INCLUDE_MMU_BASIC
}

/*
 * Group2: used to initialize the device without any particular init
 * dependencies. Generally, this group will init at the end of usrRoot
 *  DevIO interface must be installed before any of the CAN boards
*/
InitGroup usrWindNetCAN_Group2
{
        SYNOPSIS        WindNetCAN Initialization Group
        INIT_RTN        usrWindNetCAN_G2 ();
        _INIT_ORDER     usrRoot
        INIT_ORDER      INCLUDE_WNCAN_DEVIO         \
                        INCLUDE_PPC5XX_WNCAN        \
			INCLUDE_MCF5282_WNCAN       \
			INCLUDE_DAYTONA_WNCAN       \
                        INCLUDE_ESD_CAN_PCI_200_2   \
                        INCLUDE_ESD_CAN_PC104_200   \
                        INCLUDE_MSMCAN_PC104        \
                        INCLUDE_MPC5200_WNCAN       \
                        INCLUDE_MCF5485WNCAN       \
                        INCLUDE_TOLAPAI_CAN        \
                        INCLUDE_PR6120_CAN_2

        INIT_BEFORE     INCLUDE_USER_APPL
}
