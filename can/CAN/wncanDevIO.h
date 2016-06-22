/* wncanDevIO.h - WIND NET CAN Device I/O Interface header file. */

/* Copyright 2002-2003 Wind River Systems, Inc. */
#include "copyright_wrs.h"

/*
modification history
--------------------
01e,20sep04,lsg   Gave enum to setFuncSwitch an identifier
01d,17jul04,lsg   Modified _wncan_ctlrconfig to access multiple ctrlSetConfig
                  functions for FlexCAN
01c,08apr04,lsg   Added FlexCAN related info to WNCAN_CTLRCONFIG
01b,21apr04,bjn   Added WNCAN_REG, WNCAN_REG_SET and WNCAN_REG_GET
01a,19Dec02,emcw  created
*/

/*
DESCRIPTION

This file contains the type and function definitions of the Device I/O Interface feature
of WIND NET CAN. 

NOTE

RESTRICTIONS

INCLUDE FILES

  CAN/wnCAN.h
*/

#ifndef __INCwncanDevIOh
#define __INCwncanDevIOh

#ifdef __cplusplus
extern "C" {
#endif


/* ==== Include files ==== */

#include <vxWorks.h>
#include <iosLib.h>
#include <rngLib.h>
#include <semLib.h>
#include <selectLib.h>
#include <string.h>
#include <CAN/wnCAN.h>


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



/* ==== Internal Device I/O Structures (maybe move to private header file?) ==== */

typedef enum
{
    FD_WNCAN_NONE,
    FD_WNCAN_DEVICE,
    FD_WNCAN_CHANNEL
} WNCAN_FD_TYPE;


typedef STATUS (*CTRLRCONFIGFNTYPE)(void*, void*);

typedef struct  /* DevIO driver information */
{
    DEV_HDR           devHdr;         /* standard I/O System device header */
    WNCAN_BoardType   boardType;      /* CAN board type */
    int               boardIdx;       /* CAN board index */
    int               ctlrIdx;        /* CAN controller index */
    int               numOpenChans;   /* number of open CAN channels */
    BOOL              isDeviceOpen;   /* whether CAN device has been init'ed;
                                         set after CAN_Init() called  */
    SEM_ID            mutex;          /* mutex semaphore for memory access */
    WNCAN_DEVICE     *wncDevice;      /* WNCAN device data structure */

    CTRLRCONFIGFNTYPE ctrlSetConfig;  /* controller-specific functions */
    CTRLRCONFIGFNTYPE ctrlGetConfig;  

} WNCAN_DEVIO_DRVINFO;


typedef struct _devio_fdinfo           /* DevIO file descriptor table info */
{
    WNCAN_DEVIO_DRVINFO* wnDevIODrv;  /* pointer to WNCAN_DEVIO_DRVINFO struct */
    WNCAN_FD_TYPE        devType;     /* indicates which type of data stored here,
                                          device or channel */
    SEL_WAKEUP_LIST      selWakeupList; /* wakeup select list */

    union
    {
        /* CAN device info */
        struct {
            struct _devio_fdinfo **chnInfo; /* allocated channel infos */
        } device;

        /* CAN channel info */
        struct {
            BOOL           enabled;    /* channel up and running flag */
            int            flag;       /* channel access flag */
            ULONG          channel;    /* channel identifier */
            WNCAN_IntType  intType;    /* interrupt status type, WNCAN_INT_TX
                                          or WNCAN_INT_RX */
            RING_ID        inputBuf;   /* input data buffer */
            RING_ID        outputBuf;  /* output data buffer */
        } channel;
    } fdtype;

} WNCAN_DEVIO_FDINFO;


/* ==== Function Prototypes ==== */

#if defined(__STDC__)
extern STATUS wncDevIODevCreate(char*,int,int,int,WNCAN_DEVIO_DRVINFO**);
extern int wncDevIOCreate(WNCAN_DEVIO_DRVINFO*, char*, int);
extern int wncDevIODelete(WNCAN_DEVIO_DRVINFO*, char*);
extern int wncDevIOOpen(WNCAN_DEVIO_DRVINFO*, char*, int, int);
extern STATUS wncDevIOClose(WNCAN_DEVIO_FDINFO*);
extern int wncDevIOReadBuf(WNCAN_DEVIO_FDINFO*, char*, size_t);
extern int wncDevIOWriteBuf(WNCAN_DEVIO_FDINFO*, char*, size_t);
STATUS wncDevIOIoctl(WNCAN_DEVIO_FDINFO*, int, int);
#else
extern STATUS wncDevIODevCreate();
extern int wncDevIOCreate();
extern int wncDevIODelete();
extern int wncDevIOOpen();
extern STATUS wncDevIOClose();
extern int wncDevIOReadBuf();
extern int wncDevIOWriteBuf();
STATUS wncDevIOIoctl();
#endif

#ifdef __cplusplus
}
#endif

#endif /* __INCwncanDevIOh */
