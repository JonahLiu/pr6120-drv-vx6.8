/* pr6120_can.c - implementation of Board Interface for PR6120 CAN */

/* 
modification history 
--------------------
2016/05/27 Jonah Liu Created on base of PR6120_CAN.c

*/

/* 

DESCRIPTION
This file contains the functions that provide a board-level interface
to the PR6120 CAN card.

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
#include "CAN/private/pr6120_can.h"

/* external reference */
extern STATUS sys_PR6120_CAN_Init(UINT brdNum);
UINT PR6120_CAN_MaxBrdNumGet(void);
struct PR6120_CAN_DeviceEntry* PR6120_CAN_DeviceEntryGet(UINT brdNum);
void sys_PR6120_CAN_canOutByte(struct WNCAN_Device *pDev, unsigned int reg,
                                UCHAR value);
UCHAR sys_PR6120_CAN_canInByte(struct WNCAN_Device *pDev, unsigned int reg);
void sys_PR6120_CAN_IntConnect(UINT brdNum);

/*****************************************************************************		
 * PR6120_CAN_EnableIRQ - The ESD specific function for enabling interrupts
 * on the PCI bus.
 *               
 *	RETURNS:		N/A
 *
 * ERRNO: N/A
 */
void PR6120_CAN_EnableIRQ
(
    struct WNCAN_Device *pDev
)
{
    return;
}

/*****************************************************************************		
 * PR6120_CAN_DisableIRQ - The ESD specific function for disabling interrupts
 * on the PCI bus.
 *               
 *	RETURNS:		N/A
 *
 * ERRNO: N/A
 */
void PR6120_CAN_DisableIRQ
(
    struct WNCAN_Device *pDev
)
{
    return;
}


/************************************************************************
*
* PR6120_CAN_IntConnect - connect board-level interrupts
*
* This routine connects and enables the board level interrupt of the 
* specified PR6120 CAN card.
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/    
void PR6120_CAN_IntConnect
(
    UINT brdNum
)
{
    sys_PR6120_CAN_IntConnect(brdNum);
    return;
}

/************************************************************************
*
* PR6120_CAN_IntConnectAll - connect all board-level interrupts
*
* This routine connects and enables the board level interrupts of all 
* installed PR6120 CAN cards.
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/    
void PR6120_CAN_IntConnectAll
(
    void
)
{
    int brdNum = PR6120_CAN_MaxBrdNumGet();
    int i;
    struct PR6120_CAN_DeviceEntry *pDE;

    for (i = 0; i < brdNum; i++)
    {
       if ((pDE = PR6120_CAN_DeviceEntryGet(i)) == NULL)
           continue;
       else
       {
          if (pDE->intConnect == FALSE)
            sys_PR6120_CAN_IntConnect(i);
       }
    }
    return;
}

/************************************************************************
*
* pr6120_can_open - open a CAN device
*
* RETURNS: pointer to a WNCAN_Device structure or 0 if error
*   
* ERRNO: S_can_illegal_board_no
*        S_can_illegal_ctrl_no
*        S_can_busy
*
*/
struct WNCAN_Device *pr6120_can_open
(
    UINT brdNdx, 
    UINT ctrlNdx
)
{
    struct WNCAN_Device *pRet = 0;   /* pessimistic */
    UINT maxBrdNum;
    struct PR6120_CAN_DeviceEntry *pDE;

    maxBrdNum = PR6120_CAN_MaxBrdNumGet();    
    pDE = PR6120_CAN_DeviceEntryGet(brdNdx);
   
    if((brdNdx >= maxBrdNum) || (pDE->inUse == FALSE))
    {
        errnoSet(S_can_illegal_board_no);
    }
    else if(ctrlNdx >= PR6120_CAN_MAX_CONTROLLERS)
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
* pr6120_can_close - close a CAN device
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/
void pr6120_can_close
(
    struct WNCAN_Device *pDev
)
{
    UINT   brdNdx;
    UINT   ctrlNdx;
    struct PR6120_CAN_DeviceEntry *pDE;
    
    if(pDev != 0)
    {
        brdNdx = ((pDev->deviceId) & 0xFFFFFF00) >> 8;
        ctrlNdx = (pDev->deviceId) & 0xFF;

        pDE = PR6120_CAN_DeviceEntryGet(brdNdx);
        pDE->allocated[ctrlNdx] = 0;
    }

    return;
}

/************************************************************************
*
* PR6120_CAN_Init - install driver for the PR6120 CAN board
*
* This routine initializes the PR6120 CAN by calling the BSP dependent
* routine sys_PR6120_CAN_Init().
*
* RETURNS: OK or ERROR
*   
* ERRNO: N/A
*
*/
STATUS PR6120_CAN_Init
(
    UINT brdNum
)
{
    STATUS status;
    status = sys_PR6120_CAN_Init(brdNum);
    return(status);
}

/************************************************************************
*
* PR6120_CAN_InitAll - init all installed PR6120 CAN boards.
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/
void PR6120_CAN_InitAll(void)
{
    UINT i, 
         maxBrds;

    /* get the maximum number of PR6120 CAN boards */
    maxBrds = PR6120_CAN_MaxBrdNumGet();

    for(i = 0; i < maxBrds; i++)
    {
        PR6120_CAN_Init(i);
    }
    return;
}

/*******************************************************************
 *  PR6120_CAN_canOutByte - The ESD specific function for CAN writes 
 *  on the PR6120 CAN board.
 *
 * RETURNS: NONE
 *
 * ERRNO: N/A
 */
void PR6120_CAN_canOutByte
(
    struct WNCAN_Device *pDev,
    unsigned int reg,
    UCHAR value
)
{
    sys_PR6120_CAN_canOutByte(pDev,reg,value);
    return;
}



/*******************************************************************
 *  PR6120_CAN_canInByte - The ESD specific function for CAN reads 
 *  on the PCI/200 board.
 *
 * RETURNS: UCHAR - The register contents
 *
 * ERRNO: N/A
 */
UCHAR PR6120_CAN_canInByte
(
    struct WNCAN_Device *pDev,
    unsigned int reg
)
{
    UCHAR value;
    value = sys_PR6120_CAN_canInByte(pDev,reg);
    return(value);
}


/************************************************************************
*
* pr6120_can_establishLinks - set up function pointers
*
*
* RETURNS: OK or ERROR
*   
* ERRNO: S_can_illegal_config
*
*/
STATUS pr6120_can_establishLinks(struct WNCAN_Device *pDev)
{

    STATUS retCode = OK;

    if(pDev->pCtrl->ctrlType == WNCAN_SJA1000)
    {
        pDev->pBrd->onEnterISR = 0;
        pDev->pBrd->onLeaveISR = 0;
        pDev->pBrd->enableIrq  = PR6120_CAN_EnableIRQ;
        pDev->pBrd->disableIrq = PR6120_CAN_DisableIRQ;

        pDev->pBrd->canInByte = PR6120_CAN_canInByte;
        pDev->pBrd->canOutByte = PR6120_CAN_canOutByte;
    }
    else
    {
        errnoSet(S_can_illegal_config);
        retCode = ERROR;
    }

    return retCode;
}
