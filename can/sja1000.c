/* sja1000.c - implementation of CAN API for Philips SJA1000 */

/* Copyright 2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,06oct05,lsg  Rename macros that are common with             
                 target/h/arch/mips/archMips
22aug02,lsg modified for WindNet CAN 1,2
09nov01,dnb modified for integration into Tornado
12jul01,jac written
*/

/*

DESCRIPTION

This module contains the functions, specific to the Philips SJA1000 CAN
controller, that implement the interface defined in the wnCAN.h
header file.

*/

/* includes */
#include <vxWorks.h>
#include <errnoLib.h>
#include <taskLib.h>
#include <intLib.h>
#include <iv.h>
#include <sysLib.h>

#include <CAN/wnCAN.h>
#include <CAN/canController.h>
#include <CAN/canBoard.h>
#include <CAN/canFixedLL.h>

#include <CAN/sja1000.h>
#include <CAN/sja1000Offsets.h>
/* forward declarations */
const WNCAN_ChannelType g_sja1000chnType[SJA1000_MAX_MSG_OBJ] = { 
WNCAN_CHN_RECEIVE,WNCAN_CHN_TRANSMIT};

/*
   In Pelican mode, the addresses of the transmit and receive buffers are
   identical. Access to the correct register depends on the access operation:
   write for accessing the transmit buffer, and read for accessing the receive
   buffer. Also, the address of the start of the msg data changes depending
   on whether the CAN Id is standard or extended; therefore, a SJA1000_WriteData()
   operation requires a copy of the data until SJA1000_Tx() is called at which
   time the frame format is known. Because read operations on the transmit
   buffer (SJA1000_ReadData(), SJA1000_ReadID()) are also not possible,
   a local tranmit message data stucture is necessary to store the transmit data.
   Copies to and from this data structure represents additional overhead.
*/

/************************************************************************
*
* SJA1000_Init - intitialize the CAN controller
*
* This function initializes the CAN controller and makes default selections.
* 1. Puts the CAN controller into reset mode
* 2. Disables interrupts at the CAN controller level as well as channel level
* 3. Sets bit timing values according to values stored in the CAN controller
*    struct. If user has changed these values before init is called, the
*    default bit timing values are set to a baud rate of 250K
* 4. Clears error counters
* 5. Sets local and global receive masks to don't care (accept all)
* 6. Makes all channels inactive in hardware
* 7. The CAN controller has now been initialized but will not participate
*    in CAN bus communication. In order to bring the CAN controller online,
*    and enable active participation in CAN bus activity, CAN_Start must
*    be called following CAN_Init.
*
* Call these functions in this order:
* 1. CAN_Open
* 2. CAN_Init
* 3. CAN_Start
*
* RETURNS: OK, or ERROR
*
* ERRNO:   N/A
*
*/
static STATUS SJA1000_Init(struct WNCAN_Device *pDev)
{
    UCHAR        value;
    STATUS       retCode = OK;
    unsigned int i;

        /* Put the controller into reset mode */
        pDev->pBrd->canOutByte(pDev, SJA1000_MOD, MOD_RM);
        
        /* set controller to Pelican mode */
        value = pDev->pBrd->canInByte(pDev, SJA1000_CDR);
        value |= 0x80;
        pDev->pBrd->canOutByte(pDev, SJA1000_CDR, value);
        
        /* set the mode register: 0001001 (0x9)
        reset             = 1   (reset mode)
        listen only       = 0   (normal)
        self test         = 0   (ack required for successful TX)
        acceptance filter = 1   (single 32 bit filter)
        sleep mode        = 0   */
        pDev->pBrd->canOutByte(pDev, SJA1000_MOD, 0x09);
        
        /* set command register to zero */
        pDev->pBrd->canOutByte(pDev, SJA1000_CMR ,0);
        
        /* set the error warning limit to 96 */
        pDev->pBrd->canOutByte(pDev, SJA1000_EWLR, 0x60);
        
        /* disable all interrupts */
        pDev->pBrd->canOutByte(pDev, SJA1000_IER, 0x0);
        
        /* btr0 and btr1 */
        value = (pDev->pCtrl->sjw << 6) | pDev->pCtrl->brp;
        pDev->pBrd->canOutByte(pDev, SJA1000_BTR0, value);

        if(pDev->pCtrl->samples)
                value = 0x80 | (pDev->pCtrl->tseg2 << 4) | pDev->pCtrl->tseg1;  
        else
                value = (pDev->pCtrl->tseg2 << 4) | pDev->pCtrl->tseg1; 
        
        pDev->pBrd->canOutByte(pDev, SJA1000_BTR1, value);
    
        
        /* set output control */
        pDev->pBrd->canOutByte(pDev, SJA1000_OCR, 0xfb);
        
        /* reset receive error counter */
        pDev->pBrd->canOutByte(pDev, SJA1000_RXERR, 0x0);
        
        /* reset transmit error counter */
        pDev->pBrd->canOutByte(pDev, SJA1000_TXERR, 0x0);
        
        /* Set all acceptance code registers to 0xFF. Received messages must
        have these bits set. */
        pDev->pBrd->canOutByte(pDev, SJA1000_ACR0, 0xff);
        pDev->pBrd->canOutByte(pDev, SJA1000_ACR1, 0xff);
        pDev->pBrd->canOutByte(pDev, SJA1000_ACR2, 0xff);
        pDev->pBrd->canOutByte(pDev, SJA1000_ACR3, 0xff);
        
        /* Set the acceptance mask registers to don't care. The acceptance 
        mask register defines which bits of the acceptance code register 
        matter:
        1 = don't care (message bit may be one or zero), 0 = must match . 
        */
        pDev->pBrd->canOutByte(pDev, SJA1000_AMR0, 0xff);
        pDev->pBrd->canOutByte(pDev, SJA1000_AMR1, 0xff);
        pDev->pBrd->canOutByte(pDev, SJA1000_AMR2, 0xff);
        pDev->pBrd->canOutByte(pDev, SJA1000_AMR3, 0xff);
        
        /*
        * The controller is not brought out of reset here. CAN_Start must
        * be called in order to do that and enable CAN communication on the
        * bus
        */
        
        /* free all channels */
        for (i = 0; i < SJA1000_MAX_MSG_OBJ; i++)
                pDev->pCtrl->chnMode[i] = WNCAN_CHN_INVALID;             

    return retCode;
}

/************************************************************************
*
* SJA1000_GetBusStatus - get the bus status
*
* This function returns the status of the CAN bus. The bus is 
* either in a WNCAN_BUS_OFF, WNCAN_BUS_WARN, or WNCAN_BUS_OK state.
*
* WNCAN_BUS_OFF: CAN controller is in BUS OFF state
* A CAN node is bus off when the transmit error count is greater than
* or equal to 256
* WNCAN_BUS_WARN: CAN controller is in ERROR WARNING state
* A CAN node is in error warning state when the number of transmit errors
* equals or exceeds 96, or the number of receive errors equals or exceeds
* 96
* WNCAN_BUS_OK: CAN controller is in ERROR ACTIVE state
* A CAN node in error active state can normally take part in bus communication
* and sends an ACTIVE ERROR FLAG when an error has been detected. 
*
* RETURNS: status of the CAN bus = WNCAN_BUS_OK, WNCAN_BUSWARN, 
* WNCAN_BUS_OFF
*
* ERRNO: N/A
*
*/
static WNCAN_BusStatus SJA1000_GetBusStatus(struct WNCAN_Device *pDev)
{
    UCHAR regStatus;
    WNCAN_BusStatus retStat;

    /* read the status register */
    regStatus = pDev->pBrd->canInByte(pDev, SJA1000_SR);

    if(regStatus & SJA1000_SR_BS)
        retStat = WNCAN_BUS_OFF;
    else if(regStatus & SJA1000_SR_ES)
        retStat = WNCAN_BUS_WARN;
    else
        retStat = WNCAN_BUS_OK;

    return retStat;
}

/************************************************************************
*
* SJA1000_Start - Bring CAN controller online
*
* This function is called to bring the CAN controller online. The CAN controller
* can now participate in transmissions and receptions on the CAN bus.
* This function must be called after CAN_Init has been called to initialize and
* bring the CAN controller up in a known state.
*
* RETURNS: OK always
*
* ERRNO:   N/A
*
*/
static void SJA1000_Start(struct WNCAN_Device *pDev)
{
        struct TxMsg *pTxMsg;
        UCHAR value;
        UCHAR i;
    
        /* Clear the controller reset */
        value = pDev->pBrd->canInByte(pDev, SJA1000_MOD);
        value &= ~0x1;
        pDev->pBrd->canOutByte(pDev, SJA1000_MOD, value);

        /* Wait until the controller comes out of reset */
        while(pDev->pBrd->canInByte(pDev, SJA1000_MOD) != value) {};

        /* Wait for Bus OK or taskDelay(1)*/
        if(SJA1000_GetBusStatus != WNCAN_BUS_OK)
                taskDelay(1);

        /* Initialize the TX frame info. This can only be done when
        the controller is not in reset. */
        pDev->pBrd->canOutByte(pDev, SJA1000_SFF, 0);

        /* Initialize the TX msg data structure */
        pTxMsg = (struct TxMsg *)pDev->pCtrl->csData;
        pTxMsg->id  = 0;
        pTxMsg->ext = 0;
        pTxMsg->rtr = 0;
        pTxMsg->len = 0;
        for(i = 0 ; i < 8 ; i++)
                pTxMsg->data[i] = 0;

}

/************************************************************************
*
* SJA1000_Stop - Put CAN controller offline
*
* Disables communication between CAN controller and the CAN bus
*
* RETURNS: OK always
*
* ERRNO:   N/A
*
*/
static void SJA1000_Stop(struct WNCAN_Device *pDev)
{

    /* Stop the SJA1000 controller*/
        /* Put the controller into reset mode */
        pDev->pBrd->canOutByte(pDev, SJA1000_MOD, MOD_RM);

}

/************************************************************************
*
* SJA1000_SetBitTiming - Set bit timing
*
* This function sets the baud rate of the controller. The selection
* of the input parameters should be based on an established set of 
* recommendations for the particular application.
* This function sets the bit timing values in the hardware as well as the
* controller structure, so that the bit timing values are not lost if Init
* is called again. The function will preserve the state of the CAN controller.
* i.e. if the CAN controller is online when the function is called, then the 
* CAN controller will be online when the function exits. 
*
* bit time = 1 + (tseg1 + 1) + (tseg2+1) time quanta 
* The interpretation of tseg1 are tseg2 are according to the controller's 
* definition and hence can be written to directly using this function.
*
* In all cases so far, tseg2 refers to the segment of bit time after the sample
* point. However, in some controllers tseg1 refers to the segment of bit time
* after the end of sync seg upto the sample point.                     
*  ---------------------------------------------------------
* |    |                    |                              |  
*  sync <--------tseg1 --->^|^<---------tseg2 ------------->
*                          sample point 
* 
*
* RETURNS: OK, or ERROR
*
* ERRNO:   S_can_invalid_parameter
*
*/
static STATUS SJA1000_SetBitTiming
(
    struct WNCAN_Device *pDev,
    UCHAR tseg1,
    UCHAR tseg2,
    UCHAR brp,
    UCHAR sjw,
    BOOL  samples
)
{
    UCHAR       value;
    STATUS      retCode;
        
    retCode = OK;  /* assume success */
        
    /* qualify parameters */
    if((sjw > 0x03) || (brp > 0x3f) || (tseg1 > 15) ||
                (tseg1 < 2) || (tseg2 > 7) || (tseg2 < 1))
    {
        errnoSet(S_can_invalid_parameter);
        retCode = ERROR;
    }
    else
    {
                /* Check if controller is in reset mode */
        /* if not, enable change configuration */
        value = pDev->pBrd->canInByte(pDev, SJA1000_MOD);               
                
                if((value & 0x01) == 0)
                        pDev->pBrd->canOutByte(pDev, SJA1000_MOD, value | 0x01);
                
        pDev->pCtrl->brp = brp;
        pDev->pCtrl->sjw = sjw;
        pDev->pBrd->canOutByte(pDev, SJA1000_BTR0, (sjw << 6) | brp);
                
        pDev->pCtrl->tseg1 = tseg1;
        pDev->pCtrl->tseg2 = tseg2;
                
        if(samples)
                {
                        /*three sample mode*/
                        pDev->pCtrl->samples = 1;
                        pDev->pBrd->canOutByte(pDev, SJA1000_BTR1, (0x80 | (tseg2 << 4) | 
                                tseg1));
                }
        else
                        
                {
                        /*one sample mode*/
                        pDev->pCtrl->samples = 0;
                        pDev->pBrd->canOutByte(pDev, SJA1000_BTR1, (tseg2 << 4) | tseg1);
                }

                                
                /*restore original state of controller*/
                if((value & 0x01) == 0)
                {
                        pDev->pBrd->canOutByte(pDev, SJA1000_MOD, value);
                        
                        /* Wait until the controller comes out of reset */
                        while(pDev->pBrd->canInByte(pDev, SJA1000_MOD) != value) {};
                        
                        /* Wait for Bus OK or a task delay of 1*/                       
                        if(SJA1000_GetBusStatus != WNCAN_BUS_OK)
                                taskDelay(1);
                        
                        
                }
                
    }
        
    return retCode;
}

/***************************************************************************
* SJA1000_GetBaudRate: Returns baud rate by recalculating bit timing 
* parameters
*
* RETURNS: baud rate in bps
*
* ERRNO:   N/A
*/
static UINT SJA1000_GetBaudRate
      (
      struct WNCAN_Device *pDev,
          UINT   *samplePoint
      )
{
        ULONG             sysClkFreq;        
        USHORT            num_time_quanta;
        UCHAR             brp, tseg1, tseg2;
        UCHAR             value;

        value = pDev->pBrd->canInByte(pDev, SJA1000_CDR);

        value = value & 0x7;
        
        sysClkFreq = (pDev->pBrd->xtalFreq / (2 + 2*value));

        brp = (pDev->pBrd->canInByte(pDev, SJA1000_BTR0) & 0x3f) + 1;

        tseg1 = pDev->pBrd->canInByte(pDev, SJA1000_BTR1);

        tseg2 = ((tseg1 & 0xf0) >> 4) + 1;

        tseg1 = (tseg1 & 0x0f) + 1;

        /*Calculate baud rate*/
        num_time_quanta = 1 + tseg1 + tseg2;

         *samplePoint = ((1 + tseg1) * 100)/num_time_quanta;

        return( sysClkFreq / (num_time_quanta * brp));

}

/************************************************************************
*
* SJA1000_SetIntMask - enable controller level interrupts on the CAN
*                      controller
*
* This function enables the specified list of controller level interrupts
* on the CAN controller. Interrupts are not enabled at the CAN board level
* or at the CPU level.
* The interrupt masks available are:
* WNCAN_INT_ERROR : enables interrupts due to errors on the CAN bus
*                   This may be implemented as a single or multiple
*                   interrupt sources on the CAN controller.
* WNCAN_INT_BUS_OFF: enables interrupt indicating bus off condition  
* WNCAN_INT_WAKE_UP: enables interrupt on wake up
* All interrupt masks that need to be enabled must be specified in the list
* passed to the function, in order to be set, every time the function
* is called. 
*
* RETURNS: OK or ERROR
*
* ERRNO:   S_can_invalid_parameter
*
*/
static STATUS SJA1000_SetIntMask(struct WNCAN_Device *pDev, WNCAN_IntType intMask)
{
    int         oldLevel;
    UCHAR       value=0;
        STATUS      retCode = OK;

        /*
        return error if masks other than error, busoff and wakeup
        are passed to the function
        */
        if((intMask > (WNCAN_INT_ERROR | WNCAN_INT_BUS_OFF | WNCAN_INT_WAKE_UP)) &&
                (intMask != WNCAN_INT_ALL)) 
        {
                errnoSet(S_can_invalid_parameter);
                retCode = ERROR;        
        }
        else
        {

                value = pDev->pBrd->canInByte(pDev, SJA1000_IER);
                
                /*Enable all error interreupts. Bus error,  data overrun*/
                if(intMask & WNCAN_INT_ERROR)
                        value |= IER_BEIE;
                else
                        value &= ~IER_BEIE;
                
                
                if(intMask & WNCAN_INT_WAKE_UP)
                        value |= IER_WUIE;
                else
                        value &= ~IER_WUIE;
                
                
                        /*
                        The error warning interrupt is generated when errors on the bus exceed
                        warning limits and if the controller becomes bus off. The interrupt 
                        is also raised when the controller goes into error active state from 
                        being bus offf
                */
                if(intMask & WNCAN_INT_BUS_OFF)
                        value |= IER_EIE;
                else
                        value &= ~IER_EIE;
                
                
                oldLevel = intLock();
                
                pDev->pBrd->canOutByte(pDev, SJA1000_IER, value);
                
                intUnlock(oldLevel);
        }
    return retCode;
}

/***************************************************************************   
* SJA1000_EnableInt - enable interrupts from the CAN device to the CPU.
*
* This function enables interrupts from the CAN controller to reach the
* CPU. Typically, enables a global interrupt mask bit at the CPU level.
*
* RETURNS: N/A
*
* ERRNO:   N/A
*
*/
static void SJA1000_EnableInt(struct WNCAN_Device *pDev)
{
    int         oldLevel;
    UCHAR       value=0;

        value = pDev->pBrd->canInByte(pDev, SJA1000_IER);

        /*Enable RxIE*/
        value |= 0x01;
            
    oldLevel = intLock();
    pDev->pBrd->canOutByte(pDev, SJA1000_IER, value);
    intUnlock(oldLevel);
    return;
}


/************************************************************************
*
* SJA1000_DisableInt -  disable interrupts from the CAN device to the CPU
*
* This function disables interrupts from the CAN controller from reaching the
* CPU. Typically, disables a global interrupt mask bit at the CPU level.
*
* RETURNS: N/A
*
* ERRNO:   N/A
*
*/
static void SJA1000_DisableInt(struct WNCAN_Device *pDev)
{
    int    oldLevel;

    oldLevel = intLock();
    pDev->pBrd->canOutByte(pDev, SJA1000_IER, 0);
    intUnlock(oldLevel);

    return;
}

/************************************************************************
*
* SJA1000_GetBusError - get the bus error
*
* This function returns an ORed bit mask of all the bus errors that have
* occured during the last bus activity.
* Bus errors returned by this function can have the following values:
* WNCAN_ERR_NONE: No errors detected
* WNCAN_ERR_BIT: Bit error. 
* WNCAN_ERR_ACK: Acknowledgement error. 
* WNCAN_ERR_CRC: CRC error
* WNCAN_ERR_FORM: Form error
* WNCAN_ERR_STUFF: Stuff error
* WNCAN_ERR_UNKNOWN: this condition should not occur
* The five errors are not mutually exclusive. 
* The occurence of an error will be indicated by an interrupt. Typically, 
* this function will be called from the error interrupt handling case in the
* user's ISR callback function, to find out the kind of error that occured
* on the bus.
*
* RETURNS: the bus error
*
* ERRNO: N/A
*
*/
static WNCAN_BusError SJA1000_GetBusError(struct WNCAN_Device *pDev)
{
    UCHAR value;
    WNCAN_BusError error=0;

    /* read the error code capture register */
    value = pDev->pBrd->canInByte(pDev, SJA1000_ECC);

    switch(value >> 6)
    {
        case 0:
            error |= WNCAN_ERR_BIT;
            break;
        case 1:
            error |= WNCAN_ERR_FORM;
            break;
        case 2:
            error |= WNCAN_ERR_STUFF;
            break;
        case 3:
            switch(value & 0x1F)
            {
                case 8:
                    error |= WNCAN_ERR_CRC;
                    break;
                case 24:
                    error |= WNCAN_ERR_CRC;
                    break;
                case 25:
                    error |= WNCAN_ERR_ACK;
                    break;
                case 27:
                    error |= WNCAN_ERR_ACK;
                    break;
                default:
                    error |= WNCAN_ERR_NONE;
                    break;
            }
            break;
    }
    return error;
}

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
* sja1000ISR - interrupt service routine
*
* RETURNS: N/A
*
* ERRNO: N/A
*
*/
void sja1000ISR(ULONG context)
{
    WNCAN_DEVICE     *pDev;
    WNCAN_IntType  intStatus=WNCAN_INT_NONE;
    UCHAR          chnNum;      


    pDev = (WNCAN_DEVICE *)context;

    /* notify board that we're entering isr */
    if(pDev->pBrd->onEnterISR)
        pDev->pBrd->onEnterISR(pDev);

    /* Get the interrupt status */
    intStatus = SJA1000_GetIntStatus((void*)context, &chnNum);

         if (intStatus != WNCAN_INT_NONE)
         {
                 
                 pDev->pISRCallback(pDev, intStatus, chnNum);
                 
                 if (intStatus == WNCAN_INT_RX)
                 {
                         /* release the receive buffer */
                         /* this will temporarily cleat RI bit */
                         pDev->pBrd->canOutByte(pDev, SJA1000_CMR, CMR_RRB);
                 }
                 else if (intStatus == WNCAN_INT_ERROR)
                 {
                         /* clear bei interrupt flag */
                         pDev->pBrd->canInByte(pDev, SJA1000_ECC);
                 }
                 else if (intStatus == WNCAN_INT_TX)
                 {
                         /* notify channel available to TX again */
                         pDev->pISRCallback(pDev, WNCAN_INT_TXCLR, chnNum);
                 }
         }
        
        
    /* notify board that we're leaving isr */
    if(pDev->pBrd->onLeaveISR)
        pDev->pBrd->onLeaveISR(pDev);

   return;
}


/************************************************************************
*
* SJA1000_ReadID - read the CAN Id
*
* This function reads the CAN Id corresponding to the channel number on
* the controller. The standard or extended flag is set by the function.
* The mode of the channel cannot be WNCAN_CHN_INACTIVE or WNCAN_CHN_INVALID
*
* RETURNS:        LONG: the CAN Id; on error return -1
*
* ERRNO:          S_can_illegal_channel_no,
*                 S_can_illegal_config
*
*/
static long SJA1000_ReadID(struct WNCAN_Device *pDev, UCHAR channelNum, 
                           BOOL* ext)
{
    UCHAR           value;
    struct TxMsg    *pTxMsg;
    ULONG           msgID=0xffffffff;

    if (channelNum >= SJA1000_MAX_MSG_OBJ)
    {
        errnoSet(S_can_illegal_channel_no);
    }
    else if((pDev->pCtrl->chnMode[channelNum] == WNCAN_CHN_INACTIVE) ||
            (pDev->pCtrl->chnMode[channelNum] == WNCAN_CHN_INVALID))
    {
        errnoSet(S_can_illegal_config);
    }
    else
    {
        /* Set up frame and ID registers based on channel mode */
        if(pDev->pCtrl->chnMode[channelNum] == WNCAN_CHN_RECEIVE)
        {
            /* get the frame format */
            value = pDev->pBrd->canInByte(pDev, SJA1000_SFF);

            /* test if message ID is extended or standard */
            if (value & 0x80)
            {
                *ext = 1;
                value = pDev->pBrd->canInByte(pDev,SJA1000_RXID);
                msgID = (value << (24-3));

                value = pDev->pBrd->canInByte(pDev,SJA1000_RXID+1);
                msgID |= (value << (16-3));

                value = pDev->pBrd->canInByte(pDev,SJA1000_RXID+2);
                msgID |= (value << (8-3));

                value = pDev->pBrd->canInByte(pDev,SJA1000_RXID+3);
                msgID |= (value >> 3);
            }
            else
            {
                *ext = 0;
                value = pDev->pBrd->canInByte(pDev,SJA1000_RXID);
                msgID = (value << (8-5));
                value = pDev->pBrd->canInByte(pDev,SJA1000_RXID+1);
                msgID |= (value >> 5);
            }
        }
        else
        {
            pTxMsg = (struct TxMsg *)pDev->pCtrl->csData;
            *ext = pTxMsg->ext;
            msgID = pTxMsg->id;
        }
    }
    return ((long) msgID);
}

/************************************************************************
*
* SJA1000_WriteID - write the CAN Id to the channel number
*
* This function writes the CAN Id to the channel. The type of Id 
* (standard or extended) is specified. the behaviour of the function for
* every channel mode is specified as follows:
* WNCAN_CHN_INVALID: not allowed. ERROR is returned.
* WNCAN_CHN_INACTIVE: not allowed. ERROR is returned.
* WNCAN_CHN_RECEIVE: CAN messages with this Id will be received.
* WNCAN_CHN_TRANSMIT: CAN meesage with the specified ID will be transmitted
* when CAN_Tx is called
*
* If this function is called while the controller is online, receive errors
* may occur while the receive buffer id is being changed. 
*
*
* RETURNS:        OK, or ERROR
*
* ERRNO:          S_can_illegal_channel_no,
*                 S_can_illegal_config
*
*/

static STATUS SJA1000_WriteID(struct WNCAN_Device *pDev, UCHAR channelNum,
                             unsigned long canId, BOOL ext)
{
        struct TxMsg    *pTxMsg;
    UCHAR           value, regVal, rxIEOff;             
    STATUS          retCode = ERROR; /* pessimistic */  
    UCHAR       regMod;
        
    if (channelNum >= SJA1000_MAX_MSG_OBJ)
    {
        errnoSet(S_can_illegal_channel_no);
    }
    else if((pDev->pCtrl->chnMode[channelNum] == WNCAN_CHN_INACTIVE) ||
                (pDev->pCtrl->chnMode[channelNum] == WNCAN_CHN_INVALID))
    {
        errnoSet(S_can_illegal_config);
    }
    else
    {
        if(pDev->pCtrl->chnMode[channelNum] == WNCAN_CHN_RECEIVE)
        {
                        /*check if receive interrupts are turned on. If yes, disable
                        interrupts */
                        regVal = pDev->pBrd->canInByte(pDev,SJA1000_IER);
                        if(regVal & IER_RIE)
                        {
                                rxIEOff = regVal & ~(IER_RIE);
                                pDev->pBrd->canOutByte(pDev, SJA1000_IER, rxIEOff);
                                
                        }
                        
            /* Put the controller into reset mode */
            regMod = pDev->pBrd->canInByte(pDev, SJA1000_MOD);
            if((regMod&MOD_RM)==0)
            	pDev->pBrd->canOutByte(pDev, SJA1000_MOD, regMod | MOD_RM);
                        
                        
            if (ext == TRUE)
            {
                value = canId >> (24-3);
                pDev->pBrd->canOutByte(pDev, SJA1000_ACR0, value);
                value = canId >> (16-3);
                pDev->pBrd->canOutByte(pDev, SJA1000_ACR1, value);
                value = canId >> (8-3);
                pDev->pBrd->canOutByte(pDev, SJA1000_ACR2, value);
                value = canId << 3;
                pDev->pBrd->canOutByte(pDev, SJA1000_ACR3, value);
                                
            }
            else
            {
                value = canId >> 3;
                pDev->pBrd->canOutByte(pDev, SJA1000_ACR0, value);
                value = canId << 5;
                pDev->pBrd->canOutByte(pDev, SJA1000_ACR1, value);
                                
                        }
            /* put controller back into normal mode */
            if((regMod&MOD_RM)==0)
            	pDev->pBrd->canOutByte(pDev,  SJA1000_MOD, regMod);
                        
            /* restore receive interrupts if previously set*/
                        if(regVal & IER_RIE)
                        {
                                pDev->pBrd->canOutByte(pDev, SJA1000_IER, regVal);
                        }
                        
            
        }
        else
        {
            pTxMsg = (struct TxMsg *)pDev->pCtrl->csData;
            pTxMsg->id = canId;
            pTxMsg->ext = ext;
        }
                
        retCode = OK;
    }
        
    return retCode;
}


/************************************************************************
*
* SJA1000_ReadData - read 0 to 8 bytes of data from channel
*
* This function reads "len" bytes of data from the channel and sets the value
* of len to the number of bytes read. The range of "len" is zero (for zero 
* length data) to a maximum of eight. The mode of the channel must not be 
* WNCAN_CHN_INVALID or WNCAN_CHN_INACTIVE; however, the newData flag is valid 
* WNCAN_CHN_RECEIVE channel mode.
* For receive channels, if no new data has been received since the last
* CAN_ReadData  function call, the newData flag is set to FALSE; 
* otherwise, the flag is TRUE. In both cases, the data and length of the
* data currently in the channel are read. 
*
* RETURNS:        OK, or ERROR
*
* ERRNO:          S_can_illegal_channel_no,
*                 S_can_illegal_config,
*                 S_can_buffer_overflow,
*                 S_can_null_input_buffer
*
*/
static STATUS SJA1000_ReadData(struct WNCAN_Device *pDev, UCHAR channelNum,
                       UCHAR *data, UCHAR *len, BOOL *newData)
{
    UCHAR           value;
    UCHAR           hwLength=0;
    UINT            i;
    UINT            offset;
    struct TxMsg    *pTxMsg;

    STATUS          retCode = ERROR; /* pessimistic */

    if (channelNum >= SJA1000_MAX_MSG_OBJ)
    {
        errnoSet(S_can_illegal_channel_no);
    }
    else if((pDev->pCtrl->chnMode[channelNum] == WNCAN_CHN_INACTIVE) ||
            (pDev->pCtrl->chnMode[channelNum] == WNCAN_CHN_INVALID))
    {
        errnoSet(S_can_illegal_config);
    }
    else
    {
        if(data == NULL)
        {
         errnoSet(S_can_null_input_buffer);
             return retCode;     
        }

        if(pDev->pCtrl->chnMode[channelNum] == WNCAN_CHN_RECEIVE)
        {
            /* get the frame info and read the data */
            value = pDev->pBrd->canInByte(pDev, SJA1000_SFF);
            offset = (value & 0x80)? 2 : 0;
            hwLength = value & 0xF;

            if (hwLength > *len)
            {
             /*
              If the actual number of bytes in the message are greater than
              expected bytes by user, set error no and do not copy message
              DLC into *len
             */
             errnoSet(S_can_buffer_overflow);

            }
            else
            {
             /*If the message DLC and the expected data length is equal,
               copy message DLC into *len and continue */
               *len = hwLength;
               retCode = OK;
            }

            for (i = 0; i < *len; i++)
                data[i] = pDev->pBrd->canInByte(pDev, (SJA1000_SFDATA + i + offset));

            /* check the status register to see if the message was new */
            value = pDev->pBrd->canInByte(pDev, SJA1000_SR);
            if(value & SJA1000_SR_RBS)
            {
                *newData = 1;
                /* release the receive buffer */
                pDev->pBrd->canOutByte(pDev, SJA1000_CMR, CMR_RRB);
            }
            else
                *newData = FALSE;
        }
        else
        {

            pTxMsg = (struct TxMsg *)pDev->pCtrl->csData;
                        
                        /*transmit channel*/
                        /*check for overflow*/
                        if(pTxMsg->len > *len)
                        {
                                errnoSet(S_can_buffer_overflow);
                        }
                        else
                        {
                                *len = pTxMsg->len;
                                retCode = OK;
                        }
            
                        for(i = 0 ; i < *len ; i++)
                data[i] = pTxMsg->data[i];
                                
        }

    }

    return retCode;
}

/************************************************************************
*
* SJA1000_GetMessageLength - get the message length
*
* This function returns the length of the message data in the channel on the 
* controller. The minimum value returned is 0, and the maximum value is 8. 
* This number is equal to the "len" argument in CAN_ReadData. If the data 
* has zero length, this function returns zero. The mode of the channel
* must not be WNCAN_CHN_INACTIVE or WNCAN_CHN_INVALID
*
* RETURNS:        length of data or -1 if error
*
* ERRNO:          S_can_illegal_channel_no, S_can_illegal_config
*
*/
static int SJA1000_GetMessageLength
      (
          struct WNCAN_Device *pDev,
          UCHAR channelNum
          )
{
    struct TxMsg *pTxMsg;
    int retLen = -1; /* pessimistic */


    if (channelNum >= SJA1000_MAX_MSG_OBJ)
    {
        errnoSet(S_can_illegal_channel_no);
    }
    else if((pDev->pCtrl->chnMode[channelNum] == WNCAN_CHN_INACTIVE) ||
            (pDev->pCtrl->chnMode[channelNum] == WNCAN_CHN_INVALID))
    {
        errnoSet(S_can_illegal_config);
    }
    else
    {
        if(pDev->pCtrl->chnMode[channelNum] != WNCAN_CHN_TRANSMIT)
                {
            retLen = pDev->pBrd->canInByte(pDev, SJA1000_SFF ) & 0x0f;
                }
        else
        {
            pTxMsg = (struct TxMsg *)pDev->pCtrl->csData;
            retLen = pTxMsg->len;
        }
    }

    return retLen;
}

/************************************************************************
*
* SJA1000_WriteData - write "len" bytes of data to the channel
*
* This function writes "len" bytes of data to the channel. An error is returned
* if the number of bytes to be written exceed 8. The mode of the channel must
* WNCAN_CHN_TRANSMIT.
*
* RETURNS:        ERROR if an input parameter is invalid, OK otherwise.
*   
* ERRNO:          S_can_illegal_channel_no,
*                 S_can_illegal_config,
*                 S_can_illegal_data_length,
*                 S_can_null_input_buffer
*
*/
static STATUS
SJA1000_WriteData(struct WNCAN_Device *pDev, UCHAR channelNum,
                 UCHAR *data, UCHAR len)
{
    STATUS retCode = ERROR; /* pessimistic */

    struct TxMsg    *pTxMsg;
    UINT            i;


    if (channelNum >= SJA1000_MAX_MSG_OBJ)
    {
        errnoSet(S_can_illegal_channel_no);
        return retCode;
    }

    if(pDev->pCtrl->chnMode[channelNum] != WNCAN_CHN_TRANSMIT)
    {
        errnoSet(S_can_illegal_config);
        return retCode;
    }

    if(len > 8)
    {
       errnoSet(S_can_illegal_data_length);
       return retCode;
    }

    if(data == NULL)
    {
       errnoSet(S_can_null_input_buffer);
       return retCode;
    }

    pTxMsg = (struct TxMsg *)pDev->pCtrl->csData;
    pTxMsg->len  = len;
    for(i = 0 ; i < pTxMsg->len; i++)
        pTxMsg->data[i] = data[i];

    retCode = OK;

    return retCode;
}

/************************************************************************
*
* SJA1000_TxMsg - transmits a CAN message
*
* This function performs the same function as the following series
* of function calls:
*
* 1. CAN_WriteID(context,channelNum,canID,ext);
* 2. CAN_WriteData(context,channelNum,data,len);
* 3. CAN_Tx(context,channel);
*
* The mode of the channel must be WNCAN_CHN_TRANSMIT. If the length
* specified exceeds 8 byes an error will be returned.
*
* RETURNS:        ERROR if an input parameter is invalid, OK otherwise.
*   
* ERRNO:          S_can_illegal_channel_no,
*                 S_can_illegal_config
*                 S_can_illegal_data_length,
*                 S_can_null_input_buffer
*
*/
static STATUS SJA1000_TxMsg
      (
          struct WNCAN_Device *pDev,
          UCHAR channelNum,
      ULONG canId,
          BOOL ext,
          UCHAR *data,
          UCHAR len
       )
{

    UCHAR  value;
    UINT   i;
    UINT   ndx;
    struct TxMsg *pTxMsg;

    STATUS retCode = ERROR; /* pessimistic */

    if (channelNum >= SJA1000_MAX_MSG_OBJ)
    {
       errnoSet(S_can_illegal_channel_no);
       return retCode;
    }
     
        if(pDev->pCtrl->chnMode[channelNum] != WNCAN_CHN_TRANSMIT)           
    {
                errnoSet(S_can_illegal_config);
                return retCode;
        }

    if(len > 8)
    {
      errnoSet(S_can_illegal_data_length);
      return retCode;
    }

    if(data == NULL)
    {
     errnoSet(S_can_null_input_buffer);
     return retCode;
    }

    /* check if tx request is set before transmitting */
     value = pDev->pBrd->canInByte(pDev, SJA1000_SR);

        if((value & SJA1000_SR_TBS) == 0)
        {
                errnoSet(S_can_busy);
                return retCode;
        }

        /* save a copy */
        pTxMsg = (struct TxMsg *)pDev->pCtrl->csData;

        pTxMsg->id = canId;
        pTxMsg->ext = ext;
        pTxMsg->len = len;
        
        /*
        * write data if channel mode is WNCAN_CHN_TX
        */
        for(i = 0 ; i < len ; i++)
                pTxMsg->data[i] = data[i];
        

        /* set up the frame info reg. */
        value = (ext)? 0x80 : 0;

        if(pTxMsg->rtr)
                value |= 0x40;

        value |= len;

        pDev->pBrd->canOutByte(pDev, SJA1000_SFF, value);


        /* write the ID */
        ndx = SJA1000_TXID;
        if (ext == TRUE)
        {
                value = canId >> (24 - 3);
                pDev->pBrd->canOutByte(pDev, ndx++, value);

                value = canId >> (16 - 3);
                pDev->pBrd->canOutByte(pDev, ndx++, value);

                value = canId >> (8 - 3);
                pDev->pBrd->canOutByte(pDev, ndx++, value);

                value = canId << 3;
                pDev->pBrd->canOutByte(pDev, ndx++, value);
        }
        else
        {
                value = canId >> ( 8 - 5);
                pDev->pBrd->canOutByte(pDev, ndx++, value);

                value = canId << 5;
                pDev->pBrd->canOutByte(pDev, ndx++, value);
        }


        /* write data */
        for(i = 0 ; i < len ; i++)
                pDev->pBrd->canOutByte(pDev, ndx++, data[i]);

        /* Request a transmission */
        pDev->pBrd->canOutByte(pDev, SJA1000_CMR, CMR_TR);

        retCode = OK;
    return retCode;
}


/************************************************************************
*
* SJA1000_Tx - transmit the CAN message
*
* This function transmits the CAN Id and data currently in the channel of
* the specified controller on the device. The mode of the channel must be
* WNCAN_CHN_TRANSMIT
*
* RETURNS:        OK, or ERROR
*
* ERRNO:          S_can_illegal_channel_no, S_can_illegal_config
*
*/
static STATUS SJA1000_Tx
      (
          struct WNCAN_Device *pDev,
          UCHAR channelNum
          )
{
    UCHAR        value;
    UINT         ndx;
    UINT         i;
    struct TxMsg *pTxMsg;
    STATUS retCode = ERROR; /* pessimistic */

    if (channelNum >= SJA1000_MAX_MSG_OBJ)
    {
        errnoSet(S_can_illegal_channel_no);
                return retCode;
    }
    
        if(pDev->pCtrl->chnMode[channelNum] != WNCAN_CHN_TRANSMIT)     
    {
        errnoSet(S_can_illegal_config);
                return retCode;
    }
        /* Check the transmit request buffer. If locked, return CAN_BUSY. */
        value = pDev->pBrd->canInByte(pDev, SJA1000_SR);

        if ((value & SJA1000_SR_TBS) == 0)
        {
                errnoSet(S_can_busy);
                return retCode;
        }

        pTxMsg = (struct TxMsg *)pDev->pCtrl->csData;

        value = (pTxMsg->ext)? 0x80 : 0;
        if(pTxMsg->rtr)
                value |= 0x40;
        value |= pTxMsg->len;
        pDev->pBrd->canOutByte(pDev, SJA1000_SFF, value);

        ndx = SJA1000_TXID;

        /* write the identifier */
        if(pTxMsg->ext)
        {
                pDev->pBrd->canOutByte(pDev, ndx++, pTxMsg->id >> (24 - 3));
                pDev->pBrd->canOutByte(pDev, ndx++, pTxMsg->id >> (16 - 3));
                pDev->pBrd->canOutByte(pDev, ndx++, pTxMsg->id >> (8 - 3));
                pDev->pBrd->canOutByte(pDev, ndx++, pTxMsg->id << 3);
        }
        else
        {
                pDev->pBrd->canOutByte(pDev, ndx++, pTxMsg->id >> (8 - 5));
                pDev->pBrd->canOutByte(pDev, ndx++, pTxMsg->id << 5);
        }

        /* write data */
        for (i = 0; i < pTxMsg->len ; i++)
                pDev->pBrd->canOutByte(pDev, ndx++, pTxMsg->data[i]);

        /* Request a transmission */
        pDev->pBrd->canOutByte(pDev, SJA1000_CMR, CMR_TR);

        retCode = OK;

    return retCode;
}


/************************************************************************
*
* SJA1000_SetGlobalRxFilter - set the global receive filter
*
* This function sets the global HW filter mask for incoming messages on the 
* device controller. If the controller does not have a global filter this
* function is a no-op. If the ext parameter is TRUE, the mask value applies
* to extended messages; otherwise, the mask values applies to standard 
* messages. A value of "0" for a particular bit position of the mask means 
* don't care (i.e. the incoming message Id could have a value of zero or 
* one for this bit position); otherwise, a value of 1 means the 
* corresponding bit of the message Id must match identically.
*
* WARNING: Only one global filter exists on the controller. This is shared
* by standard and extended identifiers. These routines cannot be called from
* an interript level.
*
* RETURNS: OK 
*
* ERRNO:   N/A
*
*/
static STATUS SJA1000_SetGlobalRxFilter
(
    struct WNCAN_Device *pDev,
    long  inputMask,
    BOOL  ext
)
{
        STATUS      retCode=OK;
    UCHAR       value;
    UCHAR       regMod;
        ULONG       mask;

   /* Take the complement of the mask because a 1
      means don't care with the SJA1000 */

    mask = ~inputMask;

    /* Put the controller into reset mode */
    regMod = pDev->pBrd->canInByte(pDev, SJA1000_MOD);
    pDev->pBrd->canOutByte(pDev, SJA1000_MOD, regMod | MOD_RM);

    if (ext == FALSE)
    {
                value = mask >> 3;
        pDev->pBrd->canOutByte(pDev, SJA1000_AMR0, value);

        value = (mask << 5) | 0x1f;     
        pDev->pBrd->canOutByte(pDev, SJA1000_AMR1, value);

         /* write don't care in the rest */
        pDev->pBrd->canOutByte(pDev, SJA1000_AMR2, 0xff);
        pDev->pBrd->canOutByte(pDev, SJA1000_AMR3, 0xff);               

    }
    else
    {
        value = mask  >> (24-3);
        pDev->pBrd->canOutByte(pDev, SJA1000_AMR0, value);

        value = mask  >> (16-3);
        pDev->pBrd->canOutByte(pDev, SJA1000_AMR1, value);

        value = mask  >> (8-3);
        pDev->pBrd->canOutByte(pDev, SJA1000_AMR2, value);

        value = (mask  << 3) | 0x07;    
        pDev->pBrd->canOutByte(pDev, SJA1000_AMR3, value);
    }

    /* put controller back into normal mode */
    pDev->pBrd->canOutByte(pDev,  SJA1000_MOD, regMod);

    /* Wait until the controller comes out of reset */
    while(pDev->pBrd->canInByte(pDev, SJA1000_MOD) != regMod) {};

    /* Wait for Bus OK*/    
    if(SJA1000_GetBusStatus != WNCAN_BUS_OK)
                taskDelay(1);

    return retCode;
}

/************************************************************************
*
* SJA1000_GetGlobalRxFilter - Gets the global HW filter mask programmed.
*
* This function return the programmed value in the Global filter resgiter
* Based on the value of ext passed to the function, this function reads the 
* appropriate format of the global mask, and returns the value in the specified,
* extended or standard identifier format.
* If the controller does not have a global mask, a value of -1 is returned.
*
* RETURNS: long: mask
*
* ERRNO:   N/A
*
*/
static long SJA1000_GetGlobalRxFilter
(
    struct WNCAN_Device *pDev,
    BOOL   ext
)
{
        ULONG       mask;    
        UCHAR       value;
    UCHAR       regMod;
    
    /* Put the controller into reset mode */
    regMod = pDev->pBrd->canInByte(pDev, SJA1000_MOD);
    pDev->pBrd->canOutByte(pDev, SJA1000_MOD, regMod | MOD_RM);
    
    if(ext == FALSE)
    {
        value = pDev->pBrd->canInByte(pDev, SJA1000_AMR0);
        mask  = (UINT)(value) << 3;
        
        value = pDev->pBrd->canInByte(pDev, SJA1000_AMR1) & 0xe0;
        mask |= ((UINT)(value) >> 5);
        
        /* Take the complement of the mask because a 1
            means don't care with the SJA1000 */
        mask = (~mask) & 0x000007ff;
        
    }
    else
    {
        
        value = pDev->pBrd->canInByte(pDev, SJA1000_AMR0);
        mask  = (UINT)(value) << (24-3);
        
        value = pDev->pBrd->canInByte(pDev, SJA1000_AMR1);
        mask |= (UINT)(value) << (16-3);
        
        value = pDev->pBrd->canInByte(pDev, SJA1000_AMR2);
        mask |= (UINT)value << (8-3);
        
        value = pDev->pBrd->canInByte(pDev, SJA1000_AMR3) & 0xf8;
        mask |= ((UINT)(value) >> 3);
        
        /* Take the complement of the mask because a 1
            means don't care with the SJA1000 */
        mask = (~mask) & 0x1fffffff;
        
    }
    
    /* put controller back into normal mode */
    pDev->pBrd->canOutByte(pDev,  SJA1000_MOD, regMod);
    
    /* Wait until the controller comes out of reset */
    while(pDev->pBrd->canInByte(pDev, SJA1000_MOD) != regMod) {};
    
    /* Wait for Bus OK or a count of 1000*/    
    if(SJA1000_GetBusStatus != WNCAN_BUS_OK)
        taskDelay(1);
    
    return (long)mask;
}

/************************************************************************
* SJA1000_SetLocalMsgFilter -  set local message object filter
*
* This function sets a local message object filter for incoming messages on a 
* particular channel. 
*
* If the ext parameter is TRUE, the mask value applies to extended messages;
* otherwise, the mask value applies to standard messages. A value of "0" for
* a particular bit position of the mask means don't care (i.e. the incoming
* message Id could have a value of zero or one for this bit position);
* otherwise, a value of 1 means the corresponding bit of the message Id must
* match identically. Channel number is provided for controllers that have
* more than one local message object filter. 
*
* RETURNS: ERROR
*
* ERRNO: S_can_hwfeature_not_available
*
*/
static STATUS SJA1000_SetLocalMsgFilter
      (
          struct WNCAN_Device *pDev, 
          UCHAR channel,
          long mask,
          BOOL ext
          )
{
        errnoSet(S_can_hwfeature_not_available);
    return ERROR;
}

/************************************************************************
*
* SJA1000_GetLocalMsgFilter -  get local message object filter
*
* This function returns the programmed value in the local filter resgiter
* Based on the value of ext passed to the function, this function reads the 
* appropriate format of the local mask, and returns the value in the specified,
* extended or standard identifier format.
* The channel argument identifies the local filter associated with the particular
* channel number
*
* If the controller does not have a global mask, a value of -1 is returned.
* This is not a valid CAN ID.
*
* RETURNS: -1 
*
* ERRNO: S_can_hwfeature_not_available
*
*/
static long SJA1000_GetLocalMsgFilter
      (
          struct WNCAN_Device *pDev,
          UCHAR channel,
          BOOL ext
          )
{
        errnoSet(S_can_hwfeature_not_available);
    return -1;
}

/************************************************************************
*
* SJA1000_IsMessageLost - test if the message is lost
*
* This function tests if the current message data in the channel overwrote 
* the old message data before CAN_ReadData() was called. Valid only for 
* channels with mode = WNCAN_CHN_RECEIVE.
*
* RETURNS:        0 if FALSE, 1 if TRUE, or -1 if ERROR
*
* ERRNO:          S_can_illegal_channel_no, S_can_illegal_config
*
*/
static int SJA1000_IsMessageLost
      (
          struct WNCAN_Device *pDev,
          UCHAR channelNum
          )
{
    int retCode = -1; /* pessimistic */
    UCHAR  value;

    if (channelNum >= SJA1000_MAX_MSG_OBJ)
    {
        errnoSet(S_can_illegal_channel_no);
    }
    else if(pDev->pCtrl->chnMode[channelNum] != WNCAN_CHN_RECEIVE)
    {
        errnoSet(S_can_illegal_config);
    }
    else
    {
        value = pDev->pBrd->canInByte(pDev, SJA1000_SR);
        if (value & SJA1000_SR_DOS)                
            retCode = 1;        
        else
            retCode = 0;
    }

    return retCode;
}

/************************************************************************
*
* SJA1000_ClearMessageLost - Clear message lost indication 
*
* This function clears data overrun flag for a particular channel
* Valid only for channels with mode = WNCAN_CHN_RECEIVE
*
* RETURNS:        OK or ERROR
*
* ERRNO:          S_can_illegal_channel_no,
*                 S_can_illegal_config
*
*/
static STATUS SJA1000_ClearMessageLost
      (
          struct WNCAN_Device *pDev,
          UCHAR channelNum
          )
{
        STATUS retCode = ERROR; 

    if (channelNum >= SJA1000_MAX_MSG_OBJ)
    {
        errnoSet(S_can_illegal_channel_no);
    }
    else if(pDev->pCtrl->chnMode[channelNum] != WNCAN_CHN_RECEIVE)
    {
        errnoSet(S_can_illegal_config);
    }
    else
    {
        /* clear data overrun */
         pDev->pBrd->canOutByte(pDev, SJA1000_CMR, CMR_CDO);
                 retCode = OK;
    }

    return retCode;


}

/************************************************************************
* SJA1000_IsRTR - test if the RTR bit is set
*
* This function tests if the message has the RTR bit set. The mode of the 
* channel must be WNCAN_CHN_TRANSMIT or WNCAN_CHN_RECEIVE
* This function can be used on simple controllers such as the SJA1000 only
*
* RETURNS:        0 if FALSE, 1 if TRUE, or -1 if ERROR
*   
* ERRNO:          S_can_illegal_channel_no,
*                 S_can_illegal_channel_mode,
*
*/
static int SJA1000_IsRTR
      (
          struct WNCAN_Device *pDev,
          UCHAR channelNum
          )
{
    int retCode = -1; /* pessimistic */
    struct TxMsg *pTxMsg;
        UCHAR value;



    if (channelNum >= SJA1000_MAX_MSG_OBJ)
    {
        errnoSet(S_can_illegal_channel_no);
    }
    else if((pDev->pCtrl->chnMode[channelNum] != WNCAN_CHN_TRANSMIT) &&
                    (pDev->pCtrl->chnMode[channelNum] != WNCAN_CHN_RECEIVE))
    {
        errnoSet(S_can_illegal_channel_mode);
    }
    else
    {
                if(pDev->pCtrl->chnMode[channelNum] == WNCAN_CHN_TRANSMIT)
                {
                        pTxMsg = (struct TxMsg *)pDev->pCtrl->csData;
                        if(pTxMsg->rtr)
                                retCode = 1;
                        else
                                retCode = 0;
                }
                else
                {
                        /* Read RTR field in the received channel hardware */
                        value = pDev->pBrd->canInByte(pDev,SJA1000_RXID+2);

                        if(value & 0x10)
                                retCode = 1;
                        else
                                retCode = 0;
                }
    }

    return retCode;
}


/************************************************************************
*
* SJA1000_SetRTR - set the RTR bit
*
* This routine sets the RTR bit in the CAN message. The mode of the channel
* must be WNCAN_CHN_TRANSMIT.
*
* RETURNS:        OK, or ERROR
*   
* ERRNO:          S_can_illegal_channel_no,
*                 S_can_illegal_channel_mode
*
*/
static STATUS SJA1000_SetRTR
      (
          struct WNCAN_Device *pDev,
          UCHAR channelNum,
          BOOL rtr
          )
{
    STATUS retCode = ERROR; /* pessimistic */
    struct TxMsg *pTxMsg;
    
    if (channelNum >= SJA1000_MAX_MSG_OBJ)
    {
        errnoSet(S_can_illegal_channel_no);
    }
    else if(pDev->pCtrl->chnType[channelNum] != WNCAN_CHN_TRANSMIT)
    {
        errnoSet(S_can_illegal_channel_mode);        
    }
    else
    {
        pTxMsg = (struct TxMsg *)pDev->pCtrl->csData;
        pTxMsg->rtr = rtr;
        retCode = OK;
    }

    return retCode;
}


/************************************************************************
*
* SJA1000_TxAbort - abort the current CAN transmission
*
* This routine aborts any current CAN transmissions on the controller.
*
* RETURNS:        N/A
*
* ERRNO:          N/A
*
*/
static void SJA1000_TxAbort
      (
          struct WNCAN_Device *pDev
          )
{
    UCHAR value;

    value = pDev->pBrd->canInByte(pDev, SJA1000_CMR);
    pDev->pBrd->canOutByte(pDev, SJA1000_CMR, value | CMR_AT);
    return;
}


/************************************************************************
*
* SJA1000_Sleep - put the CAN controller to sleep.
*
* This routine puts the CAN controller of the device into sleep mode
* if the hardware supports this functionality; otherwise, this function
* is a no-op.aborts any current CAN transmissions on the controller
*
* RETURNS:        Ok or ERROR
*
* ERRNO:          S_can_cannot_sleep_in_reset_mode
*
*/
static STATUS SJA1000_Sleep
      (
          struct WNCAN_Device *pDev
          )
{
    UCHAR value;
        STATUS retCode = ERROR;

        /*Setting of the Sleep bit is not possible in reset mode
          Check if controller is in reset mode, if yes set error no and
          return error
         */
        value = pDev->pBrd->canInByte(pDev, SJA1000_MOD);

        if(value & MOD_RM)
        {
                errnoSet(S_can_cannot_sleep_in_reset_mode);
                return retCode; 
        }

        /*Set Sleep bit */
    pDev->pBrd->canOutByte(pDev, SJA1000_MOD, value | MOD_SM);
        retCode = OK;

    return retCode;
}

/************************************************************************
*
* SJA1000_WakeUp - brings CAN controller out of sleep mode
*
* Auto awake feature is always enabled
*
* RETURNS:        OK always
*   
* ERRNO:          N/A

*/
static STATUS SJA1000_WakeUp
(
 struct WNCAN_Device *pDev
 )
{
    
    volatile UCHAR value;
    
    /*Reset Sleep bit */
    value = pDev->pBrd->canInByte(pDev, SJA1000_MOD);
    /*MOD.4 is sleep bit*/
    value &= ~(MOD_SM);
    pDev->pBrd->canOutByte(pDev, SJA1000_MOD, value);
    
    return OK;
    
}

/************************************************************************
*
* SJA1000_EnableChannel -  enable channel and set interrupts
*                           
* This function marks the channel valid. It also sets the type of channel
* interrupt requested. If the channel type does not match the type of
* interrupt requested it returns an error.
*
* RETURNS: OK if successful, ERROR otherwise
*   
* ERRNO: S_can_illegal_channel_no
*        S_can_illegal_config
*        S_can_invalid_parameter
*        S_can_incompatible_int_type
*
*/

static STATUS SJA1000_EnableChannel
(
    struct WNCAN_Device *pDev,
    UCHAR                channelNum,
    WNCAN_IntType        intSetting
)
{
    UCHAR  regVal;
    STATUS retCode = ERROR; /* pessimistic */

    if (channelNum >= SJA1000_MAX_MSG_OBJ)
    {
        errnoSet(S_can_illegal_channel_no);
                return retCode;
    }
        regVal = pDev->pBrd->canInByte(pDev,SJA1000_IER);

        switch(pDev->pCtrl->chnMode[channelNum]) {
        case WNCAN_CHN_TRANSMIT:
                
                if(intSetting == WNCAN_INT_RX)
                {
                        errnoSet(S_can_incompatible_int_type);
                        return retCode;
                }

                if(intSetting == WNCAN_INT_TX)
                        regVal |= IER_TIE;
                else
                        regVal &= ~(IER_TIE);           
                        

                pDev->pBrd->canOutByte(pDev, SJA1000_IER, regVal);
                retCode = OK;
                break;

        case WNCAN_CHN_RECEIVE:

                if(intSetting == WNCAN_INT_TX)
                {
                        errnoSet(S_can_incompatible_int_type);
                        return retCode;
                }

                if(intSetting == WNCAN_INT_RX)
                        regVal |= IER_RIE;
                else
                        regVal &= ~(IER_RIE);
                
                pDev->pBrd->canOutByte(pDev, SJA1000_IER, regVal);
                retCode = OK;
                break;

    default:
                 errnoSet(S_can_illegal_config);
         break;
        }

    return retCode;
}

/************************************************************************
*
* SJA1000_DisableChannel -  reset channel interrupts and mark channel
*                           invalid
*
* RETURNS: OK if successful, ERROR otherwise
*
* ERRNO: S_can_illegal_channel_no
*        S_can_illegal_config 
*
*/

static STATUS SJA1000_DisableChannel
(
    struct WNCAN_Device *pDev,
    UCHAR channelNum
)
{
    UCHAR  regVal;
    STATUS retCode = ERROR; /* pessimistic */

    if (channelNum >= SJA1000_MAX_MSG_OBJ)
    {
        errnoSet(S_can_illegal_channel_no);
    }
    else
    {
        regVal = pDev->pBrd->canInByte(pDev,SJA1000_IER);

        switch(pDev->pCtrl->chnMode[channelNum])
        {
            case WNCAN_CHN_TRANSMIT:
                regVal &= ~(IER_TIE);
                retCode = OK;
                break;

            case WNCAN_CHN_RECEIVE:
                regVal &= ~(IER_RIE);
                pDev->pBrd->canOutByte(pDev, SJA1000_IER, regVal);
                retCode = OK;
                break;
                                
            default:
                errnoSet(S_can_illegal_config);
                break;
        }

                pDev->pBrd->canOutByte(pDev, SJA1000_IER, regVal);
    }
    return retCode;
}

/******************************************************************************
*
* SJA1000_WriteReg -  Access hardware register and write data
*
* This function accesses the CAN controller memory at the offset specified. 
* A list of valid offsets are defined as macros in a CAN controller specific
* header file, named as controllerOffsets.h (e.g. toucanOffsets.h or sja1000Offsets.h)
* A pointer to the data buffer holding the data to be written is passed to the
* function in UBYTE *data. The number of data bytes to be copied from the data
* buffer to the CAN controller's memory area is specified by the length parameter.
*
* RETURNS: OK or ERROR
*
* ERRNO: S_can_illegal_offset
*
*/
static STATUS SJA1000_WriteReg
      (
          struct WNCAN_Device *pDev,
          UINT offset,
          UCHAR * data,
          UINT length
          )
{
        STATUS retCode = ERROR;
        UINT i;

        if((offset + length) > SJA1000_MAX_OFFSET)      
        {
                errnoSet(S_can_illegal_offset);         
                return retCode;
        }


        for(i=0;i<length;i++)
        {
                pDev->pBrd->canOutByte(pDev, (offset + i),data[i]);
        }

        retCode = OK;
        return retCode;

}

/******************************************************************************
*
* SJA1000_ReadReg -  Access hardware register and reads data
*
* This function accesses the CAN controller memory at the offset specified. A list
* of valid offsets are defined as macros in a CAN controller specific header file,
* named as controllerOffsets.h (e.g. toucanOffsets.h or sja1000Offsets.h)
* A pointer to the data buffer to hold the data to be read, is passed through 
* UINT *data. The length of the data to be copied is specified through the 
* length parameter.
*
*
* RETURNS: N/A
*
* ERRNO: S_can_illegal_offset
*
*/
static STATUS SJA1000_ReadReg
      (
          struct WNCAN_Device *pDev,
          UINT offset,
          UCHAR *data,
          UINT  length
          )
{
        UINT  i;
        STATUS retCode = ERROR;
        
        if(data == NULL) 
        {
                errnoSet(S_can_null_input_buffer);                                      
                return retCode;
        }

        if(offset > SJA1000_MAX_OFFSET) 
        {
                errnoSet(S_can_illegal_offset);                                 
                return retCode;
        }
        
        
        for(i=0;i<length;i++)
        {               
                data[i] = (UCHAR)pDev->pBrd->canInByte(pDev, (offset + i));                                                     
        }

        retCode = OK;
        return retCode;

}

/************************************************************************
*
* SJA1000_establishLinks -  set up function pointers in CAN_Device
*
*
* RETURNS: N/A
*
* ERRNO: N/A
*
*/

void  SJA1000_establishLinks(struct WNCAN_Device *pDev)
{
    /* it is assumed that the calling routine has checked
       that pDev is not null */

    pDev->Init              = SJA1000_Init;
        pDev->Start             = SJA1000_Start;
        pDev->Stop              = SJA1000_Stop;
    pDev->SetBitTiming      = SJA1000_SetBitTiming;
        pDev->GetBaudRate       = SJA1000_GetBaudRate;
        pDev->SetIntMask        = SJA1000_SetIntMask;
    pDev->EnableInt         = SJA1000_EnableInt;
    pDev->DisableInt        = SJA1000_DisableInt;
        pDev->GetBusStatus      = SJA1000_GetBusStatus;
    pDev->GetBusError       = SJA1000_GetBusError;
    pDev->ReadID            = SJA1000_ReadID;
    pDev->WriteID           = SJA1000_WriteID;
    pDev->ReadData          = SJA1000_ReadData;
    pDev->GetMessageLength  = SJA1000_GetMessageLength;
    pDev->WriteData         = SJA1000_WriteData;
    pDev->Tx                = SJA1000_Tx;
    pDev->TxMsg             = SJA1000_TxMsg;
    pDev->SetGlobalRxFilter = SJA1000_SetGlobalRxFilter;
    pDev->GetGlobalRxFilter = SJA1000_GetGlobalRxFilter;
    pDev->SetLocalMsgFilter = SJA1000_SetLocalMsgFilter;
    pDev->GetLocalMsgFilter = SJA1000_GetLocalMsgFilter;
    pDev->GetIntStatus      = SJA1000_GetIntStatus;
    pDev->IsMessageLost     = SJA1000_IsMessageLost;
    pDev->ClearMessageLost  = SJA1000_ClearMessageLost;
    pDev->IsRTR             = SJA1000_IsRTR;
    pDev->SetRTR            = SJA1000_SetRTR;
    pDev->TxAbort           = SJA1000_TxAbort;
    pDev->Sleep             = SJA1000_Sleep;
    pDev->WakeUp            = SJA1000_WakeUp;
    pDev->EnableChannel     = SJA1000_EnableChannel;
    pDev->DisableChannel    = SJA1000_DisableChannel;
        pDev->WriteReg          = SJA1000_WriteReg;
        pDev->ReadReg           = SJA1000_ReadReg;

    return;
}


/************************************************************************
*
* sja1000_registration -  add controller node into global list
*
* RETURNS: N/A
*
*/
void sja1000_registration
(
void
)
{
    static ControllerLLNode sja1000_LLNODE;

    sja1000_LLNODE.key = WNCAN_SJA1000;
    sja1000_LLNODE.nodedata.controllerdata.establinks_fn = SJA1000_establishLinks;
    
    CONTROLLERLL_ADD( &sja1000_LLNODE );
}

