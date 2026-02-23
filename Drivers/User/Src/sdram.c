/***
	************************************************************************************************
	*	@version V1.0
	*  @date    2023-12-13
	*	@author  ���ͿƼ�	
   *************************************************************************************************
   *  @description
	*
	*	ʵ��ƽ̨������STM32H743XIH6���İ� ���ͺţ�FK743M5-XIH6��
	*	�Ա���ַ��https://shop212360197.taobao.com
	*	QQ����Ⱥ��536665479
	*
>>>>> �ļ�˵����
	*
	*  SDRAM��س�ʼ������
	*
	************************************************************************************************
***/

#include "sdram.h"   

extern SDRAM_HandleTypeDef hsdram1;
FMC_SDRAM_CommandTypeDef command;	// ����ָ��


/*************************************************************************************************
*	�� �� ��:	HAL_FMC_MspInit
*	��ڲ���:	��
*	�� �� ֵ:��
*	��������:	��ʼ��sdram����
*	˵    ��:
*************************************************************************************************/

static void User_HAL_FMC_MspInit(void)
{
  GPIO_InitTypeDef GPIO_InitStruct ={0};
  /* Peripheral clock enable */
  __HAL_RCC_FMC_CLK_ENABLE();

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();  
  __HAL_RCC_GPIOF_CLK_ENABLE();  
  __HAL_RCC_GPIOG_CLK_ENABLE();  
  __HAL_RCC_GPIOH_CLK_ENABLE();  
  
  /** FMC GPIO Configuration
  PF0   ------> FMC_A0			PD14   ------> FMC_D0			PC0    ------> FMC_SDNWE
  PF1   ------> FMC_A1        PD15   ------> FMC_D1         PG15   ------> FMC_SDNCAS 
  PF2   ------> FMC_A2        PD0    ------> FMC_D2         PF11   ------> FMC_SDNRAS
  PF3   ------> FMC_A3        PD1    ------> FMC_D3         PH3    ------> FMC_SDNE0  
  PF4   ------> FMC_A4        PE7    ------> FMC_D4         PG4    ------> FMC_BA0
  PF5   ------> FMC_A5        PE8    ------> FMC_D5         PG5    ------> FMC_BA1
  PF12  ------> FMC_A6       	PE9    ------> FMC_D6         PH2    ------> FMC_SDCKE0 
  PF13  ------> FMC_A7       	PE10   ------> FMC_D7         PG8    ------> FMC_SDCLK
  PF14  ------> FMC_A8       	PE11   ------> FMC_D8         PE1    ------> FMC_NBL1
  PF15  ------> FMC_A9       	PE12   ------> FMC_D9         PE0    ------> FMC_NBL0
  PG0   ------> FMC_A10       PE13   ------> FMC_D10
  PG1   ------> FMC_A11       PE14   ------> FMC_D11
  PG2   ------> FMC_A12       PE15   ------> FMC_D12
                              PD8    ------> FMC_D13
                              PD9    ------> FMC_D14
                              PD10   ------> FMC_D15

  */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3
                          |GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_11|GPIO_PIN_12
                          |GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_4
                          |GPIO_PIN_5|GPIO_PIN_8|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10
                          |GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15|GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_14
                          |GPIO_PIN_15|GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  GPIO_InitStruct.Pin =  GPIO_PIN_0;                    
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

/*************************************************************************************************
*	�� �� ��:	HAL_SDRAM_MspInit
*	��ڲ���:	hsdram - SDRAM_HandleTypeDef����ı���������ʾ�����sdram
*	�� �� ֵ:	��
*	��������:	��ʼ��sdram���ţ��ں��� HAL_SDRAM_Init �б�����
*	˵    ��:	��		
*************************************************************************************************/

void User_HAL_SDRAM_MspInit(SDRAM_HandleTypeDef* hsdram)
{
	User_HAL_FMC_MspInit();
}

/******************************************************************************************************
*	�� �� ��: SDRAM_Initialization_Sequence
*	��ڲ���: hsdram - SDRAM_HandleTypeDef����ı���������ʾ�����sdram
*				 Command	- ����ָ��
*	�� �� ֵ: ��
*	��������: SDRAM ��������
*	˵    ��: ����SDRAM���ʱ��Ϳ��Ʒ�ʽ
*******************************************************************************************************/

void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram, FMC_SDRAM_CommandTypeDef *Command)
{
	__IO uint32_t tmpmrd = 0;

	/* Configure a clock configuration enable command */
	Command->CommandMode 				= FMC_SDRAM_CMD_CLK_ENABLE;	// ����SDRAMʱ�� 
	Command->CommandTarget 				= FMC_COMMAND_TARGET_BANK; 	// ѡ��Ҫ���Ƶ�����
	Command->AutoRefreshNumber 		= 1;
	Command->ModeRegisterDefinition 	= 0;

	HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT);	// ���Ϳ���ָ��
	HAL_Delay(1);		// ��ʱ�ȴ�

	/* Configure a PALL (precharge all) command */ 
	Command->CommandMode 				= FMC_SDRAM_CMD_PALL;		// Ԥ�������
	Command->CommandTarget 				= FMC_COMMAND_TARGET_BANK;	// ѡ��Ҫ���Ƶ�����
	Command->AutoRefreshNumber 		= 1;
	Command->ModeRegisterDefinition 	= 0;

	HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT);  // ���Ϳ���ָ��

	/* Configure a Auto-Refresh command */ 
	Command->CommandMode 				= FMC_SDRAM_CMD_AUTOREFRESH_MODE;	// ʹ���Զ�ˢ��
	Command->CommandTarget 				= FMC_COMMAND_TARGET_BANK;          // ѡ��Ҫ���Ƶ�����
	Command->AutoRefreshNumber			= 8;                                // �Զ�ˢ�´���
	Command->ModeRegisterDefinition 	= 0;

	HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT);	// ���Ϳ���ָ��

	/* Program the external memory mode register */
	tmpmrd = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_2         |
							SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL   |
							SDRAM_MODEREG_CAS_LATENCY_3           |
							SDRAM_MODEREG_OPERATING_MODE_STANDARD |
							SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

	Command->CommandMode					= FMC_SDRAM_CMD_LOAD_MODE;	// ����ģʽ�Ĵ�������
	Command->CommandTarget 				= FMC_COMMAND_TARGET_BANK;	// ѡ��Ҫ���Ƶ�����
	Command->AutoRefreshNumber 		= 1;
	Command->ModeRegisterDefinition 	= tmpmrd;

	HAL_SDRAM_SendCommand(hsdram, Command, SDRAM_TIMEOUT);	// ���Ϳ���ָ��
	
	HAL_SDRAM_ProgramRefreshRate(hsdram, 918);  // ����ˢ����

}

/******************************************************************************************************
*	�� �� ��: MX_FMC_Init
*	��ڲ���: ��
*	�� �� ֵ: ��
*	��������: SDRAM��ʼ��
*	˵    ��: ��ʼ��FMC��SDRAM����
*******************************************************************************************************/

void User_MX_FMC_Init(void)
{
	FMC_SDRAM_TimingTypeDef SdramTiming = {0};

	hsdram1.Instance = FMC_SDRAM_DEVICE;
	/* hsdram1.Init */
	hsdram1.Init.SDBank					 	= FMC_SDRAM_BANK1;							// ѡ��BANK��
	hsdram1.Init.ColumnBitsNumber 		= FMC_SDRAM_COLUMN_BITS_NUM_9;         // �е�ַ����
	hsdram1.Init.RowBitsNumber 			= FMC_SDRAM_ROW_BITS_NUM_13;           // �е�ַ�߿���
	hsdram1.Init.MemoryDataWidth 			= FMC_SDRAM_MEM_BUS_WIDTH_16;          // ���ݿ���  
	hsdram1.Init.InternalBankNumber 		= FMC_SDRAM_INTERN_BANKS_NUM_4;        // bank����
	hsdram1.Init.CASLatency 				= FMC_SDRAM_CAS_LATENCY_3;             //	CAS 
	hsdram1.Init.WriteProtection 			= FMC_SDRAM_WRITE_PROTECTION_DISABLE;  // ��ֹд����
	hsdram1.Init.SDClockPeriod 			= FMC_SDRAM_CLOCK_PERIOD_2;            // ��Ƶ
	hsdram1.Init.ReadBurst 					= FMC_SDRAM_RBURST_ENABLE;             // ͻ��ģʽ  
	hsdram1.Init.ReadPipeDelay 			= FMC_SDRAM_RPIPE_DELAY_1;             // ���ӳ�
	
	/* SdramTiming */
	SdramTiming.LoadToActiveDelay 		= 2;
	SdramTiming.ExitSelfRefreshDelay 	= 7;
	SdramTiming.SelfRefreshTime 			= 4;
	SdramTiming.RowCycleDelay 				= 7;
	SdramTiming.WriteRecoveryTime 		= 3;
	SdramTiming.RPDelay 						= 2;
	SdramTiming.RCDDelay 					= 2;

	HAL_SDRAM_Init(&hsdram1, &SdramTiming);	// ��ʼ��FMC�ӿ�
									
	SDRAM_Initialization_Sequence(&hsdram1,&command);//����SDRAM
}
