#include "mysys.h"
#include "main.h"
void mysysinit(void)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  PeriphClkInitStruct.PLL3.PLL3M = 25;
  PeriphClkInitStruct.PLL3.PLL3N = 330;
  PeriphClkInitStruct.PLL3.PLL3P = 2;
  PeriphClkInitStruct.PLL3.PLL3Q = 2;
  PeriphClkInitStruct.PLL3.PLL3R = 10;
  PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_0;
  PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOMEDIUM;
  PeriphClkInitStruct.PLL3.PLL3FRACN = 0;

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC|RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_FMC;               
  PeriphClkInitStruct.FmcClockSelection = RCC_FMCCLKSOURCE_D1HCLK;
  PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;

  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  } 

}