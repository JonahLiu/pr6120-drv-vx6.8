/* wncan_api.h - Wind River's CAN Common API header file. */

/* Copyright 1984-2001 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01, 18Dec01, jac created
*/

/*
DESCRIPTION

This file contains the function definitions of Wind River's CAN Common API. 

NOTE

For normal usage, these function calls are replaced by equivalent macro 
definitions in CAN/wnCAN.h

RESTRICTIONS

INCLUDE FILES

CAN/wnCAN.h
CAN/canBoard.h

*/

#ifndef WNCAN_API_H
#define WNCAN_API_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Uncomment the next line if you want to use actual function
   definitions for the CAN Common API */
/* #define USE_CAN_FUNCTION_DEFS */

#ifdef USE_CAN_FUNCTION_DEFS
#undef CAN_Open
#undef CAN_Close
#undef CAN_GetMode
#undef CAN_SetMode
#undef CAN_GetTxChannel
#undef CAN_GetRxChannel
#undef CAN_GetRTRRequesterChannel
#undef CAN_GetRTRResponderChannel
#undef CAN_FreeChannel
#undef CAN_GetVersion
#undef CAN_GetBusStatus
#undef CAN_GetBusError
#undef CAN_Init
#undef CAN_Start
#undef CAN_Stop
#undef CAN_SetBitTiming
#undef CAN_GetBaudRate
#undef CAN_SetIntMask
#undef CAN_EnableInt
#undef CAN_DisableInt
#undef CAN_ReadID
#undef CAN_WriteID
#undef CAN_ReadData
#undef CAN_GetMessageLength
#undef CAN_WriteData
#undef CAN_Tx
#undef CAN_TxMsg
#undef CAN_SetGlobalRxFilter
#undef CAN_GetGlobalRxFilter
#undef CAN_SetLocalMsgFilter
#undef CAN_GetLocalMsgFilter
#undef CAN_GetIntStatus
#undef CAN_IsMessageLost
#undef CAN_ClearMessageLost
#undef CAN_SetRTR
#undef CAN_IsRTR
#undef CAN_TxAbort
#undef CAN_Sleep
#undef CAN_WakeUp
#undef CAN_EnableChannel
#undef CAN_DisableChannel
#undef CAN_WriteReg
#undef CAN_ReadReg

/* variable access */
#undef CAN_GetXtalFreq
#undef CAN_GetControllerType
#undef CAN_GetNumChannels

/* controller independent function prototypes */
struct WNCAN_Device *CAN_Open(unsigned int brdType, unsigned int brdNdx, 
                              unsigned int ctrlNdx);

void CAN_Close(struct WNCAN_Device *pDev);

WNCAN_ChannelMode CAN_GetMode(struct WNCAN_Device *pDev, UCHAR chn);

STATUS CAN_SetMode(struct WNCAN_Device *pDev, UCHAR channelNum, 
                   WNCAN_ChannelMode mode);

STATUS CAN_GetTxChannel(struct WNCAN_Device *pDev, UCHAR *channelNum);

STATUS CAN_GetRxChannel(struct WNCAN_Device *pDev, UCHAR *channelNum);

STATUS CAN_GetRTRRequesterChannel(struct WNCAN_Device *pDev, UCHAR *channelNum);

STATUS CAN_GetRTRResponderChannel(struct WNCAN_Device *pDev, UCHAR *channelNum);

STATUS CAN_FreeChannel(struct WNCAN_Device *pDev, UCHAR channelNum);

const WNCAN_VersionInfo* CAN_GetVersion(void);

/* controller dependent function prototypes */

WNCAN_BusStatus CAN_GetBusStatus(struct WNCAN_Device *pDev);

WNCAN_BusError CAN_GetBusError(struct WNCAN_Device *pDev);

STATUS CAN_Init(struct WNCAN_Device *pDev);

void CAN_Start(struct WNCAN_Device *pDev);

void CAN_Stop(struct WNCAN_Device *pDev);

STATUS CAN_SetBitTiming(struct WNCAN_Device *pDev, UCHAR tseg1, UCHAR tseg2,
                        UCHAR brp, UCHAR sjw, BOOL numSamples);

UINT   CAN_GetBaudRate(struct WNCAN_Device *pDev, UINT* samplePoint);                   

STATUS CAN_SetIntMask(struct WNCAN_Device *pDev, WNCAN_IntType intMask);

void CAN_EnableInt(struct WNCAN_Device *pDev);

void CAN_DisableInt(struct WNCAN_Device *pDev);

long CAN_ReadID(struct WNCAN_Device *pDev, UCHAR channelNum, BOOL* ext);

STATUS CAN_WriteID(struct WNCAN_Device *pDev, UCHAR chnNum, ULONG canID, BOOL ext);

STATUS CAN_ReadData(struct WNCAN_Device  *pDev, UCHAR chnNum, UCHAR *data,
                    UCHAR *len, BOOL *newData);

int CAN_GetMessageLength(struct WNCAN_Device *pDev, UCHAR channelNum);

STATUS CAN_WriteData(struct WNCAN_Device *pDev, UCHAR channelNum, 
                     UCHAR *data, UCHAR len);

STATUS CAN_Tx(struct WNCAN_Device *pDev, UCHAR channelNum);

STATUS CAN_TxMsg(struct WNCAN_Device *pDev, UCHAR channelNum, ULONG canId, 
                 BOOL ext, UCHAR *data, UCHAR len);
                    
STATUS CAN_SetGlobalRxFilter(struct WNCAN_Device *pDev, long mask, BOOL ext);

long CAN_GetGlobalRxFilter(struct WNCAN_Device *pDev, BOOL ext);

STATUS CAN_SetLocalMsgFilter(struct WNCAN_Device *pDev, UCHAR channel, long mask, BOOL ext);

long CAN_GetLocalMsgFilter(struct WNCAN_Device *pDev, UCHAR channel, BOOL ext);

WNCAN_IntType CAN_GetIntStatus(struct WNCAN_Device *pDev, UCHAR *channelNum);

int CAN_IsMessageLost(struct WNCAN_Device *pDev, UCHAR channelNum);

STATUS CAN_ClearMessageLost(struct WNCAN_Device *pDev, UCHAR channelNum);

STATUS CAN_SetRTR(struct WNCAN_Device *pDev, UCHAR channelNum, BOOL rtr);

int CAN_IsRTR(struct WNCAN_Device *pDev, UCHAR channelNum);

void CAN_TxAbort(struct WNCAN_Device *pDev);

STATUS CAN_Sleep(struct WNCAN_Device *pDev);

STATUS CAN_WakeUp(struct WNCAN_Device *pDev);

STATUS CAN_EnableChannel(struct WNCAN_Device *pDev, UCHAR channelNum,
						 WNCAN_IntType intSetting);

STATUS CAN_DisableChannel(struct WNCAN_Device *pDev, UCHAR channelNum);

STATUS CAN_WriteReg(struct WNCAN_Device *pDev, UINT offset,
		               UCHAR *data, UINT length);

STATUS CAN_ReadReg(struct WNCAN_Device *pDev, UINT offset,
		               UCHAR *data, UINT length);					

XtalFreq CAN_GetXtalFreq(struct WNCAN_Device *pDev);

WNCAN_ControllerType CAN_GetControllerType(struct WNCAN_Device *pDev);

UCHAR CAN_GetNumChannels(struct WNCAN_Device *pDev);

#endif /* USE_CAN_FUNCTION_DEFS */

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* WNCAN_API_H */
