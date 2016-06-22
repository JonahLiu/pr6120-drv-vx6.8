/* canBoard.h - CAN board definitions for Wind River CAN Common Interface */

/*
 * Copyright (c) 2001, 2004-2005, 2007-2008 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/* 
modification history 
--------------------
2016/05/27  Jonah Liu  Add PR6120 board CAN port support
01c,30dec08,x_f  Added fsl_ads5121e CAN support
01b,04dec07,d_z  Added tolapai can support
01a,28dec05,jb3  Add WNCAN_MCF5485 for spring release
08apr04,lsg added support for ColdFire 5282 FlexCAN
26feb04,bjn Added MPC5200 support
09nov01,dnb modified for integration into Tornado
12jul01,jac written

*/

/* 

DESCRIPTION
This file contains CAN board definitions for the 
CAN Common Interface.

*/

#ifndef CAN_BOARD_H_
#define CAN_BOARD_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* forward reference these datatypes */
typedef UINT WNCAN_BoardType;
struct WNCAN_Device;


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


/* Datatypes used for board's linked list */
typedef STATUS (*pfn_Board_EstabLnks_Type)(struct WNCAN_Device*);
typedef STATUS (*pfn_Board_Close_Type)(struct WNCAN_Device*);
typedef struct WNCAN_Device* (*pfn_Board_Open_Type)(UINT, UINT);
typedef void   (*pfn_Board_Show_Type)(void);


/* The CAN_BoardType lists the available CAN boards or modules */
#define WNCAN_NO_BOARD_ASSIGNED 0
#define WNCAN_ESD_PC104_200     1
#define WNCAN_ESD_PCI_200       2
#define WNCAN_MSMCAN            3
#define WNCAN_PPC5XX            4
#define WNCAN_ADLINK_7841       5
#define WNCAN_DAYTONA           6
#define WNCAN_SHTAHOEAMANDA     7
#define WNCAN_SHBISCAYNE        8
#define WNCAN_MPC5200           9
#define WNCAN_MCF5282           10
#define WNCAN_MCF5485           11
#define WNCAN_BRD_TOLAPAI       12
#define WNCAN_MPC5121E          13
#define WNCAN_PR6120_CAN        14

struct WNCAN_Board
{
    void (*onEnterISR)(struct WNCAN_Device *);
    void (*onLeaveISR)(struct WNCAN_Device *);
    UCHAR (*canInByte)(struct WNCAN_Device *, unsigned int);
    void  (*canOutByte)(struct WNCAN_Device *, unsigned int, UCHAR);
    void (*enableIrq)(struct WNCAN_Device *);
    void (*disableIrq)(struct WNCAN_Device *);

    WNCAN_BoardType brdType;
    UINT            irq;
    ULONG           ioAddress;
    UINT32          bar0;
    UINT32          bar1;
    UINT32          bar2;
    XtalFreq        xtalFreq;
};    

/* function prototypes */
STATUS WNCAN_Board_establishLinks(struct WNCAN_Device *, WNCAN_BoardType);
struct WNCAN_Device *WNCAN_Board_Open(UINT,UINT,UINT);
void WNCAN_Board_Close(struct WNCAN_Device *);
ULONG stringToUlong(const char *pStr);


/* define access macros for use with the board's linked list */
#define BOARDLL_INIT()   FLL_Create(&g_BoardLLRoot)
#define BOARDLL_ADD(n)   FLL_Add(&g_BoardLLRoot, n)
#define BOARDLL_GET(key) ((BoardLLNode*)FLL_Find(&g_BoardLLRoot, key))
#define BOARDLL_FIRST()  ((BoardLLNode*)FLL_FirstNode(&g_BoardLLRoot))
#define BOARDLL_NEXT(n)  ((BoardLLNode*)FLL_NextNode(n))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CAN_BOARD_H_ */
