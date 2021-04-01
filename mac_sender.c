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
void sendTokenList(osStatus_t	retCode,	struct queueMsg_t* queueMsg,	char * msg);
void sendToken(osStatus_t	retCode,	struct queueMsg_t* queueMsg,	char * msg);
void receiveToken(osStatus_t	retCode,	struct queueMsg_t* queueMsg,	char * msg);


void receiveToken(osStatus_t	retCode,	struct queueMsg_t* queueMsg,	char * msg){
	//Check si on a des trucs dans la queue
	//Et on envoie normalement
	//Send to Physical
	msg = queueMsg->anyPtr;
	for (int i = 0; i < TOKENSIZE-1; i++){
		gTokenInterface.station_list[i] = msg[i+1];
	}
	sendToken(retCode, queueMsg, msg);
	sendTokenList(retCode, queueMsg, msg);
}

void sendToken(osStatus_t	retCode,	struct queueMsg_t* queueMsg,	char * msg){
	//Send to LCD TOKEN_LIST
	
	
	//Send to Physical
	
	//------------------------------------------------------------------------
	// MEMORY ALLOCATION				
	//------------------------------------------------------------------------
	msg = osMemoryPoolAlloc(memPool,osWaitForever);
	memset(msg, 0x00, TOKENSIZE);
	msg[0] = TOKEN_TAG;
	msg[1+MYADDRESS] = CHAT_SAPI; //Set station sapi in token frame
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
void sendTokenList(osStatus_t	retCode,	struct queueMsg_t* queueMsg,	char * msg){
		//------------------------------------------------------------------------
	// MEMORY ALLOCATION				
	//------------------------------------------------------------------------
	msg = osMemoryPoolAlloc(memPool,osWaitForever);
	memset(msg, 0x00, TOKENSIZE);
	queueMsg->type = TOKEN_LIST;
	queueMsg->anyPtr = msg;
		//------------------------------------------------------------------------
		// QUEUE SEND								
		//------------------------------------------------------------------------
		retCode = osMessageQueuePut(
			queue_lcd_id,
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
				sendToken(retCode, &queueMsg, msg);
				break;
			case TOKEN:
				receiveToken(retCode, &queueMsg, msg);
				break;
			default :
				
		}
	}
}
