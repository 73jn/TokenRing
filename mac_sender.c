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
	//Check si on a des trucs dans la queue
	//Et on envoie normalement
	//Send to Physical
	
	//------------------------------------------------------------------------
	// MEMORY ALLOCATION				
	//------------------------------------------------------------------------
		msg = osMemoryPoolAlloc(memPool,osWaitForever);
		memset(msg, 0x00, TOKENSIZE);
		msg[0] = TOKEN_TAG;
		queueMsg->type = TO_PHY;
		queueMsg->anyPtr = msg;
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

void sendNewToken(osStatus_t	retCode,	struct queueMsg_t* queueMsg, char* tokenTag,	char * msg){
	//Send to LCD TOKEN_LIST
	
	
	//Send to Physical
	
	//------------------------------------------------------------------------
	// MEMORY ALLOCATION				
	//------------------------------------------------------------------------
	msg = osMemoryPoolAlloc(memPool,osWaitForever);
	memset(msg, 0x00, TOKENSIZE);
	msg[0] = TOKEN_TAG;
	queueMsg->type = TO_PHY;
	queueMsg->anyPtr = msg;
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
				sendNewToken(retCode, &queueMsg, &tokenTag, msg);
				break;
			case TOKEN:
				sendToken(retCode, &queueMsg, &tokenTag, msg);
				break;
			default :
				
		}
	}
}
