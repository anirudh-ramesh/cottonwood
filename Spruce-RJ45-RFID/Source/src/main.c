

#include "stm32f10x.h"
#include <stdio.h>
#include "usart_printf.h"
#include "systick.h"


/* ʵ��uip��Ҫ������h�ļ� */
#include "uip.h"
#include "uip_arp.h"
#include "tapdev.h"
#include "timer.h"
#include "enc28j60.h"
#define BUF ((struct uip_eth_hdr *)&uip_buf[0])


static void Init_fd(void);
static void DispLogo(void);
void InitNet(void);
void UserPro(void);
void UipPro(void);
void Delay(vu32 nCount);
/*******************************************************************************
*	��������main
*	��  ��:
*	��  ��:
*	����˵�����û��������
*/
int main(void)
{
	Init_fd();	//������Դ�ĳ�ʼ��
	SPI1_Init();
    
	InitNet();		/* ��ʼ�������豸�Լ�UIPЭ��ջ������IP��ַ */

	/* ����һ��TCP�����˿ں�http�����˿ڣ��˿ں�Ϊ1200��80 */
	uip_listen(HTONS(1200));
	uip_listen(HTONS(80));
	while (1)
	{
		UipPro();		/* ����uip�¼���������뵽�û������ѭ������ */


	}
}

/*******************************************************************************
*	��������UipPro
*	��  ��:
*	��  ��:
*	����˵����uipЭ��ջ��ʵ����ڣ����뱻��ѯ����δ���ж�ģʽ
*/
void UipPro(void)
{
	uint8_t i;
	static struct timer periodic_timer, arp_timer;
	static char timer_ok = 0;	/* fd*/

	/* ����2����ʱ����ֻ��ִ��1�� */
	if (timer_ok == 0)
	{
		timer_ok = 1;
		timer_set(&periodic_timer, CLOCK_SECOND / 2);  /* ����1��0.5��Ķ�ʱ�� */
		timer_set(&arp_timer, CLOCK_SECOND * 10);	   /* ����1��10��Ķ�ʱ�� */
	}

	/*
		�������豸��ȡһ��IP��,�������ݳ��� (������)
		����ط�û��ʹ��DM9000AEP���жϹ��ܣ����õ��ǲ�ѯ��ʽ
	*/
	uip_len = tapdev_read();	/* uip_len ����uip�ж����ȫ�ֱ��� */
	if(uip_len > 0)
	{
		/* ����IP���ݰ�(ֻ��У��ͨ����IP���Żᱻ����) */
		if(BUF->type == htons(UIP_ETHTYPE_IP))
		{
			uip_arp_ipin();
			uip_input();
			/*
				������ĺ���ִ�к������Ҫ�������ݣ���ȫ�ֱ��� uip_len > 0
				��Ҫ���͵�������uip_buf, ������uip_len  (����2��ȫ�ֱ���)
			*/
			if (uip_len > 0)
			{
				uip_arp_out();
				tapdev_send();
			}
		}
		/* ����arp���� */
		else if (BUF->type == htons(UIP_ETHTYPE_ARP))
		{
			uip_arp_arpin();
			/*
				������ĺ���ִ�к������Ҫ�������ݣ���ȫ�ֱ��� uip_len > 0
				��Ҫ���͵�������uip_buf, ������uip_len  (����2��ȫ�ֱ���)
			*/
			if (uip_len > 0)
			{
				tapdev_send();
			}
		}
	}
	else if(timer_expired(&periodic_timer))	/* 0.5�붨ʱ����ʱ */
	{
		timer_reset(&periodic_timer);	/* ��λ0.5�붨ʱ�� */

		/* ��������ÿ��TCP����, UIP_CONNSȱʡ��10�� */
		for(i = 0; i < UIP_CONNS; i++)
		{
			uip_periodic(i);	/* ����TCPͨ���¼� */
			/*
				������ĺ���ִ�к������Ҫ�������ݣ���ȫ�ֱ��� uip_len > 0
				��Ҫ���͵�������uip_buf, ������uip_len  (����2��ȫ�ֱ���)
			*/
			if(uip_len > 0)
			{
				uip_arp_out();
				tapdev_send();
			}
		}

	#if UIP_UDP
		/* ��������ÿ��UDP����, UIP_UDP_CONNSȱʡ��10�� */
		for(i = 0; i < UIP_UDP_CONNS; i++)
		{
			uip_udp_periodic(i);	/*����UDPͨ���¼� */
			/* If the above function invocation resulted in data that
			should be sent out on the network, the global variable
			uip_len is set to a value > 0. */
			if(uip_len > 0)
			{
			uip_arp_out();
			tapdev_send();
			}
		}
	#endif /* UIP_UDP */

		/* ÿ��10�����1��ARP��ʱ������ */
		if (timer_expired(&arp_timer))
		{
			timer_reset(&arp_timer);
			uip_arp_timer();
		}
	}
}

/*******************************************************************************
*	��������InitNet
*	��  ��:
*	��  ��:
*	����˵������ʼ������Ӳ����UIPЭ��ջ�����ñ���IP��ַ
*/
void InitNet(void)
{
	uip_ipaddr_t ipaddr;


	tapdev_init();

//	printf("uip_init\n\r");
	uip_init();

	//printf("uip ip address : 192,168,0,15\n\r");
	uip_ipaddr(ipaddr, 192,168,1,15);
	uip_sethostaddr(ipaddr);

//	printf("uip route address : 192,168,0,1\n\r");
	uip_ipaddr(ipaddr, 192,168,1,1);
	uip_setdraddr(ipaddr);

	//printf("uip net mask : 255,255,255,0\n\r");
	uip_ipaddr(ipaddr, 255,255,255,0);
	uip_setnetmask(ipaddr);

}

void GPIO_Configuration(void)
{
  
  GPIO_InitTypeDef GPIO_InitStructure;
  RCC_APB2PeriphClockCmd( RCC_APB2Periph_USART1 |RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                         RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD |
                         RCC_APB2Periph_GPIOE, ENABLE);
  	
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;				     //LED1
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);					 

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_3;		 //LED2, LED3
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;		
  GPIO_Init(GPIOC, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_7;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  GPIO_ResetBits(GPIOE, GPIO_Pin_0);  
  GPIO_ResetBits(GPIOE, GPIO_Pin_1);
  Delay(0xAFFF);					   
  GPIO_SetBits(GPIOE, GPIO_Pin_1 );		 	 	   //��λENC28J60
  Delay(0xAFFF);		  	 	


 //��ֹSPI1�����ϵ������豸
  GPIO_SetBits(GPIOC, GPIO_Pin_4);			            //SPI CS1	       
  GPIO_SetBits(GPIOB, GPIO_Pin_12|GPIO_Pin_7);			//SPI CS4   CS3
 

}


void Delay(vu32 nCount) {
  for(; nCount != 0; nCount--);
  }
/*******************************************************************************
	��������InitBoard
	��  ��:
	��  ��:
	����˵������ʼ��Ӳ���豸
*/
static void Init_fd(void)
{
	/*
		���������ST���еĺ���������ʵ����
		Libraries\CMSIS\Core\CM3\system_stm32f10x.c

		�����ڲ�Flash�ӿڣ���ʼ��PLL������ϵͳƵ��
		ϵͳʱ��ȱʡ����Ϊ72MHz���������Ҫ���ģ�����Ҫȥ�޸���ص�ͷ�ļ��еĺ궨��
	 */
	SystemInit();

	/* ���ð���GPIO��LED GPIO */
	GPIO_Configuration();

	/* ���ô��� */
	USART_Configuration();



	/* ����systic��Ϊ1ms�ж�,���������
	\Libraries\CMSIS\Core\CM3\core_cm3.h */
	SysTick_Config(SystemFrequency / 1000);
}


#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
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

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/

