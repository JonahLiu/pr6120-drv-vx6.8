/* canController.h - CAN controller definitions for Wind River CAN Common Interface */

/*
 * Copyright (c) 2001, 2004, 2007 Wind River Systems, Inc.
 *
 * The right to copy, distribute, modify or otherwise make use
 * of this software may be licensed only pursuant to the terms
 * of an applicable Wind River license agreement.
 */

/* 
modification history 
--------------------
01a,04dec07,d_z Added tolapai can support
08apr04,lsg added FlexCAN related macros
26feb04,bjn Added MPC5200 support
26dec01,lsg added TOUCAN (PPC5xx) related macros
09nov01,dnb modified for integration into Tornado
12jul01,jac written

*/

/* 

DESCRIPTION
This file contains CAN controller definitions for the 
CAN Common Interface.

*/

#ifndef CAN_CONTROLLER_H_
#define CAN_CONTROLLER_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct WNCAN_Device;
typedef UINT WNCAN_ControllerType;

/* Datatypes used for controller's linked list */
typedef void  (*pfn_Ctrlr_EstabLnks_Type)(struct WNCAN_Device *pDev);


/* The CAN_ControllerType lists the available controllers */
#define WNCAN_I82527   1
#define WNCAN_SJA1000  2
#define WNCAN_TOUCAN   3    
#define WNCAN_MCP2510  4
#define WNCAN_HCAN2    5
#define WNCAN_MOTMSCAN 6
#define WNCAN_FLEXCAN  7    
#define WNCAN_CTL_TOLAPAI 8    

struct WNCAN_Controller
{
    WNCAN_ControllerType ctrlType;  /* type of controller */
    UCHAR                ctrlID;    /* handle identifier of this controller */
    UCHAR                brp;       
    UCHAR                sjw;  
    UCHAR                tseg1;
    UCHAR                tseg2; 
    UCHAR                samples;
    struct WNCAN_Device  *pDev;    /* pointer to parent CAN_Device structure */
    const UINT           *chnType; /* number of ways a message buffer can be configured */    
    UINT                 *chnMode; /* current way a message buffer is configured */
    UCHAR                numChn;   /* number of available channels */
    void                 *csData;  /* pointer to chip-specific data */    
};


/* function prototypes */
STATUS WNCAN_Controller_establishLinks(struct WNCAN_Device *pDev, 
    WNCAN_ControllerType ctrlType);


/* define access macros for use with the controller's linked list */
#define CONTROLLERLL_INIT()   FLL_Create(&g_ControllerLLRoot)
#define CONTROLLERLL_ADD(n)   FLL_Add(&g_ControllerLLRoot, n)
#define CONTROLLERLL_GET(key) ((ControllerLLNode*)FLL_Find(&g_ControllerLLRoot, key))
#define CONTROLLERLL_FIRST()  ((ControllerLLNode*)FLL_FirstNode(&g_ControllerLLRoot))
#define CONTROLLERLL_NEXT(n)  ((ControllerLLNode*)FLL_NextNode(n))

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CAN_CONTROLLER_H_ */
