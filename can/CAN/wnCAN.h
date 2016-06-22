/* wnCAN.h - definitions for Windnet CAN Interface */

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
  This file contains the declarations and definitions that comprise the
  Windnet CAN Interface.
  
*/

#ifndef WN_CAN_H
#define WN_CAN_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <CAN/canController.h>
#include <CAN/canBoard.h>

typedef struct tagCANVersionInfo
{
    UCHAR major;
    UCHAR minor;
} WNCAN_VersionInfo;

#define WNCAN_ILLEGAL_CHN_NUM      0xff

/* types of channel configurations */
#define WNCAN_CHN_INVALID          0x00
#define WNCAN_CHN_TRANSMIT         0x01
#define WNCAN_CHN_RECEIVE          0x02
#define WNCAN_CHN_TRANSMIT_RECEIVE 0x03
#define WNCAN_CHN_INACTIVE         0x04
#define WNCAN_CHN_RTR_REQUESTER    0x11
#define WNCAN_CHN_RTR_RESPONDER    0x12

typedef UINT WNCAN_ChannelType;
typedef UINT WNCAN_ChannelMode;

#define WNCAN_IS_CHN_RTR          0x10

/* CAN bus error type */
#define WNCAN_ERR_NONE    0
#define WNCAN_ERR_BIT     0x1
#define WNCAN_ERR_ACK     0x2
#define WNCAN_ERR_CRC     0x4
#define WNCAN_ERR_FORM    0x8
#define WNCAN_ERR_STUFF   0x10
#define WNCAN_ERR_UNKNOWN 0x20

typedef UINT WNCAN_BusError;

/* status code returned by CAN_GetBusStatus */
#define WNCAN_BUS_OK   0
#define WNCAN_BUS_WARN 1
#define WNCAN_BUS_OFF  2

typedef UINT WNCAN_BusStatus;

/* Interrupt status */
#define WNCAN_INT_NONE     0
#define WNCAN_INT_ERROR    0x1
#define WNCAN_INT_BUS_OFF  0x2
#define WNCAN_INT_WAKE_UP  0x4
#define WNCAN_INT_TX       0x8
#define WNCAN_INT_RX       0x10
#define WNCAN_INT_RTR_RESPONSE 0x20
#define WNCAN_INT_TXCLR    0x40
#define WNCAN_INT_SPURIOUS 0xffffeeee
#define WNCAN_INT_ALL      0xffffffff 

typedef UINT WNCAN_IntType;


/*Macro definitions to be used with filtering*/
#define COMPARE_ALL_STD_IDS 0x7FF
#define COMPARE_ALL_EXT_IDS 0x1FFFFFFF
#define ACCEPT_ALL_STD_IDS  0
#define ACCEPT_ALL_STD_IDS  0

/* STATUS error codes returned by CAN interface functions */
#define WNCAN_NOERROR                 0
#define WNCAN_BUSY                    80
#define WNCAN_NOTINITIALIZED          81
#define WNCAN_NULL_POINTER            82
#define WNCAN_ILLEGAL_CHANNEL_NO      83
#define WNCAN_ILLEGAL_CONFIG          84
#define WNCAN_ILLEGAL_CTRL_NO         85
#define WNCAN_ILLEGAL_BOARD_INDEX     86
#define WNCAN_NO_OP                   87
#define WNCAN_BUFFER_UNDERFLOW        89
#define WNCAN_BUFFER_OVERFLOW         90
#define WNCAN_ILLEGAL_DATA_LENGTH     91
#define WNCAN_NULL_BUFFER             92 
#define WNCAN_ILLEGAL_MASK_VALUE      93
#define WNCAN_BUS_FAULT               94 
#define WNCAN_HWFEATURE_NOT_AVAILABLE 95
#define WNCAN_ILLEGAL_CHANNEL_MODE    96
#define WNCAN_RTR_MODE_NOT_SUPPORTED  97
#define WNCAN_NO_AVAIL_CHANNELS       98
#define WNCAN_ILLEGAL_OFFSET          101
#define WNCAN_CANNOT_SET_ERRINT       102
#define WNCAN_CANNOT_SET_BOFFINT      104
#define WNCAN_CANNOT_SET_WAKEUPINT    108
#define WNCAN_INCOMPATIBLE_TYPE       109
#define WNCAN_SLEEP_IN_RESET_MODE     110
#define WNCAN_USE_AUTO_RESPONSE       111
#define WNCAN_USE_RTR_REQ_CHN_MODE    112
#define WNCAN_TSEG_ZERO               113
#define WNCAN_NUMTQ_LT_NINE           114
#define WNCAN_UNKNOWN_ERROR           255

/* defined error codes */
#define M_wnCan (496 << 16)

#define S_can_invalid_parameter          (M_wnCan + 1)
#define S_can_unknown_controller         (M_wnCan + 2)
#define S_can_invalid_timing_combination (M_wnCan + 3)
#define S_can_out_of_memory              (M_wnCan + 4)
#define S_can_unknown_board              (M_wnCan + 5)

#define S_can_busy                       (M_wnCan + WNCAN_BUSY)
#define S_can_illegal_channel_no         (M_wnCan + WNCAN_ILLEGAL_CHANNEL_NO)
#define S_can_illegal_config             (M_wnCan + WNCAN_ILLEGAL_CONFIG)
#define S_can_illegal_ctrl_no            (M_wnCan + WNCAN_ILLEGAL_CTRL_NO)
#define S_can_illegal_board_no           (M_wnCan + WNCAN_ILLEGAL_BOARD_INDEX)
#define S_can_no_op                      (M_wnCan + WNCAN_NO_OP)
#define S_can_buffer_underflow           (M_wnCan + WNCAN_BUFFER_UNDERFLOW)
#define S_can_buffer_overflow            (M_wnCan + WNCAN_BUFFER_OVERFLOW)
#define S_can_illegal_data_length        (M_wnCan + WNCAN_ILLEGAL_DATA_LENGTH)
#define S_can_null_input_buffer          (M_wnCan + WNCAN_NULL_BUFFER)
#define S_can_illegal_mask_value         (M_wnCan + WNCAN_ILLEGAL_MASK_VALUE) 
#define S_can_bus_fault                  (M_wnCan + WNCAN_BUS_FAULT)
#define S_can_hwfeature_not_available    (M_wnCan + WNCAN_HWFEATURE_NOT_AVAILABLE) 
#define S_can_illegal_channel_mode       (M_wnCan + WNCAN_ILLEGAL_CHANNEL_MODE) 
#define S_can_illegal_offset			 (M_wnCan + WNCAN_ILLEGAL_OFFSET)
#define S_can_cannot_set_error_int       (M_wnCan + WNCAN_CANNOT_SET_ERRINT)
#define S_can_cannot_set_boff_int        (M_wnCan + WNCAN_CANNOT_SET_BOFFINT)
#define S_can_cannot_set_wakeup_int      (M_wnCan + WNCAN_CANNOT_SET_WAKEUPINT)
#define S_can_rtr_mode_not_supported     (M_wnCan + WNCAN_RTR_MODE_NOT_SUPPORTED)
#define S_can_no_available_channels      (M_wnCan + WNCAN_NO_AVAIL_CHANNELS)
#define S_can_incompatible_int_type      (M_wnCan + WNCAN_INCOMPATIBLE_TYPE)
#define S_can_cannot_sleep_in_reset_mode (M_wnCan + WNCAN_SLEEP_IN_RESET_MODE)
#define S_can_use_auto_response_feature  (M_wnCan + WNCAN_USE_AUTO_RESPONSE)
#define S_can_use_rtr_requester_channel_mode (M_wnCan + WNCAN_USE_RTR_REQ_CHN_MODE)
#define S_can_tseg2_cannot_be_zero       (M_wnCan + WNCAN_TSEG_ZERO)    
#define S_can_numTq_less_than_nine       (M_wnCan + WNCAN_NUMTQ_LT_NINE)      
#define S_can_Unknown_Error              (M_wnCan + WNCAN_UNKNOWN_ERROR)

struct WNCAN_Controller;
struct WNCAN_Board;

typedef struct WNCAN_Device
{
    STATUS            (*Init)(struct WNCAN_Device *p);
	
    void              (*Start)(struct WNCAN_Device *p);
	
    void              (*Stop)(struct WNCAN_Device *p);

    STATUS            (*SetIntMask)(struct WNCAN_Device *pDev, WNCAN_IntType intMask); 
	
    void              (*EnableInt)(struct WNCAN_Device *pDev);
	
    void              (*DisableInt)(struct WNCAN_Device *pDev);                    
	
    STATUS            (*SetBitTiming)(struct WNCAN_Device *pDev,UCHAR tseg1,
                       UCHAR tseg2,UCHAR brp,UCHAR sjw, BOOL numSamples);
	
    UINT              (*GetBaudRate)(struct WNCAN_Device *pDev, UINT* samplePoint);
    
    WNCAN_BusStatus   (*GetBusStatus)(struct WNCAN_Device *pDev);
	
    WNCAN_BusError    (*GetBusError)(struct WNCAN_Device *pDev);
	
    WNCAN_IntType     (*GetIntStatus)(struct WNCAN_Device *pDev,UCHAR *chnNum);
	
    long              (*ReadID)(struct WNCAN_Device *pDev,UCHAR chnNum,
		               BOOL *ext);
	
    STATUS            (*WriteID)(struct WNCAN_Device *pDev,UCHAR chnNum,
		               ULONG canID,BOOL ext);
	
    STATUS            (*ReadData)(struct WNCAN_Device *pDev,UCHAR chnNum,
		               UCHAR *data,UCHAR *len,BOOL *newData);
	
    int               (*GetMessageLength)(struct WNCAN_Device *pDev,
		               UCHAR chnNum);    
    
    STATUS            (*WriteData)(struct WNCAN_Device *pDev,UCHAR chnNum,
		               UCHAR* data,UCHAR len);
	
    STATUS            (*Tx)(struct WNCAN_Device *pDev,UCHAR chnNum);
	
    STATUS            (*TxMsg)(struct WNCAN_Device *pDev,UCHAR chnNum,
		               ULONG canId,BOOL ext,UCHAR* data,UCHAR len);
	
    STATUS            (*SetGlobalRxFilter)(struct WNCAN_Device *pDev,
		               long mask,BOOL ext);
	
    long              (*GetGlobalRxFilter)(struct WNCAN_Device *pDev, BOOL ext);
	
    STATUS            (*SetLocalMsgFilter)(struct WNCAN_Device *pDev,
                       UCHAR channel, long mask, BOOL ext);

    long              (*GetLocalMsgFilter)(struct WNCAN_Device *pDev, UCHAR channel, BOOL ext);
    
    int               (*IsMessageLost)(struct WNCAN_Device *pDev,
                       UCHAR chnNum);

	STATUS            (*ClearMessageLost)(struct WNCAN_Device *pDev,
                       UCHAR chnNum);
    
    int               (*IsRTR)(struct WNCAN_Device *pDev,UCHAR chnNum);

	STATUS            (*SetRTR)(struct WNCAN_Device *pDev,UCHAR chnNum, BOOL rtr);
	
    void              (*TxAbort)(struct WNCAN_Device *pDev);
	
    STATUS            (*Sleep)(struct WNCAN_Device *pDev);	

	STATUS            (*WakeUp)(struct WNCAN_Device *pDev);	
	
    void              (*pISRCallback)(struct WNCAN_Device *pDev,
		               WNCAN_IntType intStatus, UCHAR chnNum);
	
    STATUS            (*EnableChannel )(struct WNCAN_Device *pDev,
		               UCHAR chnNum, WNCAN_IntType useInterrupts);
	
    STATUS            (*DisableChannel )(struct WNCAN_Device *pDev,
		               UCHAR chnNum);
	
	
    STATUS            (*WriteReg)(struct WNCAN_Device *pDev, UINT offset,
		               UCHAR *data, UINT length);
	
    STATUS            (*ReadReg)(struct WNCAN_Device *pDev, UINT offset,
		               UCHAR *data, UINT length);
	
    const char        *deviceName;
    UINT               deviceId;
	
    struct WNCAN_Controller *pCtrl;
    struct WNCAN_Board      *pBrd;
    void                    *userData;          /* user data context pointer */
	
} WNCAN_DEVICE;

/* function prototypes */
STATUS CAN_InstallISRCallback(struct WNCAN_Device *pDev,
							  void (*pFun)(struct WNCAN_Device *pDev2,
							  WNCAN_IntType intStatus,UCHAR chnNum));

struct WNCAN_Device *WNCAN_Open(UINT,UINT,UINT);

void WNCAN_Close(struct WNCAN_Device *);

WNCAN_ChannelMode WNCAN_GetMode(struct WNCAN_Device *pDev, UCHAR chn);

STATUS WNCAN_SetMode(struct WNCAN_Device *pDev, UCHAR channelNum, 
					 WNCAN_ChannelMode mode);

STATUS WNCAN_GetTxChannel(struct WNCAN_Device *pDev,UCHAR *chnNum);

STATUS WNCAN_GetRxChannel(struct WNCAN_Device *pDev,UCHAR *chnNum);

STATUS WNCAN_GetRTRRequesterChannel(struct WNCAN_Device *pDev,UCHAR *chnNum);

STATUS WNCAN_GetRTRResponderChannel(struct WNCAN_Device *pDev,UCHAR *chnNum);

STATUS WNCAN_FreeChannel(struct WNCAN_Device *pDev,UCHAR chnNum);

const WNCAN_VersionInfo* WNCAN_GetVersion(void);

void wncan_core_init(void);

/* additional functions */
#ifdef INCLUDE_TOUCAN
STATUS TouCAN_SetPropseg(struct WNCAN_Device *pDev, UCHAR propseg);
#endif

#ifdef INCLUDE_FLEXCAN
STATUS FlexCAN_SetPropseg(struct WNCAN_Device *pDev, UCHAR propseg);
STATUS FlexCAN_SetIntLevel(struct WNCAN_Device *pDev, UCHAR srcNum, UCHAR intLevel, UCHAR intPriorityInLevel);
STATUS FlexCAN_GetIntLevel(struct WNCAN_Device *pDev, UCHAR srcNum, UCHAR *intLevel, UCHAR *intPriorityInLevel);
#endif

/* The following Macros map the interface function calls to the implementation
functions set by the installation routine of the device driver. */

/* controller independent function prototypes */

#define CAN_Open(a,b,c)             WNCAN_Open(a,b,c)

#define CAN_Close(a)                WNCAN_Close(a)

#define CAN_GetMode(a,b)            WNCAN_GetMode(a,b)

#define CAN_SetMode(a,b,c)          WNCAN_SetMode(a,b,c)

#define CAN_GetTxChannel(a,b)       WNCAN_GetTxChannel(a,b)

#define CAN_GetRxChannel(a,b)       WNCAN_GetRxChannel(a,b)

#define CAN_GetRTRResponderChannel(a,b)       WNCAN_GetRTRResponderChannel(a,b)

#define CAN_GetRTRRequesterChannel(a,b)       WNCAN_GetRTRRequesterChannel(a,b)

#define CAN_FreeChannel(a,b)        WNCAN_FreeChannel(a,b)

#define CAN_GetVersion()           WNCAN_GetVersion()

/* controller dependent function prototypes */

#define CAN_GetBusStatus(a)         a->GetBusStatus(a)

#define CAN_GetBusError(a)          a->GetBusError(a)

#define CAN_Init(a)                 a->Init(a)

#define CAN_Start(a)                a->Start(a)

#define CAN_Stop(a)                 a->Stop(a)

#define CAN_SetBitTiming(a,b,c,d,e,f) a->SetBitTiming(a,b,c,d,e,f)

#define CAN_GetBaudRate(a,b)        a->GetBaudRate(a,b)

#define CAN_SetIntMask(a,b)         a->SetIntMask(a,b)

#define CAN_EnableInt(a)            a->EnableInt(a)

#define CAN_DisableInt(a)           a->DisableInt(a)

#define CAN_ReadID(a,b,c)           a->ReadID(a,b,c)

#define CAN_WriteID(a,b,c,d)        a->WriteID(a,b,c,d)

#define CAN_ReadData(a,b,c,d,e)     a->ReadData(a,b,c,d,e)

#define CAN_GetMessageLength(a,b)   a->GetMessageLength(a,b)

#define CAN_WriteData(a,b,c,d)      a->WriteData(a,b,c,d)

#define CAN_Tx(a,b)                 a->Tx(a,b)

#define CAN_TxMsg(a,b,c,d,e,f)       a->TxMsg(a,b,c,d,e,f)

#define CAN_SetGlobalRxFilter(a,b,c) a->SetGlobalRxFilter(a,b,c)

#define CAN_GetGlobalRxFilter(a,b)   a->GetGlobalRxFilter(a,b)

#define CAN_SetLocalMsgFilter(a,b,c,d) a->SetLocalMsgFilter(a,b,c,d)

#define CAN_GetLocalMsgFilter(a, b, c) a->GetLocalMsgFilter(a, b, c)

#define CAN_GetIntStatus(a,b)        a->GetIntStatus(a,b)

#define CAN_IsMessageLost(a,b)       a->IsMessageLost(a,b)

#define CAN_ClearMessageLost(a,b)    a->ClearMessageLost(a,b)

#define CAN_IsRTR(a,b)               a->IsRTR(a,b)

#define CAN_SetRTR(a,b,c)            a->SetRTR(a,b,c)

#define CAN_TxAbort(a)               a->TxAbort(a)

#define CAN_Sleep(a)                 a->Sleep(a)

#define CAN_WakeUp(a)                a->WakeUp(a)

#define CAN_WriteReg(a,b,c,d)        a->WriteReg(a,b,c,d) 

#define CAN_ReadReg(a,b,c,d)         a->ReadReg(a,b,c,d) 

#define CAN_EnableChannel(a,b,c)     a->EnableChannel(a,b,c)
#define CAN_DisableChannel(a,b)      a->DisableChannel(a,b)

/* variable access */
#define CAN_GetXtalFreq(a)           a->pBrd->xtalFreq
#define CAN_GetControllerType(a)     a->pCtrl->ctrlType
#define CAN_GetNumChannels(a)        a->pCtrl->numChn
#define CAN_SetSjw(a,b)             (a->pCtrl->sjw = b)
#define CAN_SetBrp(a,b)             (a->pCtrl->brp = b)
#define CAN_SetTseg1(a,b)           (a->pCtrl->tseg1 = b)
#define CAN_SetTseg2(a,b)           (a->pCtrl->tseg2 = b)
#define CAN_SetNumOfSamples(a,b)    (a->pCtrl->samples = b)

/*These macros are provided for backward compatibility */
#define I82527_SetMsgObj15RxFilter(a, b) \
CAN_SetLocalMsgFilter(a,14,b,1)

#define TouCAN_SetMsgObj14or15RxFilter(a,b,c,d) \
CAN_SetLocalMsgFilter(a,d,b,c)

#include <CAN/wncan_api.h>

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* WN_CAN_H */
