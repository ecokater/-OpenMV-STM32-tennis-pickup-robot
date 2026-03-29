#ifndef __LED_H
#define __LED_H

/*------------------------------------------ LED���ú� ----------------------------------*/

#define LED1_PIN            			 GPIO_PIN_13        				 	// LED1 ����      
#define LED1_PORT           			 GPIOC                 			 	// LED1 GPIO�˿�     
#define __HAL_RCC_LED1_CLK_ENABLE    __HAL_RCC_GPIOC_CLK_ENABLE() 	// LED1 GPIO�˿�ʱ��
 

  
/*----------------------------------------- LED���ƺ� ----------------------------------*/
						
#define LED1_ON 	  	HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_RESET)		// ����͵�ƽ������LED1	
#define LED1_OFF 	  	HAL_GPIO_WritePin(LED1_PORT, LED1_PIN, GPIO_PIN_SET)			// ����ߵ�ƽ���ر�LED1	
#define LED1_Toggle	    HAL_GPIO_TogglePin(LED1_PORT,LED1_PIN);							// ��תIO��״̬
			
/*---------------------------------------- �������� ------------------------------------*/

void LED_Init(void);

#endif //__LED_H


