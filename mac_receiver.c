//////////////////////////////////////////////////////////////////////////////////
/// \file mac_receiver.c
/// \brief MAC receiver thread
/// \author Pascal Sartoretti (sap at hevs dot ch)
/// \version 1.0 - original
/// \date  2018-02
//////////////////////////////////////////////////////////////////////////////////
#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>
#include "main.h"


//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacReceiver(void *argument)
{
	struct queueMsg_t queueMsg;		// queue message
	uint8_t * msg;
	uint8_t * qPtr;
	size_t	size;
	osStatus_t retCode;
	uint8_t src_addr;
	
	for(;;){
		//----------------------------------------------------------------------------
		// QUEUE READ										
		//----------------------------------------------------------------------------
		// frame come always FROM_PHY
		retCode = osMessageQueueGet( 	
			queue_macR_id,
			&queueMsg,
			NULL,
			osWaitForever); 		// thread is blocking while queue is empty
    CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
			
		qPtr = queueMsg.anyPtr;
		src_addr = qPtr[0]>>3;
		
		if(qPtr[0] == TOKEN_TAG){
			
			//change type of message before dispatch it
			queueMsg.type = TOKEN;
			//put token frame in queue_macS_id
			retCode = osMessageQueuePut(
			queue_macS_id,
			&queueMsg,
			osPriorityNormal,
			osWaitForever);	//blocking
		
			CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
		}
		else{
			switch(src_addr){
				case BROADCAST_ADDRESS :
					//change type of message before dispatch it
					queue.Msg.type = DATA_IND;
					//put time frame in queue_timeR_id
					retCode = osMessageQueuePut(
					queue_timeR_id,
					&queueMsg,
					osPriorityNormal,
					osWaitForever);	//blocking
					break;
				
				case MYADDRESS :
					//change type of message before dispatch it
					queue.Msg.type = DATABACK;
					//put time frame in queue_macS_id
					retCode = osMessageQueuePut(
					queue_macS_id,
					&queueMsg,
					osPriorityNormal,
					osWaitForever);	//blocking
					break;
				
				default :		//source is another 
					//change type of message before dispatch it
					queue.Msg.type = DATA_IND;
					//put time frame in queue_chatR_id
					retCode = osMessageQueuePut(
					queue_chatR_id,
					&queueMsg,
					osPriorityNormal,
					osWaitForever);	//blocking
					break;
			}
		}	
		
		//----------------------------------------------------------------------------
		// MEMORY RELEASE	(created frame : phy layer style)
		//----------------------------------------------------------------------------
		retCode = osMemoryPoolFree(memPool,qPtr);
		CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
	}
}


