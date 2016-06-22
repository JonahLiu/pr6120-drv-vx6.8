/* can_api.c - WIND NET CAN API Library */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01, 18Dec01, jac created
02, 7Jul02,  lsg modified
03, 20Aug02, lsg changed resulting from api review
*/

/*
DESCRIPTION

This library defines the WIND NET CAN API, an interface for
implementations of CAN communications.

MACRO USE

For normal use, these routines are replaced by equivalent macro
definitions in CAN/wnCAN.h. To avoid namespace collisions in your
application code, or to implement a custom naming convention, you can
redefine the macro names. To dispense with the macro definitions and
use the function calls defined in this file, manually set the #define
USE_CAN_FUNCTION_DEFS in the header file CAN/wncan_api.h, then
recompile the CAN library with the make utility.


INCLUDE FILES

CAN/wnCAN.h
CAN/canBoard.h

*/

/* includes */
#include <vxWorks.h>
#include <errnoLib.h>
#include <stdlib.h>
#include <string.h>
#include <CAN/wnCAN.h>
#include <CAN/canBoard.h>
#include <CAN/canController.h>

#ifdef USE_CAN_FUNCTION_DEFS

/* subroutines */
/************************************************************************
* CAN_Open - get a handle to the requested WNCAN_DEVICE
*
* This is the first routine called. This routine establishes links to the
* CAN controller specific API implementation. The CAN API cannot be
* accessed before the device pointer has been correctly initialized
* by this routine.
*
* RETURNS: Pointer to valid WNCAN_DEVICE, or 0 if an error occurred.
*
* ERRNO: S_can_unknown_board, S_can_illegal_board_no,
* S_can_illegal_ctrl_no, S_can_busy
*
*/
struct WNCAN_Device *CAN_Open
    (
    unsigned int brdType,   /* board type       */
    unsigned int brdNdx,    /* board index      */
    unsigned int ctrlNdx    /* controller index */
    )
    {
        return  WNCAN_Open(brdType, brdNdx, ctrlNdx);
    }

/************************************************************************
* CAN_Close - close the handle to the requested WNCAN_DEVICE
*
* This routine deallocates the device struct that is passed in. The
* links to the CAN controller's API implementations do not exist after
* CAN_Close() is called.
*
* RETURNS: N/A
*
* ERRNO: N/A
*
*
*/

void CAN_Close
    (
    struct WNCAN_Device *pDev   /* CAN device pointer */
    )
    {
        WNCAN_Close(pDev);
    }

/***************************************************************************
* CAN_GetMode - get the current mode of the channel
*
* This routine returns the mode of the channel:
* WNCAN_CHN_TRANSMIT, WNCAN_CHN_RECEIVE, WNCAN_CHN_INACTIVE, or
* WNCAN_CHN_INVALID. For advanced controllers, two additional modes exist:
* WNCAN_CHN_RTR_REQUESTER, or WNCAN_CHN_RTR_RESPONDER.
*
* The mode of the channel is relevant in software only, and is indirectly
* related to the hardware capability (transmit, receive or both) of a channel.
*
* \ts
* Mode           | Function
* ---------------+--------------------
* 'TRANSMIT'     | Transmits data frames. In case of simple controllers, this mode can transmit a remote frame by calling CAN_SetRTR().
* 'RECEIVE'      | Receives data frames. In case of simple controllers can receive remote frames as well.
* 'INVALID'      | Channel is available to the CAN driver, but is free and has not been assigned a mode.
* 'INACTIVE'     | Channel is unavailable to the CAN driver.
* 'RTR_REQUESTER'| Valid only for advanced controllers. Channel is set up to send out a remote request and receives the response to the remote frame in the same hardware channel.
* 'RTR_RESPONDER'| Valid only for advanced controllers. Channel is set up to respond to an incoming remote request with the same identifier. On receiving such a frame, the response will be sent out automatically.
* \te
*
* Note, if the channel has not been allocated by a previous call to
* CAN_SetMode(), CAN_GetTxChannel(), CAN_GetRxChannel(),
* CAN_GetRTRResponderChannel() or CAN_GetRTRRequesterChannel(),
* the return value will be WNCAN_CHN_INVALID even though the channel
* number is within the range of channels available on the
* controller.
*
* RETURNS: The channel mode.
*
* ERRNO: N/A
*/
WNCAN_ChannelMode CAN_GetMode
    (
    struct WNCAN_Device *pDev, /* CAN Device pointer */
    UCHAR                chn   /* device channel */
    )
    {
       return(WNCAN_GetMode(pDev, chn));
    }

/***************************************************************************
* CAN_SetMode - set the mode of the channel
*
* This routine sets the mode of the channel to one of five values:
* WNCAN_CHN_TRANSMIT, WNCAN_CHN_RECEIVE, WNCAN_CHN_INACTIVE,
* WNCAN_CHN_RTR_REQUESTER, or WNCAN_CHN_RTR_RESPONDER.
* WNCAN_CHN_RTR_REQUESTER and WNCAN_CHN_RTR_RESPONDER are used for
* advanced controllers. All available channels can be configured to be
* WNCAN_CHN_INACTIVE. The channels available for transmitting or receiving
* messages are determined by the device hardware, and therefore, may or may
* not be configurable with this function call. If an attempt is made to set
* the mode of a channel to WNCAN_CHN_RTR_RESPONDER or WNCAN_CHN_RTR_REQUESTER
* for a simple CAN controller such as SJA1000, WNCAN_CHN_INVALID is returned
* and an errno is set to reflect the error. The preferred approach is to
* allow the device driver to manage the channels internally using the
* CAN_GetTxChannel(), CAN_GetRxChannel(), CAN_GetRTRRequesterChannel(),
* CAN_GetRTRResponderChannel() and CAN_FreeChannel() function calls.
*
* RETURNS: 'OK', or 'ERROR' if the requested channel number is out of range.
*
* ERRNO: S_can_illegal_channel_no
*/
STATUS CAN_SetMode
    (
    struct WNCAN_Device *pDev,       /* CAN device pointer */
    UCHAR                channelNum, /* channel number */
    WNCAN_ChannelMode    mode        /* channel mode */
    )
    {
       return(WNCAN_SetMode(pDev,channelNum,mode));
    }


/***************************************************************************
* CAN_GetBusStatus - get the status of the CAN bus
*
* This routine returns the status of the CAN bus. The bus is
* either in a WNCAN_BUS_OFF, WNCAN_BUS_WARN, or WNCAN_BUS_OK state.
*
* \is
* \i WNCAN_BUS_OFF:
* CAN controller is in BUS OFF state. A CAN node is bus off when the
* transmit error count is greater than or equal to 256.
*
* \i WNCAN_BUS_WARN:
* CAN controller is in ERROR PASSIVE state. A CAN node is in error warning
* state when the number of transmit errors equals or exceeds 128, or the
* number of receive errors equals or exceeds 128.
*
* \i WNCAN_BUS_OK:
* CAN controller is in ERROR ACTIVE state. A CAN node in error active state
* can normally take part in bus communication and sends an ACTIVE ERROR FLAG
* when an error has been detected.
* \ie
*
* RETURNS: The status of the CAN bus.
*
* ERRNO:   N/A
*/
WNCAN_BusStatus CAN_GetBusStatus
    (
    struct WNCAN_Device *pDev  /* CAN Device pointer */
    )
    {
       return (pDev->GetBusStatus(pDev));
    }

/***************************************************************************
* CAN_GetBusError - get the bus errors
*
* This routine returns an \'OR\'ed bit mask of all the bus errors that have
* occurred during the last bus activity.
*
* Bus errors returned by this routine can have the following values:
*
* \ml
* \m -
* WNCAN_ERR_NONE: No errors detected.
* \m -
* WNCAN_ERR_BIT: Bit error.
* \m -
* WNCAN_ERR_ACK: Acknowledgement error.
* \m -
* WNCAN_ERR_CRC: CRC error.
* \m -
* WNCAN_ERR_FORM: Form error.
* \m -
* WNCAN_ERR_STUFF: Stuff error.
* \m -
* WNCAN_ERR_UNKNOWN: Unable to determine cause of error.
* \me
*
*
* The five errors are not mutually exclusive. The occurrence of an error
* will be indicated by an interrupt. Typically, this routine will be called
* from the error interrupt handling case in the user's ISR callback function,
* to find out the kind of error that occurred on the bus.
*
* RETURNS: An \'OR\'ed bit mask of all the bus errors.
*
* ERRNO: S_can_Unknown_error
*/
WNCAN_BusError CAN_GetBusError
    (
    struct WNCAN_Device *pDev  /* CAN Device pointer */
    )
    {
       return(pDev->GetBusError(pDev));
    }

/***************************************************************************
* CAN_Init - initialize the CAN device controller
*
* This routine initializes the CAN controller and makes default selections.
*
* \ml
* \m 1.
* Puts the CAN controller into debug mode.
* \m 2.
* Disables interrupts at the CAN controller level as well as channel.
* \m 3.
* Sets bit timing values according to values stored in the CAN controller
* struct. Unless the user has changed these values before CAN_Init() is called,
* the default bit timing values are set to a baud rate of 250K.
* \m 4.
* Clears error counters.
* \m 5.
* Sets local and global receive masks to don't care (accept all).
* \m 6.
* Makes all channels inactive in hardware.
* \m 7.
* Other controller specific initializations.
* \m 8.
* On exiting the CAN_Init() routine, the CAN controller has been initialized,
* but is offline and unable to participate in CAN bus communication.
* \me
*
*
* CAN_Open() is called to obtain a valid device pointer. Subsequently,
* CAN_Init() and CAN_Open() are called to initialize the controller, bring
* it online, and enable active participation in CAN bus activity.
*
* RETURNS: 'OK'.
*
* ERRNO:   N/A
*/
STATUS CAN_Init
    (
    struct WNCAN_Device *pDev  /* CAN Device pointer */
    )
    {
       return(pDev->Init(pDev));
    }

/***************************************************************************
* CAN_Start - bring the CAN controller online
*
* This routine is called to bring the CAN controller online. The CAN
* controller can now participate in transmissions and receptions on the CAN
* bus. This routine must be called after CAN_Init() has been called to
* initialize and bring the CAN controller up in a known state. This routine
* negates the INIT/HALT (offline) bit.
*
* RETURNS: 'OK'.
*
*/
void CAN_Start
    (
    struct WNCAN_Device *pDev  /* CAN Device pointer */
    )
    {
       return(pDev->Start(pDev));
    }

/***************************************************************************
* CAN_Stop - put the CAN controller offline
*
* This routine disables communication between the CAN controller and the
* CAN bus.
*
* RETURNS: 'OK'.
*
* ERRNO: N/A
*
*/
void CAN_Stop
    (
    struct WNCAN_Device *pDev  /* CAN Device pointer */
    )
    {
       return(pDev->Stop(pDev));
    }

/***************************************************************************
* CAN_SetBitTiming - set the bit timing parameters of the controller
*
* This routine sets the baud rate and sample point of the controller. The
* values of the input parameters should be based on an established set of
* recommendations.
*
* This routine sets the bit timing values in the hardware as well as the
* controller structure, so that the bit timing values are not lost if
* CAN_Init() is called again.
*
* The routine will preserve the state of the CAN controller.
* For example, if the CAN controller is online when the routine
* is called, then the CAN controller will be online when the
* routine exits.
*
* <tseg1> and <tseg2> are interpreted according to controller specific
* definitions and, therefore, can be written to directly using this routine.
* In all supported controllers, <tseg2> refers to the segment of bit time
* after the sample point. However, <tseg1> interpretations vary.
*
* In some controllers <tseg1> refers to the segment of bit time from the
* end of the sync segment, up to the sample point. Bit time = 1 + (<tseg1> + 1)
* + (<tseg2> + 1) time quanta.
*
*
* \ss
*  ---------------------------------------------------------
* |sync|<-------tseg1------>|<---------tseg2 -------------->|
*                         sample
*                         point
* \se
*
* For other controllers, <tseg1> refers to the segment of bit time after the
* end of propseg and up to the sample point. Bit time = 1 + (propseg + 1)
* + (<tseg1> + 1) + (<tseg2> + 1) time quanta.
*
*
* \ss
*  ---------------------------------------------------------
* |sync|<--propseg-->|<--tseg1-->|<---------tseg2 --------->|
*                              sample
*                              point
* \se
*
* There is a controller specific function provided to set the propseg of
* these controllers (see TouCAN_SetPropseg(), for example). Also, there
* will be a place holder for the propseg (with the other bit timing
* parameters) in the CAN controller struct.
*
* For TouCAN the default value of propseg is 7.
*
* The number of samples taken at the sample point is decided by the input
* argument <numSamples> to the function. <numSamples> = 0 ('FALSE') indicates
* one sample will be taken. <numSamples> = 1 ('TRUE') indicates three samples
* will be taken.
*
* RETURNS: 'OK', or 'ERROR' if any of the input parameters have invalid values.
*
* ERRNO:   S_can_invalid_parameter
*/
STATUS  CAN_SetBitTiming
    (
    struct WNCAN_Device *pDev,      /* CAN device pointer */
    UCHAR                tseg1,     /* number of time quanta for segment 1 */
    UCHAR                tseg2,     /* number of time quanta for segment 2 */
    UCHAR                brp,       /* baud rate prescaler */
    UCHAR                sjw,       /* syncro jump width */
    BOOL                 numSamples /* number of samples */
    )
    {
       return (pDev->SetBitTiming(pDev,tseg1,tseg2,brp,sjw, numSamples));
    }

/***************************************************************************
* CAN_GetBaudRate - get the programmed baud rate value
*
* This routine returns the programmed baud rate value in bits per second.
* The sample point value is returned by reference in the input parameter
* <samplePoint>. The sample point value is returned as a percentage.
* It is the percentage of the bit time at which the bit is sampled.
*
* RETURNS: The baud rate.
*
* ERRNO:   N/A
*/
UINT CAN_GetBaudRate
    (
    struct WNCAN_Device *pDev, /* CAN device pointer */
    UINT                *samplePoint
    )
    {
        return (pDev->GetBaudRate(pDev, samplePoint));
    }

/***************************************************************************
* CAN_SetIntMask - enable controller level interrupts on the CAN device
*
* This routine enables the specified list of controller level interrupts
* on the CAN controller. Interrupts are not enabled at the CAN board level
* or at the CPU level.
*
* The interrupt masks available are:
*
* \ml
* \m -
* WNCAN_INT_ERROR : Enables interrupts due to errors on the CAN bus.
* This may be implemented as a single or multiple interrupt source on the
* CAN controller.
* \m -
* WNCAN_INT_BUS_OFF: Enables interrupt indicating bus off condition.
* \m -
* WNCAN_INT_WAKE_UP: Enables interrupt on wake up.
* \me
*
*
* In order to be set, all interrupt masks that need to be enabled must be
* specified in the list passed to the routine every time the routine is
* called.
*
* RETURNS: 'OK', or 'ERROR' if interrupt setting not possible.
*
* ERRNO: S_can_hwfeature_not_available, S_can_invalid_parameter
*/
STATUS CAN_SetIntMask
    (
    struct WNCAN_Device *pDev,         /* CAN Device pointer */
    WNCAN_IntType        intMask       /* \'OR\'ed interrupt masks */
    )
    {
       STATUS status;
       status = pDev->SetIntMask(pDev, intMask);
       return (status);
    }

/***************************************************************************
* CAN_EnableInt - enable interrupts from the CAN device to the CPU
*
* This routine enables interrupts from the CAN controller to reach the
* CPU. Typically, it enables a global interrupt mask bit at the CPU level.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
void CAN_EnableInt
    (
    struct WNCAN_Device *pDev  /* CAN Device pointer */
    )
    {
       pDev->EnableInt(pDev);
       return;
    }

/***************************************************************************
* CAN_DisableInt - disable interrupts from the CAN device to the CPU
*
* This routine disables interrupts from the CAN controller from reaching the
* CPU. Typically, it disables a global interrupt mask bit at the CPU level.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
void CAN_DisableInt
    (
    struct WNCAN_Device *pDev  /* CAN Device pointer */
    )
    {
       pDev->DisableInt(pDev);
       return;
    }

/***************************************************************************
* CAN_GetNumChannels - get the number of channels on the CAN controller
*
* This routine returns the number of channels on the controller.
* This number is fixed by the hardware and does not change during operation.
*
* RETURNS: The number of channels on the controller.
*
* ERRNO: N/A
*/
UCHAR CAN_GetNumChannels
    (
    struct WNCAN_Device *pDev     /* CAN Device pointer */
    )
    {
       return(pDev->pCtrl->numChn);
    }

/***************************************************************************
* CAN_GetTxChannel - get a free transmit channel
*
* This routine returns a free channel uniquely identified by the output
* argument <channelNum>. The mode of the assigned channel is
* WNCAN_CHN_TRANSMIT. A subsequent call to CAN_GetTxChannel() assigns the
* next available channel with a different channel number.
* The channel number remains unavailable to subsequent CAN_GetXXChannel()
* calls until a CAN_FreeChannel() call is invoked on the channel.
* CAN_FreeChannel() makes the channel available and resets the mode to
* WNCAN_CHN_INVALID. After the CAN_GetTxChannel() call, the mode of
* the channel can be changed by the CAN_SetMode() function call.
*
* RETURNS:  'OK', or 'ERROR' if no channels are available.
*
* ERRNO: S_can_no_available_channels
*/
STATUS CAN_GetTxChannel
    (
    struct WNCAN_Device *pDev,       /* CAN device pointer */
    UCHAR               *channelNum  /* channel number */
    )
    {
       return(WNCAN_GetTxChannel(pDev,channelNum));
    }

/***************************************************************************
* CAN_GetRxChannel - get a free receive channel
*
* This routine gets a free receive channel uniquely identified by the output
* argument <channelNum>. The mode of the assigned channel is
* WNCAN_CHN_RECEIVE. A subsequent call to CAN_GetRxChannel() assigns the
* next available receive channel with a different number. The channel number
* remains unavailable to subsequent CAN_GetXXChannel() calls until a
* CAN_FreeChannel() call is invoked on the channel. CAN_FreeChannel() makes
* the channel available and resets the mode to WNCAN_CHN_INVALID. After the
* CAN_GetRxChannel() call, the mode of the channel can be changed by the
* CAN_SetMode() function call.
*
* RETURNS: 'OK', or 'ERROR' if no channel is available.
*
* ERRNO: S_can_no_available_channels
*/
STATUS CAN_GetRxChannel
    (
    struct WNCAN_Device *pDev,        /* CAN device pointer */
    UCHAR               *channelNum   /* channel number */
    )
    {
       return(WNCAN_GetRxChannel(pDev,channelNum));
    }

/*****************************************************************************
* CAN_GetRTRRequesterChannel - get a free channel for remote transmit receive
*
* This routine is relevant to advanced controllers only. Advanced controllers
* are controllers, such as TouCAN, whose channels have the capability in
* hardware to both transmit and receive a message in the same channel.
* When a remote request is sent out from a particular channel, the reply
* will be received in the same channel. This routine will return an error
* and set an error number for simple controllers, such as the SJA1000.
* In the case of simple controllers, the channels do not have the capability
* to both transmit and receive messages in the same channel. The channel has a
* fixed property of either transmitting or receiving a message.
*
* This function returns a free channel uniquely identified by the output
* argument <channelNum>. The mode of the channel is assigned
* WNCAN_CHN_RTR_REQUESTER. A subsequent call to CAN_GetRTRRequesterChannel()
* assigns the next available channel with a different channel number.
* The channel number remains unavailable to subsequent CAN_GetXXChannel()
* calls until a CAN_FreeChannel() call is invoked on the channel.
* CAN_FreeChannel() makes the channel available and resets the mode to
* WNCAN_CHN_INVALID. After the CAN_GetRTRRequesterChannel() call, the mode of
* the channel can be changed by the CAN_SetMode() function call.
*
* RETURNS:  For advanced controllers, such as the TouCAN and I82527:
* 'OK', or 'ERROR' if no channels are available.
*
* For simple controllers, such as the SJA1000: 'ERROR' since this mode cannot
* be assigned.
*
* ERRNO: For advanced controllers, such as the TouCAN and I82527:
* S_can_no_available_channels
*
* For simple controllers, such as the SJA1000: S_can_rtr_mode_not_supported
*/
STATUS CAN_GetRTRRequesterChannel
    (
    struct WNCAN_Device *pDev,       /* CAN device pointer */
    UCHAR               *channelNum  /* channel number */
    )
    {
       return(WNCAN_GetRTRRequesterChannel(pDev,channelNum));
    }

/******************************************************************************
* CAN_GetRTRResponderChannel - get a free channel for an automatic remote response
*
* This routine is relevant to advanced controllers only. Advanced controllers
* are controllers, such as TouCAN, whose channels have the capability in
* hardware to both transmit and receive a message in the same channel.
* An WNCAN_CHN_RTR_RESPONDER channel, can be programmed with data and ID.
* If a matching remote request is received in the channel, the hardware will
* automatically respond with the programmed data.
*
* This routine will return an error and set an error number for simple
* controllers, such as the SJA1000. In the case of simple controllers, the
* channels do not have the capability to both transmit and receive messages
* in the same channel. The channel has a fixed property of either transmitting
* or receiving a message.
*
* This routine returns a free transmit-receive channel uniquely identified by
* the output argument <channelNum>. The mode assigned to the channel is
* WNCAN_CHN_RTR_RESPONDER. A subsequent call to CAN_GetRTRResponderChannel()
* assigns the next available channel with a different channel number.
* The channel number remains unavailable to subsequent CAN_GetXXChannel() and
* calls until a CAN_FreeChannel() call is invoked on the channel.
* CAN_FreeChannel() makes the channel available and resets the mode to
* WNCAN_CHN_INVALID. After the CAN_GetRTRResponderChannel() call, the mode of
* the channel can be changed by the CAN_SetMode() function call.
*
* RETURNS:  For advanced controllers, such as the TouCAN and I82527:
* 'OK', or 'ERROR' if no channels are available.
*
* For simple controllers, such as the SJA1000: 'ERROR' since this mode cannot
* be assigned.
*
* ERRNO:  For advanced controllers, such as the TouCAN and I82527:
* S_can_no_available_channels
*
* For simple controllers, such as the SJA1000: S_can_rtr_mode_not_supported
*
*/
STATUS CAN_GetRTRResponderChannel
    (
    struct WNCAN_Device *pDev,       /* CAN device pointer */
    UCHAR               *channelNum  /* channel number */
    )
    {
       return(WNCAN_GetRTRResponderChannel(pDev,channelNum));
    }

/***************************************************************************
* CAN_FreeChannel - free the specified channel
*
* This routine frees a channel by setting its mode to WNCAN_CHN_INVALID.
*
* RETURNS: 'OK', or 'ERROR' if requested channel is out of range.
*
* ERRNO: S_can_illegal_channel_no
*/
STATUS CAN_FreeChannel
    (
    struct WNCAN_Device *pDev,       /* CAN device pointer   */
    UCHAR                channelNum  /* channelNum           */
    )
    {
       return(WNCAN_FreeChannel(pDev,channelNum));
    }

/************************************************************************
* CAN_GetVersion - get the version number of the CAN drivers
*
* This routine returns a pointer to the struct WNCAN_VersionInfo,
* which holds the WIND NET CAN API's version number.
*
* RETURNS: The structure containing version information.
*
* ERRNO: N/A
*
*/
const WNCAN_VersionInfo * CAN_GetVersion ()
    {
        return(WNCAN_GetVersion());
    }

/***************************************************************************
* CAN_ReadID - read the CAN ID from the specified controller and channel
*
* This routine reads the CAN ID corresponding to the channel number on
* the controller. The standard or extended flag is set by the routine.
* The mode of the channel can not be WNCAN_CHN_INACTIVE or WNCAN_CHN_INVALID.
* The extended flag is set to 'TRUE' if the ID has extended format, otherwise
* it is set to 'FALSE'.
*
* RETURNS: The CAN ID, or -1 if an input parameter is invalid.
*
* ERRNO: S_can_illegal_channel_no, S_can_illegal_config
*/
long CAN_ReadID
    (
    struct WNCAN_Device *pDev,       /* CAN device pointer */
    UCHAR                channelNum, /* channel number */
    BOOL*                ext         /* extended flag */
    )
    {
       return(pDev->ReadID(pDev,channelNum,ext));
    }

/***************************************************************************
* CAN_WriteID - write the CAN ID to the specified channel
*
* This routine writes the CAN ID to the channel. The type of ID
* (standard or extended) is specified. The behavior of the routine for
* every channel mode is specified as follows:
*
* \ml
* \m -
* WNCAN_CHN_INVALID: Not allowed. 'ERROR' is returned.
* \m -
* WNCAN_CHN_INACTIVE: Not allowed. 'ERROR' is returned.
* \m -
* WNCAN_CHN_RECEIVE: CAN messages with this ID will be received.
* \m -
* WNCAN_CHN_RTR_RESPONDER: The data bytes to respond with must be set up with
* a call to CAN_WriteData() previously. CAN messages with this ID will be
* received and a remote response message sent back.
* \m -
* WNCAN_CHN_RTR_REQUESTER: A CAN message with RTR bit set and the specified ID
* will be transmitted when CAN_Tx() is called.
* \m -
* WNCAN_CHN_TRANSMIT: A CAN message with the specified ID will be transmitted
* when CAN_Tx() is called.
* \me
*
* RETURNS: 'OK', or 'ERROR' if an input parameter is invalid.
*
* ERRNO: S_can_illegal_channel_no, S_can_illegal_config
*
*/
STATUS CAN_WriteID
    (
    struct WNCAN_Device *pDev,    /* CAN device pointer   */
    UCHAR                chnNum,  /* channel number       */
    ULONG                canID,   /* CAN ID               */
    BOOL                 ext      /* TRUE = extended CAN msg */
    )
    {
       return(pDev->WriteID(pDev,chnNum,canID,ext));
    }

/***************************************************************************
* CAN_ReadData - read data from the specified channel
*
* This routine reads <len> bytes of data from the channel and sets the value
* of <len> to the number of bytes read. The range of <len> is zero (for zero
* length data) to a maximum of eight. The mode of the channel must not be
* WNCAN_CHN_INVALID or WNCAN_CHN_INACTIVE; however, the newData flag is
* valid only for channels with mode equal to WNCAN_CHN_RECEIVE or
* WNCAN_CHN_RTR_REQUESTER. For receive channels, if no new data has been
* received since the last CAN_ReadData() function call, the newData flag
* is set to 'FALSE'; otherwise, the flag is 'TRUE'. In both cases, the data and
* length of the data currently in the channel are read.
* The parameter <len>, contains length of data buffer on entering the
* routine. On return <len> contains number of bytes copied into data buffer.
*
*
* RETURNS: 'OK', or 'ERROR' if an input parameter is invalid.
*
* ERRNO: S_can_illegal_channel_no, S_can_illegal_config,
* S_can_buffer_overflow, S_can_illegal_data_length
*
*/
STATUS CAN_ReadData
    (
    struct WNCAN_Device  *pDev,      /* CAN device pointer */
    UCHAR                 chnNum,    /* channel number */
    UCHAR                *data,      /* pointer to buffer for data */
    UCHAR                *len,       /* length pointer */
    BOOL                 *newData    /* new data pointer */
    )
    {
       return(pDev->ReadData(pDev,chnNum,data,len,newData));
    }

/***************************************************************************
* CAN_GetMessageLength - get the message length
*
* This routine returns the length of the message data in the channel on the
* controller. The minimum value returned is 0, and the maximum value is 8.
* This number is equal to the <len> argument in CAN_ReadData(). If the data
* has zero length, this routine returns zero. The mode of the channel
* must not be WNCAN_CHN_INACTIVE or WNCAN_CHN_INVALID
*
* RETURNS: Length of data, or -1 if an input parameter is invalid.
*
* ERRNO: S_can_illegal_channel_no, S_can_illegal_config
*/
int CAN_GetMessageLength
    (
    struct WNCAN_Device *pDev,       /* CAN device pointer   */
    UCHAR                channelNum  /* channelNum           */
    )
    {
       return(pDev->GetMessageLength(pDev,channelNum));
    }

/***************************************************************************
* CAN_WriteData - write the data to the specified channel
*
* This routine writes <len> bytes of data to the channel. An error is
* returned if the number of bytes to be written exceed 8. The mode of the
* channel must WNCAN_CHN_TRANSMIT or WNCAN_CHN_RTR_RESPONDER.
*
* RETURNS: 'OK', or 'ERROR' if an input parameter is invalid.
*
* ERRNO: S_can_illegal_channel_no, S_can_illegal_config,
* S_can_illegal_data_length
*
*/
STATUS CAN_WriteData
    (
    struct WNCAN_Device *pDev,       /* CAN device pointer */
    UCHAR                channelNum, /* channel number */
    UCHAR               *data,       /* pointer to data buffer */
    UCHAR                len         /* length of data buffer */
    )
    {
       return(pDev->WriteData(pDev,channelNum,data,len));
    }

/***************************************************************************
* CAN_Tx - transmit the CAN message on the specified channel
*
* This routine transmits the CAN ID and data currently in the channel of
* the specified controller on the device. The mode of the channel must be
* WNCAN_CHN_TRANSMIT or WNCAN_CHN_RTR_REQUESTER.
*
*
* RETURNS: 'OK', or 'ERROR' if an input parameter is invalid.
*
* ERRNO: S_can_illegal_channel_no, S_can_illegal_config
*/
STATUS CAN_Tx
    (
    struct WNCAN_Device *pDev,        /* CAN device pointer */
    UCHAR                channelNum   /* channel number */
    )
    {
       return(pDev->Tx(pDev,channelNum));
    }

/***************************************************************************
* CAN_TxMsg - transmit a CAN message with all parameters specified
*
* This routine performs the same function as the following series
* of function calls:
*     \cs
*     CAN_WriteID(context,channelNum,canID,ext);
*     CAN_WriteData(context,channelNum,data,len);
*     CAN_Tx(context,channel);
*     \ce
* The mode of the channel must be WNCAN_CHN_TRANSMIT or
* WNCAN_CHN_RTR_REQUESTER. If the length specified exceeds 8 bytes an error
* will be returned. The extended flag must be set to 'TRUE' if the ID has
* extended format, or 'FALSE' if the ID has standard format.
*
* RETURNS: 'OK', or 'ERROR' if an input parameter is invalid.
*
* ERRNO: S_can_illegal_channel_no, S_can_illegal_config,
* S_can_illegal_data_length
*
*/
STATUS CAN_TxMsg
    (
    struct WNCAN_Device *pDev,       /* CAN device pointer */
    UCHAR                channelNum, /* channel number */
    ULONG                canId,      /* CAN ID */
    BOOL                 ext,        /* extended message flag */
    UCHAR               *data,       /* pointer to buffer for data */
    UCHAR                len         /* message length */
    )
    {
       return(pDev->TxMsg(pDev,channelNum,canId,ext,data,len));
    }

/***************************************************************************
* CAN_SetGlobalRxFilter - set the global filter in the controller
*
* This routine sets the global HW filter mask for incoming messages on the
* device controller. If the controller does not have a global filter this
* routine is a no-op. If the <ext> parameter is 'TRUE', the mask value applies
* to extended messages; otherwise, the mask values applies to standard
* messages. A value of 0 for a particular bit position of the mask means
* don't care (for example, the incoming message ID could have a value of
* zero or one for this bit position); otherwise, a value of 1 means the
* corresponding bit of the message ID must match identically.
*
* If the controller does not have a global mask, an error is returned.
*
* RETURNS: 'OK', or 'ERROR'.
*
* ERRNO: S_can_hwfeature_not_available
*
*/
STATUS CAN_SetGlobalRxFilter
    (
        struct WNCAN_Device *pDev,   /* CAN device pointer */
        long                 mask,   /* filter mask value */
        BOOL                 ext     /* extended flag */
    )
    {
       return(pDev->SetGlobalRxFilter(pDev,mask,ext));
    }

/***************************************************************************
* CAN_GetGlobalRxFilter - get the global filter value
*
* This routine returns the programmed value in the Global filter register.
* Based on the value of <ext> passed to the routine, this routine reads the
* appropriate format of the global mask and returns the value in the
* specified extended or standard identifier format. If the controller does
* not have a global mask, a value of -1 is returned. This is not a valid
* CAN ID.
*
* RETURNS: Mask, or -1 on error.
*
* ERRNO:   S_can_hwfeature_not_available
*
*/
long CAN_GetGlobalRxFilter
    (
    struct WNCAN_Device *pDev,   /* CAN device pointer */
    BOOL                 ext     /* extended flag */
    )
    {
       return(pDev->GetGlobalRxFilter(pDev,ext));
    }

/***************************************************************************
* CAN_SetLocalMsgFilter - set a local message object filter
*
* This routine sets a local message object filter for incoming messages on a
* particular channel.
*
* If the <ext> parameter is 'TRUE', the mask value applies to extended messages;
* otherwise, the mask value applies to standard messages. A value of 0 for
* a particular bit position of the mask means don't care (for example, the
* incoming message ID could have a value of zero or one for this bit position);
* otherwise, a value of 1 means the corresponding bit of the message ID must
* match identically. Channel number is provided for controllers that have
* more than one local message object filter.
*
* If the controller does not have a local mask for the channel specified,
* an error is returned.
*
*
* RETURNS: 'OK', 'ERROR'.
*
* ERRNO: S_can_illegal_channel_no, S_can_hwfeature_not_available
*
*
*/
STATUS CAN_SetLocalMsgFilter
    (
    struct WNCAN_Device *pDev,      /* CAN device pointer */
    UCHAR                channel,   /* channel number     */
    long                 mask,      /* filter mask value  */
    BOOL                 ext        /* extended flag      */
    )
    {
       return(pDev->SetLocalMsgFilter(pDev,channel,mask,ext));
    }

/***************************************************************************
* CAN_GetLocalMsgFilter - get the specified local filter value
*
* This routine returns the programmed value in the local filter register.
* Based on the value of <ext> passed to the routine, this routine reads the
* appropriate format of the local mask and returns the value in the
* specified extended or standard identifier format. The channel argument
* identifies the local filter associated with the particular channel number.
*
* If the controller does not have a global mask, a value of -1 is returned.
* This is not a valid CAN ID.
*
* RETURNS: Mask, or -1 on error.
*
* ERRNO: S_can_hwfeature_not_available
*
*/
long CAN_GetLocalMsgFilter
    (
    struct WNCAN_Device *pDev,      /* CAN device pointer */
    UCHAR                channel,   /* channel number */
    BOOL                 ext        /* extended flag */
    )
    {
       return(pDev->GetLocalMsgFilter(pDev, channel, ext));
    }

/***************************************************************************
* CAN_GetIntStatus - get the interrupt status of a CAN interrupt
*
* This routine returns the interrupt status on the controller:
*
*    WNCAN_INT_NONE = no interrupt occurred
*    WNCAN_INT_ERROR = bus error
*    WNCAN_INT_TX = interrupt resulting from a CAN transmission
*    WNCAN_INT_RX = interrupt resulting form message reception
*    WNCAN_INT_BUS_OFF = interrupt resulting from bus off condition
*    WNCAN_INT_WAKE_UP = interrupt resulting from controller waking up after
*    being put in sleep mode
*    WNCAN_INT_SPURIOUS = unknown interrupt
*
* NOTE: If the interrupt was caused by the transmission or reception of a
* message, the channel number that generated the interrupt is set; otherwise,
* this value is undefined.
*
* RETURNS: The interrupt status.
*
* ERRNO: N/A
*/
WNCAN_IntType CAN_GetIntStatus
    (
    struct WNCAN_Device *pDev,       /* CAN device pointer */
    UCHAR               *channelNum  /* channel pointer */
    )
    {
       return(pDev->GetIntStatus(pDev,channelNum));
    }


/***************************************************************************
* CAN_IsMessageLost - test if a received CAN message has been lost
*
* This routine tests if the current message data in the channel overwrote
* the old message data before CAN_ReadData() was called. Valid only for
* channels with mode = WNCAN_CHN_RECEIVE or WNCAN_CHN_RTR_REQUESTER.
*
* RETURNS: 0 if 'FALSE', 1 if 'TRUE', or -1 if 'ERROR'.
*
* ERRNO: S_can_illegal_channel_no, S_can_illegal_config
*/
int CAN_IsMessageLost
    (
    struct WNCAN_Device *pDev,       /* CAN device pointer   */
    UCHAR                channelNum  /* channel number       */
    )
    {
       return(pDev->IsMessageLost(pDev,channelNum));
    }

/************************************************************************
* CAN_ClearMessageLost - clear message lost indication
*
* This routine clears the data overrun flag for a particular channel.
* Valid only for channels with mode = WNCAN_CHN_RECEIVE in the case of simple
* controllers or channels with mode = WNCAN_CHN_RECEIVE and
* WNCAN_CHN_RTR_REQUESTER in the case of advanced CAN controllers.
*
* RETURNS: 'OK', or 'ERROR'.
*
* ERRNO: S_can_illegal_channel_no, S_can_illegal_config
*
*/
STATUS CAN_ClearMessageLost
    (
    struct WNCAN_Device *pDev,       /* CAN device pointer   */
    UCHAR                channelNum  /* channel number       */
    )
    {
       return(pDev->IsMessageLost(pDev,channelNum));
    }


/***************************************************************************
* CAN_IsRTR - test if the message has the RTR bit set
*
* This routine tests if the message has the RTR bit set. The mode of the
* channel must be WNCAN_CHN_TRANSMIT or WNCAN_CHN_RECEIVE.
* This routine can only be used on simple controllers, such as the SJA1000.
* The routine will return an error if called on an advanced controller, such
* as the TouCAN and I82527.
* To receive an RTR message for advanced controllers, set the mode of
* the channel to WNCAN_CHN_RTR_RESPONDER.
*
* RETURNS: 0 if 'FALSE', 1 if 'TRUE', or -1 if 'ERROR'.
*
* ERRNO: S_can_illegal_channel_mode, S_can_illegal_config
*
*/
int CAN_IsRTR
    (
    struct WNCAN_Device *pDev,       /* CAN device pointer */
    UCHAR                channelNum  /* channel number */
    )
    {
       return(pDev->IsRTR(pDev,channelNum));
    }

/***************************************************************************
* CAN_SetRTR - test if the message has the RTR bit set
*
* This routine tests if the message has the RTR bit set. The mode of the
* channel must be WNCAN_CHN_TRANSMIT.
* This routine can only be used on simple controllers, such as the SJA1000.
* The routine will return an error if called on an advanced controller, such
* as the TouCAN and I82527. To send an RTR message for advanced controllers,
* set the mode of the channel to WNCAN_CHN_RTR_REQUESTER and call CAN_Tx().
*
* RETURNS: 'OK', or 'ERROR'.
*
* ERRNO: S_can_illegal_channel_mode, S_can_illegal_config
*
*/
STATUS CAN_SetRTR
    (
    struct WNCAN_Device *pDev,          /* CAN device pointer */
    UCHAR                channelNum ,   /* channel number */
    BOOL                 rtr            /* rtr flag ('TRUE' = set bit) */
    )
    {
       return(pDev->SetRTR(pDev,channelNum, rtr));
    }

/***************************************************************************
* CAN_TxAbort - abort the current CAN transmission
*
* This routine aborts any current CAN transmissions on the controller.
*
* RETURNS: N/A
*
* ERRNO: N/A
*/
void CAN_TxAbort
    (
    struct WNCAN_Device *pDev  /* CAN device pointer */
    )
    {
       return(pDev->TxAbort(pDev));
    }


/************************************************************************
* CAN_Sleep - put the CAN controller of the device into sleep mode
*
* This routine puts the CAN controller into sleep mode if the hardware
* supports this functionality; otherwise, this routine is a no-op.
*
* RETURNS: 'OK', or 'ERROR'.
*
* ERRNO: S_can_cannot_sleep_in_reset_mode
*
*/
STATUS CAN_Sleep
    (
    struct WNCAN_Device *pDev  /* CAN device pointer */
    )
    {
       return(pDev->Sleep(pDev));
    }

/************************************************************************
* CAN_WakeUp - force the CAN controller out of sleep mode
*
* This routine negates the Sleep bit to force the CAN controller out of
* sleep mode.
*
* RETURNS: 'OK', always.
*
* ERRNO: N/A
*
*/
STATUS CAN_WakeUp
    (
    struct WNCAN_Device *pDev
    )
    {
        return(pDev->WakeUp(pDev));
    }

/************************************************************************
*
* CAN_EnableChannel - enable channel and set interrupts
*
* This routine marks the channel valid. It also sets the type of channel
* interrupt requested. If the channel type does not match the type of
* interrupt requested it returns an error.
*
* The interrupt settings that are available are: (choose only one)
*
* \ml
* \m -
* WNCAN_INT_RX: Message reception interrupt.
* \m -
* WNCAN_INT_TX: Message transmission interrupt.
* \me
*
* RETURNS: 'OK' if successful, 'ERROR' otherwise.
*
* ERRNO: S_can_illegal_channel_no, S_can_illegal_config,
* S_can_invalid_parameter, S_can_type_mismatch
*
*/

STATUS CAN_EnableChannel
    (
    struct WNCAN_Device *pDev,       /* CAN device pointer */
    UCHAR                channelNum, /* channel number */
    WNCAN_IntType        intSetting  /* interrupt type */
    )
    {
        return(pDev->EnableChannel(pDev, channelNum, intSetting));
    }

/************************************************************************
*
* CAN_DisableChannel - reset channel interrupts and disable channel
*
* This routine resets channel interrupts and marks it invalid in hardware.
* After this routine is called the channel will not transmit or receive
* any messages.
*
* RETURNS: 'OK' if successful, 'ERROR' otherwise.
*
* ERRNO: S_can_illegal_channel_no
*
*/

STATUS CAN_DisableChannel
    (
    struct WNCAN_Device *pDev,        /*CAN device pointer*/
    UCHAR                channelNum   /* channel number */
    )
    {
        return(pDev->DisableChannel(pDev, channelNum));
    }

/******************************************************************************
*
* CAN_WriteReg - write data to offset in CAN controller's memory area
*
* This routine accesses the CAN controller memory at the offset specified.
* A list of valid offsets are defined as macros in a CAN controller specific
* header file, named as <controller>Offsets.h (for example, toucanOffsets.h or
* sja1000Offsets.h). A pointer to the data buffer holding the data to be written is passed to the
* routine in UCHAR *data. The number of data bytes to be copied from the data
* buffer to the CAN controller's memory area is specified by the length
* parameter.
*
* RETURNS: 'OK'.
*
* ERRNO: S_can_illegal_offset
*
*/
STATUS CAN_WriteReg
    (
    struct WNCAN_Device *pDev,    /* CAN device pointer */
    UINT                 offset,  /* from base of CAN controller memory */
    UCHAR               *data,    /* byte pointer to data to be written */
    UINT                 length   /* number of bytes to be written*/
    )
    {
        return (pDev->WriteReg(pDev, offset, data, length));
    }


/******************************************************************************
*
* CAN_ReadReg - access memory and read data
*
* This routine accesses the CAN controller memory at the offset specified.
* A list of valid offsets are defined as macros in a CAN controller specific
* header file, named as <controller>Offsets.h (for example, toucanOffsets.h or
* sja1000Offsets.h). A pointer to the data buffer to hold the data to be read,
* is passed through UINT *data. The length of the data to be copied is specified
* through the length parameter.
*
* RETURNS: 'OK'.
*
* ERRNO: S_can_illegal_offset
*
*/
STATUS CAN_ReadReg
    (
    struct WNCAN_Device *pDev,    /* CAN device pointer */
    UINT                 offset,  /* from base of CAN controller memory */
    UCHAR               *data,    /* return data place holder */
    UINT                 length   /* number of bytes to be copied */
    )
    {
        return (pDev->ReadReg(pDev, offset, data, length));
    }

/************************************************************************
*
* CAN_GetXtalFreq - get the crystal frequency of the CAN controller
*
* This routine returns the crystal frequency of the CAN controller.
*
* RETURNS: The crystal frequency.
*
* ERRNO: N/A
*/
XtalFreq CAN_GetXtalFreq
    (
    struct WNCAN_Device *pDev  /* CAN device pointer */
    )
    {
       return(pDev->pBrd->xtalFreq);
    }


/************************************************************************
* CAN_GetControllerType - get the controller type
*
* This routine returns the controller type of the CAN controller.
*
* RETURNS: The controller type.
*
* ERRNO: N/A
*/
WNCAN_ControllerType CAN_GetControllerType
    (
    struct WNCAN_Device *pDev  /* CAN device pointer */
    )
    {
       return(pDev->pCtrl->ctrlType);
    }


/* function prototypes */
/***************************************************************************
* CAN_InstallISRCallback - install the ISR callback function
*
* The ISR callback is written by the user, has three arguments, and returns
* void. The callback function executes user written message processing code
* inside the CAN ISR. The ISR is hardware specific and is implemented
* at the hardware layer of the device driver. The callback function is entered
* with board level CAN interrupts disabled. The first argument to the callback
* is the CAN device pointer. The second argument indicates the interrupt
* status. The third argument indicates the channel on which the interrupt
* was generated. The channel number is undefined if the interrupt is not due
* to a transmission or reception.
*
* RETURNS: 'OK', or 'ERROR'.
*
* ERRNO: S_can_invalid_parameter
*/
STATUS CAN_InstallISRCallback
    (
    struct WNCAN_Device *pDev,                /* CAN device pointer */
    void (*pFun)(struct WNCAN_Device *pDev2,  /* Isr callback func ptr */
                 WNCAN_IntType        intStatus,
                 UCHAR                chnNum)
    );

    #endif /* USE_CAN_FUNCTION_DEFS */

