/* canFixedLL.c - WN-CAN fixed-node linked lists */

/* Copyright 2002 Wind River Systems, Inc. */

/* 
modification history 
--------------------
07nov02,rgp written

*/

/* 

DESCRIPTION
implementation of generic fixed-node linked list

*/

/* includes */
#include <vxWorks.h>
#include <taskLib.h>
#include <errnoLib.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <CAN/canFixedLL.h>




/************************************************************************
*
* FLL_Create - Initialize the linked list
*
* RETURNS: none
*   
*/
void FLL_Create
(
FLL_RootType *pRoot
)
{
    assert(pRoot);         /* internal error */
  
    /* initialize the linked list instance */
    pRoot->pFirst = NULL;
}


/************************************************************************
*
* FLL_Add - Add a node into the linked list
*
* NOTE: a node with a duplicate key will *not* be added!
*
* RETURNS: none
* 
*
*/
void FLL_Add
(
FLL_RootType  *pRoot, 
FLL_NodeType  *pNode
)
{
    assert(pRoot);
  
    if (!pNode) return;   /* nothing to add, then exit */

    taskLock();
    /* only add a unique instance of the node's key into the list */
    if (FLL_Find(pRoot, pNode->key) == NULL)
    {
        pNode->pNext = pRoot->pFirst;
	pRoot->pFirst = pNode; 
    }
    taskUnlock();
}


/************************************************************************
*
* FLL_Find - find a node matching the specified key
*
* RETURNS: ptr to matching node or NULL
*   
*/
FLL_NodeType *FLL_Find
(
FLL_RootType *pRoot, 
int key
)
{
    FLL_NodeType  *pCur;
  
    assert(pRoot);
    
    /* linear seach for the key */
    for(pCur = pRoot->pFirst; pCur; pCur = pCur->pNext)
        if (pCur->key == key) break;
      
    return pCur;
}


