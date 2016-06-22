/*****************************************************************************
 * %LICENSE_DUAL%
 * This file is provided under a dual BSD/GPLv2 license.  When using or 
 *   redistributing this file, you may do so under either license.
 * 
 *   GPL LICENSE SUMMARY
 * 
 *   Copyright(c) 2007 Intel Corporation. All rights reserved.
 * 
 *   This program is free software; you can redistribute it and/or modify 
 *   it under the terms of version 2 of the GNU General Public License as
 *   published by the Free Software Foundation.
 * 
 *   This program is distributed in the hope that it will be useful, but 
 *   WITHOUT ANY WARRANTY; without even the implied warranty of 
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 *   General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License 
 *   along with this program; if not, write to the Free Software 
 *   Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *   The full GNU General Public License is included in this distribution 
 *   in the file called LICENSE.GPL.
 * 
 *   Contact Information:
 *   Intel Corporation
 * 
 *   BSD LICENSE 
 * 
 *   Copyright(c) 2007 Intel Corporation. All rights reserved.
 *   All rights reserved.
 * 
 *   Redistribution and use in source and binary forms, with or without 
 *   modification, are permitted provided that the following conditions 
 *   are met:
 * 
 *     * Redistributions of source code must retain the above copyright 
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright 
 *       notice, this list of conditions and the following disclaimer in 
 *       the documentation and/or other materials provided with the 
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its 
 *       contributors may be used to endorse or promote products derived 
 *       from this software without specific prior written permission.
 * 
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 *****************************************************************************/
#include <vxWorks.h>
#include <stdio.h>
#include <stdlib.h>

#include <CAN/icp_can.h>
#include <CAN/can_fifo.h>

/******************************************************************************
 * A FIFO element is a CAN msg.
 *****************************************************************************/
typedef struct can_fifo_item {
	icp_can_msg_t		msg;
	struct can_fifo_item	*next;
} can_fifo_item_t;

/******************************************************************************
 * The FIFO.
 *****************************************************************************/
typedef struct can_fifo {
	can_fifo_item_t *head;
	can_fifo_item_t *tail;
	unsigned int size;
} can_fifo_t;

/******************************************************************************
 * Check if FIFO is empty.
 *****************************************************************************/
int can_fifo_empty(icp_can_handle_t handle)
{
	can_fifo_t *f = (can_fifo_t *) handle;

	return (f->head==f->tail);
}

/******************************************************************************
 * Check if FIFO is full.
 *****************************************************************************/
int can_fifo_full(icp_can_handle_t handle)
{
	can_fifo_t *f = (can_fifo_t *) handle;

	return (f->head->next==f->tail);
}

/******************************************************************************
 * Create a CAN messsage FIFO of size specifified.
 *****************************************************************************/
icp_can_handle_t can_fifo_create(unsigned int num_nodes)
{
	unsigned int i;
	can_fifo_item_t  *curr;
	can_fifo_t *f;
	
	f = (can_fifo_t *) CAN_MEM_ALLOC(sizeof(can_fifo_t));
	
	if (!f) {
		CAN_PRINT_DEBUG(ICP_CAN_ERR_ALLOC, "msg queue");
		return (icp_can_handle_t) 0;
	}
	
	f->head = (can_fifo_item_t *) CAN_MEM_ALLOC(sizeof(can_fifo_item_t));

	f->tail = f->head;
	
	if (!(f->head)) {
		CAN_PRINT_DEBUG(ICP_CAN_ERR_ALLOC, "msg queue head");
		CAN_MEM_FREE(f);
		return (icp_can_handle_t) 0;
	}
	curr = f->head;
	
	for (i=1; i<num_nodes; i++) {
		curr->next = (can_fifo_item_t *) CAN_MEM_ALLOC(sizeof(can_fifo_item_t));

		if (!(curr->next)) {
			CAN_PRINT_DEBUG(ICP_CAN_ERR_ALLOC, "msg queue node");
			f->size = i-1; 
			CAN_MEM_FREE(f);
			return (icp_can_handle_t) 0;
		}
		
		curr = curr->next;
	}
	
	curr->next = f->head;
	f->size = num_nodes;
	
	return (icp_can_handle_t) f;
}

/******************************************************************************
 * Delete the FIFO.
 *****************************************************************************/
void can_fifo_destroy(icp_can_handle_t handle)
{
	unsigned int i;
	can_fifo_item_t *curr;
	can_fifo_item_t *next;
	can_fifo_t *f = (can_fifo_t *) handle;
		
	if (handle) {
		curr = f->head;
		next = curr->next;
		
		for (i=0; i<f->size; i++) {
			if (!curr) {
				CAN_PRINT_DEBUG(ICP_CAN_ERR_FREE, 
					"msg queue node");
			}
			CAN_MEM_FREE(curr);
			curr = next;
			next = (can_fifo_item_t *) curr->next;
		}
		
		CAN_MEM_FREE(f);
	}
}

/******************************************************************************
 * Get the first element of the FIFO.
 *****************************************************************************/
int can_fifo_get(
	icp_can_handle_t	handle, 
	icp_can_msg_t		*msg)
{
	int i;
	can_fifo_t *f = (can_fifo_t *) handle;
	icp_can_msg_t msg_tmp = f->tail->msg;
	
	if ((!handle) || (!msg)) {
		return -1;
	}
#if TOLAPAI_CAN_DRV_DEBUG
    printf("can_fifo_get() \n");
#endif
	
	if (f->head==f->tail) {
#if TOLAPAI_CAN_DRV_DEBUG
	    printf("can_fifo_get() queue empty\n");
#endif
		return -1;
	}
		
	msg->ide = msg_tmp.ide;
	msg->id  = msg_tmp.id;
	msg->dlc = msg_tmp.dlc;
	msg->rtr = msg_tmp.rtr;
	
	for (i=0; i<ICP_CAN_MSG_DATA_LEN; i++) {
		msg->data[i] = msg_tmp.data[i];
	}
	
	f->tail = f->tail->next;
	return 0;
}

/******************************************************************************
 * Put a message into the FIFO.
 *****************************************************************************/
int can_fifo_put(
	icp_can_handle_t	handle, 
	icp_can_msg_t		*msg)
{
	int i;
	can_fifo_t *f = (can_fifo_t *) handle;
	icp_can_msg_t *msg_tmp = &(f->head->msg);
	
	if ((!handle) || (!msg)) {
		return -1;
	}
#if TOLAPAI_CAN_DRV_DEBUG
    printf("can_fifo_put() \n");
#endif
	
	if (f->head->next==f->tail) {
#if TOLAPAI_CAN_DRV_DEBUG
	    printf("can_fifo_put() queue full\n");
#endif
		return -1;
	}
	
	msg_tmp->ide = msg->ide;
	msg_tmp->rtr = msg->rtr;
	msg_tmp->id  = msg->id;
	msg_tmp->dlc = msg->dlc;
	
	for (i=0; i<ICP_CAN_MSG_DATA_LEN; i++) {
		msg_tmp->data[i] = msg->data[i];
	}
	
	f->head = f->head->next;
	return 0;
}
