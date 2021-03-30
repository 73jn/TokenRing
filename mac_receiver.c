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
	
	for(;;){
		//----------------------------------------------------------------------------
		// QUEUE READ										
		//----------------------------------------------------------------------------
		retCode = osMessageQueueGet( 	
			queue_macR_id,
			&queueMsg,
			NULL,
			osWaitForever); 	
    CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);				
		qPtr = queueMsg.anyPtr;
		
		switch(queueMsg.type){
			
			case TOKEN :
				//put token frame in queue_macS_id
				retCode = osMessageQueuePut(
				queue_macS_id,
				&queueMsg,
				osPriorityNormal,
				osWaitForever);
			
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
				break;
			
			case FROM_PHY :
				break;
			
			default : 
				int i;
				i=0;
				break;
			
		}
	}
}


