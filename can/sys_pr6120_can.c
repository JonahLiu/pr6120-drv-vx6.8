/* sys_pr6120_can.c - bsp dependent init routine for PR6120 CAN */

/* 
modification history 
--------------------
2016/05/27 Jonah Liu Created on base of sys_pr6120_can.c

*/

/* 

DESCRIPTION
This file contains the BSP dependent initialization function of
the PR6120 CAN card.

*/

/* includes */
#include <vxWorks.h>
#include <errnoLib.h>
#include <intLib.h>
#include <iv.h>
#include <sysLib.h>

#if (CPU_FAMILY == I80X86)
   #include "vmLib.h"
#endif

#include "config.h"
#include "drv/pci/pciConfigLib.h"
#include "drv/pci/pciIntLib.h"

#include "CAN/wnCAN.h"
#include "CAN/canController.h"
#include "CAN/canBoard.h"
#include "CAN/i82527.h"
#include "CAN/sja1000.h"
#include <CAN/sja1000Offsets.h>
#include "CAN/private/pr6120_can.h"

#define DEVICE_ID_PR6120_CAN       0xC204L /* device ID */
#define VENDOR_ID_PR6120_CAN       0x13FEL /* subsystem ID */


#ifndef PCI_MSTR_MEMIO_BUS
    #define PCI_MSTR_MEMIO_BUS 0x0
#endif

#ifndef PCI_MSTR_MEMIO_LOCAL
    #define PCI_MSTR_MEMIO_LOCAL 0x0
    #if ((CPU==MIPS64) ||  (CPU==MIPS32))
        #undef  PCI_MSTR_MEMIO_LOCAL
        #define PCI_MSTR_MEMIO_LOCAL PHYS_PCI_MEM_BASE
    #endif
#endif

#ifndef INCLUDE_PCI
#error INCLUDE_PCI must be added to config.h
#endif

#if (CPU_FAMILY == I80X86)
IMPORT STATUS sysMmuMapAdd (void* address, UINT len, UINT initialStateMask, UINT initialState);
#endif

static const char PR6120_CAN_deviceName[] ="PR6120 CAN";

/* external reference */
#ifdef TARGET_BIGSUR7751
    extern UINT32 sysPciMemBaseAddressGet(void);
    extern void   sysPciIntDisable(void);
    extern void   sysPciIntEnable (void);
#endif

#ifdef TARGET_MS7751SE
    extern STATUS sysPicIntEnable(int intNum);
#endif

extern UINT PR6120_CAN_MaxBrdNumGet(void);
extern struct PR6120_CAN_DeviceEntry* PR6120_CAN_DeviceEntryGet(UINT brdNum);
extern void PR6120_CAN_EnableIRQ(struct WNCAN_Device *pDev);
extern void PR6120_CAN_DisableIRQ(struct WNCAN_Device *pDev);
extern STATUS CAN_DEVICE_establishLinks(WNCAN_DEVICE *pDev, WNCAN_BoardType brdType, WNCAN_ControllerType ctrlType);


/************************************************************************
*
* SJA1000_GetIntStatus - get the interrupt status on the controller
*
* This function returns the interrupt status on the controller: 
*
*    WNCAN_INT_NONE     = no interrupt occurred
*    WNCAN_INT_ERROR    = bus error
*    WNCAN_INT_TX       = interrupt resuting from a CAN transmission
*    WNCAN_INT_RX       = interrupt resulting form message reception
*    WNCAN_INT_BUS_OFF  = interrupt resulting from bus off condition
*    WNCAN_INT_WAKE_UP  = interrupt resulting from controller waking up
*                         after being put in sleep mode
*    WNCAN_INT_SPURIOUS = unknown interrupt
*
* NOTE: If the interrupt was caused by the transmission or reception of a 
* message, the channel number that generated the interrupt is set; otherwise,
* this value is undefined.
*
* ERRNO: N/A
*
*/
static WNCAN_IntType SJA1000_GetIntStatus
      (
          struct WNCAN_Device *pDev,
          UCHAR *channelNum
          )
{
    UCHAR       regInt;
    WNCAN_IntType   intStatus=WNCAN_INT_NONE;
        UCHAR regStatus;

    /* read the interrupt register */
    regInt = pDev->pBrd->canInByte(pDev, SJA1000_IR);

        if(regInt & IR_RI) {
                /*receive interrupt*/
        intStatus = WNCAN_INT_RX;
        *channelNum = 0;
        }
        
        if( regInt & IR_TI) {
                /*transmit interrupt*/
        intStatus = WNCAN_INT_TX;
        *channelNum = 1;
        }

        if(regInt & IR_WUI)
        {
                intStatus = WNCAN_INT_WAKE_UP;
        }

        /*flag WNCAN_INT_ERROR, if data overrun error int, or
          bus error int is detected */
        if(regInt & IR_BEI) {
                /*error interrupt*/
        intStatus = WNCAN_INT_ERROR;
        }

        if(regInt & IR_EI) {
                /*bus off interrupt*/
                /* read the status register */
        regStatus = pDev->pBrd->canInByte(pDev, SJA1000_SR);

                if(regStatus & SJA1000_SR_BS)
                        intStatus = WNCAN_INT_BUS_OFF;
        }
        
    return intStatus;
}

/************************************************************************
*
* PR6120_CAN_ISR - board-level isr for PR6120 CAN board
*
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/    
static void PR6120_CAN_ISR
(
    ULONG param
)
{
    struct PR6120_CAN_DeviceEntry *pDE = 
        (struct PR6120_CAN_DeviceEntry *)param;
    UINT i;

    /* disable the interrupt. */
    //PR6120_CAN_DisableIRQ(&pDE->canDevice[0]);

    for (i = 0; i < PR6120_CAN_MAX_CONTROLLERS; i++)
    {
        if(pDE->allocated[i])
        {
        	WNCAN_DEVICE     *pDev = &pDE->canDevice[i];
        	WNCAN_IntType   intStatus=WNCAN_INT_NONE;
        	UCHAR chnNum;
        	
        	/* notify board that we're entering isr */
        	if(pDev->pBrd->onEnterISR)
        	    pDev->pBrd->onEnterISR(pDev);
        	
        	/* Get the interrupt status */
        	intStatus = SJA1000_GetIntStatus((void*)pDev, &chnNum);

        	if (intStatus != WNCAN_INT_NONE)
        	{
        	                 
        	    pDev->pISRCallback(pDev, intStatus, chnNum);

				if (intStatus == WNCAN_INT_RX) {
					/* release the receive buffer */
					/* this will temporarily cleat RI bit */
					//pDev->pBrd->canOutByte(pDev, SJA1000_CMR, CMR_RRB);
				} else if (intStatus == WNCAN_INT_ERROR) {
					/* clear bei interrupt flag */
					pDev->pBrd->canInByte(pDev, SJA1000_ECC);
				} else if (intStatus == WNCAN_INT_TX) {
					/* notify channel available to TX again */
					pDev->pISRCallback(pDev, WNCAN_INT_TXCLR, chnNum);
				}
			}
        	
        	/* notify board that we're leaving isr */
        	if(pDev->pBrd->onLeaveISR)
        	    pDev->pBrd->onLeaveISR(pDev);
        }
    }
    /* Enable the interrupt. */
    //PR6120_CAN_EnableIRQ(&pDE->canDevice[0]);
}


/************************************************************************
*
* sys_PR6120_CAN_IntConnect - connect board-level interrupts
*
* This routine connects and enables the board level interrupt of the 
* PR6120 CAN CAN card.
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/    
void sys_PR6120_CAN_IntConnect
(
    UINT brdNum
)
{
    int                             oldLevel;
    VOIDFUNCPTR*                    irqVec        = 0;
    struct WNCAN_Board              *pBrd         = 0;
    struct WNCAN_Device             *pDev         = 0;
    struct PR6120_CAN_DeviceEntry  *pDeviceEntry = 0;

    /* Get the device entry */
    pDeviceEntry = PR6120_CAN_DeviceEntryGet(brdNum);

    if (pDeviceEntry == NULL)
        return;

    /* Check if the device entry has already been initialized.
       If not, return now */
    if (pDeviceEntry->inUse == FALSE)
        return;

    /* Point to the can board */
    pBrd = &pDeviceEntry->canBoard;

    /* Point to the first CAN device */
    pDev = &(pDeviceEntry->canDevice[0]);

    /* connect to isr */
    oldLevel = intLock();

    #if CPU_FAMILY == I80X86
        irqVec = INUM_TO_IVEC(pBrd->irq + 0x20);
    #elif CPU_FAMILY == PPC
        irqVec = INUM_TO_IVEC(pBrd->irq);
    #elif (CPU==SH7750) || (CPU==SH4)
        #ifdef TARGET_BIGSUR7751
            irqVec = INUM_TO_IVEC(pBrd->irq);
        #elif TARGET_MS7751SE
            irqVec = PIC_IRQ_TO_IVEC(pBrd->irq);
        #else
            #error irqVec not defined
        #endif
    #elif (CPU==MIPS64) ||  (CPU==MIPS32)
      /* for Malta 4kc board, comment out if not using Malta */  
       irqVec = INUM_TO_IVEC(pBrd->irq);
      /* uncomment for Rock Hopper board */
      /* irqVec = IRQ_TO_IVEC(pBrd->irq);*/
    #else
        #error irqVec not defined 
    #endif

    /* connect the interrupt handler */
    pciIntConnect(irqVec,PR6120_CAN_ISR,(ULONG)pDeviceEntry);

    /* enable PCI board level interrupts */
    PR6120_CAN_EnableIRQ(pDev);

    /* enable CPU level interrupts */
    #if (CPU_FAMILY == I80X86)
        sysIntEnablePIC(pBrd->irq);
    #elif CPU_FAMILY == PPC
        intEnable(pBrd->irq);
    #elif (CPU == SH7750) || (CPU==SH4)
        #ifdef TARGET_BIGSUR7751
            sysPciIntEnable();
        #elif TARGET_MS7751SE
            sysPicIntEnable(pBrd->irq);
        #else
            #error system level interrupts cannot be enabled
        #endif
    #elif (CPU==MIPS64) ||  (CPU==MIPS32)
	  /* for Malta 4kc board, comment out if not using Malta */  
	  INT_ENABLE(pBrd->irq - INT_NUM_IRQ0);
	  /* uncomment for Rock Hopper board */
	  /* sysIntEnablePCI(0, 2); *//* INT#A enable, arbitrary level */
    #else
        #error system level interrupts cannot be enabled
    #endif

    intUnlock(oldLevel);

    pDeviceEntry->intConnect = TRUE;
}


/************************************************************************
*
* sys_PR6120_CAN_Init - init the specified PR6120 CAN board
*
* RETURNS: OK or ERROR
*   
* ERRNO: N/A
*
*/
STATUS sys_PR6120_CAN_Init
(
    UINT brdNum
)
{
    int           bus,
                  dev,
                  func;
    UINT          ctrlNum;
    STATUS        retCode;
    
    struct PR6120_CAN_DeviceEntry  *pDeviceEntry = 0;
    struct WNCAN_Device             *pDev[2]      = {0,0};
    struct WNCAN_Board              *pBrd         = 0;

    retCode = ERROR;      /* pessimistic */

    /* check that the board number is within range */
    if (brdNum >= PR6120_CAN_MaxBrdNumGet())
        goto exit;

    /* check if the board is actually installed */
    if (pciFindDevice(VENDOR_ID_PR6120_CAN, DEVICE_ID_PR6120_CAN, brdNum, &bus, 
        &dev, &func) == ERROR)
        goto exit;

    /* Get the device entry */
    pDeviceEntry = PR6120_CAN_DeviceEntryGet(brdNum);

    if (pDeviceEntry == NULL)
        goto exit;

    /* Check if the device entry has already been initialized.
       If so, return now */
    if (pDeviceEntry->inUse == TRUE)
    {
        retCode = OK;
        goto exit;
    }

    /* Point to the can board */
    pBrd = &pDeviceEntry->canBoard;

    /* Initialize the board data structure: Note, brdType is set inside
       pr6120_can_establishLinks()  */
    {
        UINT16 pciCmd  = PCI_CMD_IO_ENABLE |
                         PCI_CMD_MEM_ENABLE |
                         PCI_CMD_MASTER_ENABLE;
        UINT8  tmpByte = 0;

        #if (CPU_FAMILY == I80X86)
            ULONG    pciMask = (~(VM_PAGE_SIZE - 1));
            ULONG    pciSize = VM_PAGE_SIZE;

            /* VM_STATE_MASK_VALID | VM_STATE_MASK_WRITABLE | 
               VM_STATE_MASK_CACHEABLE; */
            UINT  mask = VM_STATE_MASK_FOR_ALL;
            
            /* VM_STATE_VALID | VM_STATE_WRITABLE | VM_STATE_CACHEABLE_NOT; */
            UINT  state = VM_STATE_FOR_PCI;
        #endif

        pciConfigInLong (bus, dev, func,
                     PCI_CFG_BASE_ADDRESS_0, &(pBrd->bar0));
        /*             
        pciConfigInLong (bus, dev, func,
                     PCI_CFG_BASE_ADDRESS_1, &(pBrd->bar1));

        pciConfigInLong (bus, dev, func,
                     PCI_CFG_BASE_ADDRESS_2, &(pBrd->bar2));
        */
        pciConfigInByte (bus, dev, func,
                     PCI_CFG_DEV_INT_LINE, &tmpByte);

				pBrd->irq = (UINT)tmpByte;
        pBrd->bar0 &= PCI_MEMBASE_MASK;
        /*
        pBrd->bar1 &= PCI_MEMBASE_MASK;
        pBrd->bar2 &= PCI_IOBASE_MASK;
        */

#if ((CPU==MIPS64) ||  (CPU==MIPS32))
        pBrd->bar0 |= K1BASE;
        /*
        pBrd->bar1 |= K1BASE;
        pBrd->bar2 |= K1BASE;
        */
#else
        pBrd->bar0 -= PCI_MSTR_MEMIO_BUS;
        /*
        pBrd->bar1 -= PCI_MSTR_MEMIO_BUS;
        pBrd->bar2 -= PCI_MSTR_MEMIO_BUS;
        */
#endif

        pciConfigOutWord (bus, dev, func, PCI_CFG_COMMAND, pciCmd);

        #if (CPU_FAMILY == I80X86)
            /* can only add page aligned memory addresses; 
            otherwise, system hangs */
            /* FIXME: This fails for unknown reason
            if (sysMmuMapAdd((void *)(pBrd->bar0 & pciMask), 
                (UINT)pciSize, mask, state) == ERROR)
            {
                logMsg("sysMmuMapAdd failed bar0.\n",0,0,0,0,0,0);
                return ERROR;
            }*/
						/*
            if (sysMmuMapAdd((void *)(pBrd->bar2 & pciMask), 
                (UINT)pciSize, mask, state) == ERROR)
            {
                logMsg("sysMmuMapAdd failed for bar2.\n",0,0,0,0,0,0);
                return ERROR;
            }
            */
            pBrd->ioAddress = 0;
        #endif

        pBrd->xtalFreq = _16MHZ;

        pBrd->ioAddress = PCI_MSTR_MEMIO_LOCAL;
    }

    /* Initialize each controller contained in the board */
    for (ctrlNum = 0; ctrlNum < PR6120_CAN_MAX_CONTROLLERS; ctrlNum++)
    {
        pDev[ctrlNum] = &(pDeviceEntry->canDevice[ctrlNum]);

        /* Point to the statically allocated memory for the WNCAN_Controller 
           and WNCAN_Board data structures */
        pDev[ctrlNum]->pCtrl = 
            &(pDeviceEntry->canControllerArray[ctrlNum]);

        pDev[ctrlNum]->pBrd = pBrd;

        /* Initialize the controller data structure: Note, ctrlType is set 
           inside pr6120_can_establishLinks() */
        pDev[ctrlNum]->pCtrl->ctrlID     = (UCHAR)ctrlNum;
        pDev[ctrlNum]->pCtrl->pDev       = pDev[ctrlNum];
        pDev[ctrlNum]->pCtrl->chnType    = g_sja1000chnType;
        pDev[ctrlNum]->pCtrl->numChn     = SJA1000_MAX_MSG_OBJ;

        pDev[ctrlNum]->pCtrl->chnMode    = 
            &(pDeviceEntry->chData[ctrlNum].sja1000chnMode[0]);

        pDev[ctrlNum]->pCtrl->csData     = 
            &(pDeviceEntry->txMsg[ctrlNum]);

        /* set default baud rate to 125 Kbits/sec */
        pDev[ctrlNum]->pCtrl->brp = 3;       
        pDev[ctrlNum]->pCtrl->sjw = 0;  
        pDev[ctrlNum]->pCtrl->tseg1 = 0xc;
        pDev[ctrlNum]->pCtrl->tseg2 = 0x1;
        pDev[ctrlNum]->pCtrl->samples = 0;


        /* This will call pr6120_can_establishLinks() */
        if(CAN_DEVICE_establishLinks(pDev[ctrlNum], WNCAN_PR6120_CAN, 
            WNCAN_SJA1000) == ERROR)
        {
            pDev[ctrlNum] = 0;
            goto exit;
        }

        /* Assign the device name and Id */
        pDev[ctrlNum]->deviceName = PR6120_CAN_deviceName;
        pDev[ctrlNum]->deviceId = (brdNum<<8) | ctrlNum;
    }

    /* save the bdf and mark as inUse*/
    pDeviceEntry->bus   = bus;
    pDeviceEntry->dev   = dev;
    pDeviceEntry->func  = func;
    pDeviceEntry->inUse = 1;
    pDeviceEntry->intConnect = FALSE;

    retCode = OK;

exit:
    return retCode;
}


/*******************************************************************
 *  sys_PR6120_CAN_canOutByte - The ESD specific function for CAN 
 * writes on the PCI/200 board.
 *
 * RETURNS: NONE
 *
 * ERRNO: N/A
 */
void sys_PR6120_CAN_canOutByte
(
    struct WNCAN_Device *pDev,
    unsigned int reg,
    UCHAR value
)
{
    UCHAR    net     = pDev->pCtrl->ctrlID;
    UINT32   offset  = pDev->pBrd->bar0;
    ULONG    memBase = pDev->pBrd->ioAddress;

    UCHAR*   addr;

    #ifdef DEBUG
        UCHAR    data;
    #endif

    #if ((CPU == SH7750) || (CPU==SH4)) && (defined(TARGET_BIGSUR7751))
    #ifdef INCLUDE_CPCI
      /* The SJA1000 registers are numbered as follows because of big to 
         little endian byte swapping on the CPCI bus:
         3 2 1 0    7 6 5 4    11 10 9 8    15 14 13 12 ... 31 30 29 28 */
      /*
      reg = (reg/4 + 1)*4 - 1 - (reg%4);
      */
    #endif
    #endif

    addr = (UCHAR*)(memBase) + (offset + reg*4 + net*0x100);

    #ifdef DEBUG
    /* Read the data first */
    data = *(addr);
    #endif

    /* write new data */
    *(addr) = value;

    #ifdef DEBUG
    /* Read the data back and compare */
    data = *(addr);
    #endif

   return;
}

/*******************************************************************
 *  sys_PR6120_CAN_canInByte - The ESD specific function for CAN 
 * reads on the PCI/200 board.
 *
 * RETURNS: UCHAR - The register contents
 *
 * ERRNO: N/A
 */
UCHAR sys_PR6120_CAN_canInByte
(
    struct WNCAN_Device *pDev,
    unsigned int reg
)
{
    UCHAR    net     = pDev->pCtrl->ctrlID;
    UINT32   offset  = pDev->pBrd->bar0;
    ULONG    memBase = pDev->pBrd->ioAddress;
    UCHAR    data;
    UCHAR*   addr;
    
    #if ((CPU == SH7750) || (CPU==SH4)) && (defined(TARGET_BIGSUR7751))
    #ifdef INCLUDE_CPCI
    /* The SJA1000 registers are numbered as follows because of big to 
    little endian byte swapping on the CPCI bus:
    3 2 1 0    7 6 5 4    11 10 9 8    15 14 13 12 ... 31 30 29 28 */
    /*
    reg = (reg/4 + 1)*4 - 1 - (reg%4);
    */
    #endif
    #endif
    
    addr = (UCHAR*)(memBase) + (offset + reg*4 + net*0x100);
    data = *(addr);
    
    return(data);
}
