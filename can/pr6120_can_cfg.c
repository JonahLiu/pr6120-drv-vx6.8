/* pr6120_can_cfg.c - configlette for PR6120 CAN boards */

/* 
modification history 
--------------------
2016/05/27 Jonah Liu Created on base of pr6120_can_cfg.c
*/

/* 

 DESCRIPTION
 This file contains the initialization functions for the  PR6120 CAN card.
 
*/

/* includes */
#include <vxWorks.h>

#include "CAN/wnCAN.h"
#include "CAN/canController.h"
#include "CAN/canBoard.h"
#include "CAN/canFixedLL.h"
#include "CAN/i82527.h"
#include "CAN/sja1000.h"
#include "CAN/private/pr6120_can.h"

#ifdef INCLUDE_WNCAN_DEVIO
#include "CAN/wncanDevIO.h"
#endif


/* extern references */
extern STATUS pr6120_can_establishLinks(struct WNCAN_Device *pDev);
extern STATUS pr6120_can_close(struct WNCAN_Device *pDev);
extern struct WNCAN_Device *pr6120_can_open(UINT brdNdx, UINT ctrlNdx);
extern void PR6120_CAN_InitAll();
extern void PR6120_CAN_IntConnectAll();

/* forward references */
#ifdef INCLUDE_WNCAN_SHOW
LOCAL void   pr6120_can_show(void);
#endif

#ifdef INCLUDE_WNCAN_DEVIO
LOCAL void   pr6120_can_devio_init(void);
#endif

/* reserve memory for the requisite data structures */
LOCAL struct PR6120_CAN_DeviceEntry
	pr6120_can_cfg_deviceArray[MAX_PR6120_CAN_BOARDS];

LOCAL BoardLLNode PR6120_CAN_LLNode;


/************************************************************************
*
* wncan_pr6120_can_init - 
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/ 
void wncan_pr6120_can_init(void)
{
    /* Add this board into linked list of boards */
    PR6120_CAN_LLNode.key = WNCAN_PR6120_CAN;
    PR6120_CAN_LLNode.nodedata.boarddata.establinks_fn = pr6120_can_establishLinks;
    PR6120_CAN_LLNode.nodedata.boarddata.close_fn      = pr6120_can_close;
    PR6120_CAN_LLNode.nodedata.boarddata.open_fn       = pr6120_can_open;
#ifdef INCLUDE_WNCAN_SHOW
    PR6120_CAN_LLNode.nodedata.boarddata.show_fn       = pr6120_can_show;
#else
    PR6120_CAN_LLNode.nodedata.boarddata.show_fn       = NULL;
#endif
    
    BOARDLL_ADD( &PR6120_CAN_LLNode );
    
    /* register the possible controllers for this board */
    sja1000_registration();
    
    /* init the board */
    PR6120_CAN_InitAll();
}


/************************************************************************
*
* wncan_pr6120_can_init2 - 
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/ 
void wncan_pr6120_can_init2(void)
{
    /* PR6120 CAN board including two sja1000 controllers (2) */
    PR6120_CAN_IntConnectAll();
    
#ifdef INCLUDE_WNCAN_DEVIO
    /* Initialize the DevIO interface */
    pr6120_can_devio_init();
#endif
}




/************************************************************************
*
* pr6120_can_MaxBrdNumGet -  get the number of PR6120 CAN boards
*
* RETURNS: the number of PR6120 CAN boards.
*   
* ERRNO: N/A
*
*/ 
UINT PR6120_CAN_MaxBrdNumGet(void)
{
    return(MAX_PR6120_CAN_BOARDS);
}


/************************************************************************
*
* pr6120_can_DevEntryGet -  get the specified PR6120 CAN device entry.
*
* This function returns a pointer to the device entry structure of the
* specified PR6120 CAN device. The board number argument is zero based;
* therefore, the first board has number zero.
* 
* RETURNS: the PR6120 CAN device entry structure.
*   
* ERRNO: N/A
*
*/ 
struct PR6120_CAN_DeviceEntry* PR6120_CAN_DeviceEntryGet(UINT brdNum)
{
    if (MAX_PR6120_CAN_BOARDS > 0 && 
        brdNum < MAX_PR6120_CAN_BOARDS)
        return(&pr6120_can_cfg_deviceArray[brdNum]);
    else
        return(NULL);
}


/************************************************************************
*
* pr6120_can_link - links API definitions in wncan_api.c
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/ 
#ifdef USE_CAN_FUNCTION_DEFS
void pr6120_can_link(void)
{
    BOOL flag = FALSE;
    struct WNCAN_Device *pDev;
    if (flag == TRUE)
    {
        pDev = CAN_Open(WNCAN_PR6120_CAN,0,0);
        CAN_Close(pDev);
    }
    return;
}
#endif


#ifdef INCLUDE_WNCAN_SHOW
/************************************************************************
*
* pr6120_can_show - display board to stdout
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/
LOCAL void pr6120_can_show(void)
{
    UINT i;
    struct PR6120_CAN_DeviceEntry *pDE;
    UINT maxBrdNum;
    
    maxBrdNum = PR6120_CAN_MaxBrdNumGet();
    
    if (maxBrdNum == 0)
        return;
    
    pDE = PR6120_CAN_DeviceEntryGet(0);
    
    printf("\n\tBoard Name: %s\n",
        pDE->canDevice[0].deviceName);
    
    for(i = 0 ; i < maxBrdNum; i++)
    {
        pDE = PR6120_CAN_DeviceEntryGet(i);
        if(pDE->inUse == TRUE)
        {
            printf("\t\tBoard Index: %d\n", i);
            printf("\t\tBase Memory Address: 0x%0x\n", 
                (UINT)pDE->canBoard.ioAddress);
            
            printf("\t\tBase Address 0: 0x%0x\n", 
                pDE->canBoard.bar0);
            /*
            printf("\t\tBase Address 1: 0x%0x\n", 
                pDE->canBoard.bar1);
            
            printf("\t\tBase Address 2: 0x%0x\n", 
                pDE->canBoard.bar2);
            */
            printf("\t\tInterrupt Number: 0x%0x\n",
                pDE->canBoard.irq);
        }
    }
    return;
}
#endif


#ifdef INCLUDE_WNCAN_DEVIO
/************************************************************************
*
* pr6120_can_devio_init - initialize the DevIO interface 
*
* RETURNS: N/A
*   
* ERRNO: N/A
*
*/
LOCAL void pr6120_can_devio_init(void)
{
    char*                  pCurBrdName = NULL;
    char*                  pLastBrdName = NULL;
    char*                  copyConstString;
    char                   sep[] = " \"";
    int                    boardidx, ctrlidx;
    STATUS                 status;
    WNCAN_DEVIO_DRVINFO*   wncDrv = NULL;
    
    /* 
    Read list of DevIO board names from component properties; 
    extract first name 
    Make a copy of the devio name parameter before operating on it.
    */
    
    copyConstString = (char *) malloc(strlen(PR6120_CAN_DEVIO_NAME) + 1);
    
    memcpy(copyConstString, PR6120_CAN_DEVIO_NAME, (strlen(PR6120_CAN_DEVIO_NAME) + 1));
    
    pCurBrdName = strtok_r(copyConstString, sep, &pLastBrdName);
    
    /* Parse name list and initialize each controller */
    
    for(boardidx=0; (pCurBrdName && (boardidx < MAX_PR6120_CAN_BOARDS)); boardidx++)
    {
        for(ctrlidx=0; ctrlidx < PR6120_CAN_MAX_CONTROLLERS; ctrlidx++)
        {
            /* Construct DevIO name for each controller, /boardName/ctlrNum */
            status = wncDevIODevCreate(pCurBrdName, 
                WNCAN_PR6120_CAN,
                boardidx, ctrlidx, &wncDrv);
            
            if( status == ERROR )
            {
                free(copyConstString);
                return;
            }
        }
        
        pCurBrdName = strtok_r(NULL, sep, &pLastBrdName);
    }
    
    free(copyConstString);
}
#endif

