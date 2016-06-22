/* wnCAN_show.c - implementation of Windnet CAN Show Routines */

/* Copyright 2001 Wind River Systems, Inc. */

/* 
modification history 
--------------------
21dec01, dnb written

*/

/* 

DESCRIPTION
This file contains the functions that display information about installed
CAN drivers

*/

/* includes */
#include <vxWorks.h>
#include <stdio.h>
#include <CAN/wnCAN.h>
#include <CAN/canBoard.h>
#include <CAN/canController.h>
#include <CAN/canFixedLL.h>


/************************************************************************
*
* WNCAN_Show - display information about installed CAN devices
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/
void WNCAN_Show(void)
{
    BoardLLNode *pBoardNode;

    printf("Installed CAN boards:\n");


    /* call show routine for all installed board types */ 
    for(pBoardNode = BOARDLL_FIRST(); pBoardNode; pBoardNode = BOARDLL_NEXT(pBoardNode))
    {
        if (pBoardNode->nodedata.boarddata.show_fn)
	    (*pBoardNode->nodedata.boarddata.show_fn)();
    }

    return;
}
