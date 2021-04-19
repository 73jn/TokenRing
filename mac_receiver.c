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


void putTokenOnQueue(struct queueMsg_t msg){
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

uint8_t calculateCRC(uint8_t* data){
	uint8_t i = 0;
	uint8_t temp = 0;
	do{
		temp += data[i];
		i++;
	}while(data[i] != '\0');
	
	return temp;
}

//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacReceiver(void *argument)
{
	struct queueMsg_t queueMsg;		// queue message
	uint8_t * msg;
	uint8_t * qPtr;
	size_t	size;
	
	
	uint8_t src_addr;
	uint8_t dst_addr;
	uint8_t* status;
	uint8_t msg_length;
	
	
	for(;;){
		do{
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
			
			if(qPtr[0] == TOKEN_TAG){
				putTokenOnQueue(queueMsg);			
			}
			else{
				//get msg from frame
				msg = &qPtr[3];		//pointer to first char of msg
				//get length from frame
				msg_length = qPtr[2];
				//get status from frame
				status = msg + msg_length;
			
				if(src_addr == MYADDRESS){
					//databack
					if(status[BIT_READ] == 1){	//READ bit = 1
						if(status[BIT_ACK] == 1){	//ACK bit = 1
							//put TOKEN in mac sender queue
							putTokenOnQueue(queueMsg);
						}
						else{	//ACK bit = 0
							//reset READ and ACK bits
							status[BIT_ACK] = 0;
							status[BIT_READ] = 0;
							//put data in mac sender queue 
							putInQueue(DATABACK, queueMsg, queue_macS_id);
							//put token frame in queue_macS_id
							
						}
					}
					else{	//READ bit = 0
						//MAC_ERROR_INDICATION
						
						//put TOKEN in mac sender queue
						putTokenOnQueue(queueMsg);
					}
				}
				else{
					if(dst_addr == MYADDRESS){
						//compute CRC
						if(calculateCRC(msg) == (uint8_t)(&status)&&0xFC){
							//set READ and ACK bits
							status[BIT_ACK] = 1;
							status[BIT_READ] = 1;
							//put message in chat queue + MAC_DATA_INDICATION (?)
							putInQueue(DATA_IND, queueMsg, queue_chatR_id);
							
						}
						else{
							//set READ bit only (ACK = 0)
							status[BIT_ACK] = 0;
							status[BIT_READ] = 1;
							//resend data to sender
							putInQueue(TO_PHY, queueMsg, queue_phyS_id);							
						}
					}
					else{
						//broadcast data
					}
				}
			}
		}while(osMessageQueueGetCount(queue_macR_id) != 0);	
		
		//----------------------------------------------------------------------------
		// MEMORY RELEASE	(created frame : phy layer style)
		//----------------------------------------------------------------------------
		/*
		retCode = osMemoryPoolFree(memPool,qPtr);
		CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
		*/
	}
}