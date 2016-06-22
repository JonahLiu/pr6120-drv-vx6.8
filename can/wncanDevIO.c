/* wncanDevIO.c - implementation of WIND NET CAN Device I/O Interface */

/* Copyright 2002-2003 Wind River Systems, Inc. */

/*
modification history
--------------------
01d,11may05,lsg   Fix in macro checks for malloc definition 
01c,21apr04,bjn   Added WNCAN_REG_SET and WNCAN_REG_GET.
01b,21feb03,rgp   finished
01a,19dec02,emcw  created

*/

/*
DESCRIPTION
This file contains the functions that implement the DevIO interface defined 
in the wncanDevIO.h header file.
*/

/* log message debugging */
#define DEVIO_DEBUG  0


/* includes */

#include <vxWorks.h>
#include <stdlib.h>
#include <errnoLib.h>
#include <string.h>
#include <stdio.h>
#include <CAN/wnCAN.h>
#include <CAN/wncanDevIO.h>
#include <intLib.h>

#ifndef _WRS_VXWORKS_5_X
#include <memLib.h>
#include <memPartLib.h>
#endif

#if DEVIO_DEBUG
#include <logLib.h>
#include <private/selectLibP.h>
#endif

/* global variables */

/* local variables */

LOCAL int wncDevIODrvNum = 0;     /* driver number assigned to this driver */
LOCAL int wncDevDrvInstance = 0;  /* # times wncDevIODevCreate() called */

/* local prototypes */
LOCAL int wncUtilCalcRingBufSize(int numCanMsgs);
LOCAL STATUS wncUtilIoctlFioCmds(WNCAN_DEVIO_FDINFO*,int,int);
LOCAL STATUS wncUtilIoctlDeviceFioCmds(WNCAN_DEVIO_FDINFO*,int,int);
LOCAL STATUS wncUtilIoctlChannelCmds(WNCAN_DEVIO_FDINFO*,int,int);
LOCAL STATUS wncUtilIoctlCtlrCmds(WNCAN_DEVIO_FDINFO*,int,int);
LOCAL STATUS wncUtilIoctlDeviceCmds(WNCAN_DEVIO_FDINFO*,int,int);
LOCAL void wncDevIOIsrHandler(struct WNCAN_Device*,WNCAN_IntType,UCHAR);

/* memory allocation for 5.5 and AE are different */
#ifdef _WRS_KERNEL 
#define WNCDEV_MALLOC(s)  malloc(s)
#define WNCDEV_FREE(s)    free(s)
#endif

#ifndef _WRS_KERNEL

#ifdef _WRS_VXWORKS_5_X
#define WNCDEV_MALLOC(s)  malloc(s)
#define WNCDEV_FREE(s)    free(s)
#else
#define WNCDEV_MALLOC(s)  KHEAP_ALIGNED_ALLOC(s, 4)
#define WNCDEV_FREE(s)    KHEAP_FREE(s)
#endif

#endif

/* define helper macros to access the device's info struct
** which we store within the WNC's device structure
**/
#define WNCDEV_PUT_DEVICEINFO(pDev, pInfo) \
(pDev)->userData = (void*)pInfo
#define WNCDEV_GET_DEVICEINFO(pDev) \
((WNCAN_DEVIO_FDINFO*)((pDev)->userData))
#define WNCDRV_PUT_DEVICEINFO(pDrv, pInfo) \
WNCDEV_PUT_DEVICEINFO(pDrv->wncDevice, pInfo)
#define WNCDRV_GET_DEVICEINFO(pDrv) \
WNCDEV_GET_DEVICEINFO(pDrv->wncDevice)


/************************************************************************
*
* wncDevIODrvInstall - install the DevIO interface driver 
*
* This routine installs the DevIO interface driver into the VxWorks I/O system.
*
* This routine should be called exactly once, before any reads, writes, or calls 
* to wncDevIODevCreate().  Normally, it is called by usrRoot() in usrConfig.c.
*
* RETURNS: OK or ERROR if the driver cannot be installed
*
* ERRNO: N/A
*
*/

STATUS wncDevIODrvInstall(void)
{
    /* Check whether driver is already installed */
    
    if (wncDevIODrvNum > 0)
    {
#if DEVIO_DEBUG
        logMsg("wncDevIODrvInstall() OK: DevIO driver already installed\n", 
            0,0,0,0,0,0);
#endif
        
        return OK;
    }
    
    /* 
    VxWorks startup code takes care of creation, and VxWorks shutdown
    code takes care of destruction.  Don't need creat() and delete()
    functions here.
    */
    
#if DEVIO_DEBUG
    wncDevIODrvNum = iosDrvInstall (wncDevIOCreate, wncDevIODelete, wncDevIOOpen, 
        wncDevIOClose, wncDevIOReadBuf, wncDevIOWriteBuf, 
        wncDevIOIoctl);
#else
    wncDevIODrvNum = iosDrvInstall ((FUNCPTR) NULL, (FUNCPTR) NULL, wncDevIOOpen, 
        wncDevIOClose, wncDevIOReadBuf, wncDevIOWriteBuf, 
        wncDevIOIoctl);
#endif
    
    return (wncDevIODrvNum == ERROR ? ERROR : OK);
}


/************************************************************************
*
* wncDevIODrvRemove - remove the DevIO interface driver 
*
* This routine removes the DevIO interface driver from the VxWorks I/O system.
*
* RETURNS: OK or ERROR 
*
* ERRNO: N/A
*
*/

STATUS wncDevIODrvRemove(void)
{
    STATUS  status;
    
    /* Check whether driver is already removed */
    
    if (wncDevIODrvNum == 0)
    {
#if DEVIO_DEBUG
        logMsg("wncDevIODrvRemove() OK: DevIO driver already removed\n", 
            0,0,0,0,0,0);
#endif
        
        return OK;
    }
    
    /* Check whether other instances of driver need to be removed */
    
    if (wncDevDrvInstance > 0)
    {
#if DEVIO_DEBUG
        logMsg("wncDevIODrvRemove() ERROR: Still have DevIO driver instances in use\n", 
            0,0,0,0,0,0);
#endif
        
        return ERROR;
    }
    
    /* Delete driver entry from VxWorks I/O system */
    
    status = iosDrvRemove (wncDevIODrvNum, TRUE);
    if (status == OK)
        wncDevIODrvNum = 0;
    
    return (status);
}


/************************************************************************
*
* wncDevIODevCreate - device creation routine
*
* This routine creates an instance of the DevIO driver for the specified
* device name and adds it to the VxWorks I/O System
*
* RETURNS: OK or ERROR
*
* ERRNO: S_ioLib_NO_DRIVER
*        S_can_invalid_parameter
*        S_can_out_of_memory        
*/

STATUS wncDevIODevCreate
(
 char                 *name,       /* device name */
 int                   boardtype,  /* WNC board type */
 int                   boardindex, /* board index */
 int                   ctlrindex,  /* controller number */
 WNCAN_DEVIO_DRVINFO **pwncDrv     /* pointer to driver descriptor */
 )
{
    STATUS      status = OK;
    char        ctlrName[WNCAN_LG_BUF_SIZE];
    char       *pCtlrName = ctlrName;
    
    /* Check for errors */
    
    if (wncDevIODrvNum <= 0)
    {
#if DEVIO_DEBUG
        logMsg("wncDevIODevCreate() ERROR: Invalid wncDevIODrvNum\n", 0,0,0,0,0,0);
#endif
        
        errnoSet (S_ioLib_NO_DRIVER);
        return ERROR;
    }
    
    if (name == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncDevIODevCreate() ERROR: device name not specified\n", 0,0,0,0,0,0);
#endif
        
        errnoSet (S_can_invalid_parameter);
        return ERROR;
    }
    
    
    /* convert to device name  "board/controller" */
    {
        int namelen = strlen(name)+2;  /* add trailing '/' and nul */
        int idx = ctlrindex;
        
        /* account for controller number (one for each digit) 
        ** (i.e. namelen += (log10(ctlrindex)+1) ) 
        */
        for(namelen +=1; idx>=10; idx /= 10, namelen++) ;
        
        if (namelen > WNCAN_LG_BUF_SIZE)
        {
            pCtlrName = (char*)WNCDEV_MALLOC(namelen+1);
            if (pCtlrName == NULL)
            {
                errnoSet (S_can_out_of_memory);
                status = ERROR;
            }
        }
    }
    
    if (status == OK)
    {    
        WNCAN_DEVIO_DRVINFO *wncDrv;
        
        /* create the device name */
        sprintf(pCtlrName, "%s/%d", name, ctlrindex);
        
        /* Initialize DevIO struct */
        
        wncDrv = (WNCAN_DEVIO_DRVINFO *) WNCDEV_MALLOC(sizeof(WNCAN_DEVIO_DRVINFO));
        if (wncDrv == NULL)
        {
            errnoSet (S_can_out_of_memory);
            status = ERROR;
        } 
        else
        {
            wncDrv->boardType    = boardtype;
            wncDrv->boardIdx     = boardindex;
            wncDrv->ctlrIdx      = ctlrindex;
            wncDrv->numOpenChans = 0; 
            wncDrv->isDeviceOpen = FALSE;
            wncDrv->wncDevice    = NULL;
            wncDrv->ctrlSetConfig = NULL;
            wncDrv->ctrlGetConfig = NULL;
            
            /* create device mutex */
            wncDrv->mutex        = semBCreate(SEM_Q_PRIORITY, SEM_FULL);
            if (wncDrv->mutex == NULL)
            {
#if DEVIO_DEBUG
                logMsg("wncDevIODevCreate() ERROR: failed to create semaphore\n", 
                    0,0,0,0,0,0);
#endif
                
                status = ERROR;
            }
            else
            {
                /* Add device to VxWorks I/O System and associate with DevIO driver */
                status = iosDevAdd (&(wncDrv->devHdr), pCtlrName, wncDevIODrvNum);
            }
            
            if (status == OK)
            {
                wncDevDrvInstance++;
                if (pwncDrv)           /* pass back to use the dev pointer */
                    *pwncDrv = wncDrv;
            }
            else
            {  /* error, clean up allocated memory */
                WNCDEV_FREE((char*)wncDrv);
            }
        }
    }
    
    /* clean-up locally allocated memory, if needed */
    if (pCtlrName != ctlrName) 
        WNCDEV_FREE((char*)pCtlrName);
    
    return status;
}


/************************************************************************
*
* wncDevIODevDestroy - device destroy routine
*
* This routine destroys an instance of the DevIO driver for the specified
* driver descriptor and removes it from the VxWorks I/O System.  The user
* must ensure that any open channels and the CAN device must be closed before
* calling this routine.
*
* RETURNS: OK or ERROR
*
* ERRNO: S_ioLib_NO_DRIVER, S_ioLib_DEVICE_ERROR, S_iosLib_DEVICE_NOT_FOUND
*
*/

STATUS wncDevIODevDestroy
(
 WNCAN_DEVIO_DRVINFO*  wncDrv  /* pointer to driver descriptor */
 )
{
    /* Check for installed DevIO driver */
    
    if (wncDevIODrvNum <= 0)
    {
#if DEVIO_DEBUG
        logMsg("wncDevIODevDestroy() ERROR: Invalid wncDevIODrvNum\n", 0,0,0,0,0,0);
#endif
        
        errnoSet (S_ioLib_NO_DRIVER);
        return ERROR;
    }
    
    /* Check if at least one instance of the driver is installed */
    
    if (wncDevDrvInstance == 0)
    {
        errnoSet (S_iosLib_DEVICE_NOT_FOUND);
        return ERROR;
    }
    
    /* Ensure that all CAN channels are closed */
    
    if (wncDrv->numOpenChans > 0)
    {
#if DEVIO_DEBUG
        logMsg("wncDevIODevDestroy() ERROR: CAN device still has open channels\n", 
            0,0,0,0,0,0);
#endif
        
        errnoSet (S_ioLib_DEVICE_ERROR);
        return ERROR;
    }
    
    /* Ensure that the CAN device is closed */
    
    if (wncDrv->isDeviceOpen)
    {
#if DEVIO_DEBUG
        logMsg("wncDevIODevDestroy() ERROR: CAN device still open\n", 
            0,0,0,0,0,0);
#endif
        
        errnoSet (S_ioLib_DEVICE_ERROR);
        return ERROR;
    }
    
    /* Delete DevIO driver from I/O System */
    
    iosDevDelete (&wncDrv->devHdr);
    
    /* Free DevIO driver struct resources */
    
    wncDrv->boardType = 0;
    wncDrv->boardIdx = 0;
    wncDrv->ctlrIdx = 0;
    semDelete(wncDrv->mutex);
    wncDrv->wncDevice = NULL;
    
    WNCDEV_FREE((char*)wncDrv);
    wncDrv = NULL;
    
    wncDevDrvInstance--;
    
    return OK;
}


#if DEVIO_DEBUG
/************************************************************************
*
* wncDevIOCreate - device create routine
*
* This routine is only for debugging purposes to test the VxWorks I/O System
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
*/

int wncDevIOCreate
(
 WNCAN_DEVIO_DRVINFO*  wncDev,  /* pointer to device descriptor */
 char*                 name,    /* device or channel name */
 int                   mode     /* not used */
 )
{
    /* Debugging only */
    logMsg("wncDevIOCreate() called\n", 0,0,0,0,0,0);
    return OK;
}


/************************************************************************
*
* wncDevIODelete - device delete routine
*
* This routine is only for debugging purposes to test the VxWorks I/O System
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
*/

int wncDevIODelete
(
 WNCAN_DEVIO_DRVINFO*  wncDev,  /* pointer to device descriptor */
 char*                 name     /* device or channel name */
 )
{
    /* Debugging only */
    logMsg("wncDevIODelete() called\n", 0,0,0,0,0,0);
    return OK;
}
#endif    /* DEVIO_DEBUG */


/************************************************************************
*
* wncDevIOOpen - device open routine
*
* This routine opens a CAN device or CAN channel for access using the VxWorks
* I/O System via the returned file descriptor
*
* RETURNS: identifier of opened CAN device or channel
*
* ERRNO: ELOOP
*
*/

int wncDevIOOpen
(
 WNCAN_DEVIO_DRVINFO*  wncDrv,  /* pointer to driver descriptor */
 char*                 name,    /* device or channel name */
 int                   flags,   /* device or channel access */
 int                   mode     /* not used */
 )
{
    WNCAN_DEVIO_FDINFO*   fdInfo = NULL;
    STATUS                status = ERROR;    /* pessimistic */
    
                                             /* 
                                             How to determine if opening CAN device or channel: 
                                             -- device is named /boardName/controller#
                                             -- channel is named /boardName/controller#/channel#
                                             -- User's call to open() results in I/O System searching its internal device
                                             list using that entire name or an initial substring of that name
                                             -- The matching substring is removed and the remaining part of the original
                                             name argument is passed to THIS function
                                             -- For a CAN device, the name argument to this function is a null string.  
                                             For a CAN channel, the name argument to this function is "/channel#".
    */
    
    /* lock device */
    semTake(wncDrv->mutex, WAIT_FOREVER);
    
    /* Open CAN device */
    
    if ((name == NULL) || (!*name))
    {
        WNCAN_DEVICE   *canDev;
        int             bufSize;
        
        /* Check flags argument */
        
        if (flags != O_RDWR)
        {
#if DEVIO_DEBUG
            logMsg("wncDevIOOpen() ERROR: Invalid flags argument for CAN device \n", 
                0,0,0,0,0,0);
#endif
            
            goto ErrorExit;
        }
        
        /* Call WNCAN API to open CAN device */
        
        canDev = CAN_Open (wncDrv->boardType, wncDrv->boardIdx, wncDrv->ctlrIdx);
        
        status = CAN_Init (canDev);
        if (status == ERROR)
            goto ErrorExit;
        
        /* install interrupt handler */
        CAN_InstallISRCallback(canDev, wncDevIOIsrHandler);
        
        /* allow all interrupts */
        CAN_SetIntMask(canDev, WNCAN_INT_ALL);
        CAN_EnableInt(canDev);
        
        /* update devIO info */
        wncDrv->isDeviceOpen = TRUE;
        wncDrv->wncDevice = canDev;
        
        /* Allocate and initialize DevIO file descriptor structure */
        
        fdInfo = (WNCAN_DEVIO_FDINFO *) WNCDEV_MALLOC(sizeof(WNCAN_DEVIO_FDINFO));
        if (fdInfo == NULL)
        {
#if DEVIO_DEBUG
            logMsg("wncDevIOOpen() ERROR: malloc() failed for "
                "WNCAN_DEVIO_FDINFO struct\n", 0,0,0,0,0,0);
#endif
            
            goto ErrorExit;
        }
        
        fdInfo->wnDevIODrv = wncDrv;
        fdInfo->devType = FD_WNCAN_DEVICE;
        
        /* initialize select's wakeup list */            
        selWakeupListInit(&fdInfo->selWakeupList);
        
        /* allocate array of infos for each channel */
        bufSize = sizeof(WNCAN_DEVIO_FDINFO*)*CAN_GetNumChannels(canDev);
        fdInfo->fdtype.device.chnInfo = (WNCAN_DEVIO_FDINFO**)WNCDEV_MALLOC(bufSize);
        if (fdInfo->fdtype.device.chnInfo == NULL)
        {
#if DEVIO_DEBUG
            logMsg("wncDevIOOpen() ERROR: malloc() failed for "
                "WNCAN_DEVIO_FDINFO* array\n", 0,0,0,0,0,0);
#endif
            
            goto ErrorExit;
        }
        
        /* store into can dev pointer */
        WNCDRV_PUT_DEVICEINFO(wncDrv, fdInfo);
    }
    
    /* Open CAN channel */
    
    else
    {
        WNCAN_DEVIO_FDINFO  *pDevInfo = NULL;
        int                  ringBufSize = 0;
        
        /* Check flags argument */
        
        if ( !((flags == O_RDWR) || (flags == O_RDONLY) || (flags == O_WRONLY)) )
        {
#if DEVIO_DEBUG
            logMsg("wncDevIOOpen() ERROR: Invalid flags argument for CAN channel\n", 
                0,0,0,0,0,0);
#endif
            
            goto ErrorExit;
        }
        
        /* Ensure that CAN device is open */
        
        if (wncDrv->isDeviceOpen == FALSE)
        {
#if DEVIO_DEBUG
            logMsg("wncDevIOOpen() ERROR: CAN device not open\n", 0,0,0,0,0,0);
#endif
            
            goto ErrorExit;
        }
        
        /* Allocate and initialize DevIO file descriptor structure */
        
        fdInfo = (WNCAN_DEVIO_FDINFO *) WNCDEV_MALLOC(sizeof(WNCAN_DEVIO_FDINFO));
        if (fdInfo == NULL)
        {
#if DEVIO_DEBUG
            logMsg("wncDevIOOpen() ERROR: malloc() failed for "
                "WNCAN_DEVIO_FDINFO struct\n", 0,0,0,0,0,0);
#endif
            
            goto ErrorExit;
        }
        
        fdInfo->wnDevIODrv = wncDrv;
        fdInfo->devType = FD_WNCAN_CHANNEL;
        
        /* initialize select's wakeup list */            
        selWakeupListInit(&fdInfo->selWakeupList);
        
        /* Create input and output data buffers as indicated by flags argument */
        
        if ( (flags == O_RDWR) || (flags == O_RDONLY) )
        {
            /* Create input ring buffer */
            
            ringBufSize = wncUtilCalcRingBufSize (WNCAN_DEFAULT_RINGBUF_SIZE);
            fdInfo->fdtype.channel.inputBuf = rngCreate (ringBufSize);
            if (fdInfo->fdtype.channel.inputBuf == NULL)
            {
#if DEVIO_DEBUG
                logMsg("wncDevIOOpen() ERROR: Could not create input ring buffer\n", 
                    0,0,0,0,0,0);
#endif
                
                goto ErrorExit;
            }
            fdInfo->fdtype.channel.intType = WNCAN_INT_RX;
        }
        else
            fdInfo->fdtype.channel.inputBuf = NULL;
        
        if ( (flags == O_RDWR) || (flags == O_WRONLY) )
        {
            /* Create output ring buffer */
            
            ringBufSize = wncUtilCalcRingBufSize (WNCAN_DEFAULT_RINGBUF_SIZE);
            fdInfo->fdtype.channel.outputBuf = rngCreate (ringBufSize);
            if (fdInfo->fdtype.channel.outputBuf == NULL)
            {
#if DEVIO_DEBUG
                logMsg("wncDevIOOpen() ERROR: Could not create output ring buffer\n", 
                    0,0,0,0,0,0);
#endif
                
                    /*
                    * If creation of outputBuf failed but inputBuf succeeded,
                    * delete inputBuf before proceeding
                */
                if((flags == O_RDWR) && (fdInfo->fdtype.channel.inputBuf))
                    rngDelete (fdInfo->fdtype.channel.inputBuf);
                
                goto ErrorExit;
            }
            fdInfo->fdtype.channel.intType = WNCAN_INT_TX;
        }
        else
            fdInfo->fdtype.channel.outputBuf = NULL;
        
        /* Finish initializing DevIO file descriptor struct */
        fdInfo->fdtype.channel.enabled = TRUE;
        fdInfo->fdtype.channel.flag = flags;
        /* skip leading slash */
        fdInfo->fdtype.channel.channel = (UINT32) stringToUlong(&name[1]);
        
        /* store this channel's info into the device's info */
        pDevInfo = WNCDRV_GET_DEVICEINFO(wncDrv);
        pDevInfo->fdtype.device.chnInfo[fdInfo->fdtype.channel.channel] = fdInfo; 
        
        /* Indicate in DevIO driver struct that a channel was opened */
        wncDrv->numOpenChans++;
    }
    
    /* unlock device */
    semGive(wncDrv->mutex);
    
    /* 
    Return pointer to file descriptor struct so that the I/O system can save 
    and automatically use as argument for all subsequent calls to close(), 
    read(), write(), and ioctl() functions
    */
    
    return (int) fdInfo;
    
    /* Set errno used by VxWorks for failed open() */
    
ErrorExit:
    errnoSet (ELOOP);
    
    /* de-allocate memory, if needed */
    if (fdInfo)
        WNCDEV_FREE((char*)fdInfo);
    
    /* unlock device */
    semGive(wncDrv->mutex);
    
    return ERROR;
}


/************************************************************************
*
* wncDevIOClose - device close routine
*
* This routine closes the specified CAN device or CAN channel.  In order to
* close a device, all channels on that device must be previously closed.
*
* RETURNS: OK or ERROR
*
* ERRNO: S_ioLib_DEVICE_ERROR
*
*/

STATUS wncDevIOClose
(
 WNCAN_DEVIO_FDINFO*  fdInfo  /* pointer to DevIO file descriptor */
 )
{
    WNCAN_DEVIO_DRVINFO*  wncDrv = NULL;
    WNCAN_DEVICE*         canDev = NULL;
    STATUS                status = ERROR;
    
    if (fdInfo == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncDevIOClose() ERROR: Null DevIO file descriptor\n", 
            0,0,0,0,0,0);
#endif
        return status;
    }
    
    wncDrv = fdInfo->wnDevIODrv;
    if (wncDrv == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncDevIOClose() ERROR: Null DevIO driver pointer\n", 
            0,0,0,0,0,0);
#endif
        return status;
    }
    
    canDev = wncDrv->wncDevice;
    if (canDev == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncDevIOClose() ERROR: Null device pointer\n", 
            0,0,0,0,0,0);
#endif      
        return status;
    }
    
    /* lock device */
    semTake(wncDrv->mutex, WAIT_FOREVER);
    
    /* Close CAN channel */
    if (fdInfo->devType == FD_WNCAN_CHANNEL)
    {
        WNCAN_DEVIO_FDINFO  *pDevInfo = NULL;
        
        /* Call WNCAN API functions to close channel */
        
        status = CAN_DisableChannel (canDev, fdInfo->fdtype.channel.channel);
        if (status == OK)
        {
            status=CAN_FreeChannel (canDev, fdInfo->fdtype.channel.channel);
            if (status == OK)
            {
                /* Delete ring buffers */
                
                if (fdInfo->fdtype.channel.inputBuf)
                    rngDelete (fdInfo->fdtype.channel.inputBuf);
                fdInfo->fdtype.channel.inputBuf = 0;
                if (fdInfo->fdtype.channel.outputBuf)
                    rngDelete (fdInfo->fdtype.channel.outputBuf);
                fdInfo->fdtype.channel.outputBuf = 0;
                
                
                /* remove channel's info from the device's info */
                pDevInfo = WNCDRV_GET_DEVICEINFO(wncDrv);
                pDevInfo->fdtype.device.chnInfo[fdInfo->fdtype.channel.channel] =NULL; 
                
                
                /* Cleanup DevIO file descriptor struct */
                fdInfo->wnDevIODrv = NULL;
                fdInfo->devType = FD_WNCAN_NONE;
                fdInfo->fdtype.channel.enabled = FALSE;
                fdInfo->fdtype.channel.flag = 0;
                fdInfo->fdtype.channel.channel = 0;
                
                
                /* release wake up list */
                selWakeupListTerm(&fdInfo->selWakeupList);
                
                /* Free DevIO file descriptor struct */
                WNCDEV_FREE((char*)fdInfo);
                fdInfo = NULL;
                
                /* Decrement open channel counter */
                wncDrv->numOpenChans--;
            }
#if DEVIO_DEBUG
            else
            {
                logMsg("wncDevIOClose() ERROR: channel free failed on channel num %d\n", 
                    fdInfo->fdtype.channel.channel,0,0,0,0,0);
            }
#endif      
        }
#if DEVIO_DEBUG
        else
        {
            logMsg("wncDevIOClose() ERROR: channel disable failed on channel num %d\n", 
                fdInfo->fdtype.channel.channel,0,0,0,0,0);
        }
#endif      
        
    }
    /* Close CAN device */
    else if (fdInfo->devType == FD_WNCAN_DEVICE)
    {
        /* Ensure that user has closed any open channels */
        
        if (wncDrv->numOpenChans > 0)
        {
#if DEVIO_DEBUG
            logMsg("wncDevIOClose() ERROR: CAN device still has open channels\n", 
                0,0,0,0,0,0);
#endif
            
            errnoSet (S_ioLib_DEVICE_ERROR);
            status = ERROR;
        } 
        else
        {
            /* Call WNCAN API functions for closing device */
            CAN_TxAbort (canDev);
            CAN_Stop (canDev);
            CAN_DisableInt (canDev);
            
            /* Close the device */
            CAN_Close (canDev);
            
            /* free internal struct memory */
            WNCDEV_FREE((char*)fdInfo->fdtype.device.chnInfo);
            
            /* release wake up list */
            selWakeupListTerm(&fdInfo->selWakeupList);
            
            /* Free DevIO file descriptor struct */
            WNCDEV_FREE((char*)fdInfo);
            fdInfo = NULL;
            
            wncDrv->isDeviceOpen = FALSE;
            status = OK;
        }
    }
    /* Error case; Should not ever get here */
    else
    {
#if DEVIO_DEBUG
        logMsg("wncDevIOClose() ERROR: Invalid DevIO file descriptor type\n", 
            0,0,0,0,0,0);
#endif
        
        errnoSet (S_ioLib_DEVICE_ERROR);
        status = ERROR;
    }
    
    /* unlock device */
    semGive(wncDrv->mutex);
    
    return status;
}


/************************************************************************
*
* wncDevIOReadBuf - device read routine
*
* This routine is called by the VxWorks I/O System when user code calls read().
* It dequeues the next available CAN data message from the internal input data 
* ring buffer and returns it to the caller.  A separate function responds to the
* actual receive interrupt from the CAN controller and queues the CAN data message
* to the input data buffer.
*
* RETURNS: number of bytes read, or ERROR
*
* ERRNO: S_ioLib_DEVICE_ERROR
*
*/

int wncDevIOReadBuf
(
 WNCAN_DEVIO_FDINFO  *fdInfo,  /* pointer to DevIO file descriptor */
 char                *buffer,  /* pointer to WNCAN_CHNMSG buffer receiving bytes*/
 size_t               maxbytes /* max number of bytes to read */
 )
{
    int  key;
    int  bytesRead = 0;
    int  msgSize = sizeof(WNCAN_CHNMSG);
    
    if ( (fdInfo == NULL) || (buffer == NULL) )
    {       
#if DEVIO_DEBUG
        logMsg("wncDevIOReadBuf() ERROR: Null parameters passed in \n", 
            0,0,0,0,0,0);
#endif
        goto ErrorExit;
    }
    
    if (maxbytes < msgSize)
    {
#if DEVIO_DEBUG
        logMsg("wncDevIOReadBuf() ERROR: Incomplete CAN message data requested "
            "(maxbytes too small)\n", 0,0,0,0,0,0);
#endif
        
        goto ErrorExit;
    }
    
    /* ----------------- critical section to Read complete CAN message */
    key = intLock();
    bytesRead = rngBufGet (fdInfo->fdtype.channel.inputBuf, buffer, msgSize);
    intUnlock(key);
    /* ----------------- */
    
    if (bytesRead < 1)
    {
        /* Input data buffer is empty; Not an error */
        
#if DEVIO_DEBUG
        logMsg("wncDevIOReadBuf() INFO: Input data buffer is empty\n", 
            0,0,0,0,0,0);
#endif
    } else if (bytesRead < msgSize)
    {
        /* Incomplete CAN message received */
        
#if DEVIO_DEBUG
        logMsg("wncDevIOReadBuf() ERROR: Incomplete CAN message received\n", 
            0,0,0,0,0,0);
#endif
        
        goto ErrorExit;
    }
    
    return bytesRead;
    
ErrorExit:
    errnoSet (S_ioLib_DEVICE_ERROR);
    return ERROR;
}




/************************************************************************
*
* wncDevIOWriteBuf - device write routine 
*
* This routine is called by the VxWorks I/O System when user code calls write().
* It queues the CAN data message in "buffer" to the internal output data ring 
* buffer.  A separate function responds to the actual transmit interrupt from 
* the CAN controller to send the CAN data message from the output data buffer.
*
* RETURNS: number of bytes written, or ERROR
*
* ERRNO: S_ioLib_DEVICE_ERROR
*        S_can_invalid_parameter
*        S_can_buffer_overflow
* 
*/
int wncDevIOWriteBuf
(
 WNCAN_DEVIO_FDINFO   *fdInfo,     /* pointer to DevIO file descriptor */
 char                 *buffer,     /* pointer to WNCAN_CHNMSG buffer to write */
 size_t                maxbytes    /* number of bytes to write */
 )
{
    int  bytesWritten = ERROR;
    int  msgSize = sizeof(WNCAN_CHNMSG);
    int key;
    
    
    
    
    if ( (fdInfo == NULL) || (buffer == NULL) || (maxbytes < msgSize) )
    {
#if DEVIO_DEBUG
        logMsg("wncDevIOWriteBuf() ERROR: Incomplete CAN message data to be "
            "written\n", 0,0,0,0,0,0);
#endif
        
        errnoSet (S_can_invalid_parameter);
    }
    else
    {       
        
        BOOL ringEmptyBeforeAdd;
        
        /* ----------------- critical section to test and add to the queue */
        
        key = intLock();
        
        ringEmptyBeforeAdd = rngIsEmpty(fdInfo->fdtype.channel.outputBuf);
        bytesWritten = rngBufPut (fdInfo->fdtype.channel.outputBuf, buffer, msgSize);
        
        intUnlock(key);
        
        /* ----------------- */
        
        if (bytesWritten < msgSize)
        {
            /* Output data buffer is full */
            
#if DEVIO_DEBUG
            logMsg("wncDevIOWriteBuf() ERROR: Output data buffer is full; write() "
                "failed\n", 0,0,0,0,0,0);
#endif
            
                /* note: since the size of the message is a multiple of the total
                ** buffer size, if the buffer is full, then no portion of the message
                ** was put into the buffer (i.e. bytesWritten=0 when trying to add
                ** to a full buffer).
            */
            errnoSet (S_can_buffer_overflow);
            bytesWritten = ERROR;
        }
        
        /* if there was nothing in the buffer before adding this message,
        ** need to 'jump start' the TX process
        */
        if ((bytesWritten != ERROR) && (ringEmptyBeforeAdd))
        {
            /* initiate a TX by calling our handler */
            key = intLock();
            wncDevIOIsrHandler(fdInfo->wnDevIODrv->wncDevice, 
                WNCAN_INT_TXCLR, 
                fdInfo->fdtype.channel.channel);
            intUnlock(key);
        }
    }
    
    return bytesWritten;
}




/************************************************************************
*
* wncDevIOIsrHandler - WNC interrupt callback
*
* Service the WNC callback for TX,RX and error interrupts. 
*
* RETURNS: nothing
*
* ERRNO: S_can_buffer_overflow
*
*/
LOCAL void wncDevIOIsrHandler
(
 struct WNCAN_Device *pDev,
 WNCAN_IntType intStatus,
 UCHAR chnNum
 )
{
    WNCAN_DEVIO_FDINFO  *pDevInfo = WNCDEV_GET_DEVICEINFO(pDev);
    WNCAN_DEVIO_FDINFO  *pChnInfo = pDevInfo->fdtype.device.chnInfo[chnNum];
    WNCAN_CHNMSG         canMsg;
    STATUS               status;
    
    int     numBytes = 0;
    int     msgSize = sizeof(WNCAN_CHNMSG);
    BOOL    newdata;  /* unused, but needed for the api call */    
    
    switch(intStatus)
    {
    case WNCAN_INT_ERROR:
    case WNCAN_INT_BUS_OFF:
    case WNCAN_INT_WAKE_UP:
    /* error or bus type of interrupt, wake up the device in case
    ** application is blocking on the device's file descriptor
        */  
        selWakeupAll (&pDevInfo->selWakeupList, SELREAD);
        break;
        
    case WNCAN_INT_TX:
    /* when TX interrupt detected, free the blocked tasks. Note that
    ** we do this here instead of TXCLR because of the initial state.
    ** The initial state is when user writes into an empty buffer and
    ** there is no interrupt pending. Thus, the write will directly
    ** call this routine simulating the TXCLR interrupt. Thus, the TXCLR
    ** is called first and thus a message is transmitted. Then, when
    ** the interrupt is acknowledge, we unblock the task because now
    ** we are sure that there is enough space in the buffer for a new
    ** message.
        */
        selWakeupAll (&pChnInfo->selWakeupList, SELWRITE);
        break;
        
    case WNCAN_INT_RX:
    case WNCAN_INT_RTR_RESPONSE:
        /* get the message from the controller */
        canMsg.id = CAN_ReadID(pDev, chnNum, &canMsg.extId);  /* get ID, extId */
        /* read in the message, indicate full size (8) data buffer len */
        canMsg.len = WNCAN_MAX_DATA_LEN;
        CAN_ReadData(pDev, chnNum, canMsg.data, &canMsg.len, &newdata);
        /* do a test for RTR because the api can return an error, and if no, then
        ** the message is definately does not have RTR set
        */
        canMsg.rtr = (CAN_IsRTR(pDev, chnNum) == TRUE ? TRUE : FALSE);
        
        /* add into our buffer */
        if (rngBufPut(pChnInfo->fdtype.channel.inputBuf, 
            (char*)&canMsg, msgSize) == msgSize)
        {
            /* wake up blocked tasks */
            selWakeupAll (&pChnInfo->selWakeupList, SELREAD);
        }
        else 
        {
        /* internal buffer full, report error 
        ** NOTE: it is not advised to use logmsg here because if the controller
        ** is getting flooded, the logging of the error message will
        ** just make the overflow condition worse.
            */
#if DEVIO_DEBUG
            logMsg("wncDevIOIsrHandler() ERROR: Internal buffer full \n",0,0,0,0,0,0);
#endif
            errnoSet (S_can_buffer_overflow);
        }
        break;
        
        
    case WNCAN_INT_TXCLR:
        /* device ready to TX again if there is something in the buffer */
        numBytes = rngBufGet(pChnInfo->fdtype.channel.outputBuf, 
            (char*)&canMsg, msgSize);
        if (numBytes == msgSize)
        {
            /* message is buffer, transmit it */
            status = CAN_TxMsg(pDev, chnNum, canMsg.id, canMsg.extId, 
                canMsg.data, canMsg.len);
            
            if (status == ERROR)
            {  /* error in TX, just re-added back to the queue */
                rngBufPut(pChnInfo->fdtype.channel.outputBuf, (char*)&canMsg, msgSize);
            }
        }
        /* else nothing in the buffer, so do nothing */
        
        break;
        
    default:
        break;
    }
    
}



/************************************************************************
*
* wncDevIOIoctl - device I/O control routine
*
* This routine is called by the VxWorks I/O System when user code calls ioctl().
* It calls the appropriate WNCAN API functions to process the specified command.
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
*/

STATUS wncDevIOIoctl
(
 WNCAN_DEVIO_FDINFO   *fdInfo,      /* pointer to DevIO file descriptor */
 int                   command,     /* command function code */
 int                   arg          /* arbitrary argument */
 )
{
    WNCAN_DEVIO_DRVINFO*  wncDrv = NULL;
    WNCAN_DEVICE*         canDev = NULL;
    STATUS                status = ERROR;    /* pessimistic */
    
    if (fdInfo == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncDevIOIoctl() ERROR: pointer to DevIO file descriptor is null\n", 
            0,0,0,0,0,0);
#endif
        return ERROR;
    }
    
    wncDrv = fdInfo->wnDevIODrv;
    if (wncDrv == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncDevIOIoctl() ERROR: pointer to DevIO driver is null\n", 
            0,0,0,0,0,0);
#endif
        return ERROR;
    }
    
    canDev = wncDrv->wncDevice;
    if (canDev == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncDevIOIoctl() ERROR: pointer to DevIO device is null\n", 
            0,0,0,0,0,0);
#endif
        return ERROR;
    }
    
    switch (command)
    {
        /* Internal input and output data buffer commands */
    case FIOSELECT:
    case FIOUNSELECT:
        if (fdInfo->devType == FD_WNCAN_CHANNEL)
            status = wncUtilIoctlFioCmds(fdInfo, command, arg);
        else
            status = wncUtilIoctlDeviceFioCmds(fdInfo, command, arg);
        break;
        
        /* assumes the following only apply to channels */
    case FIONFREE:
    case FIONREAD:
    case FIONWRITE:
    case FIOFLUSH:
    case FIORFLUSH:
    case FIOWFLUSH:
    case FIORBUFSET:
    case FIOWBUFSET:
        status = wncUtilIoctlFioCmds (fdInfo, command, arg);
        break;
        
        
        /* CAN device commands */
    case WNCAN_HALT:
    case WNCAN_SLEEP:
    case WNCAN_WAKE:
    case WNCAN_TX_ABORT:
    case WNCAN_RXCHAN_GET:
    case WNCAN_TXCHAN_GET:
    case WNCAN_RTRREQCHAN_GET:
    case WNCAN_RTRRESPCHAN_GET:
    case WNCAN_BUSINFO_GET:
    case WNCAN_CONFIG_SET:
    case WNCAN_CONFIG_GET:
    case WNCAN_REG_SET:
    case WNCAN_REG_GET:
        status = wncUtilIoctlDeviceCmds (fdInfo, command, arg);
        break;
        
        /* CAN channel commands */
    case WNCAN_CHNCONFIG_SET:
    case WNCAN_CHNCONFIG_GET:
    case WNCAN_CHN_ENABLE:
    case WNCAN_CHN_TX:
    case WNCAN_CHNMSGLOST_GET:
    case WNCAN_CHNMSGLOST_CLEAR:
        status = wncUtilIoctlChannelCmds (fdInfo, command, arg);
        break;
        
        /* Controller-specific commands */
    case WNCAN_CTLRCONFIG_SET:
    case WNCAN_CTLRCONFIG_GET:
        status = wncUtilIoctlCtlrCmds (fdInfo, command, arg);
        break;
    }
    
    return status;
    
}    

/************************************************************************
*
* wncUtilIoctlFioCmds - utility routine to process FIO* ioctl() commands
* for a channel.
*
* This routine performs the processing for FIO* (internal input and
* output data buffer) commands specified via ioctl()
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
*/

LOCAL STATUS wncUtilIoctlFioCmds
(
 WNCAN_DEVIO_FDINFO   *fdInfo,      /* pointer to DevIO file descriptor */
 int                   command,     /* command function code */
 int                   arg          /* arbitrary argument */
 )
{
    WNCAN_DEVIO_DRVINFO*  wncDrv = NULL;
    WNCAN_DEVICE*         canDev = NULL;
    int*                  numBytes = NULL;
    int                   bufSize = 0;
    int                   key;
    STATUS                retCode=OK;
    
    if (fdInfo == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncUtilIoctlFioCmds() ERROR: pointer to DevIO file descriptor is null\n", 
            0,0,0,0,0,0);
#endif
        return ERROR;
    }
    
    wncDrv = fdInfo->wnDevIODrv;
    if (wncDrv == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncUtilIoctlFioCmds() ERROR: pointer to DevIO driver is null\n", 
            0,0,0,0,0,0);
#endif
        return ERROR;
    }
    
    canDev = wncDrv->wncDevice;
    if (canDev == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncUtilIoctlFioCmds() ERROR: pointer to DevIO device is null\n", 
            0,0,0,0,0,0);
#endif
        return ERROR;
    }
    
    /* lock device */
    semTake(wncDrv->mutex, WAIT_FOREVER);
    
    /* Process the specified command */
    
    switch (command)
    {
        /* Internal input and output data buffer commands */
    case FIOSELECT:
        selNodeAdd (&fdInfo->selWakeupList, (SEL_WAKEUP_NODE *) arg); 
        
        key = intLock();
        if ((selWakeupType ((SEL_WAKEUP_NODE *) arg) == SELREAD) &&
            (!rngIsEmpty(fdInfo->fdtype.channel.inputBuf)))
        { 
            /* data available, make sure task does not pend */ 
            selWakeup ((SEL_WAKEUP_NODE *) arg); 
        } 
        if ((selWakeupType ((SEL_WAKEUP_NODE *) arg) == SELWRITE) &&
            (!rngIsFull(fdInfo->fdtype.channel.outputBuf)))
        { 
            /* device ready for writing, make sure task does not pend */ 
            selWakeup ((SEL_WAKEUP_NODE *) arg); 
        } 
        
        intUnlock(key);
        break;
        
    case FIOUNSELECT:
        /* delete node from wakeup list */ 
        retCode=selNodeDelete (&fdInfo->selWakeupList, (SEL_WAKEUP_NODE *) arg); 
#if DEVIO_DEBUG
        if(retCode == ERROR)
            logMsg("wncUtilIoctlFioCmds() ERROR: FIOUNSELECT node not in list\n", 
            0,0,0,0,0,0);       
#endif
        break;
        
    case FIONFREE:
        /* Get #free bytes in output data buffer */
        numBytes = (int *) arg;
        *numBytes = rngFreeBytes (fdInfo->fdtype.channel.outputBuf);
        break;
        
    case FIONREAD:
        /* Get #bytes ready to be read from the input data buffer */
        numBytes = (int *) arg;
        *numBytes = rngNBytes (fdInfo->fdtype.channel.inputBuf);
        break;
        
    case FIONWRITE:
        /* Get #bytes written to the output data buffer */
        numBytes = (int *) arg;
        *numBytes = rngNBytes (fdInfo->fdtype.channel.outputBuf);
        break;
        
    case FIOFLUSH:
        /* Discard all bytes in both input and output data buffers */
        key = intLock();
        rngFlush (fdInfo->fdtype.channel.inputBuf);
        rngFlush (fdInfo->fdtype.channel.outputBuf);
        intUnlock(key);
        break;
        
    case FIORFLUSH:
        /* Discard all bytes in the input data buffer */
        key = intLock();
        rngFlush (fdInfo->fdtype.channel.inputBuf);
        intUnlock(key);
        break;
        
    case FIOWFLUSH:
        /* Discard all bytes in the output data buffer */
        key = intLock();
        rngFlush (fdInfo->fdtype.channel.outputBuf);
        intUnlock(key);
        break;
        
    case FIORBUFSET:
        /* Set the input data buffer size; User specifies #msgs */
        bufSize = wncUtilCalcRingBufSize (arg);
        if (bufSize)
        {
            key = intLock();
            rngFlush(fdInfo->fdtype.channel.inputBuf);
            rngDelete(fdInfo->fdtype.channel.inputBuf);
            fdInfo->fdtype.channel.inputBuf = rngCreate(bufSize);
            if(fdInfo->fdtype.channel.inputBuf == NULL)
            {
#if DEVIO_DEBUG
                logMsg("wncUtilIoctlFioCmds() ERROR: FIORBUFSET ring create failed\n", 
                    0,0,0,0,0,0);
#endif
                retCode=ERROR;
                
            }
            intUnlock(key);
        }
        break;
        
    case FIOWBUFSET:
        /* Set the output data buffer size; User specifies #msgs */
        bufSize = wncUtilCalcRingBufSize (arg);
        if (bufSize)
        {
            key = intLock();
            rngFlush(fdInfo->fdtype.channel.outputBuf);
            rngDelete(fdInfo->fdtype.channel.outputBuf);
            fdInfo->fdtype.channel.outputBuf = rngCreate(bufSize);
            if(fdInfo->fdtype.channel.outputBuf == NULL)
            {
#if DEVIO_DEBUG
                logMsg("wncUtilIoctlFioCmds() ERROR: FIOWBUFSET ring create failed\n", 
                    0,0,0,0,0,0);
#endif
                retCode=ERROR;
                
            }            
            intUnlock(key);
        }
        break;
    }
    
    /* unlock device */
    semGive(wncDrv->mutex);
    
    return retCode;
}


/************************************************************************
*
* wncUtilIoctlDeviceFioCmds - utility routine to process FIO* ioctl() 
* commands for a device
*
* This routine performs the processing for FIO* (internal input and
* output data buffer) commands specified via ioctl()
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
*/

LOCAL STATUS wncUtilIoctlDeviceFioCmds
(
 WNCAN_DEVIO_FDINFO   *fdInfo,      /* pointer to DevIO file descriptor */
 int                   command,     /* command function code */
 int                   arg          /* arbitrary argument */
 )
{
    WNCAN_DEVIO_DRVINFO*  wncDrv = NULL;
    WNCAN_DEVICE*         canDev = NULL;
    int                   key;
    
    if (fdInfo == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncUtilIoctlDeviceFioCmds() ERROR: pointer to DevIO file descriptor is null\n", 
            0,0,0,0,0,0);
#endif
        return ERROR;
    }
    
    wncDrv = fdInfo->wnDevIODrv;
    if (wncDrv == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncUtilIoctlDeviceFioCmds() ERROR: pointer to DevIO driver is null\n", 
            0,0,0,0,0,0);
#endif
        return ERROR;
    }
    
    canDev = wncDrv->wncDevice;
    if (canDev == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncUtilIoctlDeviceFioCmds() ERROR: pointer to DevIO device is null\n", 
            0,0,0,0,0,0);
#endif
        return ERROR;
    }
    
    /* lock device */
    semTake(wncDrv->mutex, WAIT_FOREVER);
    
    /* Process the specified command */
    
    switch (command)
    {
        /* Internal input and output data buffer commands */
    case FIOSELECT:
        selNodeAdd (&fdInfo->selWakeupList, (SEL_WAKEUP_NODE *) arg); 
        
        key = intLock();
        if ((selWakeupType ((SEL_WAKEUP_NODE *) arg) == SELREAD) &&
            (CAN_GetBusStatus(canDev) != WNCAN_BUS_OK))
        { 
            /* bus in an error/warning state, make sure task does not pend */ 
            selWakeup ((SEL_WAKEUP_NODE *) arg); 
        }
        
        /* there is no blocking for WRITE descriptors for the device */
        if (selWakeupType ((SEL_WAKEUP_NODE *) arg) == SELWRITE) 
        {             
            selWakeup ((SEL_WAKEUP_NODE *) arg); 
        } 
        
        intUnlock(key);
        break;
        
    case FIOUNSELECT:
        /* delete node from wakeup list */ 
        selNodeDelete (&fdInfo->selWakeupList, (SEL_WAKEUP_NODE *) arg); 
        break;
    }
    
    /* unlock device */
    semGive(wncDrv->mutex);
    
    return OK;
}




/************************************************************************
*
* wncUtilIoctlDeviceCmds - utility routine to process CAN device ioctl() 
*                          commands
*
* This routine performs the processing for CAN device commands specified 
* via ioctl()
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
*/

LOCAL STATUS wncUtilIoctlDeviceCmds
(
 WNCAN_DEVIO_FDINFO   *fdInfo,      /* pointer to DevIO file descriptor */
 int                   command,     /* command function code */
 int                   arg          /* arbitrary argument */
 )
{
    WNCAN_DEVIO_DRVINFO*  wncDrv = NULL;
    WNCAN_DEVICE*         canDev = NULL;
    STATUS                status = ERROR;    /* pessimistic */
    UCHAR*                chNum = NULL;
    WNCAN_CONFIG*         devCfg = NULL;
    WNCAN_REG*            regCfg = NULL;
    
    if (fdInfo == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncUtilIoctlDeviceCmds() ERROR: pointer to DevIO file descriptor is null\n", 
            0,0,0,0,0,0);
#endif
        return ERROR;
    }
    
    wncDrv = fdInfo->wnDevIODrv;
    if (wncDrv == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncUtilIoctlDeviceCmds() ERROR: pointer to DevIO driver is null\n", 
            0,0,0,0,0,0);
#endif
        return ERROR;
    }
    
    canDev = wncDrv->wncDevice;
    if (canDev == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncUtilIoctlDeviceCmds() ERROR: pointer to DevIO device is null\n", 
            0,0,0,0,0,0);
#endif
        return ERROR;
    }
    
    
    /* lock device */
    semTake(wncDrv->mutex, WAIT_FOREVER);
    
    /* Process the specified command */
    
    switch (command)
    {
        /* CAN device commands */
    case WNCAN_HALT:
        {
            BOOL  stop = (BOOL) arg;
            if (stop == TRUE)
                CAN_Stop (canDev);
            else
                CAN_Start (canDev);
        }
        status = OK;
        break;
        
    case WNCAN_SLEEP:
        status = CAN_Sleep (canDev);
        break;
        
    case WNCAN_WAKE:
        status = CAN_WakeUp (canDev);
        break;
        
    case WNCAN_TX_ABORT:
        CAN_TxAbort (canDev);
        status = OK;
        break;
        
    case WNCAN_RXCHAN_GET:
        chNum = (UCHAR *) arg;
        status = CAN_GetRxChannel (canDev, chNum);
        break;
        
    case WNCAN_TXCHAN_GET:
        chNum = (UCHAR *) arg;
        status = CAN_GetTxChannel (canDev, chNum);
        break;
        
    case WNCAN_RTRREQCHAN_GET:
        chNum = (UCHAR *) arg;
        status = CAN_GetRTRRequesterChannel (canDev, chNum);
        break;
        
    case WNCAN_RTRRESPCHAN_GET:
        chNum = (UCHAR *) arg;
        status = CAN_GetRTRResponderChannel (canDev, chNum);
        break;
        
    case WNCAN_BUSINFO_GET:
        {
            WNCAN_BUSINFO*  busInfo = (WNCAN_BUSINFO *) arg;
            busInfo->busStatus = CAN_GetBusStatus (canDev);
            busInfo->busError = CAN_GetBusError (canDev);
        }
        status = OK;
        break;
        
    case WNCAN_CONFIG_SET:
        devCfg = (WNCAN_CONFIG *) arg;
        
        /* 
        Determine which configuration options are being set;
        User will 'OR' selected items together into flags, so
        we need to parse flags here 
        */
        
        if ( (devCfg->flags & WNCAN_CFG_INFO) == WNCAN_CFG_INFO)
        {
            /* WNCAN_CFG_INFO not applicable for SET; ignore */
            
            status = OK;
        }
        
        if ( (devCfg->flags & WNCAN_CFG_GBLFILTER) == WNCAN_CFG_GBLFILTER)
        {
            status = CAN_SetGlobalRxFilter (canDev, devCfg->filter.mask,
                devCfg->filter.extended);
            if(status == ERROR)
                break;
        }
        
        if ( (devCfg->flags & WNCAN_CFG_BITTIMING) == WNCAN_CFG_BITTIMING)
        {
            status = CAN_SetBitTiming (canDev, devCfg->bittiming.tseg1,
                devCfg->bittiming.tseg2, 
                devCfg->bittiming.brp,
                devCfg->bittiming.sjw,
                devCfg->bittiming.oversample);           
        }
        
        break;
        
    case WNCAN_CONFIG_GET:
        devCfg = (WNCAN_CONFIG *) arg;
        
        /* 
        Determine which configuration options are to be retrieved;
        User will 'OR' selected items together into flags, so
        we need to parse flags here 
        */
        
        if ( (devCfg->flags & WNCAN_CFG_INFO) == WNCAN_CFG_INFO)
        {
            const WNCAN_VersionInfo *verInfo = CAN_GetVersion ();
            devCfg->info.version.major = verInfo->major;
            devCfg->info.version.minor = verInfo->minor;
            
            devCfg->info.ctrlType = CAN_GetControllerType (canDev);
            devCfg->info.xtalfreq = CAN_GetXtalFreq (canDev);
            devCfg->info.numChannels = CAN_GetNumChannels (canDev);
            devCfg->info.baudRate = CAN_GetBaudRate (canDev, 
                &(devCfg->info.samplePoint));
            
            status = OK;
        }
        
        if ( (devCfg->flags & WNCAN_CFG_GBLFILTER) == WNCAN_CFG_GBLFILTER)
        {
            devCfg->filter.mask = CAN_GetGlobalRxFilter (canDev, 
                devCfg->filter.extended);
            status = OK;
        }
        
        if ( (devCfg->flags & WNCAN_CFG_BITTIMING) == WNCAN_CFG_BITTIMING)
        {
            /* WNCAN_CFG_BITTIMING not applicable for GET */
            devCfg->bittiming.tseg1=canDev->pCtrl->tseg1;
            devCfg->bittiming.tseg2=canDev->pCtrl->tseg2;
            devCfg->bittiming.brp=canDev->pCtrl->brp;
            devCfg->bittiming.sjw=canDev->pCtrl->sjw;
            devCfg->bittiming.oversample=canDev->pCtrl->samples;
            
            status = OK;
        }
        
        break;
        
    case WNCAN_REG_SET:
        regCfg = (WNCAN_REG *) arg;
        
        status = CAN_WriteReg (canDev,
            regCfg->offset,
            regCfg->pData, 
            regCfg->length);
        
        break;
        
    case WNCAN_REG_GET:
        regCfg = (WNCAN_REG *) arg;
        
        status = CAN_ReadReg (canDev,
            regCfg->offset,
            regCfg->pData, 
            regCfg->length);
        
        break;
    }
    
    /* unlock device */
    semGive(wncDrv->mutex);
    
    return status;
}


/************************************************************************
*
* wncUtilIoctlChannelCmds - utility routine to process CAN channel ioctl() 
*                           commands
*
* This routine performs the processing for CAN channel commands specified 
* via ioctl()
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
*/

LOCAL STATUS wncUtilIoctlChannelCmds
(
 WNCAN_DEVIO_FDINFO   *fdInfo,      /* pointer to DevIO file descriptor */
 int                   command,     /* command function code */
 int                   arg          /* arbitrary argument */
 )
{
    WNCAN_DEVIO_DRVINFO*  wncDrv = NULL;
    WNCAN_DEVICE*         canDev = NULL;
    STATUS                status = ERROR;    /* pessimistic */
    WNCAN_CHNCONFIG*      chnCfg = NULL;
    
    if (fdInfo == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncUtilIoctlChannelCmds() ERROR: pointer to DevIO file descriptor is null\n", 
            0,0,0,0,0,0);
#endif
        return ERROR;
    }
    
    wncDrv = fdInfo->wnDevIODrv;
    if (wncDrv == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncUtilIoctlChannelCmds() ERROR: pointer to DevIO driver is null\n", 
            0,0,0,0,0,0);
#endif
        return ERROR;
    }
    
    canDev = wncDrv->wncDevice;
    if (canDev == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncUtilIoctlChannelCmds() ERROR: pointer to DevIO device is null\n", 
            0,0,0,0,0,0);
#endif
        return ERROR;
    }
    
    /* lock device */
    semTake(wncDrv->mutex, WAIT_FOREVER);
    
    /* Process the specified command */
    
    switch (command)
    {
        /* CAN channel commands */
    case WNCAN_CHNCONFIG_SET:
        chnCfg = (WNCAN_CHNCONFIG *) arg;
        
        /* 
        Determine which configuration options are being set;
        User will 'OR' selected items together into flags, so
        we need to parse flags here 
        */
        
        if ( (chnCfg->flags & WNCAN_CHNCFG_CHANNEL) == WNCAN_CHNCFG_CHANNEL)
        {
            status = CAN_WriteID(canDev, fdInfo->fdtype.channel.channel,
                chnCfg->channel.id, chnCfg->channel.extId);
            
            if(status == ERROR)
            {
#if DEVIO_DEBUG             
                logMsg("wncUtilIoctlChannelCmds() Error: Cannot write id",0,0,0,0,0,0);
#endif
                break;
            }
            
        }
        
        if ( (chnCfg->flags & WNCAN_CHNCFG_LCLFILTER) == WNCAN_CHNCFG_LCLFILTER)
        {
            status = CAN_SetLocalMsgFilter (canDev, fdInfo->fdtype.channel.channel,
                chnCfg->filter.mask, 
                chnCfg->filter.extended);
            if(status == ERROR)
            {
#if DEVIO_DEBUG             
                logMsg("wncUtilIoctlChannelCmds() Error: Cannot set local"
                    "local msg filter \n",0,0,0,0,0,0);
#endif
                break;
            }
        }
        
        if ( (chnCfg->flags & WNCAN_CHNCFG_RTR) == WNCAN_CHNCFG_RTR)
        {
            status = CAN_SetRTR (canDev, fdInfo->fdtype.channel.channel,
                chnCfg->rtr);
            if(status == ERROR)
            {
#if DEVIO_DEBUG             
                logMsg("wncUtilIoctlChannelCmds() Error: Cannot set rtr\n",0,0,0,0,0,0);
#endif
                break;
            }
        }
        
        if ( (chnCfg->flags & WNCAN_CHNCFG_MODE) == WNCAN_CHNCFG_MODE)
        {
            status = CAN_SetMode (canDev, fdInfo->fdtype.channel.channel,
                chnCfg->mode);
            if(status == ERROR)
            {
#if DEVIO_DEBUG             
                logMsg("wncUtilIoctlChannelCmds() Error: Cannot set mode \n",0,0,0,0,0,0);
#endif
                break;
            }
        }
        
        break;
        
    case WNCAN_CHNCONFIG_GET:
        chnCfg = (WNCAN_CHNCONFIG *) arg;
        
        /* 
        Determine which configuration options are to be retrieved;
        User will 'OR' selected items together into flags, so
        we need to parse flags here 
        */
        
        if ( (chnCfg->flags & WNCAN_CHNCFG_CHANNEL) == WNCAN_CHNCFG_CHANNEL)
        {
            BOOL newdata; /* unused, needed by api */
            
            chnCfg->channel.id = CAN_ReadID(canDev, fdInfo->fdtype.channel.channel,
                &chnCfg->channel.extId);
            chnCfg->channel.len = WNCAN_MAX_DATA_LEN; /* load the whole message */
            CAN_ReadData(canDev, fdInfo->fdtype.channel.channel,
                chnCfg->channel.data, &chnCfg->channel.len, &newdata);
        }
        
        if ( (chnCfg->flags & WNCAN_CHNCFG_LCLFILTER) == WNCAN_CHNCFG_LCLFILTER)
        {
            chnCfg->filter.mask = CAN_GetLocalMsgFilter(canDev, 
                fdInfo->fdtype.channel.channel,                                        
                chnCfg->filter.extended);
            if(chnCfg->filter.mask == -1)
            {
#if DEVIO_DEBUG             
                logMsg("wncUtilIoctlChannelCmds() Error: Cannot determine local"
                    "messsage filter",0,0,0,0,0,0);
#endif
                status=ERROR;
                break;
            }
        }
        
        if ( (chnCfg->flags & WNCAN_CHNCFG_RTR) == WNCAN_CHNCFG_RTR)
        {
            chnCfg->rtr = CAN_IsRTR (canDev, fdInfo->fdtype.channel.channel);
            if(chnCfg->rtr == -1)
            {
#if DEVIO_DEBUG             
                logMsg("wncUtilIoctlChannelCmds() Error: Cannot determine RTR\n", 
                    0,0,0,0,0,0);
#endif
                
                status=ERROR;
                break;
            }
        }
        
        if ( (chnCfg->flags & WNCAN_CHNCFG_MODE) == WNCAN_CHNCFG_MODE)
        {
            chnCfg->mode = CAN_GetMode (canDev, fdInfo->fdtype.channel.channel);
        }
        
        break;
        
    case WNCAN_CHN_ENABLE:
        {
            BOOL  enable = (BOOL) arg;
            if (enable == TRUE)
            {
                status = CAN_EnableChannel (canDev, fdInfo->fdtype.channel.channel,
                    fdInfo->fdtype.channel.intType);
                if (status == OK)
                    fdInfo->fdtype.channel.enabled = TRUE;
#if DEVIO_DEBUG             
                else
                    
                    logMsg("wncUtilIoctlChannelCmds() Error: Channel cannot be enabled\n", 
                    0,0,0,0,0,0);
#endif
                
                
            }
            else
            {
                status = CAN_DisableChannel (canDev, fdInfo->fdtype.channel.channel);
                if (status == OK)
                    fdInfo->fdtype.channel.enabled = FALSE;
#if DEVIO_DEBUG             
                else
                    
                    logMsg("wncUtilIoctlChannelCmds() Error: Channel cannot be disabled\n", 
                    0,0,0,0,0,0);
#endif
                
            }
        }
        break;
        
    case WNCAN_CHN_TX:
        status = CAN_Tx (canDev, fdInfo->fdtype.channel.channel);
        break;
        
    case WNCAN_CHNMSGLOST_GET:
        *((int *)arg) = CAN_IsMessageLost (canDev, fdInfo->fdtype.channel.channel);
        if(*((int *)arg) == -1)
            status=ERROR;
        else
            status = OK;
        break;
        
    case WNCAN_CHNMSGLOST_CLEAR:
        status = CAN_ClearMessageLost (canDev, fdInfo->fdtype.channel.channel);
        break;
    }
    
    /* unlock device */
    semGive(wncDrv->mutex);
    
    return status;
}


/************************************************************************
*
* wncUtilIoctlCtlrCmds - utility routine to process CAN controller-specific 
*                        ioctl() commands
*
* This routine performs the processing for CAN controller-specific commands  
* specified via ioctl()
*
* RETURNS: OK or ERROR
*
* ERRNO: N/A
*
*/

LOCAL STATUS wncUtilIoctlCtlrCmds
(
 WNCAN_DEVIO_FDINFO   *fdInfo,      /* pointer to DevIO file descriptor */
 int                   command,     /* command function code */
 int                   arg          /* arbitrary argument */
 )
{
    WNCAN_DEVIO_DRVINFO*  wncDrv = NULL;
    WNCAN_DEVICE*         canDev = NULL;
    WNCAN_CTLRCONFIG*     ctlrCfg = NULL;
    STATUS                status = ERROR;    /* pessimistic */
    
    if (fdInfo == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncUtilIoctlCtlrCmds() ERROR: pointer to DevIO file descriptor is null\n", 
            0,0,0,0,0,0);
#endif
        return ERROR;
    }
    
    wncDrv = fdInfo->wnDevIODrv;
    if (wncDrv == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncUtilIoctlCtlrCmds() ERROR: pointer to DevIO driver is null\n", 
            0,0,0,0,0,0);
#endif
        return ERROR;
    }
    
    canDev = wncDrv->wncDevice;
    if (canDev == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncUtilIoctlCtlrCmds() ERROR: pointer to DevIO device is null\n", 
            0,0,0,0,0,0);
#endif
        return ERROR;
    }
    
    
    ctlrCfg = (WNCAN_CTLRCONFIG *) arg;
    if (ctlrCfg == NULL)
    {
#if DEVIO_DEBUG
        logMsg("wncUtilIoctlCtlrCmds() ERROR: pointer to controller config is null\n", 
            0,0,0,0,0,0);
#endif
        return ERROR;
    }
    
    /* lock device */
    semTake(wncDrv->mutex, WAIT_FOREVER);
    
    /* Process the specified command */
    
    switch (command)
    {
        /* Controller-specific commands */
    case WNCAN_CTLRCONFIG_SET:
        /* Process TouCAN controller command */
        
        if (wncDrv->ctrlSetConfig)
            (*wncDrv->ctrlSetConfig)(wncDrv, (void*)arg);
        
        break;
        
    case WNCAN_CTLRCONFIG_GET:
        /* Process TouCAN controller command */
        
        if (wncDrv->ctrlGetConfig)
            (*wncDrv->ctrlGetConfig)(wncDrv, (void*)arg);
        
        break;
    }
    
    /* unlock device */
    semGive(wncDrv->mutex);
    
    return status;
}


/************************************************************************
*
* wncUtilCalcRingBufSize - utility routine to calculate ring buffer size
*
* This routine calculates the size of the ring buffer in bytes when the
* user specifies the number of CAN data messages to be queued
*
* RETURNS: size in bytes
*
* ERRNO: N/A
*
*/

LOCAL int wncUtilCalcRingBufSize
(
 int  numCanMsgs    /* #CAN msgs to queue in ring buffer */
 )
{
/* User will specify internal input and output data buffer size in #CAN msgs;
    Calculate #bytes here */
    
    int  numBytes = 0;
    
    numBytes = numCanMsgs * sizeof(WNCAN_CHNMSG);
    return numBytes;
}


