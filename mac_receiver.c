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


#define BIT_ACK 	0
#define BIT_READ 	1

osStatus_t retCode;


void putTokenInQueue(struct queueMsg_t msg){
	//change type of message before dispatch it
	msg.type = TOKEN;
	//put token frame in queue_macS_id
	retCode = osMessageQueuePut(
	queue_macS_id,
	&msg,
	osPriorityNormal,
	osWaitForever);	//blocking

	CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
}

void putInQueue(enum msgType_e type, struct queueMsg_t msg, osMessageQueueId_t queue){
	//change type of message before dispatch it
	msg.type = type;
	//put token frame in queue_macS_id
	retCode = osMessageQueuePut(
	queue,
	&msg,
	osPriorityNormal,
	osWaitForever);	//blocking

	CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
}

//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacReceiver(void *argument)
{
	struct queueMsg_t queueMsg;		// queue message
	struct queueMsg_t internMsg; 	// msg to send to chat and time queues
	struct queueMsg_t externMsg;	// msg to send to mac sender
	uint8_t * msg;
	uint8_t * qPtr;
	size_t	size;
	uint8_t* temp_str;
	
	uint8_t src_addr;
	uint8_t dst_addr;
	uint8_t* status;
	uint8_t msg_length;
	uint8_t sapi;

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
				
			//get frame
			qPtr = queueMsg.anyPtr;
			//get source address
			src_addr = qPtr[0]>>3;	
			//get dest address
			dst_addr = qPtr[1]>>3;		
			//get sapi
			sapi = qPtr[1]&0x7;
		
			if(qPtr[0] == TOKEN_TAG){
				putTokenInQueue(queueMsg);			
			}
			else{
				//get msg from frame
				msg = &qPtr[3];		//pointer to first char of msg
				//get length from frame
				msg_length = qPtr[2];
				//get status from frame
				status = msg + msg_length;
			
				if(src_addr == MYADDRESS){	//source = this station
					//databack
					//put data in mac sender queue 
					externMsg.anyPtr = qPtr;
					externMsg.type = DATABACK;
					externMsg.sapi = sapi;	
					externMsg.addr = src_addr;
					retCode = osMessageQueuePut(
						queue_macS_id,
						&externMsg,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
				} else {
					//to phy
					//put data un mac sender queues
					externMsg.anyPtr = qPtr;
					externMsg.type = TO_PHY;
					retCode = osMessageQueuePut(
						queue_macS_id,
						&externMsg,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
				}
				
				if(dst_addr == MYADDRESS || dst_addr == BROADCAST_ADDRESS){	//destination = this station or broadcast
						//compute CRC
						uint8_t checksum = 0;
						for(uint8_t i=0;i<(qPtr[2]+3);i++)	// calculate checksum of data + header + length
							checksum += qPtr[i];
						
						if((checksum&0x3F) == (*status >> 2)){
							// CRC is valid
							//set READ and ACK bits
							*status |= 0x3;
							
							switch(sapi){
								case CHAT_SAPI :
									temp_str = osMemoryPoolAlloc(memPool,osWaitForever);
									memcpy(temp_str,msg,msg_length);	//save message
									temp_str[msg_length] = '\0';
								
									internMsg.type = DATA_IND;			
									internMsg.anyPtr = temp_str;
									internMsg.sapi = sapi;	
									internMsg.addr = src_addr;
								
									retCode = osMessageQueuePut(		//put in queue_chatR_id
										queue_chatR_id,	
										&internMsg,
										osPriorityNormal,
										osWaitForever);
									CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
									break;
									
								case TIME_SAPI : 
									temp_str = osMemoryPoolAlloc(memPool,osWaitForever);	
									memcpy(temp_str, msg, msg_length);	//save message
									temp_str[msg_length] = '\0';
								
									internMsg.type = DATA_IND;			
									internMsg.anyPtr = temp_str;
									internMsg.sapi = sapi;	
									internMsg.addr = src_addr;
								
									retCode = osMessageQueuePut(		//put in queue_timeR_id
										queue_timeR_id,
										&internMsg,
										osPriorityNormal,
										osWaitForever);
									CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);		
									break;
								
								default : 
									//nothing
									break;
								
							} // SWITCH
						} else{	//CRC is not valid
							//set READ bit only (ACK = 0)
							*status |=  0x2;						
						} 	// ELSE CRC NOT VALID
					} 		// IF DESTINATION IS THIS STATION
			}					//ELSE NOT TOKEN TAG
	}
}