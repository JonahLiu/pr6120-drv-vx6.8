Folder FOLDER_PR6120 {
        NAME            PR6120 device drivers
        SYNOPSIS       Folder containing PR6120 device drivers
        _CHILDREN      FOLDER_DRIVERS
        CHILDREN       DRV_PR6120_NET \
                        DRV_PR6120_UART \
                        DRV_PR6120_CAN 		         
}

Component DRV_PR6120_NET {
        NAME            PR6120 NET driver
        SYNOPSIS       PR6120 ethernet board with fault-tolerant ports
        _CHILDREN       FOLDER_PR6120
        REQUIRES       INCLUDE_GEI825XX_VXB_END
}

Component DRV_PR6120_UART {
        NAME            PR6120 UART driver
        SYNOPSIS       PR6120 UART board including four NS16550 controllers
        _CHILDREN      FOLDER_PR6120
        REQUIRES       DRV_SIO_NS16550
}


Component DRV_PR6120_CAN {
        NAME            PR6120 CAN Driver
        SYNOPSIS        PR6120 CAN board including two sja1000 controllers
        _CHILDREN       FOLDER_PR6120                         
        CFG_PARAMS      MAX_PR6120_CAN_BOARDS \
                         PR6120_CAN_DEVIO_NAME     
        INIT_RTN        wncan_pr6120_can_init();
        REQUIRES        INCLUDE_PR6120_WNCAN \
		                 INCLUDE_PR6120_DEVIO \
		                 DRV_PR6120_CAN_2
}

Component DRV_PR6120_CAN_2 {
        NAME            PR6120 CAN Driver (2)
        SYNOPSIS        PR6120 CAN Driver second part
        INIT_RTN        wncan_pr6120_can_init2();
        REQUIRES        DRV_PR6120_CAN
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
        SYNOPSIS        Device I/O name, one for every board, no leading slash, separated by spaces. \
                         e.g. "/can0" "/can1"
        TYPE            string
        DEFAULT         "/can"
        }
        

Component INCLUDE_PR6120_WNCAN {
        NAME            Wind River Network CAN
        SYNOPSIS       Include support for Wind River Network CAN API  
        INIT_RTN       wncan_core_init();  
        REQUIRES       DRV_PR6120_CAN
}
/*
Component INCLUDE_PR6120_WNCAN_SHOW {
        NAME            WNCAN show routines
        SYNOPSIS        Include support for CAN show routines
        _CHILDREN       FOLDER_PR6120       
        REQUIRES        INCLUDE_PR6120_WNCAN
}
*/

Component INCLUDE_PR6120_DEVIO {
        NAME            CAN device I/O interface
        SYNOPSIS       Include support for CAN device I/O interface
        _CHILDREN      FOLDER_PR6120       
        INIT_RTN       wncDevIODrvInstall();
        REQUIRES		INCLUDE_PR6120_WNCAN \
                        INCLUDE_SELECT           \
                        INCLUDE_IO_SYSTEM
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
        INIT_ORDER      INCLUDE_PR6120_WNCAN

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
        INIT_ORDER      DRV_PR6120_CAN

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
        INIT_ORDER      INCLUDE_PR6120_DEVIO         \
                        DRV_PR6120_CAN_2

        INIT_BEFORE     INCLUDE_USER_APPL
}

