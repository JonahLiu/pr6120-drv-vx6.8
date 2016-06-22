/* canFixedLL.h - WN-CAN fixed-node linked lists */

/* Copyright 2002 Wind River Systems, Inc. */

/* 
modification history 
--------------------
07nov02,rgp written

*/

/* 

DESCRIPTION
The file defines the fixed-node linked lists. A fixed-object linked list is
a normal linked list, except that each node in the linked is statically allocated.
Therefore, this list does not use any dynamic memory.

As a result, there may only be limited use of the linked lists.

In addition, since we want to use common code with what essentially
amounts to sub-classes of the nodes (i.e. specific data for each
separate linked list), define all data within this header file.

*/

#ifndef CAN_FIXEDLL_H_
#define CAN_FIXEDLL_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "CAN/wnCAN.h"
#include "CAN/canBoard.h"
#include "CAN/canController.h"

/* Define Common Fixed-Node Data Structure */
typedef struct _fll_node 
{
    struct _fll_node *pNext;       /* next node ptr */
    int               key;         /* identifying key value for each node */
    
    /* define union for all uses of our linked list. This is effectively
    ** our sub-classing of the data structure
    **/
    union 
    {
        /* board's linked list data */
        struct 
	{
	  pfn_Board_EstabLnks_Type  establinks_fn;  /* establishlinks function */
	  pfn_Board_Open_Type       open_fn;        /* open function */
	  pfn_Board_Close_Type      close_fn;       /* close function */
          pfn_Board_Show_Type       show_fn;        /* show function */
        } boarddata;

        /* controller's linked list data */
	struct {
	  pfn_Ctrlr_EstabLnks_Type  establinks_fn;  /* establishlinks function */
	} controllerdata;    

    } nodedata;

} FLL_NodeType;


/* define types for each of the supported lists */
typedef FLL_NodeType BoardLLNode;
typedef FLL_NodeType ControllerLLNode;


/* define struct for an instance of the linked-list */
typedef struct _fll_instance
{
    FLL_NodeType *pFirst;
} FLL_RootType;



/* Linked List APIs */
void FLL_Create(FLL_RootType*);
void FLL_Add(FLL_RootType*, FLL_NodeType*);
FLL_NodeType *FLL_Find(FLL_RootType*, int key);

#define FLL_FirstNode(rootptr)    ( (rootptr) ? (rootptr)->pFirst : NULL )
#define FLL_NextNode(nodeptr)     ( (nodeptr) ? (nodeptr)->pNext : NULL )

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
/* due do the recursive includes of canboard,cancontroller and this file,
** need to declare the extern'd global variables here becase the root type
** is now fully resolved.
*/
extern FLL_RootType g_BoardLLRoot;
extern FLL_RootType g_ControllerLLRoot;

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* CAN_FIXEDLL_H_ */
