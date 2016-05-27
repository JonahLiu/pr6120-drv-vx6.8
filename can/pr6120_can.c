/* esd_pci_200.c - implementation of Board Interface for ESD CAN PCI/200 */

/* Copyright 2001 Wind River Systems, Inc. */

/* 
modification history 
--------------------
27nov01,dnb written

*/

/* 

DESCRIPTION
This file contains the functions that provide a board-level interface
to the ESD CAN PCI/200 card.

*/

/* includes */
#include <vxWorks.h>
#include <errnoLib.h>
#include <intLib.h>
#include <iv.h>
#include <sysLib.h>

#include "CAN/wnCAN.h"
#include "CAN/canController.h"
#include "CAN/canBoard.h"
#include "CAN/i82527.h"
#include "CAN/sja1000.h"
#include "CAN/private/esd_pci_200.h"

/* external reference */
extern STATUS sys_ESD_PCI_200_Init(UINT brdNum);
UINT ESD_PCI_200_MaxBrdNumGet(void);
struct ESD_PCI_200_DeviceEntry* ESD_PCI_200_DeviceEntryGet(UINT brdNum);
void sys_ESD_PCI_200_canOutByte(struct WNCAN_Device *pDev, unsigned int reg,
                                UCHAR value);
UCHAR sys_ESD_PCI_200_canInByte(struct WNCAN_Device *pDev, unsigned int reg);
void sys_ESD_PCI_200_IntConnect(UINT brdNum);

/*****************************************************************************		
 * ESD_PCI_200_EnableIRQ - The ESD specific function for enabling interrupts
 * on the PCI bus by the PLX 9052 bridge.
 *               
 *	RETURNS:		N/A
 *
 * ERRNO: N/A
 */
void ESD_PCI_200_EnableIRQ
(
    struct WNCAN_Device *pDev
)
{
    UINT32 offset = pDev->pBrd->bar0;
    ULONG  memBase = pDev->pBrd->ioAddress;

    UCHAR data;
    UCHAR *addr;

    addr = (UCHAR*)memBase + offset + 0x4c;

    data = *(addr);
    data |=  0x41;
    *(addr) = data;

    #ifdef DEBUG
        data = *(addr);
    #endif

    return;
}

/*****************************************************************************		
 * ESD_PCI_200_DisableIRQ - The ESD specific function for disabling interrupts
 * on the PCI bus by the PLX 9052 bridge.
 *               
 *	RETURNS:		N/A
 *
 * ERRNO: N/A
 */
void ESD_PCI_200_DisableIRQ
(
    struct WNCAN_Device *pDev
)
{
    UINT32 offset = pDev->pBrd->bar0;
    ULONG  memBase = pDev->pBrd->ioAddress;
    UCHAR  data;

    data = *((UCHAR*)memBase + offset + 0x4c);
    data &= ~0x41;
    *((UCHAR*)memBase + offset + 0x4c) = data;
    return;
}


/************************************************************************
*
* ESD_PCI_200_IntConnect - connect board-level interrupts
*
* This routine connects and enables the board level interrupt of the 
* specified ESD PCI/200 CAN card.
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/    
void ESD_PCI_200_IntConnect
(
    UINT brdNum
)
{
    sys_ESD_PCI_200_IntConnect(brdNum);
    return;
}

/************************************************************************
*
* ESD_PCI_200_IntConnectAll - connect all board-level interrupts
*
* This routine connects and enables the board level interrupts of all 
* installed ESD PCI/200 CAN cards.
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/    
void ESD_PCI_200_IntConnectAll
(
    void
)
{
    int brdNum = ESD_PCI_200_MaxBrdNumGet();
    int i;
    struct ESD_PCI_200_DeviceEntry *pDE;

    for (i = 0; i < brdNum; i++)
    {
       if ((pDE = ESD_PCI_200_DeviceEntryGet(i)) == NULL)
           continue;
       else
       {
          if (pDE->intConnect == FALSE)
            sys_ESD_PCI_200_IntConnect(i);
       }
    }
    return;
}

/************************************************************************
*
* esd_can_pci_200_open - open a CAN device
*
* RETURNS: pointer to a WNCAN_Device structure or 0 if error
*   
* ERRNO: S_can_illegal_board_no
*        S_can_illegal_ctrl_no
*        S_can_busy
*
*/
struct WNCAN_Device *esd_can_pci_200_open
(
    UINT brdNdx, 
    UINT ctrlNdx
)
{
    struct WNCAN_Device *pRet = 0;   /* pessimistic */
    UINT maxBrdNum;
    struct ESD_PCI_200_DeviceEntry *pDE;

    maxBrdNum = ESD_PCI_200_MaxBrdNumGet();    
    pDE = ESD_PCI_200_DeviceEntryGet(brdNdx);
   
    if((brdNdx >= maxBrdNum) || (pDE->inUse == FALSE))
    {
        errnoSet(S_can_illegal_board_no);
    }
    else if(ctrlNdx >= ESD_PCI_200_MAX_CONTROLLERS)
    {
        errnoSet(S_can_illegal_ctrl_no);
    }
    else if(pDE->allocated[ctrlNdx] == 1)
    {
        errnoSet(S_can_busy);
    }
    else
    {
        pDE->allocated[ctrlNdx] = 1;
        pRet = &(pDE->canDevice[ctrlNdx]);
    }

    return pRet;
}

/************************************************************************
*
* esd_can_pci_200_close - close a CAN device
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/
void esd_can_pci_200_close
(
    struct WNCAN_Device *pDev
)
{
    UINT   brdNdx;
    UINT   ctrlNdx;
    struct ESD_PCI_200_DeviceEntry *pDE;
    
    if(pDev != 0)
    {
        brdNdx = ((pDev->deviceId) & 0xFFFFFF00) >> 8;
        ctrlNdx = (pDev->deviceId) & 0xFF;

        pDE = ESD_PCI_200_DeviceEntryGet(brdNdx);
        pDE->allocated[ctrlNdx] = 0;
    }

    return;
}

/************************************************************************
*
* ESD_PCI_200_Init - install driver for the ESD PCI/200 board
*
* This routine initializes the ESD PCI/200 by calling the BSP dependent
* routine sys_ESD_PCI_200_Init().
*
* RETURNS: OK or ERROR
*   
* ERRNO: N/A
*
*/
STATUS ESD_PCI_200_Init
(
    UINT brdNum
)
{
    STATUS status;
    status = sys_ESD_PCI_200_Init(brdNum);
    return(status);
}

/************************************************************************
*
* ESD_PCI_200_InitAll - init all installed ESD PCI/200 boards.
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/
void ESD_PCI_200_InitAll(void)
{
    UINT i, 
         maxBrds;

    /* get the maximum number of ESD PCI/200 boards */
    maxBrds = ESD_PCI_200_MaxBrdNumGet();

    for(i = 0; i < maxBrds; i++)
    {
        ESD_PCI_200_Init(i);
    }
    return;
}

/*******************************************************************
 *  ESD_PCI_200_canOutByte - The ESD specific function for CAN writes 
 *  on the PCI/200 board.
 *
 * RETURNS: NONE
 *
 * ERRNO: N/A
 */
void ESD_PCI_200_canOutByte
(
    struct WNCAN_Device *pDev,
    unsigned int reg,
    UCHAR value
)
{
    sys_ESD_PCI_200_canOutByte(pDev,reg,value);
    return;
}



/*******************************************************************
 *  ESD_PCI_200_canInByte - The ESD specific function for CAN reads 
 *  on the PCI/200 board.
 *
 * RETURNS: UCHAR - The register contents
 *
 * ERRNO: N/A
 */
UCHAR ESD_PCI_200_canInByte
(
    struct WNCAN_Device *pDev,
    unsigned int reg
)
{
    UCHAR value;
    value = sys_ESD_PCI_200_canInByte(pDev,reg);
    return(value);
}


/************************************************************************
*
* esd_can_pci_200_establishLinks - set up function pointers
*
*
* RETURNS: OK or ERROR
*   
* ERRNO: S_can_illegal_config
*
*/
STATUS esd_can_pci_200_establishLinks(struct WNCAN_Device *pDev)
{

    STATUS retCode = OK;

    if(pDev->pCtrl->ctrlType == WNCAN_SJA1000)
    {
        pDev->pBrd->onEnterISR = 0;
        pDev->pBrd->onLeaveISR = 0;
        pDev->pBrd->enableIrq  = ESD_PCI_200_EnableIRQ;
        pDev->pBrd->disableIrq = ESD_PCI_200_DisableIRQ;

        pDev->pBrd->canInByte = ESD_PCI_200_canInByte;
        pDev->pBrd->canOutByte = ESD_PCI_200_canOutByte;
    }
    else
    {
        errnoSet(S_can_illegal_config);
        retCode = ERROR;
    }

    return retCode;
}
