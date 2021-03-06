/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************

  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f0xx_hal.h"

	/* --Lab 3 - PWM controller  */ //I have this setup for PC6 and PC7 getting Tim2 and Tim3 outputs.  Change to PBx GPIO pin
	/* --Lab 4 - USART for console control   Using PC10 and PC11*/
	/* --Lab 5 - I2C for Gyroscope    Using PB15 SDA    PB13 SCL  PC0 SPI/I2C mode   PB14 Slave addr of gyroscope*/
	/* Lab 8 Encoder Counter*/
	
/* Private function prototypes -----------------------------------------------*/
void clearString(char* stringVal,int n);
void SystemClock_Config(void);
void pwm_initialize(void);
void I2C2_Setup(void);
void USART3_Setup(void);
int16_t readGyro_Y(void);
uint16_t itoa(int16_t cNum, char *cString);
int16_t GyroToRPM(int16_t GyroData);
int16_t RPMToDutyCycle(int16_t RPM);
void transmitString(char* character);
void transmitChar(char);
uint8_t transmitComplete(void);
uint8_t GyroDataOutput(uint8_t , uint8_t);
void initiateTransaction(uint8_t Addr, uint8_t numBytes,uint8_t rd_wr);
uint8_t setupGyro(uint8_t Addr, uint8_t value);	
uint8_t readData(void);
int16_t read2byteData(void);
uint8_t transmitData(uint8_t data,uint8_t numBytes);
uint8_t transmitComplete(void);
uint16_t moveMotor(int16_t gyroY);
void updatePID(void);
void init_PID(void);
//static uint8_t transmitGyroData=1;
int16_t gyroY = 0;

int16_t GyroData = 0;
int RPM = 0;
int16_t dutyCycle = 0;
uint16_t count = 0;
char str[30] = ""; 
char str1[30] = "";
char str2[30] = "";

int16_t GyroCal = 0;
int16_t integral = 0;
int16_t control = 0;

int16_t kp = 1;
int16_t ki = 1;
int16_t kd = 1;

typedef int bool;
#define true 1
#define false 0
	
int16_t error = 0;
	
bool debug_pressed;

/**
  * @brief  The application entry point.
  *
  * @retval None
  */


uint8_t pc = 0;
int dif = 0;

int main(void)
{

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  /* Configure the system clock */
  SystemClock_Config();
  /* Enable the Peripheral Clocks*/
	RCC->AHBENR  |= RCC_AHBENR_GPIOCEN|RCC_AHBENR_GPIOBEN|RCC_AHBENR_GPIOAEN;   //enable GPIOC and GPIOB GPIOA Clock
	RCC->APB1ENR |= RCC_APB1ENR_USART3EN|RCC_APB1ENR_TIM3EN|RCC_APB1ENR_TIM2EN|RCC_APB1ENR_I2C2EN;	//enable USART3 clock Enable Timer 2 (TIM2EN) and Timer 3 (TIM3EN) registers  and I2C2 6.4.8 Peripheral Manual
	/* Set up PA0 */
	EXTI->IMR |= EXTI_IMR_IM0; //unmask interrupt on channel 0
	EXTI->RTSR |= EXTI_RTSR_RT0; //rising trigger enable on channel 0
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; //syscfg and clock enable
	SYSCFG->EXTICR[0] = SYSCFG_EXTICR1_EXTI0_PA; //set to pin 0 on channel A
	NVIC_EnableIRQ(EXTI0_1_IRQn);	
	NVIC_SetPriority(EXTI0_1_IRQn, 3);
	
	/* Set up USART3 */
	USART3_Setup();
	/* Setup PWM */
	pwm_initialize(); 
  /* I2C2 Setup */
	I2C2_Setup();
	HAL_Delay(100);
	/*Configure GPIOA Pins for Motor Direction*/
	GPIOA->MODER |= 1 << GPIO_MODER_MODER1_Pos | 1 << GPIO_MODER_MODER2_Pos; //set PC1 and PC2 to general purp OUtput
	/* Enable the Gyroscope */
	initiateTransaction(0x6B,2,0);
	uint8_t TXComplt = setupGyro(0x20,0x0B); // enable X and Y axis
  HAL_Delay(500);
		
	
	
	
  //Calibrate for the Gyro Offset
	
	GyroCal = 0;
	
	while(count<40)
	{
		GyroCal += readGyro_Y(); //get 20 samples to find Gyro Offset
		count++;
		itoa(GyroCal,str);
		transmitString(str);
	  transmitString("\n\r");
	}
		int16_t Offset = (int16_t) GyroCal/40;
    transmitString("Gyro Calibrated Offset = ");
	  itoa(Offset,str);
		transmitString(str);
		transmitString("\n\r");	
	
	int last_gyroY;
	init_PID();
	
	while (1)
  {
    //pc = 0; 
	  // ****** Read Y-AXIS ***** */
		last_gyroY = gyroY;
		
		//readGyro_Y();
		
		//pc++;
		if(last_gyroY == gyroY)
			HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);
		
		//GyroToRPM(gyroY);
		//updatePID();
		//dif = gyroY-last_gyroY;
		//dif = dif < 0? dif*-1:dif;
		//if(dif > 2000)
		//dutyCycle = RPMToDutyCycle(RPM);
		//moveMotor(control);
		//dutyCycle = moveMotor(gyroY);
		//HAL_Delay(1);
	} //end while loop
}

void init_PID()
{
	//int clock_speed = 8000000;
	RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
	TIM7->PSC = 1001;
	TIM7->ARR = 100;
	//T = ARR * (PSC - 1)/clock_speed 
	//125usec = 1 * 1000/8000000
  TIM7->DIER |= TIM_DIER_UIE;             // Enable update event interrupt
  TIM7->CR1 |= TIM_CR1_CEN;               // Enable Timer

  NVIC_EnableIRQ(TIM7_IRQn);          // Enable interrupt in NVIC
  NVIC_SetPriority(TIM7_IRQn,2);
}

void TIM7_IRQHandler(void) {
    /* Calculate the motor speed in raw encoder counts
     * Note the motor speed is signed! Motor can be run in reverse.
     * Speed is measured by how far the counter moved from center point
     */
	readGyro_Y();
	
  updatePID();
	
	moveMotor(control);

  TIM6->SR &= ~TIM_SR_UIF;        // Acknowledge the interrupt
}

void updatePID()
{
	int16_t derivative;
	int16_t lastError = error;
	error = gyroY;//+GyroCal;
	integral += error;
	derivative = error - lastError;
	
	control = (kp * error) + (ki * integral) + (kd * derivative);
	
	if (integral > 25000)  //cap the cumulative Gyro Motor  // can't run any faster than 180rpm or 1200dps
		integral = 25000;
		 
	if (integral < -25000) //cap the cumulative gyro  Motor // can't run any faster than 180rpm
		integral = -25000;
		
}
/*********End Project Main code *********************************************************************************************/
/**
  * @brief System Clock Configuration
  * @retval None
  */

uint16_t moveMotor(int16_t gyroY)
{
	int16_t duty_cycle = gyroY/300;
	static uint16_t d;
	
	
	if(duty_cycle<= 0 )
	{
		
		if (d != GPIO_PIN_6)
		{
			//if(dif > 10)
			//	HAL_Delay(20);
			GPIOC->ODR |= GPIO_PIN_6;
			GPIOC->ODR &= ~(GPIO_PIN_7);   //set the direction of motor
		  d = GPIO_PIN_6;
		}
	  duty_cycle = duty_cycle * (-1); //make the duty cycle a positive number
	}
	else
		if (d != GPIO_PIN_7 )
		{
			//if(dif > 10)
			//	HAL_Delay(20);
			GPIOC->ODR |= GPIO_PIN_7;
			GPIOC->ODR &= ~(GPIO_PIN_6);   //set the direction of motor
		  d = GPIO_PIN_7;
		}
	
	if(duty_cycle > 100) //cap the duty cycle at 100%
		duty_cycle = 100;
	if(duty_cycle  < 35) //ADJUST
		duty_cycle = 0;
	
	dutyCycle = duty_cycle;
	TIM3->CCR1  = duty_cycle; //set to duty Cycle of the CCR
	//HAL_Delay(50);
	return duty_cycle;
}

int previousDutyCycle = 0;

int16_t RPMToDutyCycle(int16_t RPM){
	int16_t DutyCycleTemp = RPM/50;//dividing by 1.6 because motor runs at 160 rpms
	previousDutyCycle = dutyCycle;
	static uint16_t direction; //if the RPM is negative turn on correct direction.
	
	
	//int dif = previousDutyCycle-DutyCycleTemp;
	//dif = dif < 0 ? dif*-1 : dif;
	
	if(DutyCycleTemp <= 0 )
	{
		
		if (direction != GPIO_PIN_6)
		{
			//if(dif > 10)
			//	HAL_Delay(20);
			GPIOC->ODR |= GPIO_PIN_6;
			GPIOC->ODR &= ~(GPIO_PIN_7);   //set the direction of motor
		  direction = GPIO_PIN_6;
		}
	  DutyCycleTemp = DutyCycleTemp * (-1); //make the duty cycle a positive number
	}
	else
		if (direction != GPIO_PIN_7 )
		{
			//if(dif > 10)
			//	HAL_Delay(20);
			GPIOC->ODR |= GPIO_PIN_7;
			GPIOC->ODR &= ~(GPIO_PIN_6);   //set the direction of motor
		  direction = GPIO_PIN_7;
		}
	
	if(DutyCycleTemp > 100) //cap the duty cycle at 100%
		DutyCycleTemp = 100;
	if(DutyCycleTemp < 25)
		DutyCycleTemp = 0;
	
	 TIM3->CCR1  = DutyCycleTemp; //set to duty Cycle of the CCR
	 //HAL_Delay(50);
	 return DutyCycleTemp;
	 
}

int16_t GyroToRPM(int16_t gyro_y_input)
{
	//if(gyro_y_input >= 0)
	RPM     = gyro_y_input/6;
//	else
	//	RPM			= ;
	return RPM;
}
void EXTI0_1_IRQHandler(void)
{
	debug_pressed = debug_pressed? false: true; //this syntax just means
	
	//value = conditional ? set_value_to_this_if_conditional_true : value_if_conditional_false;
	/*
	can also be written as
	if(debug_pressed)
		debug_pressed = false;
	else
		debug_pressed = true;
	*/
	HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_8);
	EXTI->PR |= EXTI_PR_PIF0; //shows that an interrupt happened so it can be cleared to show its been handled
	
}


uint8_t GyroDataOutput(uint8_t GetSet, uint8_t valueToSet){
	static uint8_t TxGyroData = 1;
	if(GetSet)//set value if GetSet = 1;
	 TxGyroData = valueToSet;
	return TxGyroData; 
}

const char pcDigits[] = "0123456789"; /* variable used by itoa function */
uint16_t itoa(int16_t cNum, char *cString)
{
    char* ptr;
    uint16_t uTemp = 0;
    uint16_t length = 0;
	  uint8_t neg = 0;
    
	  if (cNum < 0)
    {
       neg = 1;
			 cNum *= -1;
			length = 1;
    }
	uTemp = cNum;
	
        // Find out the length of the number, in decimal base
    
    while (uTemp > 0)
    {
        uTemp /= 10;
        length++;
    }

    // Do the actual formatting, right to left
    ptr = cString + length;

    uTemp = cNum;

    while (uTemp > 0)
    {
        --ptr;
        *ptr = pcDigits[uTemp % 10];
        uTemp /= 10;
    }
    if(neg){
			--ptr;
			*ptr = '-';
		}
		if(cNum == 0)  //if the number is 0 output a 0
			*ptr = pcDigits[0];
		
    return length;
}


int16_t readGyro_Y()
{		
		HAL_Delay(15);
		initiateTransaction(0x6B,1,0); //write transaction
	  transmitData(0xAA,1);
	  int16_t RxData;
		initiateTransaction(0x6B,2,1); //set read transaction
	  RxData = 0;
	  RxData = read2byteData();
		
		transmitComplete();
	
	  gyroY = RxData;
	    
		return gyroY;
}

void pwm_initialize(void){
 // Configure Timer2 //
	TIM2->PSC  = 8000-1;   /*Prescalar will divide by 8000 giving 1kHz clock and 1ms resolution 18.3.1 for explanation and 18.4.11 for register information Peripheral manual*/
	TIM2->ARR  = 500;      //Periph Man 18.4.3 TIMER2 count to 500ms 18.4.10 Peripheral Manual
	TIM2->DIER = 1;				 //Periph Man 18.4.4 Update interrupt enable
  TIM2->CR1  = 0x0081;   //Periph Man 18.4.1 Auto Reload not buffered; Edge-aligned; Downcounter; Counter not stop;Update enabled Counter enabled
	  // Configure Timer3 //
	TIM3->PSC  = 100-1;    //Set clock to 80kHz  
	TIM3->ARR  = 100;     // changes every 1.25ms
	  // setup timer 1 and timer 2 //
	TIM3->CCMR1= 0x6868;  // Set CC1S CC2S to output mode  Peripheral Man 18.4.7  OC1M PWM Mode2 OC2M Mode 2
	TIM3->CCMR2= 0;     
	TIM3->CCR1 = 0;			 //Set to 0% of ARR
	TIM3->CCR2 = 0;      //Set CCR to 0% of ARR
	TIM3->CCER = 0x0011;  //18.4.9 Set Channel 1 and Chan2 to output enable
  TIM3->CR1  = 0x0081;  //enable timer and 18.4.1 periph man
	  // Set outputs to Tim3_ch1 and TIM3_CH2 //
	//the cancelled out function is to set up PC6,7 to be the PWM outputs
  //GPIOC->AFR[0] |= GPIO_AF0_TIM3 << GPIO_AFRL_AFSEL6_Pos| GPIO_AF0_TIM3 << GPIO_AFRL_AFSEL7_Pos; //set PC6,7 to Alt function 0
	//GPIOC->MODER  |= (2 << GPIO_MODER_MODER6_Pos)|(2<<GPIO_MODER_MODER7_Pos)|(1<<GPIO_MODER_MODER8_Pos)|(1<<GPIO_MODER_MODER9_Pos);//Set LEDs PC6,7 alternate function , Set LEDs to general purpose PC8,9
	GPIOB->AFR[0] |= GPIO_AF1_TIM3 << GPIO_AFRL_AFSEL4_Pos| GPIO_AF1_TIM3 << GPIO_AFRL_AFSEL5_Pos; //set PB4,5 to Alt function 1
	GPIOB->MODER  |= 2<<GPIO_MODER_MODER4_Pos | 2<<GPIO_MODER_MODER5_Pos; //set PB4,5 to Alternate function mode
    // Configure Interrupt //
	NVIC_EnableIRQ(TIM2_IRQn);  		//Enable TIM2 interrupt Section 4.2 Core Programming M0 Manual
	NVIC_SetPriority(TIM2_IRQn,6);  //Configure EXTI0 interrupt priority Section 4.2.1 Core Prog M0 Manual
  /* End PWM Setup */
}
void I2C2_Setup(void){
  GPIOB->MODER  |= 2 << GPIO_MODER_MODER11_Pos | 2 << GPIO_MODER_MODER13_Pos; //Set PB11 PB13 to Alternate function mode
	GPIOB->MODER  |= 1 << GPIO_MODER_MODER14_Pos;  //Set PB14 to output mode
	GPIOB->OTYPER |= GPIO_OTYPER_OT_11|GPIO_OTYPER_OT_13; //PB11 and PB13 open-drain
	GPIOB->AFR[1] |= GPIO_AF1_I2C2 << GPIO_AFRH_AFSEL11_Pos | GPIO_AF5_I2C2 << GPIO_AFRH_AFSEL13_Pos; //PB11 and PB13 Alternate function I2C2
	GPIOC->MODER  |= 1|1 << GPIO_MODER_MODER6_Pos|1<<GPIO_MODER_MODER7_Pos|1<<GPIO_MODER_MODER8_Pos|1<<GPIO_MODER_MODER9_Pos; //PC0 to output mode LEDs PC6-9 to output
	GPIOC->ODR     = 1; //PC0 Set High for address on GYRO
	
	/* Configure I2C2 */
	I2C2->TIMINGR |= (1<<I2C_TIMINGR_PRESC_Pos)|(0x13 << I2C_TIMINGR_SCLL_Pos)|(0xF << I2C_TIMINGR_SCLH_Pos)|(0x2 << I2C_TIMINGR_SDADEL_Pos)|(0x4<<I2C_TIMINGR_SCLDEL_Pos); //Setting the timing according to Table 91
	I2C2->CR1      = I2C_CR1_PE;
	GPIOB->ODR     = GPIO_PIN_14; //configure address on Gyroscope
}	

void USART3_Setup(void){
  GPIOC->AFR[1] |= (GPIO_AF1_USART3 << GPIO_AFRH_AFSEL10_Pos)|(GPIO_AF1_USART3 << GPIO_AFRH_AFSEL11_Pos);  //Set PC10 and PC11 AFR registers to USART3
	GPIOC->MODER  |= (2 << GPIO_MODER_MODER10_Pos)|(2 << GPIO_MODER_MODER11_Pos); //PC10 USART3_TX and PC11 USART3_RX to Alternate function mode
	USART3->BRR  = HAL_RCC_GetHCLKFreq()/115200;
	USART3->CR1 |= USART_CR1_UE|USART_CR1_TE|USART_CR1_RE|USART_CR1_RXNEIE; //Enable UART,TX,RX and RXInterrupt
	  // Enable USART_Rx Interrupt //
	NVIC_EnableIRQ(USART3_4_IRQn);
	NVIC_SetPriority(USART3_4_IRQn,4); //set interrupt priority or USART
	/* End USART3 setup */
}

/* Code for I2C for Gyro */
void initiateTransaction(uint8_t Addr, uint8_t numBytes,uint8_t rd_wr)
{
	  I2C2->CR2 = 0; //Clear CR2
	  I2C2->CR2 |= Addr << 1; //write address
	  I2C2->CR2 |= numBytes << I2C_CR2_NBYTES_Pos; //num of bytes 
  	
	  if(rd_wr) //master read
			I2C2->CR2 |= I2C_CR2_RD_WRN; //assign a read
		else
	    I2C2->CR2 &= ~I2C_CR2_RD_WRN; //set to write transaction
		
		I2C2->CR2 |= I2C_CR2_START; //start transaction.
}

uint8_t setupGyro(uint8_t Addr, uint8_t value)
{
	 while(((I2C2->ISR & I2C_ISR_TXIS) != I2C_ISR_TXIS)){}//wait for TXIS ready (TX Reg empty)
	 I2C2->TXDR = Addr; //write to TX register
	 while(((I2C2->ISR & I2C_ISR_TXIS) != I2C_ISR_TXIS)){}//wait for TXIS ready (TX Reg empty)
	 I2C2->TXDR = value; 
	 while(((I2C2->ISR & I2C_ISR_TC) != I2C_ISR_TC)){} //wait for Transfer Complete
	 return 1;	
	
}
uint8_t readData(void)
{
    while(((I2C2->ISR & I2C_ISR_RXNE) != I2C_ISR_RXNE)){}
	  uint8_t RxData = I2C2->RXDR;
	  return RxData;
}
int16_t read2byteData(void)
{
	while(((I2C2->ISR & I2C_ISR_RXNE) != I2C_ISR_RXNE)){}
	uint8_t RxDataL = I2C2->RXDR;
	while(((I2C2->ISR & I2C_ISR_RXNE) != I2C_ISR_RXNE)){}
	uint8_t RxDataH = I2C2->RXDR;
		
	int16_t RxData = 0;
		RxData = RxDataH << 8 | RxDataL;
	return RxData;
}

uint8_t transmitData(uint8_t data,uint8_t numBytes)
{
	for(;numBytes>0;numBytes-=1)
	{
   while(((I2C2->ISR & I2C_ISR_TXIS) != I2C_ISR_TXIS)){}//wait for TXIS ready (TX Reg empty)
	 I2C2->TXDR = data; //write to TX register
	}
	while(((I2C2->ISR & I2C_ISR_TC) != I2C_ISR_TC)){} //wait for Transfer Complete
	return 1;	
}

uint8_t transmitComplete(void)
{
	while(((I2C2->ISR & I2C_ISR_TC) != I2C_ISR_TC)){} //wait for Transfer Complete
	I2C2->CR2 |= I2C_CR2_STOP;
	return 1;
}
/* End Code for I2C for Gyro */

/*Code for USART*/
void clearString(char* stringVal,int n)
{
	   for(;n>-1;n--)
			{
			  stringVal[n] = NULL;
			}
}


uint8_t matchString(char* str1, char* str2,uint8_t n)
{
	uint8_t pass = 0;
	
	while(n >= 1)
	{
		if(str1[n-1]==str2[n-1])
		{
			pass=1;
			n-=1;
		}
		else
		{
			 pass=0;
		   break;
		}
	}
	return pass;
}

void transmitChar(char character)
{
    int x = ((USART3->ISR)&(0x80)); 
	  if(x>0){
	    USART3->TDR = character;
		}			//Turn on the Red LED USART TX
}	

void transmitString(char* character)
{
	uint8_t x = 0; /* checks the bit 7 USART_ISR  TXE Register, 
	                                       Transmit register empty. Periph Man 27.8.8
	                                       **NOTE**This will return 0x80 if true			*/
	while(*character !='\0')
	 {
		x = ((USART3->ISR)&(0x80)); 
	  if(x>0)
		{
		  uint8_t sendChar = *(character);
	    USART3->TDR = sendChar;
			character+=1;
		}
	 }
}
/* End Code for USART*/


void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = 16;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}


/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
