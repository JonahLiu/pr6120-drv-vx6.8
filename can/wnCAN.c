/* wnCAN.c - implementation of Windnet CAN Interface */

/* Copyright 2001 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,06oct05,lsg  fix warning
09nov01,dnb modified for integration into Tornado
12jul01,jac written

*/

/*

DESCRIPTION
This file contains the functions that implement the interface defined in the
wnCAN.h header file.

*/

/* includes */
#include <vxWorks.h>
#include <errnoLib.h>
#include <CAN/wnCAN.h>

#include <CAN/canBoard.h>
#include <CAN/canController.h>
#include <CAN/canFixedLL.h>

/* global variables */
const static WNCAN_VersionInfo info = {1,3};

/************************************************************************
*
* WNCAN_GetVersion - return current major and minor revision
*
*
* RETURNS:pointer to WNCAN_VersionInfo
*
* ERRNO: N/A
*
*/

const WNCAN_VersionInfo *WNCAN_GetVersion(void)
{
    return &info;
}

/************************************************************************
*
* defaultISRCallBack - do nothing but return
*
* The default ISR callback serves as a placeholder and does nothing
*
* RETURNS: N/A
*
* ERRNO: N/A
*
*/
static void defaultISRCallback
(
    WNCAN_DEVICE *pDev,
    WNCAN_IntType stat,
    UCHAR chnNum)
{
    return;
}

/************************************************************************
*
* establishLinks - initialize function pointers in the Device structure
*
* This routine connects the function pointers in the CAN_Device
* data structure to the appropriate routines.
*
* RETURNS: OK or ERROR
*
* ERRNO: S_can_invalid_parameter
*
*/

STATUS CAN_DEVICE_establishLinks
(
    WNCAN_DEVICE *pDev,
    WNCAN_BoardType brdType,
    WNCAN_ControllerType ctrlType
)
{
    STATUS retCode = OK;

    /* check for null pointer */
    if(pDev == 0)
    {
        errnoSet(S_can_invalid_parameter);
        retCode = ERROR;
    }
    else
    {
        /* save board and controller types */
        pDev->pCtrl->ctrlType = ctrlType;
        pDev->pBrd->brdType = brdType;

        /* establish controller links */
        retCode = WNCAN_Controller_establishLinks(pDev, ctrlType);

        /* establish board links */
        retCode = WNCAN_Board_establishLinks(pDev, brdType);

        /* set default isr callback */
        pDev->pISRCallback = defaultISRCallback;
    }
    return retCode;
}

/************************************************************************
*
* CAN_InstallISRCallback - initialize ISR callback
*
* RETURNS: OK or ERROR
*
* ERRNO: S_can_invalid_parameter
*
*/

STATUS CAN_InstallISRCallback
(
    struct WNCAN_Device *pDev,
    void (*pFun)(struct WNCAN_Device *pDev2,WNCAN_IntType
                 intStatus, UCHAR channelNum)
)
{
   if(!pDev || !pFun)
   {
        errnoSet(S_can_invalid_parameter);
        return ERROR;
    }
    else
        pDev->pISRCallback = pFun;

    return OK;
}


/************************************************************************
*
* WNCAN_Open - return a handle to the requested WNCAN_DEVICE
*
* RETURNS: pointer to valid WNCAN_DEVICE, or 0 if an error occurred
*
* ERRNO: N/A
*
*/

struct WNCAN_Device *WNCAN_Open
(
    unsigned int brdType,
    unsigned int brdNdx,
    unsigned int ctrlNdx
)
{
    return  WNCAN_Board_Open(brdType, brdNdx, ctrlNdx);
}

/************************************************************************
*
* WNCAN_Close - close the handle to the requested WNCAN_DEVICE
*
* RETURNS: N/A
*
* ERRNO: N/A
*
*/

void WNCAN_Close
(
struct WNCAN_Device *pDev
)
{
        if(pDev != NULL)
                WNCAN_Board_Close(pDev);

    return;
}

/************************************************************************
*
* WNCAN_GetMode - get the channel mode
*
* This routine returns the mode of the channel.
*
* RETURNS: the channel mode
*
* ERRNO:   N/A
*
*/
WNCAN_ChannelMode WNCAN_GetMode(struct WNCAN_Device *pDev, UCHAR chn)
{

    WNCAN_ChannelMode cm = WNCAN_CHN_INVALID;

    if(chn < pDev->pCtrl->numChn)
        cm = pDev->pCtrl->chnMode[chn];

    return cm;
}


/************************************************************************
* WNCAN_SetMode - set the mode of the channel.
*
* This function sets the mode of the channel to one of five values:
* WNCAN_CHN_TRANSMIT, WNCAN_CHN_RECEIVE, WNCAN_CHN_INACTIVE, 
* and in addition for advanced controllers,
* WNCAN_CHN_RTR_REQUESTER, or WNCAN_CHN_RTR_RESPONDER.
* All available channels can be configured to be WNCAN_CHN_INACTIVE.
* The channels available for transmitting or receiving messages are
* determined by the device hardware, and therefore ,may or may not be
* configurable with this function call. If an attempt is made to set the mode 
* of a channel to WNCAN_CHN_RTR_RESPONDER or WNCAN_CHN_RTR_REQUESTER for a
* simple CAN controller such as SJA1000, WNCAN_CHN_INVALID is returned and an
* and errorno is set to reflect the error. 
* The preferred approach is to allow the device driver to manage the channels 
* internally using the CAN_GetTxChannel(), CAN_GetRxChannel(), 
* CAN_GetRTRRequesterChannel(), CAN_GetRTRResponderChannel() and
* CAN_FreeChannel() function calls.
*
* WNCAN_SetMode, does not affect channel setting in hardware.
*
* RETURNS: ERROR if the requested channel number is out of range,
*           OK otherwise 
*
* ERRNO: S_can_illegal_channel_no
*
*/
STATUS WNCAN_SetMode(struct WNCAN_Device *pDev, UCHAR channelNum,
                                         WNCAN_ChannelMode mode)
{
    STATUS retCode = OK;
        
    if(channelNum >= pDev->pCtrl->numChn)
    {
        errnoSet(S_can_illegal_channel_no);
        retCode = ERROR;
    }
    else
    {
        switch(mode)
        {
                case WNCAN_CHN_TRANSMIT:
                        if((pDev->pCtrl->chnType[channelNum] == WNCAN_CHN_TRANSMIT) ||
                                (pDev->pCtrl->chnType[channelNum] == WNCAN_CHN_TRANSMIT_RECEIVE))
                        {
                                pDev->pCtrl->chnMode[channelNum] = WNCAN_CHN_TRANSMIT;
                        }
                        else
                        {
                                errnoSet(S_can_illegal_config);
                                retCode = ERROR;
                        }
                        
                        break;
                        
                case WNCAN_CHN_RECEIVE:
                        if((pDev->pCtrl->chnType[channelNum] == WNCAN_CHN_RECEIVE) ||
                                (pDev->pCtrl->chnType[channelNum] == WNCAN_CHN_TRANSMIT_RECEIVE))
                        {
                                
                                pDev->pCtrl->chnMode[channelNum] = WNCAN_CHN_RECEIVE;
                        }
                        else
                        {
                                errnoSet(S_can_illegal_config);
                                retCode = ERROR;
                        }
                        
                        break;
                        
                case WNCAN_CHN_RTR_REQUESTER:
                /* The hardware type of the channel will be transmit and receive only
                        in advanced controllers such as TouCAN and I82527 */
                        if(pDev->pCtrl->chnType[channelNum] == WNCAN_CHN_TRANSMIT_RECEIVE)
                        {
                                pDev->pCtrl->chnMode[channelNum] = WNCAN_CHN_RTR_REQUESTER;
                        }
                        else
                        {
                                errnoSet(S_can_rtr_mode_not_supported);
                                retCode = ERROR;
                        }
                        
                        break;
                        
                case WNCAN_CHN_RTR_RESPONDER:
                        /* The hardware type of the channel will be transmit and receive only
                        in advanced controllers such as TouCAN and I82527 */            
                        if(pDev->pCtrl->chnType[channelNum] == WNCAN_CHN_TRANSMIT_RECEIVE)
                        {
                                pDev->pCtrl->chnMode[channelNum] = WNCAN_CHN_RTR_RESPONDER;
                        }
                        else
                        {
                                errnoSet(S_can_rtr_mode_not_supported);
                                retCode = ERROR;
                        }
                        
                        break;

                case WNCAN_CHN_INVALID:
                        pDev->pCtrl->chnMode[channelNum] = WNCAN_CHN_INVALID;
                        break;          
                        
                case WNCAN_CHN_INACTIVE:
                        pDev->pCtrl->chnMode[channelNum] = WNCAN_CHN_INACTIVE;
                        break;
                        
                default:
                        errnoSet(S_can_illegal_config);
                        retCode = ERROR;
                        break;
        }
    }
    return retCode;
}

/************************************************************************
*
* WNCAN_GetTxChannel - get a free transmit channel
*
* This function returns a free channel uniquely identified by the output 
* argument "channelNum". The mode of the assigned channel is WNCAN_CHN_TRANSMIT,
* A subsequent call to CAN_GetTxChannel() assigns the next available channel
* with a different channel number.
* The channel number remains unavailable to subsequent CAN_GetXXChannel()
* calls until a CAN_FreeChannel() call is invoked on the channel.
* CAN_FreeChannel() makes the channel available and resets the mode to
* WNCAN_CHN_INVALID. After the CAN_GetTxChannel() call, the mode of
* the channel can be changed by the CAN_SetMode(..) function call.
*
* By default, channel is disabled. User must call CAN_EnableChannel, to make the
* channel valid in hardware and enable channel interrupts
*
* RETURNS:  ERROR if no channels are available, OK otherwise.
*
* ERRNO: S_can_no_available_channels, S_can_chn_in_use
*
*/
STATUS WNCAN_GetTxChannel(struct WNCAN_Device *pDev, UCHAR *channelNum)
{
    UCHAR i;

    STATUS retCode = ERROR; /* assume failure */

    for(i = 0; i < pDev->pCtrl->numChn; i++)
    {
        if(pDev->pCtrl->chnMode[i] == WNCAN_CHN_INVALID)
        {
            if(WNCAN_SetMode(pDev,i,WNCAN_CHN_TRANSMIT) == OK)
            {
                *channelNum = i;
                retCode = OK;
                return retCode;
            }
        }

    }

    errnoSet(S_can_no_available_channels);
    return retCode;
}

/************************************************************************
* WNCAN_GetRxChannel - get a free receive channel.
*
* Gets a free receive channel uniquely identified by the output argument 
* "channelNum". The mode of the assigned channel is WNCAN_CHN_RECEIVE, 
* A subsequent call to CAN_GetRxChannel() assigns the next available receive
* channel with a different number.
* The channel number remains unavailable to subsequent CAN_GetXXChannel()
* calls until a CAN_FreeChannel() call is invoked on the channel. 
* CAN_FreeChannel() makes the channel available and resets the mode to
* WNCAN_CHN_INVALID. After the CAN_GetRxChannel() call, the mode of
* the channel can be changed by the CAN_SetMode() function call.
*
* By default, channel is disabled. User must call CAN_EnableChannel, to make the
* channel valid in hardware and enable channel interrupts
*
* RETURNS: ERROR if no channel is available, OK otherwise.
*
* ERRNO: S_can_no_available_channels
*
*/
STATUS WNCAN_GetRxChannel(struct WNCAN_Device *pDev, UCHAR *channelNum)
{
    UCHAR i;

    STATUS retCode = ERROR; /* assume failure */

    for(i = 0; i < pDev->pCtrl->numChn; i++)
    {
        if(pDev->pCtrl->chnMode[i] == WNCAN_CHN_INVALID)
        {
            if(WNCAN_SetMode(pDev,i,WNCAN_CHN_RECEIVE) == OK)
            {
                *channelNum = i;
                retCode = OK;
                return retCode;
            }
        }
    }
    errnoSet(S_can_no_available_channels);
    return retCode;
}

/************************************************************************
* WNCAN_GetRTRResponderChannel - get a free channel for programming an automatic
*                              response to an incoming remote request
*
* This function is relevant to advanced controllers only. Advanced controllers, 
* are controllers such as TouCAN whose channels have the capability in 
* hardware to both transmit and receive a message in the same channel. 
* An WNCAN_CHN_RTR_RESPONDER channel, can be programmed with data and id.
* If a matching remote request is received in the channel, the hardware will
* automatically respond with the programmed data 
*
* This function will return an error and set an error number, for simple 
* controllers such as the SJA1000. In case of simple controllers, the channels
* do not have the capability to both transmit and receive messages in the same
* channel. The channel has a fixed property of either transmitting or 
* receiving a message.  
*
* This function returns a free transmit-receive channel uniquely identified by
* the output argument "channelNum". The mode assigned to the  channel is 
* WNCAN_CHN_RTR_RESPONDER. A subsequent call to CAN_GetRTRResponderChannel()
* assigns the next available channel with a different channel number.
* The channel number remains unavailable to subsequent CAN_GetXXChannel() and 
* calls until a CAN_FreeChannel() call is invoked on the channel.
* CAN_FreeChannel() makes the channel available and resets the mode to 
* WNCAN_CHN_INVALID. After the CAN_GetRTRResponderChannel() call, the mode of
* the channel can be changed by the CAN_SetMode(..) function call.
*
* By default, channel is disabled. User must call CAN_EnableChannel, to make the
* channel valid in hardware and enable channel interrupts
*
* RETURNS:  for advanced controllers such as the TouCAN and I82527:
*           ERROR if no channels are available, OK otherwise.
*
*           for simple controllers such as the SJA1000:
*           ERROR since this mode cannot be assigned.  
*
* ERRNO:  for advanced controllers such as the TouCAN and I82527:
*         S_can_no_available_channels
*
*         for simple controllers such as the SJA1000:
*         S_can_rtr_mode_not_supported
*
*/
STATUS WNCAN_GetRTRResponderChannel(struct WNCAN_Device *pDev, UCHAR *channelNum)
{
    UCHAR i;
    BOOL notSupported = FALSE;

    for(i = 0; i < pDev->pCtrl->numChn; i++)
    {
        if(pDev->pCtrl->chnMode[i] == WNCAN_CHN_INVALID) {
            if(pDev->pCtrl->chnType[i] == WNCAN_CHN_TRANSMIT_RECEIVE) {
                pDev->pCtrl->chnMode[i] = WNCAN_CHN_RTR_RESPONDER;
                *channelNum = i;
                return OK;
            }
            else
                notSupported = TRUE;
        }
    }
    
    /* if we fall thru, channel not allocated, return
    ** appropriate error
    */
    errnoSet( (notSupported ? S_can_rtr_mode_not_supported
                            : S_can_no_available_channels) );

    return ERROR;
}

/************************************************************************
* WNCAN_GetRTRRequesterChannel - get a free channel for sending an RTR request
*                              and receiving the response in the same channel  
*
* This function is relevant to advanced controllers only. Advanced controllers, 
* are controllers such as TouCAN that, have channels with the capability in 
* hardware to both transmit and receive a message in the same channel. 
* When a remote request is sent out from a particular channel, the reply 
* will be received in the same channel. This function will return an error
* and set an error number, for simple controllers such as the SJA1000. 
* In case of simple controllers, the channels do not have the capability to
* both transmit and receive messages in the same channel. The channel has a 
* fixed property of either transmitting or receiving a message.  
*
* This function returns a free channel uniquely identified by the output argument
* "channelNum". The mode of the channel is assigned WNCAN_CHN_RTR_REQUESTER.
* A subsequent call to CAN_GetRTRRequesterChannel() assigns the next available
* channel with a different channel number.
* The channel number remains unavailable to subsequent CAN_GetXXChannel()  
* calls until a CAN_FreeChannel() call is invoked on the channel.
* CAN_FreeChannel() makes the channel available and resets the mode to 
* WNCAN_CHN_INVALID. After the CAN_GetRTRRequesterChannel() call, the mode of
* the channel can be changed by the CAN_SetMode(..) function call.
*
* By default, channel is disabled. User must call CAN_EnableChannel, to make the
* channel valid in hardware and enable channel interrupts
*
* RETURNS:  for advanced controllers such as the TouCAN and I82527:
*           ERROR if no channels are available, OK otherwise.
*
*           for simple controllers such as the SJA1000:
*           ERROR since this mode cannot be assigned.  
*
* ERRNO: for advanced controllers such as the TouCAN and I82527:
*        S_can_no_available_channels
*
*        for simple controllers such as the SJA1000:
*        S_can_rtr_mode_not_supported
*/
STATUS WNCAN_GetRTRRequesterChannel(struct WNCAN_Device *pDev, UCHAR *channelNum)
{
    UCHAR i;
    BOOL notSupported = FALSE;
    
    for(i = 0; i < pDev->pCtrl->numChn; i++)
    {
        if(pDev->pCtrl->chnMode[i] == WNCAN_CHN_INVALID) {
            if(pDev->pCtrl->chnType[i] == WNCAN_CHN_TRANSMIT_RECEIVE)
            {
                pDev->pCtrl->chnMode[i] = WNCAN_CHN_RTR_REQUESTER;
                *channelNum = i;
                return OK;
            }
            else
                notSupported = TRUE;
         }
    }
        
    /* if we fall thru, channel not allocated, return
    ** appropriate error
    */
    errnoSet( (notSupported ? S_can_rtr_mode_not_supported
                            : S_can_no_available_channels) );

    return ERROR;
}

/************************************************************************
*
* WNCAN_FreeChannel - free the channel
*
* This routine frees the channel such that a WNCAN_GetTxChannel() or
* WNCAN_GetRxChannel() call can use the channel number. The mode of the
* channel is set to WNCAN_CHN_INVALID.
*
* RETURNS: OK, or ERROR
*
* ERRNO: S_can_illegal_channel_no
*
*/
STATUS WNCAN_FreeChannel(struct WNCAN_Device *pDev, UCHAR channelNum)
{
        STATUS retCode = ERROR; /* assume failure */

      if(channelNum >= pDev->pCtrl->numChn)
      {
        errnoSet(S_can_illegal_channel_no);
        return retCode;
      }

         pDev->pCtrl->chnMode[channelNum] = WNCAN_CHN_INVALID;
         retCode =OK;

        return retCode;

}



/************************************************************************
*
* WNCAN_core_init - initialize persistent data structures
*
* This routine initializes any persistent data structures used
* by the WNCAN core library.
* It is called via component init routine and should occur
* before all other component init routines are called.

* RETURNS: none
*
*/
void wncan_core_init(void)
{
    /* initialize the global linked lists for the boards and controllers */
    BOARDLL_INIT();
    CONTROLLERLL_INIT();
}

