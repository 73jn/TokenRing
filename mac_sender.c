//////////////////////////////////////////////////////////////////////////////////
/// \file mac_sender.c
/// \brief MAC sender thread
/// \author Pascal Sartoretti (pascal dot sartoretti at hevs dot ch)
/// \version 1.0 - original
/// \date  2018-02
//////////////////////////////////////////////////////////////////////////////////
#include "stm32f7xx_hal.h"

#include <stdio.h>
#include <string.h>
#include "main.h"
#include "ext_led.h"

void sendToken(osStatus_t	retCode,	struct queueMsg_t* queueMsg, char* tokenTag,	char * msg){
	//Send to LCD TOKEN_LIST
	
	
	//Send to Physical
	
	//------------------------------------------------------------------------
	// MEMORY ALLOCATION				
	//------------------------------------------------------------------------
	msg = osMemoryPoolAlloc(memPool,osWaitForever);				
	queueMsg->type = TOKEN;
	queueMsg->anyPtr = msg;
	memcpy(msg,tokenTag,strlen(tokenTag)+1);
		//------------------------------------------------------------------------
		// QUEUE SEND								
		//------------------------------------------------------------------------
		retCode = osMessageQueuePut(
			queue_phyS_id,
			queueMsg,
			osPriorityNormal,
			osWaitForever);
		CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);					
}
//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacSender(void *argument)
{
	char * msg;
	char tokenTag = TOKEN_TAG;
	osStatus_t	retCode; //Message de retour de la queue
	struct queueMsg_t queueMsg;	// message queue
	for (;;){
		//----------------------------------------------------------------------------
		// QUEUE READ										
		//----------------------------------------------------------------------------
		retCode = osMessageQueueGet( 	
			queue_macS_id,
			&queueMsg,
			NULL,
			osWaitForever); 	
    CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
		
		
		switch(queueMsg.type){
			case NEW_TOKEN:
				sendToken(retCode, &queueMsg, &tokenTag, msg);
				break;
			
			default :
				
		}
	}
}