/* pr6120_can.h - definitions for PR6120 CAN board */

/* 
modification history 
--------------------
2016/05/27 Jonah Liu Created on base of PR6120_CAN.c

*/

/* 

DESCRIPTION
This file contains definitions used only in pr6120_can.
and pr6120_can_cfg.c 

*/
#ifndef PR6120_CAN_H
#define PR6120_CAN_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PR6120_CAN_MAX_CONTROLLERS (2)


struct PR6120_CAN_ChannelData
{
   WNCAN_ChannelMode sja1000chnMode[SJA1000_MAX_MSG_OBJ];
};


struct PR6120_CAN_DeviceEntry
{
    struct WNCAN_Device             canDevice[PR6120_CAN_MAX_CONTROLLERS];
    struct PR6120_CAN_ChannelData  chData[PR6120_CAN_MAX_CONTROLLERS];
    struct WNCAN_Controller         canControllerArray[PR6120_CAN_MAX_CONTROLLERS];
    struct WNCAN_Board              canBoard;
    struct TxMsg                    txMsg[PR6120_CAN_MAX_CONTROLLERS];
    int                             bus;
    int                             dev;
    int                             func;
    BOOL                            inUse;
    BOOL                            allocated[PR6120_CAN_MAX_CONTROLLERS];
    BOOL                            intConnect;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* PR6120_CAN_H */
