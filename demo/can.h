#ifndef __CAN_H__
#define __CAN_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
	
/* ==== Constants ==== */

#define WNCAN_LG_BUF_SIZE           200
#define WNCAN_MAX_DATA_LEN            8    /* max #bytes in CAN message data */
#define WNCAN_DEFAULT_RINGBUF_SIZE    4    /* default #CAN msgs in internal buffers */


/* ==== WNCAN DevIO interface ioctl() commands ==== */

/* Base command code */

#define DEVIO_CANCMD_BASE        100

/* Device commands */

#define WNCAN_HALT               (DEVIO_CANCMD_BASE + 1)
#define WNCAN_SLEEP              (DEVIO_CANCMD_BASE + 2)
#define WNCAN_WAKE               (DEVIO_CANCMD_BASE + 3)
#define WNCAN_TX_ABORT           (DEVIO_CANCMD_BASE + 4)
#define WNCAN_RXCHAN_GET         (DEVIO_CANCMD_BASE + 5)
#define WNCAN_TXCHAN_GET         (DEVIO_CANCMD_BASE + 6)
#define WNCAN_RTRREQCHAN_GET     (DEVIO_CANCMD_BASE + 7)
#define WNCAN_RTRRESPCHAN_GET    (DEVIO_CANCMD_BASE + 8)
#define WNCAN_BUSINFO_GET        (DEVIO_CANCMD_BASE + 9)
#define WNCAN_CONFIG_SET         (DEVIO_CANCMD_BASE + 10)
#define WNCAN_CONFIG_GET         (DEVIO_CANCMD_BASE + 11)

/* Channel commands */

#define WNCAN_CHNCONFIG_SET      (DEVIO_CANCMD_BASE + 12)
#define WNCAN_CHNCONFIG_GET      (DEVIO_CANCMD_BASE + 13)
#define WNCAN_CHN_ENABLE         (DEVIO_CANCMD_BASE + 14)
#define WNCAN_CHN_TX             (DEVIO_CANCMD_BASE + 15)
#define WNCAN_CHNMSGLOST_GET     (DEVIO_CANCMD_BASE + 16)
#define WNCAN_CHNMSGLOST_CLEAR   (DEVIO_CANCMD_BASE + 17)

/* Controller-specific commands */

#define WNCAN_CTLRCONFIG_SET     (DEVIO_CANCMD_BASE + 18)
#define WNCAN_CTLRCONFIG_GET     (DEVIO_CANCMD_BASE + 19)
/* Additinal Device commands */

#define WNCAN_REG_SET            (DEVIO_CANCMD_BASE + 20)
#define WNCAN_REG_GET            (DEVIO_CANCMD_BASE + 21)

/* ==== CAN configuration access options ==== */

/* 
   CAN device options 
   Used in flags field of WNCAN_CONFIG struct
*/

#define WNCAN_CFG_NONE         0      /* selects no elements */
#define WNCAN_CFG_INFO         0x1    /* selects information elements only */
#define WNCAN_CFG_GBLFILTER    0x2    /* selects global filter only */
#define WNCAN_CFG_BITTIMING    0x4    /* selects bit timing only */
#define WNCAN_CFG_ALL          0x7    /* selects all elements */

/* 
   CAN channel options 
   Used in flags field of WNCAN_CHNCONFIG struct
*/

#define WNCAN_CHNCFG_NONE         0x000    /* selects no elements */
#define WNCAN_CHNCFG_CHANNEL      0x100    /* selects channel information only */
#define WNCAN_CHNCFG_LCLFILTER    0x200    /* selects local filter only */
#define WNCAN_CHNCFG_RTR          0x400    /* selects RTR bit only */
#define WNCAN_CHNCFG_MODE         0x800    /* selects channel mode only */
#define WNCAN_CHNCFG_ALL          0xF00    /* selects all elements */

/* ==== Structures used for setting/getting CAN configuration ==== */

typedef struct tagCANVersionInfo
{
    UCHAR major;
    UCHAR minor;
} WNCAN_VersionInfo;

typedef UINT WNCAN_ControllerType;

typedef enum
{
    _16MHZ = 16000000,
    _20MHZ = 20000000,
    _25MHZ = 25000000,
    _33MHZ = 33000000,
    _39MHZ = 39000000,
    _40MHZ = 40000000,
    _56MHZ = 56000000
} XtalFreq;
	
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
#define ACCEPT_ALL_EXT_IDS  0

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


/* CAN bus information */

typedef struct _wncan_businfo
{
    WNCAN_BusStatus busStatus;  /* bus status */
    WNCAN_BusError  busError;   /* bus error */
}  WNCAN_BUSINFO;


/* CAN device configuration options */

typedef struct _wncan_config
{
    UINT  flags;  /* access options */

    /* read-only items */
    struct 
    {
        WNCAN_VersionInfo    version;     /* WNC version */
        WNCAN_ControllerType ctrlType;    /* CAN controller type */
        XtalFreq             xtalfreq;    /* crystal frequency */
        UCHAR                numChannels; /* total # of channels */
        UINT                 baudRate;    /* computed baudrate bits/sec */
        UINT                 samplePoint; /* percentage of bit time at 
                                             which the bit is sampled.*/
    } info;

    /* read/write items */
    struct 
    {
        ULONG mask;      /* filter mask */
        BOOL  extended;  /* extended flag */
    } filter;

    struct 
    {
        UCHAR tseg1;      /* time quanta for segment 1 */
        UCHAR tseg2;      /* time quanta for segment 2 */
        UCHAR brp;        /* baud rate prescaler */
        UCHAR sjw;        /* syncro jump width */
        BOOL  oversample; /* normal or over-sampling option */
    } bittiming;

}  WNCAN_CONFIG;


/* CAN device register get/set configuration */

typedef struct _wncan_reg
{
    UINT offset;   /* Register offset from base to access */
    UCHAR * pData; /* Pointer to data buffer for read/write */
    UINT length;   /* Length of data in bytes to read/write */
}  WNCAN_REG;


/* CAN channel configuration options */

typedef struct _wncan_chnconfig
{
    UINT  flags;  /* access options */

    /* local filter mask */
    struct 
    {
        ULONG mask;      /* filter mask */
        BOOL  extended;  /* extended flag */
    } filter;

    struct 
    {
        ULONG id;                        /* CAN ID */
        BOOL  extId;                     /* is ID extended or not? */
        UCHAR len;                       /* length */
        UCHAR data[WNCAN_MAX_DATA_LEN];  /* message data */
    } channel;

    BOOL              rtr;   /* RTR bit setting */
    WNCAN_ChannelMode mode;  /* channel mode */

}  WNCAN_CHNCONFIG;


/* CAN controller-specific configuration options */

typedef struct _wncan_ctlrconfig
{
    WNCAN_ControllerType  ctlrType;

    union
    {
        struct
        {
            UCHAR propseg;  /* propagation segment */
        } toucanData;

    struct
        {
        struct intLevelInfo_t{
            UCHAR intSrcNum; /*flexcan interrupt source number, always an input for WNCAN_CTLRCONFIG_SET
                                           and WNCAN_CTLRCONFIG_GET */
            UCHAR intLevel;  /*flexcan interrupt level, for WNCAN_CTLRCONFIG_SET: input, 
                                           for WNCAN_CTLRCONFIG_GET: output */
            UCHAR intPrioLevel;/* priority within interrupt level, for WNCAN_CTLRCONFIG_SET: input, 
                                           for WNCAN_CTLRCONFIG_GET: output */
        } intLevelInfo;  

        UCHAR propseg; 

        enum setFuncSwitch_t{
            WNCAN_FLEXCAN_DEVIO_SET_INTLEVEL=0,
            WNCAN_FLEXCAN_DEVIO_SET_PROPSEG
        }setFuncSwitch;

    } flexcanData;

        /* Other controller-specific structs can be defined here */
        ULONG  dummy;  /* placeholder for future defs */

    } ctlrData;

}  WNCAN_CTLRCONFIG;

/* CAN message */

typedef struct _wncan_chnmsg
{
    ULONG id;                        /* CAN ID */
    BOOL  extId;                     /* is ID extended or not? */
    BOOL  rtr;                       /* remote frame transmit request */
    UCHAR len;                       /* message length */
    UCHAR data[WNCAN_MAX_DATA_LEN];  /* message data */
}  WNCAN_CHNMSG;





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

