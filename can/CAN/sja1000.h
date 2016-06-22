/* SJA1000.h - definitions for Phillips SJA1000 CAN Controller */

/* Copyright 2001 Wind River Systems, Inc. */

/* 
modification history 
--------------------
01a,06oct05,lsg  Rename macros that are common with
                 target/h/arch/mips/archMips
09nov01,dnb modified for integration into Tornado
12jul01,jac written

*/

/* 

DESCRIPTION
This file contains the functions declarations, specific to the 
Phillips SJA1000 CAN controller, that implement the interface defined 
in the windnet_can.h header file. 

*/

#ifndef SJA1000_H_
#define SJA1000_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* mode register bits */
#define MOD_RM                      0x01
#define MOD_LOM                     0x02
#define MOD_STM                     0x04
#define MOD_AFM                     0x08
#define MOD_SM                      0x10

/* command register bits */
#define CMR_TR                      0x01
#define CMR_AT                      0x02
#define CMR_RRB                     0x04
#define CMR_CDO                     0x08
#define CMR_SRR                     0x10

/* status register bits */
#define SJA1000_SR_RBS              0x01
#define SJA1000_SR_DOS              0x02
#define SJA1000_SR_TBS              0x04
#define SJA1000_SR_TCS              0x08
#define SJA1000_SR_RS               0x10
#define SJA1000_SR_TS               0x20
#define SJA1000_SR_ES               0x40
#define SJA1000_SR_BS               0x80

/* interrupt register bits */
#define IR_RI                       0x01
#define IR_TI                       0x02
#define IR_EI                       0x04
#define IR_DOI                      0x08
#define IR_WUI                      0x10
#define IR_EPI                      0x20
#define IR_ALI                      0x40
#define IR_BEI                      0x80

/* interrupt enable register bits */
#define IER_RIE                     0x01
#define IER_TIE                     0x02
#define IER_EIE                     0x04
#define IER_DOIE                    0x08
#define IER_WUIE                    0x10
#define IER_EPIE                    0x20
#define IER_ALIE                    0x40
#define IER_BEIE                    0x80

/* error code capture register */
#define ECC_SEG0                    0x01
#define ECC_SEG1                    0x02
#define ECC_SEG2                    0x04
#define ECC_SEG3                    0x08
#define ECC_SEG4                    0x10
#define ECC_DIR                     0x20
#define ECC_ERRC0                   0x40
#define ECC_ERCC1                   0x80

/* max number of message objects for sja1000 */
#define SJA1000_MAX_MSG_OBJ 2

/* sja1000 channel types */
#define TX_CHN_NUM 0
#define RX_CHN_NUM 1

extern const UINT g_sja1000chnType[SJA1000_MAX_MSG_OBJ];

struct TxMsg
{
    ULONG id;
    BOOL  ext;
    BOOL  rtr;
    UCHAR len;
    UCHAR data[8];
};

void sja1000_registration(void);
void sja1000ISR(ULONG context);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SJA1000_H_ */
