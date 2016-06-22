/* canController.c - implementation of CAN Controller routines */

/* Copyright 2001 Wind River Systems, Inc. */

/* 
modification history 
--------------------
09nov01,dnb modified for integration into Tornado
12jul01,jac written

*/

/* 

DESCRIPTION
implementation of CAN controller routines

*/

/* includes */
#include <vxWorks.h>
#include <errnoLib.h>
#include <CAN/wnCAN.h>
#include <CAN/canBoard.h>
#include <CAN/canController.h>
#include <CAN/canFixedLL.h>


/* fixed-node linked list instance for all controllers */
FLL_RootType g_ControllerLLRoot;


/************************************************************************
*
* WNCAN_Controller_establishLinks - connect the function pointers in the
*                                 Device structure to the appropriate
*                                 routines. Set the number of channels in 
*                                 the controller structure 
*
*
* RETURNS: OK or ERROR
*   
* ERRNO: S_can_unknown_controller
*
*/
STATUS WNCAN_Controller_establishLinks
(
    struct WNCAN_Device *pDev, 
    WNCAN_ControllerType ctrlType
)
{
    STATUS retCode = OK;
    ControllerLLNode *pCtrlrNode;

    pCtrlrNode = CONTROLLERLL_GET(ctrlType);
    if (pCtrlrNode && pCtrlrNode->nodedata.controllerdata.establinks_fn)
    {
        (*pCtrlrNode->nodedata.controllerdata.establinks_fn)(pDev);
    }
    else
    {
        errnoSet(S_can_unknown_controller);
        retCode = ERROR;
    }

    return retCode;
}

