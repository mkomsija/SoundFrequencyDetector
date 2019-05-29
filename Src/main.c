/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Function and handles that need to be globaly available */
static void Error_Handler(void);

static void TIM_Config(void);
static void ADC_Config(void);
static void UART_Config(void);
static void SPI_Config(void);
static void GPIO_Config(void);
void SPI_Transmit(uint8_t toSend);
/* SPI handler declaration */
SPI_HandleTypeDef SpiHandle;

/*TIM handler declaration*/
TIM_HandleTypeDef 	tim_adc;

/* ADC handler declaration */
ADC_HandleTypeDef    AdcHandle;

/* UART handler declaration */
UART_HandleTypeDef UartHandle;


/* Used for printf functionality */
/* Private function prototypes -----------------------------------------------*/
#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

extern const uint16_t frequencies[77];
extern const uint8_t notes[77][4];


/* Variable used to get converted value */
__IO uint16_t uhADCxConvertedValue = 0;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void CPU_CACHE_Enable(void);

__IO uint16_t ADCConvertedValues[ONESECOND] = {0}; 

int16_t inputSignal[ONESECOND] = {0};
int16_t filteredSignal[4096] = {0};
int16_t fft[8192] = {0};
int16_t fftMag[4096] = {0};

/*Global variable to communicate info from DMA ISR*/
__IO uint8_t OneSec = 0;


const int16_t firCoeffs[NUM_TAPS] = {	
	0xFFDA,
	0xFFE3,
	0xFFF2,
	0x0016,
	0x0064,
	0x00EB,
	0x01B9,
	0x02CE,
	0x0425,
	0x05A9,
	0x073F,
	0x08C3,
	0x0A0F,
	0x0B03,
	0x0B83,
	0x0B83,
	0x0B03,
	0x0A0F,
	0x08C3,
	0x073F,
	0x05A9,
	0x0425,
	0x02CE,
	0x01B9,
	0x00EB,
	0x0064,
	0x0016,
	0xFFF2,
	0xFFE3,
	0xFFDA
};

uint8_t foundNoteIdx = 0;
/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void)
{
	
	int32_t i = 0;
	
	arm_fir_decimate_instance_q15  S;
	arm_rfft_instance_q15 K;
	int16_t firState[BLOCK_SIZE + NUM_TAPS - 1];
	uint32_t blockSize = BLOCK_SIZE;
	uint16_t numBlocks = 170;
  int32_t possibleMax = 0;
	int32_t max = 0;
	uint16_t foundFreq = 0;
	
  /* Enable the CPU Cache */
  CPU_CACHE_Enable();
	
  /* STM32F7xx HAL library initialization:
       - Configure the Flash ART accelerator
       - Systick timer is configured by default as source of time base, but user 
         can eventually implement his proper time base source (a general purpose 
         timer for example or other time source), keeping in mind that Time base 
         duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and 
         handled in milliseconds basis.
       - Set NVIC Group Priority to 4
       - Low Level Initialization
     */
  HAL_Init();

  /* Configure the system clock to 216 MHz */
  SystemClock_Config();

  /* Configure LED1 */
  BSP_LED_Init(LED1);

	/* Configure sampling timer */
	TIM_Config();
	
	/* Configure the ADC peripheral */
	ADC_Config();

	/* Configure aditional OLED GPIOs */
	GPIO_Config();
	
	/* Configure the SPI peripheral */
	SPI_Config();
	
	/* Configure UART for printf*/
	UART_Config();
	
		

	/* Call FIR init function to initialize the instance structure. */
  arm_fir_decimate_init_q15(&S, NUM_TAPS, M, firCoeffs, firState, blockSize);
	
	arm_rfft_init_q15(&K, 4096,0,0);
	
  /*##-3- Start the conversion process #######################################*/
  if(HAL_ADC_Start_DMA(&AdcHandle, (uint32_t*)&ADCConvertedValues, ONESECOND*2) != HAL_OK)
  {
    /* Start Conversation Error */
    Error_Handler();
  }

	if (HAL_TIM_Base_Start(&tim_adc) != HAL_OK)
  {
    /* Counter Enable Error */
    Error_Handler();
  }
  
  OLED_Initialize();
	HAL_Delay(100);
	OLED_Clear();

  while (1)
  {
		foundFreq = 0;
		max = 0;
		possibleMax = 0;
		
		if (OneSec != 0)
		{
			for (i = ONESECOND-1; i >= 0; i--) 
			{
				inputSignal[i] = ADCConvertedValues[i] - 2048;
			}
			/* ----------------------------------------------------------------------
			** Call the FIR process function for every blockSize samples
			** ------------------------------------------------------------------- */

			for(i=0; i < numBlocks; i++)
			{
				arm_fir_decimate_fast_q15(&S, inputSignal + (i * blockSize), filteredSignal + (i * blockSize/M), blockSize);
			}
			
			for(i=0; i < 4096; i++)
			{
				fft[i*2] = filteredSignal[i]; 
			}
			
			arm_cfft_q15(&arm_cfft_sR_q15_len4096, fft, 0, 1);
			
			for (i = 0; i < 4096; i++) 
			{
				fftMag[i] = fft[2*i]^2 + fft[(2*i)+1]^2;
			}
			
			for (i = 100; i < 999; i++) 
			{
				//if (i == 199) continue;
				possibleMax = fftMag[i];
				if (possibleMax > max)
				{
					max = possibleMax;
					foundFreq = i;
				}
			}
			
			
			foundNoteIdx = findClosestNote(foundFreq-1);

			OLED_SetScale(2,4);

			for(i = 0; i < 3; i++)
			{
				
				if (notes[foundNoteIdx][i] == 9)
				{
					OLED_Putchar(' ');
				}
				else
				{
					OLED_Putchar(notes[foundNoteIdx][i]);
				}
					
			}
			OLED_SetScale(1,4);
			OLED_Putchar(' ');
			OLED_SetScale(2,4);
			OLED_Write3DigitNum(foundFreq-1);
			OLED_SetScale(1,4);

			OLED_SetScale(1,4);
			OLED_Putchar(' ');
			OLED_Putchar('H');
			OLED_Putchar('z');
			HAL_Delay(100);
					
		}
  }
}

PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART3 and Loop until the end of transmission */
  HAL_UART_Transmit(&UartHandle, (uint8_t *)&ch, 1, 0xFFFF);

  return ch;
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 216000000
  *            HCLK(Hz)                       = 216000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 8
  *            PLL_N                          = 432
  *            PLL_P                          = 2
  *            PLL_Q                          = 9
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 7
  * @param  None
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  
  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 432;  
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    while(1) {};
  }
  
  /* Activate the OverDrive to reach the 216 Mhz Frequency */
  if(HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    while(1) {};
  }
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    while(1) {};
  }
}
/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  while (1)
  {
    /* LED1 blinks */
    BSP_LED_Toggle(LED1);
    HAL_Delay(20);
  }
}

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

void TIM_Config(void)
{
	static TIM_MasterConfigTypeDef tim_trig;
	
	/*Configure timer*/
	tim_adc.Instance = TIMx;
	tim_adc.Init.Prescaler = 0;
	tim_adc.Init.Period = 5294;				// ~ 108MHz / 20400Hz
	tim_adc.Init.CounterMode = TIM_COUNTERMODE_UP;
	tim_adc.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	tim_adc.Init.RepetitionCounter = 0;
	tim_adc.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	
	if (HAL_TIM_Base_Init(&tim_adc) != HAL_OK)
  {
    /* Timer initialization Error */
    Error_Handler();
  }
	
	tim_trig.MasterOutputTrigger = TIM_TRGO_UPDATE;
	tim_trig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	
	if (HAL_TIMEx_MasterConfigSynchronization(&tim_adc, &tim_trig) != HAL_OK)
  {
    /* Timer TRGO selection Error */
    Error_Handler();
  }
}

void ADC_Config(void)
{
	ADC_ChannelConfTypeDef sConfig;
	
	/* ADC Initialization */
  AdcHandle.Instance          = ADCx;
  
  AdcHandle.Init.ClockPrescaler        = ADC_CLOCKPRESCALER_PCLK_DIV4;
  AdcHandle.Init.Resolution            = ADC_RESOLUTION_12B;
  AdcHandle.Init.ScanConvMode          = DISABLE;                       /* Sequencer disabled (ADC conversion on only 1 channel: channel set on rank 1) */
  AdcHandle.Init.ContinuousConvMode    = DISABLE;                       /* Continuous mode enabled to have continuous conversion  */
  AdcHandle.Init.DiscontinuousConvMode = DISABLE;                       /* Parameter discarded because sequencer is disabled */
  AdcHandle.Init.NbrOfDiscConversion   = 0;
	AdcHandle.Init.NbrOfConversion			 = 1;
  AdcHandle.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_RISING;        /* Conversion start trigged at each external event */
  AdcHandle.Init.ExternalTrigConv      = ADC_EXTERNALTRIGCONV_T1_TRGO;
  AdcHandle.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
  AdcHandle.Init.NbrOfConversion       = 1;
  AdcHandle.Init.DMAContinuousRequests = ENABLE;
  AdcHandle.Init.EOCSelection          = EOC_SINGLE_CONV;

  if (HAL_ADC_Init(&AdcHandle) != HAL_OK)
  {
    /* ADC initialization Error */
    Error_Handler();
  }

  /*##-2- Configure ADC regular channel ######################################*/
  sConfig.Channel      = ADCx_CHANNEL;
  sConfig.Rank         = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
  sConfig.Offset       = 0;

  if (HAL_ADC_ConfigChannel(&AdcHandle, &sConfig) != HAL_OK)
  {
    /* Channel Configuration Error */
    Error_Handler();
  }
}

void UART_Config(void)
{
	/*##-1- Configure the UART peripheral ######################################*/
  /* Put the USART peripheral in the Asynchronous mode (UART Mode) */
  /* UART configured as follows:
      - Word Length = 8 Bits (7 data bit + 1 parity bit) : 
	                  BE CAREFUL : Program 7 data bits + 1 parity bit in PC HyperTerminal
      - Stop Bit    = One Stop bit
      - Parity      = ODD parity
      - BaudRate    = 9600 baud
      - Hardware flow control disabled (RTS and CTS signals) */
  UartHandle.Instance        = USARTx;

  UartHandle.Init.BaudRate   = 9600;
  UartHandle.Init.WordLength = UART_WORDLENGTH_8B;
  UartHandle.Init.StopBits   = UART_STOPBITS_1;
  UartHandle.Init.Parity     = UART_PARITY_ODD;
  UartHandle.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
  UartHandle.Init.Mode       = UART_MODE_TX_RX;
  UartHandle.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&UartHandle) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
}

void SPI_Config(void)
{
	 /* Set the SPI parameters */
  SpiHandle.Instance               = SPI1;
	SpiHandle.Init.Mode							 = SPI_MODE_MASTER;
  SpiHandle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
  SpiHandle.Init.Direction         = SPI_DIRECTION_1LINE;
  SpiHandle.Init.CLKPhase          = SPI_PHASE_1EDGE;
  SpiHandle.Init.CLKPolarity       = SPI_POLARITY_LOW;
  SpiHandle.Init.DataSize          = SPI_DATASIZE_8BIT;
  SpiHandle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
  SpiHandle.Init.TIMode            = SPI_TIMODE_DISABLE;
  SpiHandle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
  SpiHandle.Init.CRCPolynomial     = 10;
  SpiHandle.Init.NSS               = SPI_NSS_SOFT;
	
	if(HAL_SPI_Init(&SpiHandle) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
}

/* Additional GPIOs needed for SPI communication with OLED and User Button*/
void GPIO_Config(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	
	OLED_CS_GPIO_CLK_ENABLE();
	OLED_DC_GPIO_CLK_ENABLE();
	OLED_RST_GPIO_CLK_ENABLE();
	
	HAL_GPIO_WritePin(OLED_CS_GPIO_PORT, OLED_CS_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(OLED_DC_GPIO_PORT, OLED_DC_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(OLED_RST_GPIO_PORT, OLED_RST_PIN, GPIO_PIN_RESET);
	
	GPIO_InitStruct.Pin = OLED_CS_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(OLED_CS_GPIO_PORT, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = OLED_DC_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(OLED_DC_GPIO_PORT, &GPIO_InitStruct);
	
	GPIO_InitStruct.Pin = OLED_RST_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(OLED_RST_GPIO_PORT, &GPIO_InitStruct);
}

void SPI_Transmit(uint8_t toSend)
{
	switch(HAL_SPI_Transmit(&SpiHandle, &toSend, 1, 1000))
  {
    case HAL_OK:
      break;

    case HAL_TIMEOUT:
      Error_Handler();
      break;
		
    case HAL_ERROR:
      Error_Handler();
      break;
		
    default:
      break;
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
