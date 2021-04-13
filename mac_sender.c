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

	
//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacSender(void *argument)
{
	osMessageQueueId_t queue_macSdata_id = osMessageQueueNew(2,sizeof(struct queueMsg_t),NULL); 	
	bool hasChanged = false;
	char * msg;
	osStatus_t	retCode; //Message de retour de la queue
	struct queueMsg_t queueMsg;	// message queue
	struct queueMsg_t queueData;
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
				//Create new message
				msg = osMemoryPoolAlloc(memPool,osWaitForever);
				memset(msg, 0x00, TOKENSIZE);
				msg[0] = TOKEN_TAG;
				msg[1+gTokenInterface.myAddress] = 1 << CHAT_SAPI; //Set station sapi in token frame
				gTokenInterface.station_list[gTokenInterface.myAddress] = 1 << CHAT_SAPI;
				queueMsg.type = TO_PHY;
				queueMsg.anyPtr = msg;
				//------------------------------------------------------------------------
				// QUEUE SEND								
				//------------------------------------------------------------------------
				retCode = osMessageQueuePut(
					queue_phyS_id,
					&queueMsg,
					osPriorityNormal,
					osWaitForever);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
				break;
			case TOKEN:
				/////////////////////////////////////////////
				// TOKEN LIST
				///////////////////////////////////////////
				msg = queueMsg.anyPtr;//Get the token message
				for (int i = 0; i < TOKENSIZE-1; i++){ //On mets a jour la station list avec le msg recu
					if (gTokenInterface.station_list[i] != msg[i+1]){
						gTokenInterface.station_list[i] = msg[i+1];
						hasChanged = true;
					}
				}
				//We put our sapi into the station list
				gTokenInterface.station_list[gTokenInterface.myAddress]= 1 << CHAT_SAPI;
				//If changes, we send to the token list
				if (hasChanged){
					queueMsg.type = TOKEN_LIST;
					//------------------------------------------------------------------------
					// QUEUE SEND								
					//------------------------------------------------------------------------
					retCode = osMessageQueuePut(
						queue_lcd_id,
						&queueMsg,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
					hasChanged = false;
				}

				//Check if there is something into the data queue
				/////////////////////////////////////////////////////////////////////////
				// DATA CHECKIIIIIIIIIING
				//////////////////////////////////////////////////////////////////////////
				//----------------------------------------------------------------------------
				// QUEUE READ										
				//----------------------------------------------------------------------------
				retCode = osMessageQueueGet( 	
					queue_macSdata_id,
					&queueData,
					NULL,
					0); 	
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
				if (retCode != 0){  // retCode != osOk
					//We send the token back if nothing is in the queue
					msg[0] = TOKEN_TAG;
					for (int i = 0; i < TOKENSIZE-1; i++){
						msg[i+1] = gTokenInterface.station_list[i];
					}
					
					queueMsg.type = TO_PHY;
					queueMsg.anyPtr = msg;
					//------------------------------------------------------------------------
					// QUEUE SEND								
					//------------------------------------------------------------------------
					retCode = osMessageQueuePut(
						queue_phyS_id,
						&queueMsg,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
				} else {
					//We send the data and keep the token until databack
					queueData.type = TO_PHY;
					//------------------------------------------------------------------------
					// QUEUE SEND								
					//------------------------------------------------------------------------
					retCode = osMessageQueuePut(
						queue_phyS_id,
						&queueData,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
				}
				
				break;
			case START:
				
				break;
			case DATA_IND:
				//We put the data into the queue
				retCode = osMessageQueuePut(
					queue_macSdata_id,
					&queueMsg,
					osPriorityNormal,
					osWaitForever);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);

				break;
			default :
				
		}
	}
}
