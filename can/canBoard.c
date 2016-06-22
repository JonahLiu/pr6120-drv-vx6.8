/* canBoard.c - implementation of CAN Board routines */

/* Copyright 2001 Wind River Systems, Inc. */

/* 
modification history 
--------------------
09nov01,dnb modified for integration into Tornado
12jul01,jac written

*/

/* 

DESCRIPTION
implementation of CAN Board routines

*/

/* includes */
#include <vxWorks.h>
#include <errnoLib.h>
#include <stdlib.h>
#include <string.h>
#include <CAN/wnCAN.h>
#include <CAN/canBoard.h>
#include <CAN/canController.h>
#include <CAN/canFixedLL.h>

/* *************************************************************************** */
/* ****  global variables                                                      */

/* fixed-node linked list instance for all boards */
FLL_RootType g_BoardLLRoot;


/************************************************************************
*
* WNCAN_Board_establishLinks - connect the function pointers in the
* board structure to the appropriate routines
*
*
* RETURNS: OK or ERROR
*   
* ERRNO: S_can_unknown_board
*
*/
STATUS WNCAN_Board_establishLinks
(
    struct WNCAN_Device *pDev,
    WNCAN_BoardType brdType
)
{
    STATUS retCode = ERROR;
    BoardLLNode *pBoardNode;

    pBoardNode = BOARDLL_GET(brdType);
    if (pBoardNode && pBoardNode->nodedata.boarddata.establinks_fn)
    {
        retCode = (*pBoardNode->nodedata.boarddata.establinks_fn)(pDev);
    }
    else
    {
        errnoSet(S_can_unknown_board);
        retCode = ERROR;
    }
    return retCode;
}

/************************************************************************
*
* WNCAN_Board_Open - attempt to open specified board
*
* RETURNS: point to device structure, or 0 on error
*   
* ERRNO: S_can_unknown_board
*
*/
struct WNCAN_Device *WNCAN_Board_Open
(
    UINT brdType,
    UINT brdNdx,
    UINT ctrlNdx
)
{
    struct WNCAN_Device *retDev = 0;
    BoardLLNode *pBoardNode;

    pBoardNode = BOARDLL_GET(brdType);
    if (pBoardNode && pBoardNode->nodedata.boarddata.open_fn)
    {
        retDev = (*pBoardNode->nodedata.boarddata.open_fn)(brdNdx, ctrlNdx);
    }
    else
    {
        errnoSet(S_can_unknown_board);
        retDev = 0;
    }

    return retDev;
}

/************************************************************************
*
* WNCAN_Board_Close - close specified CAN Device
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/
void WNCAN_Board_Close
(
struct WNCAN_Device *pDev
)
{
    WNCAN_BoardType brdType = pDev->pBrd->brdType;
    BoardLLNode *pBoardNode;

    pBoardNode = BOARDLL_GET(brdType);
    if (pBoardNode && pBoardNode->nodedata.boarddata.close_fn)
    {
        (*pBoardNode->nodedata.boarddata.close_fn)(pDev);
    }

    return;
}

/************************************************************************
*
* stringToUlong - helper function used by boards to 
*
* RETURNS: Ulong
*   
* ERRNO: N/A
*
*/
ULONG stringToUlong(const char *pStr)
{
    ULONG retVal = 0;
    UINT  base;
    
    if(pStr)
    {
        if(!strncmp(pStr, "0x", 2) || !strncmp(pStr, "0X", 2))
            base = 16;
        else
            base = 10;

        retVal = strtoul(pStr, NULL, base);
    }

    return retVal;

}
