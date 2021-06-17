/*
 * driver_enet_8720.c
 *
 *  Created on: 17 jun. 2021
 *      Author: GOLEBRI3D
 */
#include "board.h"
#include "lwip/init.h"
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/memp.h"
#include "lwip/tcpip.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/timers.h"
#include "netif/etharp.h"
//#include "lwipopts.h"

#include "driver_enet_8720.h"

#include "arch/lpc_arch.h"
#include "arch/sys_arch.h"
#include "lpc_phy.h"
#include "arch/lpc18xx_43xx_emac.h"

#if LWIP_DHCP
#include "lwip/dhcp.h"
#endif
#include "udpecho.h"
#include "driver_enet_8720.h"
#include "lwipopts.h"

#include "udpecho.h"
#include "driver_enet_8720.h"
	static struct netif lpc_netif;
	static ip_addr_t ipaddr, netmask, gw;
	static volatile s32_t tcpipdone = 0;
	static uint32_t physts;
	static int prt_ip = 0;
//---------------------------------------------------------------------------------
static void rutina_lan8720();
static void tcpip_init_done_signal(void *arg);
void vSetupIFTask (void *pvParameters);
//---------------------------------------------------------------------------------
void iniciar_udp_server(){
	xTaskCreate(vSetupIFTask, (signed char *) "SetupIFx",configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),(xTaskHandle *) NULL);
}


void vSetupIFTask (void *pvParameters){

	LWIP_DEBUGF(LWIP_DBG_ON, ("Waiting for TCPIP thread to initialize...\n"));
	tcpip_init(tcpip_init_done_signal, (void *) &tcpipdone);
	while (!tcpipdone) {
		msDelay(1);
	}

	LWIP_DEBUGF(LWIP_DBG_ON, ("Starting LWIP TCP echo server...\n"));

#if LWIP_DHCP
	IP4_ADDR(&gw, 0, 0, 0, 0);
	IP4_ADDR(&ipaddr, 0, 0, 0, 0);
	IP4_ADDR(&netmask, 0, 0, 0, 0);
#else
	IP4_ADDR(&gw, 192, 168, 0, 1);
	IP4_ADDR(&ipaddr,  192, 168, 0, 11);
	IP4_ADDR(&netmask, 255, 255, 255, 0);
#endif

	/* Add netif interface for lpc17xx_8x */
	if (!netif_add(&lpc_netif, &ipaddr, &netmask, &gw, NULL, lpc_enetif_init,
				   tcpip_input)) {
		LWIP_ASSERT("Net interface failed to initialize\r\n", 0);
	}
	netif_set_default(&lpc_netif);
	netif_set_up(&lpc_netif);

	/* Enable MAC interrupts only after LWIP is ready */
	NVIC_SetPriority(ETHERNET_IRQn, config_ETHERNET_INTERRUPT_PRIORITY);
	NVIC_EnableIRQ(ETHERNET_IRQn);

#if LWIP_DHCP
	dhcp_start(&lpc_netif);
#endif

	udpecho_init();
	rutina_lan8720();
}

static void rutina_lan8720(){

	while (1) {
		physts = lpcPHYStsPoll();
		if (physts & PHY_LINK_CHANGED) {
			if (physts & PHY_LINK_CONNECTED) {
				Board_LED_Set(0, true);
				prt_ip = 0;

				if (physts & PHY_LINK_SPEED100) {

					Chip_ENET_SetSpeed(LPC_ETHERNET, 1);

					NETIF_INIT_SNMP(&lpc_netif, snmp_ifType_ethernet_csmacd, 100000000);
				}
				else {

					Chip_ENET_SetSpeed(LPC_ETHERNET, 0);

					NETIF_INIT_SNMP(&lpc_netif, snmp_ifType_ethernet_csmacd, 10000000);
				}
				if (physts & PHY_LINK_FULLDUPLX) {

					Chip_ENET_SetDuplex(LPC_ETHERNET, true);
				}
				else {
					Chip_ENET_SetDuplex(LPC_ETHERNET, false);
				}

				tcpip_callback_with_block((tcpip_callback_fn) netif_set_link_up,
										  (void *) &lpc_netif, 1);
			}
			else {
				Board_LED_Set(0, false);
				tcpip_callback_with_block((tcpip_callback_fn) netif_set_link_down,
										  (void *) &lpc_netif, 1);
			}

			DEBUGOUT("Link connect status: %d\r\n", ((physts & PHY_LINK_CONNECTED) != 0));


			vTaskDelay(configTICK_RATE_HZ / 4);
		}


		if (!prt_ip) {
			if (lpc_netif.ip_addr.addr) {
				static char tmp_buff[16];
				DEBUGOUT("IP_ADDR    : %s\r\n", ipaddr_ntoa_r((const ip_addr_t *) &lpc_netif.ip_addr, tmp_buff, 16));
				DEBUGOUT("NET_MASK   : %s\r\n", ipaddr_ntoa_r((const ip_addr_t *) &lpc_netif.netmask, tmp_buff, 16));
				DEBUGOUT("GATEWAY_IP : %s\r\n", ipaddr_ntoa_r((const ip_addr_t *) &lpc_netif.gw, tmp_buff, 16));
				prt_ip = 1;
			}
		}
	}
}


static void tcpip_init_done_signal(void *arg)
{
	*(s32_t *) arg = 1;
}

void msDelay(uint32_t ms)
{
	vTaskDelay((configTICK_RATE_HZ * ms) / 1000);
}
