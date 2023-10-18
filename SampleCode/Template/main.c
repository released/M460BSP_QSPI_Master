/*_____ I N C L U D E S ____________________________________________________*/
#include <stdio.h>
#include <string.h>
#include "NuMicro.h"

#include "misc_config.h"

/*_____ D E C L A R A T I O N S ____________________________________________*/

struct flag_32bit flag_PROJ_CTL;
#define FLAG_PROJ_TIMER_PERIOD_1000MS                 	(flag_PROJ_CTL.bit0)
#define FLAG_PROJ_QSPI_TX_FINISH                   		(flag_PROJ_CTL.bit1)
#define FLAG_PROJ_QSPI_RX_FINISH                 		(flag_PROJ_CTL.bit2)
#define FLAG_PROJ_REVERSE3                              (flag_PROJ_CTL.bit3)
#define FLAG_PROJ_REVERSE4                              (flag_PROJ_CTL.bit4)
#define FLAG_PROJ_REVERSE5                              (flag_PROJ_CTL.bit5)
#define FLAG_PROJ_REVERSE6                              (flag_PROJ_CTL.bit6)
#define FLAG_PROJ_REVERSE7                              (flag_PROJ_CTL.bit7)


/*_____ D E F I N I T I O N S ______________________________________________*/

volatile unsigned int counter_systick = 0;
volatile uint32_t counter_tick = 0;

#define TARGET_QSPI_PORT                                (QSPI1)

#define QSPI_TARGET_FREQ				                (100000ul)
#define QSPI_TRANSMIT_LEN                               (16)

#define QSPI_MASTER_TX_DMA_CH 			                (14)
#define QSPI_MASTER_RX_DMA_CH 		                    (15)
#define QSPI_MASTER_OPENED_CH   		                ((1 << QSPI_MASTER_TX_DMA_CH) | (1 << QSPI_MASTER_RX_DMA_CH))

#define QSPI_4_WIRE_READ_RX_CMD                         (0x5A)
#define QSPI_4_WIRE_WRITE_RX_CMD                        (0xA5)

#define QSPI_SINGLE_MODE_ENABLE(qspi)    {QSPI_DISABLE_DUAL_MODE(qspi);\
                                                QSPI_DISABLE_QUAD_MODE(qspi);}

#define QSPI_SINGLE_OUTPUT_MODE_ENABLE(qspi)    {QSPI_SINGLE_MODE_ENABLE(qspi);\
                                                    (qspi)->CTL |= QSPI_CTL_DATDIR_Msk;}

// #define QSPI_DMA_Complete_IRQHandler    (PDMA_IRQHandler)

/*_____ M A C R O S ________________________________________________________*/
// #define ENABLE_QSPI_MANUAL_SS
// #define ENALBE_PDMA_IRQ
// #define ENALBE_PDMA_POLLING
#define ENALBE_QSPI_REGULAR_TRX

/*_____ F U N C T I O N S __________________________________________________*/

unsigned int get_systick(void)
{
	return (counter_systick);
}

void set_systick(unsigned int t)
{
	counter_systick = t;
}

void systick_counter(void)
{
	counter_systick++;
}

void SysTick_Handler(void)
{

    systick_counter();

    if (get_systick() >= 0xFFFFFFFF)
    {
        set_systick(0);      
    }

    // if ((get_systick() % 1000) == 0)
    // {
       
    // }

    #if defined (ENABLE_TICK_EVENT)
    TickCheckTickEvent();
    #endif    
}

void SysTick_delay(unsigned int delay)
{  
    
    unsigned int tickstart = get_systick(); 
    unsigned int wait = delay; 

    while((get_systick() - tickstart) < wait) 
    { 
    } 

}

void SysTick_enable(unsigned int ticks_per_second)
{
    set_systick(0);
    if (SysTick_Config(SystemCoreClock / ticks_per_second))
    {
        /* Setup SysTick Timer for 1 second interrupts  */
        printf("Set system tick error!!\n");
        while (1);
    }

    #if defined (ENABLE_TICK_EVENT)
    TickInitTickEvent();
    #endif
}

uint32_t get_tick(void)
{
	return (counter_tick);
}

void set_tick(uint32_t t)
{
	counter_tick = t;
}

void tick_counter(void)
{
	counter_tick++;
    if (get_tick() >= 60000)
    {
        set_tick(0);
    }
}

void delay_ms(uint16_t ms)
{
	#if 1
    uint32_t tickstart = get_tick();
    uint32_t wait = ms;
	uint32_t tmp = 0;
	
    while (1)
    {
		if (get_tick() > tickstart)	// tickstart = 59000 , tick_counter = 60000
		{
			tmp = get_tick() - tickstart;
		}
		else // tickstart = 59000 , tick_counter = 2048
		{
			tmp = 60000 -  tickstart + get_tick();
		}		
		
		if (tmp > wait)
			break;
    }
	
	#else
	TIMER_Delay(TIMER0, 1000*ms);
	#endif
}


void QSPI_SS_SET_LOW(void)
{
    #if defined (ENABLE_QSPI_MANUAL_SS)
    QSPI_SET_SS_LOW(TARGET_QSPI_PORT); 
    #endif
}

void QSPI_SS_SET_HIGH(void)
{
    #if defined (ENABLE_QSPI_MANUAL_SS)
    QSPI_SET_SS_HIGH(TARGET_QSPI_PORT); 
    #endif
}

// void QSPI_DMA_Complete_IRQHandler(void)
void PDMA0_IRQHandler(void)
{
    uint32_t status = PDMA_GET_INT_STATUS(PDMA0);
	
    if (status & PDMA_INTSTS_ABTIF_Msk)   /* abort */
    {
		#if 1
        PDMA_CLR_ABORT_FLAG(PDMA0, PDMA_GET_ABORT_STS(PDMA0));
		#else
        if (PDMA_GET_ABORT_STS(PDMA) & (1 << QSPI_MASTER_TX_DMA_CH))
        {

        }
        PDMA_CLR_ABORT_FLAG(PDMA, (1 << QSPI_MASTER_TX_DMA_CH));

        if (PDMA_GET_ABORT_STS(PDMA) & (1 << QSPI_MASTER_RX_DMA_CH))
        {

        }
        PDMA_CLR_ABORT_FLAG(PDMA, (1 << QSPI_MASTER_RX_DMA_CH));
		#endif
    }
    else if (status & PDMA_INTSTS_TDIF_Msk)     /* done */
    {
        if((PDMA_GET_TD_STS(PDMA0) & (1 << QSPI_MASTER_TX_DMA_CH) ) == (1 << QSPI_MASTER_TX_DMA_CH)  )
        {
            /* Clear PDMA transfer done interrupt flag */
            PDMA_CLR_TD_FLAG(PDMA0, (1 << QSPI_MASTER_TX_DMA_CH) );

			//insert process
            QSPI_DISABLE_TX_PDMA(TARGET_QSPI_PORT);
            FLAG_PROJ_QSPI_TX_FINISH = 1;
        }   

        if((PDMA_GET_TD_STS(PDMA0) & (1 << QSPI_MASTER_RX_DMA_CH) ) == (1 << QSPI_MASTER_RX_DMA_CH)  )
        {
            /* Clear PDMA transfer done interrupt flag */
            PDMA_CLR_TD_FLAG(PDMA0, (1 << QSPI_MASTER_RX_DMA_CH) );

			//insert process
            QSPI_DISABLE_RX_PDMA(TARGET_QSPI_PORT);
            FLAG_PROJ_QSPI_RX_FINISH = 1;
        }  

    }
    else if (status & (PDMA_INTSTS_REQTOF0_Msk | PDMA_INTSTS_REQTOF1_Msk))     /* Check the DMA time-out interrupt flag */
    {
        PDMA_CLR_TMOUT_FLAG(PDMA0,QSPI_MASTER_TX_DMA_CH);
        PDMA_CLR_TMOUT_FLAG(PDMA0,QSPI_MASTER_RX_DMA_CH);
    }
    else
    {

    }

}

void QSPIReadDataWithDMA(unsigned char *pBuffer , unsigned short size)
{
    #if defined (ENALBE_PDMA_POLLING)    
	uint32_t u32RegValue = 0;
	uint32_t u32Abort = 0;	
    #elif defined (ENALBE_QSPI_REGULAR_TRX)
    uint8_t i = 0;
    uint8_t j = 0;    
    #endif

    uint8_t tBuffer[QSPI_TRANSMIT_LEN] = {0x00};

    #if defined (ENALBE_PDMA_IRQ) || defined (ENALBE_PDMA_POLLING)    
    FLAG_PROJ_QSPI_TX_FINISH = 0;
    FLAG_PROJ_QSPI_RX_FINISH = 0;
    PDMA_SetTransferCnt(PDMA0,QSPI_MASTER_TX_DMA_CH, PDMA_WIDTH_8, QSPI_TRANSMIT_LEN);
    PDMA_SetTransferAddr(PDMA0,QSPI_MASTER_TX_DMA_CH, (uint32_t)tBuffer, PDMA_SAR_INC, (uint32_t)&TARGET_QSPI_PORT->TX, PDMA_DAR_FIX);
    PDMA_SetTransferMode(PDMA0,QSPI_MASTER_TX_DMA_CH, PDMA_QSPI1_TX, FALSE, 0);    

    PDMA_SetTransferCnt(PDMA0,QSPI_MASTER_RX_DMA_CH, PDMA_WIDTH_8, size);
    PDMA_SetTransferAddr(PDMA0,QSPI_MASTER_RX_DMA_CH, (uint32_t)&TARGET_QSPI_PORT->RX, PDMA_SAR_FIX, (uint32_t)pBuffer, PDMA_DAR_INC);
    PDMA_SetTransferMode(PDMA0,QSPI_MASTER_RX_DMA_CH, PDMA_QSPI1_RX, FALSE, 0);   

    QSPI_TRIGGER_TX_PDMA(TARGET_QSPI_PORT);
    QSPI_TRIGGER_RX_PDMA(TARGET_QSPI_PORT);    
    #endif

    #if defined (ENALBE_PDMA_IRQ)
    while(!FLAG_PROJ_QSPI_TX_FINISH);
    while(!FLAG_PROJ_QSPI_RX_FINISH);    

    #elif defined (ENALBE_PDMA_POLLING)

    while(1)
    {
        /* Get interrupt status */
        u32RegValue = PDMA_GET_INT_STATUS(PDMA);
        /* Check the DMA transfer done interrupt flag */
        if(u32RegValue & PDMA_INTSTS_TDIF_Msk)
        {
            /* Check the PDMA transfer done interrupt flags */
            if((PDMA_GET_TD_STS(PDMA) & (1 << QSPI_MASTER_RX_DMA_CH)) == (1 << QSPI_MASTER_RX_DMA_CH))
            {
                /* Clear the DMA transfer done flags */
                PDMA_CLR_TD_FLAG(PDMA,1 << QSPI_MASTER_RX_DMA_CH);
                /* Disable SPI PDMA TX function */
                QSPI_DISABLE_RX_PDMA(TARGET_QSPI_PORT);
                break;
            }

            /* Check the DMA transfer abort interrupt flag */
            if(u32RegValue & PDMA_INTSTS_ABTIF_Msk)
            {
                /* Get the target abort flag */
                u32Abort = PDMA_GET_ABORT_STS(PDMA);
                /* Clear the target abort flag */
                PDMA_CLR_ABORT_FLAG(PDMA,u32Abort);
                break;
            }
        }
    }
    #elif defined (ENALBE_QSPI_REGULAR_TRX)
    #if 1   // improve
    j=0;
    for (i = 0 ; i < QSPI_TRANSMIT_LEN ; )
    {
        if(!QSPI_GET_TX_FIFO_FULL_FLAG(TARGET_QSPI_PORT))
        {
            QSPI_WRITE_TX(TARGET_QSPI_PORT, 0x00);
            i++;
        }
        if(!QSPI_GET_RX_FIFO_EMPTY_FLAG(TARGET_QSPI_PORT))
        {
            pBuffer[j++] = QSPI_READ_RX(TARGET_QSPI_PORT);   
        }                   
    }
    while(!QSPI_GET_RX_FIFO_EMPTY_FLAG(TARGET_QSPI_PORT))
        pBuffer[j++] = QSPI_READ_RX(TARGET_QSPI_PORT);  
    #else
    for (i = 0 ; i < QSPI_TRANSMIT_LEN ; i++)
    {
        QSPI_WRITE_TX(TARGET_QSPI_PORT, 0x00);
        while(QSPI_IS_BUSY(TARGET_QSPI_PORT));
        pBuffer[i] = QSPI_READ_RX(TARGET_QSPI_PORT);                      
    }
    #endif
    #endif

    while(QSPI_IS_BUSY(TARGET_QSPI_PORT)); 
    QSPI_SS_SET_HIGH();    
}

int ReadSlaveRxRegs(unsigned char ChipNo , 
                    unsigned char Bank , 
                    unsigned int StartAddr , 
                    unsigned char *pBuffer , 
                    unsigned short size , 
                    int bDMAMode)
{
    #if 1

    QSPI_SS_SET_LOW();
    QSPI_WRITE_TX(TARGET_QSPI_PORT,QSPI_4_WIRE_READ_RX_CMD);
    while(QSPI_IS_BUSY(TARGET_QSPI_PORT));

    QSPI_WRITE_TX(TARGET_QSPI_PORT, Bank);
    while(QSPI_IS_BUSY(TARGET_QSPI_PORT));
    QSPI_WRITE_TX(TARGET_QSPI_PORT, (StartAddr>>8)&0xFF);
    while(QSPI_IS_BUSY(TARGET_QSPI_PORT));
    QSPI_WRITE_TX(TARGET_QSPI_PORT, StartAddr&0xFF);
    while(QSPI_IS_BUSY(TARGET_QSPI_PORT));

    QSPI_ClearRxFIFO(TARGET_QSPI_PORT);
    QSPIReadDataWithDMA(pBuffer,size);

    #else
    QSPI_SS_SET_LOW();
    QSPI_SINGLE_OUTPUT_MODE_ENABLE(TARGET_QSPI_PORT);
    QSPI_WRITE_TX(TARGET_QSPI_PORT,QSPI_4_WIRE_READ_RX_CMD);
    while(QSPI_IS_BUSY(TARGET_QSPI_PORT));

    QSPI_ENABLE_QUAD_OUTPUT_MODE(TARGET_QSPI_PORT);
    QSPI_WRITE_TX(TARGET_QSPI_PORT, Bank);
    QSPI_WRITE_TX(TARGET_QSPI_PORT, (StartAddr>>8)&0xFF);
    QSPI_WRITE_TX(TARGET_QSPI_PORT, StartAddr&0xFF);

    QSPI_WRITE_TX(TARGET_QSPI_PORT,0xFF);
    QSPI_WRITE_TX(TARGET_QSPI_PORT,0xFF);
    QSPI_WRITE_TX(TARGET_QSPI_PORT,0xFF);
    QSPI_WRITE_TX(TARGET_QSPI_PORT,0xFF);
    while(QSPI_IS_BUSY(TARGET_QSPI_PORT) || !QSPI_GET_TX_FIFO_EMPTY_FLAG(TARGET_QSPI_PORT));

    QSPI_ENABLE_QUAD_INPUT_MODE(TARGET_QSPI_PORT);
    QSPI_ClearRxFIFO(TARGET_QSPI_PORT);
    QSPIReadDataWithDMA(pBuffer,size);
    #endif

    return 1;
}

void QSPIWriteDataWithDMA(unsigned char *pBuffer , unsigned short size)
{
    #if defined (ENALBE_PDMA_POLLING)    
	uint32_t u32RegValue = 0;
	uint32_t u32Abort = 0;
    #elif defined (ENALBE_QSPI_REGULAR_TRX)
    uint8_t i = 0;
    #endif

    #if defined (ENALBE_PDMA_IRQ) || defined (ENALBE_PDMA_POLLING)
    FLAG_PROJ_QSPI_TX_FINISH = 0;
    PDMA_SetTransferCnt(PDMA0,QSPI_MASTER_TX_DMA_CH, PDMA_WIDTH_8, size);
    PDMA_SetTransferAddr(PDMA0,QSPI_MASTER_TX_DMA_CH, (uint32_t)pBuffer, PDMA_SAR_INC, (uint32_t)&TARGET_QSPI_PORT->TX, PDMA_DAR_FIX);
    PDMA_SetTransferMode(PDMA0,QSPI_MASTER_TX_DMA_CH, PDMA_QSPI1_TX, FALSE, 0);    
    QSPI_TRIGGER_TX_PDMA(TARGET_QSPI_PORT);
    #endif

    #if defined (ENALBE_PDMA_IRQ)
    // while(!FLAG_PROJ_QSPI_TX_FINISH);
    #elif defined (ENALBE_PDMA_POLLING)   
    while(1)
    {
        /* Get interrupt status */
        u32RegValue = PDMA_GET_INT_STATUS(PDMA);
        /* Check the DMA transfer done interrupt flag */
        if(u32RegValue & PDMA_INTSTS_TDIF_Msk)
        {
            /* Check the PDMA transfer done interrupt flags */
            if((PDMA_GET_TD_STS(PDMA) & (1 << QSPI_MASTER_TX_DMA_CH)) == (1 << QSPI_MASTER_TX_DMA_CH))
            {
                /* Clear the DMA transfer done flags */
                PDMA_CLR_TD_FLAG(PDMA,1 << QSPI_MASTER_TX_DMA_CH);
                /* Disable SPI PDMA TX function */
                QSPI_DISABLE_TX_PDMA(TARGET_QSPI_PORT);
                break;
            }

            /* Check the DMA transfer abort interrupt flag */
            if(u32RegValue & PDMA_INTSTS_ABTIF_Msk)
            {
                /* Get the target abort flag */
                u32Abort = PDMA_GET_ABORT_STS(PDMA);
                /* Clear the target abort flag */
                PDMA_CLR_ABORT_FLAG(PDMA,u32Abort);
                break;
            }
        }
    }
    #elif defined (ENALBE_QSPI_REGULAR_TRX)

    for (i = 0 ; i < QSPI_TRANSMIT_LEN ; i++)
    {
        QSPI_WRITE_TX(TARGET_QSPI_PORT,pBuffer[i]);
        while(QSPI_IS_BUSY(TARGET_QSPI_PORT));      
    }
    #endif

    while(QSPI_IS_BUSY(TARGET_QSPI_PORT));
    // while(QSPI_IS_BUSY(TARGET_QSPI_PORT) || !QSPI_GET_TX_FIFO_EMPTY_FLAG(TARGET_QSPI_PORT));
    QSPI_SS_SET_HIGH();   

}

int WriteSlaveRxRegs(unsigned char ChipNo , 
                    unsigned char Bank , 
                    unsigned int StartAddr , 
                    unsigned char *pBuffer , 
                    unsigned short size , 
                    int bDMAMode)
{
    #if 1

    QSPI_SS_SET_LOW();
    QSPI_WRITE_TX(TARGET_QSPI_PORT,QSPI_4_WIRE_WRITE_RX_CMD);
    while(QSPI_IS_BUSY(TARGET_QSPI_PORT));

    QSPI_WRITE_TX(TARGET_QSPI_PORT, Bank);
    while(QSPI_IS_BUSY(TARGET_QSPI_PORT));
    QSPI_WRITE_TX(TARGET_QSPI_PORT, (StartAddr>>8)&0xFF);
    while(QSPI_IS_BUSY(TARGET_QSPI_PORT));
    QSPI_WRITE_TX(TARGET_QSPI_PORT, StartAddr&0xFF);
    while(QSPI_IS_BUSY(TARGET_QSPI_PORT));

    QSPIWriteDataWithDMA(pBuffer,size);

    #else
    QSPI_SS_SET_LOW();
    QSPI_SINGLE_OUTPUT_MODE_ENABLE(TARGET_QSPI_PORT);
    QSPI_WRITE_TX(TARGET_QSPI_PORT,QSPI_4_WIRE_WRITE_RX_CMD);
    while(QSPI_IS_BUSY(TARGET_QSPI_PORT));

    QSPI_ENABLE_QUAD_OUTPUT_MODE(TARGET_QSPI_PORT);
    QSPI_WRITE_TX(TARGET_QSPI_PORT, Bank);
    QSPI_WRITE_TX(TARGET_QSPI_PORT, (StartAddr>>8)&0xFF);
    QSPI_WRITE_TX(TARGET_QSPI_PORT, StartAddr&0xFF);
    while(QSPI_IS_BUSY(TARGET_QSPI_PORT) || !QSPI_GET_TX_FIFO_EMPTY_FLAG(TARGET_QSPI_PORT));

    QSPIWriteDataWithDMA(pBuffer,size);
    #endif

    return 1;
}

void InitQSPIDMA(void)
{
    PDMA_Open(PDMA0, QSPI_MASTER_OPENED_CH);

    PDMA_SetBurstType(PDMA0, QSPI_MASTER_TX_DMA_CH , PDMA_REQ_SINGLE, 0);   
    PDMA_SetBurstType(PDMA0, QSPI_MASTER_RX_DMA_CH , PDMA_REQ_SINGLE, 0);  

    PDMA0->DSCT[QSPI_MASTER_TX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;
    PDMA0->DSCT[QSPI_MASTER_RX_DMA_CH].CTL |= PDMA_DSCT_CTL_TBINTDIS_Msk;

    QSPI_DISABLE_TX_RX_PDMA(TARGET_QSPI_PORT);

    #if defined (ENALBE_PDMA_IRQ)
    PDMA_EnableInt(PDMA0, QSPI_MASTER_TX_DMA_CH, PDMA_INT_TRANS_DONE);
    PDMA_EnableInt(PDMA0, QSPI_MASTER_RX_DMA_CH, PDMA_INT_TRANS_DONE);

    NVIC_SetPriority(PDMA0_IRQn,0);
    NVIC_ClearPendingIRQ(PDMA0_IRQn);
    NVIC_EnableIRQ(PDMA0_IRQn);
    #endif

}

void InitQSPI(void)
{
    uint32_t clk = 0;

    clk = QSPI_Open(TARGET_QSPI_PORT, QSPI_MASTER, QSPI_MODE_0, 8, QSPI_TARGET_FREQ);
    printf("clk = %8d\r\n", clk);

    #if defined (ENABLE_QSPI_MANUAL_SS)
    QSPI_DisableAutoSS(TARGET_QSPI_PORT);
    QSPI_SS_SET_HIGH();
    #else
    QSPI_EnableAutoSS(TARGET_QSPI_PORT, SPI_SS, SPI_SS_ACTIVE_LOW);
    #endif

    QSPI_SET_SUSPEND_CYCLE(TARGET_QSPI_PORT,0);
    // QSPI_SetFIFO(TARGET_QSPI_PORT, 4, 4);

    InitQSPIDMA();
}

void TestQSPIFlow(void)
{
    unsigned char WriteBuf[QSPI_TRANSMIT_LEN] = {0};
    unsigned char ReadBuf[QSPI_TRANSMIT_LEN] = {0};   
    unsigned char i = 0;

     for( i = 0 ; i < QSPI_TRANSMIT_LEN ; i++)
     {
        WriteBuf[i] = i;
     }

     WriteSlaveRxRegs(0 , 0 , 0 , &WriteBuf[0] , QSPI_TRANSMIT_LEN , 1 );
    //  ReadSlaveRxRegs(0 , 0 , 0 , &ReadBuf[0] , QSPI_TRANSMIT_LEN , 1 );
}





//
// check_reset_source
//
uint8_t check_reset_source(void)
{
    uint32_t src = SYS_GetResetSrc();

    /*
        0x02 = M467.  
        0x03 = M463. 
    */

    if ((SYS->CSERVER & SYS_CSERVER_VERSION_Msk) == 0x2)  
    {
		printf("PN : M467\r\n");
    }
    else if  ((SYS->CSERVER & SYS_CSERVER_VERSION_Msk) == 0x3)
    {
		printf("PN : M463\r\n");
    }

    SYS->RSTSTS |= 0x1FF;
    printf("Reset Source <0x%08X>\r\n", src);

    #if 1   //DEBUG , list reset source
    if (src & BIT0)
    {
        printf("0)POR Reset Flag\r\n");       
    }
    if (src & BIT1)
    {
        printf("1)NRESET Pin Reset Flag\r\n");       
    }
    if (src & BIT2)
    {
        printf("2)WDT Reset Flag\r\n");       
    }
    if (src & BIT3)
    {
        printf("3)LVR Reset Flag\r\n");       
    }
    if (src & BIT4)
    {
        printf("4)BOD Reset Flag\r\n");       
    }
    if (src & BIT5)
    {
        printf("5)System Reset Flag \r\n");       
    }
    if (src & BIT6)
    {
        printf("6)HRESET Reset Flag \r\n");       
    }
    if (src & BIT7)
    {
        printf("7)CPU Reset Flag\r\n");       
    }
    if (src & BIT8)
    {
        printf("8)CPU Lockup Reset Flag\r\n");       
    }
    #endif
    
    if (src & SYS_RSTSTS_PORF_Msk) {
        SYS_ClearResetSrc(SYS_RSTSTS_PORF_Msk);
        
        printf("power on from POR\r\n");
        return FALSE;
    }    
    else if (src & SYS_RSTSTS_PINRF_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSTS_PINRF_Msk);
        
        printf("power on from nRESET pin\r\n");
        return FALSE;
    } 
    else if (src & SYS_RSTSTS_WDTRF_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSTS_WDTRF_Msk);
        
        printf("power on from WDT Reset\r\n");
        return FALSE;
    }    
    else if (src & SYS_RSTSTS_LVRF_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSTS_LVRF_Msk);
        
        printf("power on from LVR Reset\r\n");
        return FALSE;
    }    
    else if (src & SYS_RSTSTS_BODRF_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSTS_BODRF_Msk);
        
        printf("power on from BOD Reset\r\n");
        return FALSE;
    }    
    else if (src & SYS_RSTSTS_MCURF_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSTS_MCURF_Msk);
        
        printf("power on from System Reset\r\n");
        return FALSE;
    } 
    else if (src & SYS_RSTSTS_CPURF_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSTS_CPURF_Msk);

        printf("power on from CPU reset\r\n");
        return FALSE;         
    }    
    else if (src & SYS_RSTSTS_CPULKRF_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSTS_CPULKRF_Msk);
        
        printf("power on from CPU Lockup Reset\r\n");
        return FALSE;
    }   
    
    printf("power on from unhandle reset source\r\n");
    return FALSE;
}

void TMR1_IRQHandler(void)
{
	
    if(TIMER_GetIntFlag(TIMER1) == 1)
    {
        TIMER_ClearIntFlag(TIMER1);
		tick_counter();

		if ((get_tick() % 1000) == 0)
		{
            FLAG_PROJ_TIMER_PERIOD_1000MS = 1;//set_flag(flag_timer_period_1000ms ,ENABLE);
		}

		if ((get_tick() % 50) == 0)
		{

		}	
    }
}

void TIMER1_Init(void)
{
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);	
    TIMER_Start(TIMER1);
}

void loop(void)
{
	static uint32_t LOG1 = 0;
	// static uint32_t LOG2 = 0;

    if ((get_systick() % 1000) == 0)
    {
        // printf("%s(systick) : %4d\r\n",__FUNCTION__,LOG2++);    
    }

    if (FLAG_PROJ_TIMER_PERIOD_1000MS)//(is_flag_set(flag_timer_period_1000ms))
    {
        FLAG_PROJ_TIMER_PERIOD_1000MS = 0;//set_flag(flag_timer_period_1000ms ,DISABLE);

        printf("%s(timer) : %4d\r\n",__FUNCTION__,LOG1++);
        PH4 ^= 1;             
    }
}

void UARTx_Process(void)
{
	uint8_t res = 0;
	res = UART_READ(UART0);

	if (res > 0x7F)
	{
		printf("invalid command\r\n");
	}
	else
	{
		printf("press : %c\r\n" , res);
		switch(res)
		{
			case '1':
				break;

			case 'X':
			case 'x':
			case 'Z':
			case 'z':
                SYS_UnlockReg();
				// NVIC_SystemReset();	// Reset I/O and peripherals , only check BS(FMC_ISPCTL[1])
                // SYS_ResetCPU();     // Not reset I/O and peripherals
                SYS_ResetChip();    // Reset I/O and peripherals ,  BS(FMC_ISPCTL[1]) reload from CONFIG setting (CBS)	
				break;
		}
	}
}

void UART0_IRQHandler(void)
{
    if(UART_GET_INT_FLAG(UART0, UART_INTSTS_RDAINT_Msk | UART_INTSTS_RXTOINT_Msk))     /* UART receive data available flag */
    {
        while(UART_GET_RX_EMPTY(UART0) == 0)
        {
			UARTx_Process();
        }
    }

    if(UART0->FIFOSTS & (UART_FIFOSTS_BIF_Msk | UART_FIFOSTS_FEF_Msk | UART_FIFOSTS_PEF_Msk | UART_FIFOSTS_RXOVIF_Msk))
    {
        UART_ClearIntFlag(UART0, (UART_INTSTS_RLSINT_Msk| UART_INTSTS_BUFERRINT_Msk));
    }	
}

void UART0_Init(void)
{
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);

	/* Set UART receive time-out */
	UART_SetTimeoutCnt(UART0, 20);

	UART0->FIFO &= ~UART_FIFO_RFITL_4BYTES;
	UART0->FIFO |= UART_FIFO_RFITL_8BYTES;

	/* Enable UART Interrupt - */
	UART_ENABLE_INT(UART0, UART_INTEN_RDAIEN_Msk | UART_INTEN_TOCNTEN_Msk | UART_INTEN_RXTOIEN_Msk);
	
	NVIC_EnableIRQ(UART0_IRQn);

	#if (_debug_log_UART_ == 1)	//debug
	printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	printf("CLK_GetHCLKFreq : %8d\r\n",CLK_GetHCLKFreq());
	printf("CLK_GetHXTFreq : %8d\r\n",CLK_GetHXTFreq());
	printf("CLK_GetLXTFreq : %8d\r\n",CLK_GetLXTFreq());	
	printf("CLK_GetPCLK0Freq : %8d\r\n",CLK_GetPCLK0Freq());
	printf("CLK_GetPCLK1Freq : %8d\r\n",CLK_GetPCLK1Freq());
	printf("CLK_GetHCLKFreq : %8d\r\n",CLK_GetHCLKFreq());    	

//    printf("Product ID 0x%8X\n", SYS->PDID);
	
	#endif	

    #if 0
    printf("FLAG_PROJ_TIMER_PERIOD_1000MS : 0x%2X\r\n",FLAG_PROJ_TIMER_PERIOD_1000MS);
    printf("FLAG_PROJ_REVERSE1 : 0x%2X\r\n",FLAG_PROJ_REVERSE1);
    printf("FLAG_PROJ_REVERSE2 : 0x%2X\r\n",FLAG_PROJ_REVERSE2);
    printf("FLAG_PROJ_REVERSE3 : 0x%2X\r\n",FLAG_PROJ_REVERSE3);
    printf("FLAG_PROJ_REVERSE4 : 0x%2X\r\n",FLAG_PROJ_REVERSE4);
    printf("FLAG_PROJ_REVERSE5 : 0x%2X\r\n",FLAG_PROJ_REVERSE5);
    printf("FLAG_PROJ_REVERSE6 : 0x%2X\r\n",FLAG_PROJ_REVERSE6);
    printf("FLAG_PROJ_REVERSE7 : 0x%2X\r\n",FLAG_PROJ_REVERSE7);
    #endif

}

void GPIO_Init (void)
{
    SET_GPIO_PH4();
    SET_GPIO_PH5();
    SET_GPIO_PH6();

	//EVM LED
	GPIO_SetMode(PH,BIT4,GPIO_MODE_OUTPUT);
	GPIO_SetMode(PH,BIT5,GPIO_MODE_OUTPUT);
	GPIO_SetMode(PH,BIT6,GPIO_MODE_OUTPUT);
	
}

void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    /* Set XT1_OUT(PF.2) and XT1_IN(PF.3) to input mode */
//    PF->MODE &= ~(GPIO_MODE_MODE2_Msk | GPIO_MODE_MODE3_Msk);
    
    CLK_EnableXtalRC(CLK_PWRCTL_HIRCEN_Msk);
    CLK_WaitClockReady(CLK_STATUS_HIRCSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_LIRCEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LIRCSTB_Msk);

//    CLK_EnableXtalRC(CLK_PWRCTL_LXTEN_Msk);
//    CLK_WaitClockReady(CLK_STATUS_LXTSTB_Msk);

    /* Set PCLK0/PCLK1 to HCLK/2 */
    CLK->PCLKDIV = (CLK_PCLKDIV_APB0DIV_DIV2 | CLK_PCLKDIV_APB1DIV_DIV2);

    /* Enable all GPIO clock */
    CLK->AHBCLK0 |= CLK_AHBCLK0_GPACKEN_Msk | CLK_AHBCLK0_GPBCKEN_Msk | CLK_AHBCLK0_GPCCKEN_Msk | CLK_AHBCLK0_GPDCKEN_Msk |
                    CLK_AHBCLK0_GPECKEN_Msk | CLK_AHBCLK0_GPFCKEN_Msk | CLK_AHBCLK0_GPGCKEN_Msk | CLK_AHBCLK0_GPHCKEN_Msk;
    CLK->AHBCLK1 |= CLK_AHBCLK1_GPICKEN_Msk | CLK_AHBCLK1_GPJCKEN_Msk;


    /* Set core clock to 200MHz */
    CLK_SetCoreClock(FREQ_200MHZ);

    /* Enable UART clock */
    CLK_EnableModuleClock(UART0_MODULE);
    /* Select UART clock source from HXT */
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HIRC, CLK_CLKDIV0_UART0(1));

    /* Set GPB multi-function pins for UART0 RXD and TXD */
    SET_UART0_RXD_PB12();
    SET_UART0_TXD_PB13();

    CLK_EnableModuleClock(TMR1_MODULE);
    CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_HIRC, 0);
	
    CLK_EnableModuleClock(PDMA0_MODULE);    

    /* Enable QSPI1 module clock */
    CLK_EnableModuleClock(QSPI1_MODULE);

    /* Select QSPI1 module clock source as PCLK1 */
    CLK_SetModuleClock(QSPI1_MODULE, CLK_CLKSEL2_QSPI1SEL_PCLK1, MODULE_NoMsk);

    /* Setup QSPI1 multi-function pins */
    SET_QSPI1_MOSI0_PA13();
    SET_QSPI1_MISO0_PA12();
    SET_QSPI1_CLK_PG12();
    SET_QSPI1_SS_PG11();

    /* Enable QSPI1 clock pin (PG12) schmitt trigger */
    PG->SMTEN |= GPIO_SMTEN_SMTEN12_Msk;

    /* Enable QSPI1 I/O high slew rate */
    GPIO_SetSlewCtl(PA, BIT12 | BIT13 , GPIO_SLEWCTL_FAST);
    GPIO_SetSlewCtl(PG, BIT11 | BIT12 , GPIO_SLEWCTL_FAST);

    /* Update System Core Clock */
    /* User can use SystemCoreClockUpdate() to calculate SystemCoreClock. */
    SystemCoreClockUpdate();

    /* Lock protected registers */
    SYS_LockReg();
}

/*
 * This is a template project for M480 series MCU. Users could based on this project to create their
 * own application without worry about the IAR/Keil project settings.
 *
 * This template application uses external crystal as HCLK source and configures UART0 to print out
 * "Hello World", users may need to do extra system configuration based on their system design.
 */

int main()
{
    SYS_Init();

	GPIO_Init();
	UART0_Init();
	TIMER1_Init();
    check_reset_source();

    SysTick_enable(1000);
    #if defined (ENABLE_TICK_EVENT)
    TickSetTickEvent(1000, TickCallback_processA);  // 1000 ms
    TickSetTickEvent(5000, TickCallback_processB);  // 5000 ms
    #endif

    InitQSPI();
    TestQSPIFlow();

    /* Got no where to go, just loop forever */
    while(1)
    {
        loop();

    }
}

/*** (C) COPYRIGHT 2016 Nuvoton Technology Corp. ***/
