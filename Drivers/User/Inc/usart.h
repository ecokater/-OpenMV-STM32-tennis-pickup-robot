#ifndef __USART_H
#define __USART_H

#include "stdio.h"
#include "stm32h7xx_hal.h"

/*-------------------------------------------- USART���ú� ---------------------------------------*/

#define  USART1_BaudRate  115200

#define  USART1_TX_PIN									GPIO_PIN_9								// TX ����
#define	USART1_TX_PORT									GPIOA										// TX ���Ŷ˿�
#define 	GPIO_USART1_TX_CLK_ENABLE        	   __HAL_RCC_GPIOA_CLK_ENABLE()	 	// TX ����ʱ��


#define  USART1_RX_PIN									GPIO_PIN_10             			// RX ����
#define	USART1_RX_PORT									GPIOA                 				// RX ���Ŷ˿�
#define 	GPIO_USART1_RX_CLK_ENABLE         	   __HAL_RCC_GPIOA_CLK_ENABLE()		// RX ����ʱ��


/*---------------------------------------------- �������� ---------------------------------------*/

void    User_HAL_UART_MspInit(UART_HandleTypeDef* huart);		// USART1��ʼ������
void 	User_USART1_Init(void);			// USART1��ʼ��
int fputc(int ch, FILE *f);
#endif //__USART_H




