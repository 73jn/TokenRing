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

#define BUFFER_SIZE	10
#define MAX_ERROR 3

//////////////////////////////////////////////////////////////////////////////////
// THREAD MAC RECEIVER
//////////////////////////////////////////////////////////////////////////////////
void MacSender(void *argument)
{
	uint8_t * saved_token;
	uint8_t * saved_msg;
	
	
	struct queueMsg_t queueMsg;
	struct queueMsg_t queueMsgLCD;
	
	osStatus_t retCode;
	uint8_t* qPtr;
	uint8_t* msg;
	
	uint8_t error_counter = 0;
	
	osMessageQueueId_t queue_buffer = osMessageQueueNew(BUFFER_SIZE, sizeof(struct queueMsg_t), NULL);
	
	//init sapi
	gTokenInterface.station_list[MYADDRESS] = ((1 << TIME_SAPI) + (1 << CHAT_SAPI)); //place '1' at sapi addresses
	
	for(;;){	//infinite loop
		retCode = osMessageQueueGet(
			queue_macS_id,
			&queueMsg,
			NULL,
			osWaitForever);
		CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
		qPtr = queueMsg.anyPtr;
		
		switch(queueMsg.type){
			case TOKEN :
				for(int i = 0; i < TOKENSIZE-4; i++){
					if(i == MYADDRESS) {
						qPtr[i+1] = gTokenInterface.station_list[i];
					}
					else {
						gTokenInterface.station_list[i] = qPtr[i+1];	//Update Ready list
					}
					
				}
				
				queueMsg.type = TO_PHY;	//token destination
				queueMsg.anyPtr = qPtr;	//token data
				
				queueMsgLCD.type = TOKEN_LIST;	//For the LCD
				queueMsgLCD.anyPtr = NULL;
				
				retCode = osMessageQueuePut(
					queue_lcd_id,
					&queueMsgLCD,
					osPriorityNormal,
					osWaitForever);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);

				if(osMessageQueueGetCount(queue_buffer) != 0){	//If we have msg to send
					saved_token = qPtr;					
					retCode = osMessageQueueGet(	//Take the msg from waiting queue
						queue_buffer,
						&queueMsg,
						NULL,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
					
					saved_msg = queueMsg.anyPtr;	//Save for DATABACK
					
					queueMsg.anyPtr = osMemoryPoolAlloc(memPool, osWaitForever); //send a copy and keep the original
					memcpy(queueMsg.anyPtr, saved_msg, saved_msg[2]+4);
					
					//Send the msg to PHY_SENDER
					retCode = osMessageQueuePut(	
						queue_phyS_id,
						&queueMsg,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
				}
				else{	//Si la queue des messages est vide
					retCode = osMessageQueuePut(	//Send le token
						queue_phyS_id,
						&queueMsg,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
				}
				
				break;
				
			case DATABACK : //data sended coming back
				uint8_t ack = qPtr[3+qPtr[2]]&0x01;
				uint8_t read = qPtr[3+qPtr[2]]&0x02;
				if(read != 0){		//Test if Read bit is 1
					if(ack != 0){		//Test if Ack bit is 1
						error_counter = 0;
						
						//Free memory msg databack
						retCode = osMemoryPoolFree(memPool, queueMsg.anyPtr);				//FREE DATABACK
						CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
						
						//Free memory msg sent
						retCode = osMemoryPoolFree(memPool, saved_msg);				//FREE DATABACK
						CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
						
						//Send token
						queueMsg.type = TO_PHY;
						queueMsg.anyPtr = saved_token;
						retCode = osMessageQueuePut(	
							queue_phyS_id,
							&queueMsg,
							osPriorityNormal,
							osWaitForever);
						CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);											
					}
					else{
						error_counter++;
						
						if(error_counter < MAX_ERROR) {
							//send msg again
							memcpy(queueMsg.anyPtr, saved_msg, saved_msg[2]+4);
							queueMsg.type = TO_PHY;
							
							//Send the msg to PHY_SENDER
							retCode = osMessageQueuePut(	
								queue_phyS_id,
								&queueMsg,
								osPriorityNormal,
								osWaitForever);
							CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
						}
						else { 
							//abort msg
							error_counter = 0;
							
							struct queueMsg_t mac_error;
							mac_error.type = MAC_ERROR;
							mac_error.anyPtr = osMemoryPoolAlloc(memPool, osWaitForever);
							sprintf(mac_error.anyPtr, "Address %d : crc error.\n\r", (saved_msg[1]>>3)+1);
							mac_error.addr = queueMsg.addr;
							
							//send mac error
							retCode = osMessageQueuePut(	
								queue_lcd_id,
								&mac_error,
								osPriorityNormal,
								osWaitForever);
							CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
							
							//Free memory msg databack
							retCode = osMemoryPoolFree(memPool, queueMsg.anyPtr);
							CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
							
							//Free memory msg saved
							retCode = osMemoryPoolFree(memPool, saved_msg);
							CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
							
							//Send token
							queueMsg.type = TO_PHY;
							queueMsg.anyPtr = saved_token;
							retCode = osMessageQueuePut(	
								queue_phyS_id,
								&queueMsg,
								osPriorityNormal,
								osWaitForever);
							CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
							
						}						
					}
				}
				else {
					//Send MAC_ERROR
					error_counter = 0;
					
					struct queueMsg_t mac_error;
					mac_error.type = MAC_ERROR;
					mac_error.anyPtr = osMemoryPoolAlloc(memPool, osWaitForever);
					sprintf(mac_error.anyPtr, "Address %d not connected.\n\r", (saved_msg[1]>>3)+1);
					mac_error.addr = queueMsg.addr;
					
					//Free memory msg databack
					retCode = osMemoryPoolFree(memPool, queueMsg.anyPtr);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
					
					//Free memory msg saved
					retCode = osMemoryPoolFree(memPool, saved_msg);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
					
					//send mac error
					retCode = osMessageQueuePut(	
						queue_lcd_id,
						&mac_error,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
					
					//Send token TO_PHY
					queueMsg.type = TO_PHY;
					queueMsg.anyPtr = saved_token;
					retCode = osMessageQueuePut(	
						queue_phyS_id,
						&queueMsg,
						osPriorityNormal,
						osWaitForever);
					CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);	
				}
				break;
				
			case NEW_TOKEN : 
				//creat a new token
				msg = osMemoryPoolAlloc(memPool,osWaitForever);		//MALLOC for the new token
				msg[0] = 0xFF;
				for(int i = 0; i < TOKENSIZE-3; i++){
					msg[i+1] = 0x00;
				}
				queueMsg.anyPtr = msg;
				queueMsg.type = TO_PHY;
				
				retCode = osMessageQueuePut(
					queue_phyS_id,
					&queueMsg,
					osPriorityNormal,
					osWaitForever);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);			
				break;
				
			case START :
				gTokenInterface.station_list[MYADDRESS] = ((1 << TIME_SAPI) + (1 << CHAT_SAPI));
				break;
			
			case STOP :
				gTokenInterface.station_list[MYADDRESS] = (1 << TIME_SAPI);
				break;
			
			case DATA_IND : 
				msg = osMemoryPoolAlloc(memPool, osWaitForever);	
				msg[0] = (MYADDRESS << 3) + queueMsg.sapi;			
				msg[1] = (queueMsg.addr << 3) + queueMsg.sapi;	
				msg[2] = strlen(qPtr);														//Length
				memcpy(&(msg[3]), qPtr, msg[2]);									//User Data
			
				//compute checksum
				uint32_t check = msg[0]+msg[1]+msg[2];
				for(int i=0;i<msg[2];i++) {
					check += msg[3+i];
				}
				
				msg[3+msg[2]] = (check << 2);									//Status [Checksum, Read, Ack]
							
				retCode = osMemoryPoolFree(memPool, queueMsg.anyPtr);				//FREE DATA_IND
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
				
				queueMsg.type = TO_PHY;
				queueMsg.anyPtr = msg;
				
				retCode = osMessageQueuePut(
					queue_buffer,
					&queueMsg,	
					osPriorityNormal,
					osWaitForever);
				CheckRetCode(retCode,__LINE__,__FILE__,CONTINUE);
				
				break;
				
			default :
				break;
		}
	}
}


