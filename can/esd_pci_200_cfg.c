/* esd_can_pci_200_cfg.c - configlette for ESD CAN PCI/200 boards */

/* Copyright 2001-2003 Wind River Systems, Inc. */

/* 
modification history 
--------------------
01f,05oct05,lsg  fix warning
01e,09dec04,lsg  fixed problem with writing to a const string with strtok_r.
08jan03,emcw  Added DevIO interface support
23nov02,rp    version 1.3; support for shtahoeamanda (HCAN2)
27nov01,dnb   written

*/

/* 

 DESCRIPTION
 This file contains the initialization functions for the  ESD CAN PCI/200 card.
 
*/

/* includes */
#include <vxWorks.h>

#include "CAN/wnCAN.h"
#include "CAN/canController.h"
#include "CAN/canBoard.h"
#include "CAN/canFixedLL.h"
#include "CAN/i82527.h"
#include "CAN/sja1000.h"
#include "CAN/private/esd_pci_200.h"

#ifdef INCLUDE_WNCAN_DEVIO
#include "CAN/wncanDevIO.h"
#endif


/* extern references */
extern STATUS esd_can_pci_200_establishLinks(struct WNCAN_Device *pDev);
extern STATUS esd_can_pci_200_close(struct WNCAN_Device *pDev);
extern struct WNCAN_Device *esd_can_pci_200_open(UINT brdNdx, UINT ctrlNdx);
extern void ESD_PCI_200_InitAll();
extern void ESD_PCI_200_IntConnectAll();

/* forward references */
#ifdef INCLUDE_WNCAN_SHOW
LOCAL void   esd_can_pci_200_show(void);
#endif

#ifdef INCLUDE_WNCAN_DEVIO
LOCAL void   esd_can_pci_200_devio_init(void);
#endif

/* reserve memory for the requisite data structures */
LOCAL struct ESD_PCI_200_DeviceEntry
	esd_pci_200_cfg_deviceArray[MAX_ESD_CAN_PCI_200_BOARDS];

LOCAL BoardLLNode ESD_PCI_200_LLNode;


/************************************************************************
*
* wncan_esd_pci_200_init - 
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/ 
void wncan_esd_pci_200_init(void)
{
    /* Add this board into linked list of boards */
    ESD_PCI_200_LLNode.key = WNCAN_ESD_PCI_200;
    ESD_PCI_200_LLNode.nodedata.boarddata.establinks_fn = esd_can_pci_200_establishLinks;
    ESD_PCI_200_LLNode.nodedata.boarddata.close_fn      = esd_can_pci_200_close;
    ESD_PCI_200_LLNode.nodedata.boarddata.open_fn       = esd_can_pci_200_open;
#ifdef INCLUDE_WNCAN_SHOW
    ESD_PCI_200_LLNode.nodedata.boarddata.show_fn       = esd_can_pci_200_show;
#else
    ESD_PCI_200_LLNode.nodedata.boarddata.show_fn       = NULL;
#endif
    
    BOARDLL_ADD( &ESD_PCI_200_LLNode );
    
    /* register the possible controllers for this board */
    sja1000_registration();
    
    /* init the board */
    ESD_PCI_200_InitAll();
}


/************************************************************************
*
* wncan_esd_pci_200_init2 - 
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/ 
void wncan_esd_pci_200_init2(void)
{
    /* ESD PCI/200 board including two sja1000 controllers (2) */
    ESD_PCI_200_IntConnectAll();
    
#ifdef INCLUDE_WNCAN_DEVIO
    /* Initialize the DevIO interface */
    esd_can_pci_200_devio_init();
#endif
}




/************************************************************************
*
* esd_pci_200_MaxBrdNumGet -  get the number of ESD PCI/200 boards
*
* RETURNS: the number of ESD PCI/200 boards.
*   
* ERRNO: N/A
*
*/ 
UINT ESD_PCI_200_MaxBrdNumGet(void)
{
    return(MAX_ESD_CAN_PCI_200_BOARDS);
}


/************************************************************************
*
* esd_pci_200_DevEntryGet -  get the specified ESD PCI/200 device entry.
*
* This function returns a pointer to the device entry structure of the
* specified ESD PCI/200 device. The board number argument is zero based;
* therefore, the first board has number zero.
* 
* RETURNS: the ESD PCI/200 device entry structure.
*   
* ERRNO: N/A
*
*/ 
struct ESD_PCI_200_DeviceEntry* ESD_PCI_200_DeviceEntryGet(UINT brdNum)
{
    if (MAX_ESD_CAN_PCI_200_BOARDS > 0 && 
        brdNum < MAX_ESD_CAN_PCI_200_BOARDS)
        return(&esd_pci_200_cfg_deviceArray[brdNum]);
    else
        return(NULL);
}


/************************************************************************
*
* esd_pci_200_link - links API definitions in wncan_api.c
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/ 
#ifdef USE_CAN_FUNCTION_DEFS
void esd_pci_200_link(void)
{
    BOOL flag = FALSE;
    struct WNCAN_Device *pDev;
    if (flag == TRUE)
    {
        pDev = CAN_Open(WNCAN_ESD_PCI_200,0,0);
        CAN_Close(pDev);
    }
    return;
}
#endif


#ifdef INCLUDE_WNCAN_SHOW
/************************************************************************
*
* esd_can_pci_200_show - display board to stdout
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/
LOCAL void esd_can_pci_200_show(void)
{
    UINT i;
    struct ESD_PCI_200_DeviceEntry *pDE;
    UINT maxBrdNum;
    
    maxBrdNum = ESD_PCI_200_MaxBrdNumGet();
    
    if (maxBrdNum == 0)
        return;
    
    pDE = ESD_PCI_200_DeviceEntryGet(0);
    
    printf("\n\tBoard Name: %s\n",
        pDE->canDevice[0].deviceName);
    
    for(i = 0 ; i < maxBrdNum; i++)
    {
        pDE = ESD_PCI_200_DeviceEntryGet(i);
        if(pDE->inUse == TRUE)
        {
            printf("\t\tBoard Index: %d\n", i);
            printf("\t\tBase Memory Address: 0x%0x\n", 
                (UINT)pDE->canBoard.ioAddress);
            
            printf("\t\tBase Address 0: 0x%0x\n", 
                pDE->canBoard.bar0);
            
            printf("\t\tBase Address 1: 0x%0x\n", 
                pDE->canBoard.bar1);
            
            printf("\t\tBase Address 2: 0x%0x\n", 
                pDE->canBoard.bar2);
            
            printf("\t\tInterrupt Number: 0x%0x\n",
                pDE->canBoard.irq);
        }
    }
    return;
}
#endif


#ifdef INCLUDE_WNCAN_DEVIO
/************************************************************************
*
* esd_can_pci_200_devio_init - initialize the DevIO interface 
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/
LOCAL void esd_can_pci_200_devio_init(void)
{
    char*                  pCurBrdName = NULL;
    char*                  pLastBrdName = NULL;
    char*                  copyConstString;
    char                   sep[] = " \"";
    int                    boardidx, ctrlidx;
    STATUS                 status;
    WNCAN_DEVIO_DRVINFO*   wncDrv = NULL;
    
    /* 
    Read list of DevIO board names from component properties; 
    extract first name 
    Make a copy of the devio name parameter before operating on it.
    */
    
    copyConstString = (char *) malloc(strlen(ESD_CAN_PCI_200_DEVIO_NAME) + 1);
    
    memcpy(copyConstString, ESD_CAN_PCI_200_DEVIO_NAME, (strlen(ESD_CAN_PCI_200_DEVIO_NAME) + 1));
    
    pCurBrdName = strtok_r(copyConstString, sep, &pLastBrdName);
    
    /* Parse name list and initialize each controller */
    
    for(boardidx=0; (pCurBrdName && (boardidx < MAX_ESD_CAN_PCI_200_BOARDS)); boardidx++)
    {
        for(ctrlidx=0; ctrlidx < ESD_PCI_200_MAX_CONTROLLERS; ctrlidx++)
        {
            /* Construct DevIO name for each controller, /boardName/ctlrNum */
            status = wncDevIODevCreate(pCurBrdName, 
                WNCAN_ESD_PCI_200,
                boardidx, ctrlidx, &wncDrv);
            
            if( status == ERROR )
            {
                free(copyConstString);
                return;
            }
        }
        
        pCurBrdName = strtok_r(NULL, sep, &pLastBrdName);
    }
    
    free(copyConstString);
}
#endif

