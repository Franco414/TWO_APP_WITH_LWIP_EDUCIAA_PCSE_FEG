/*------------------------------------------------------------------------------
 *---------------------Simple HTTP-Server en EDU-CIAA--------------------------
 ----------------------------------------------------------------------------*/
#include "lwip/init.h"
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/memp.h"
#include "lwip/tcpip.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/timers.h"
#include "netif/etharp.h"
#include "lwipopts.h"

#if LWIP_DHCP
#include "lwip/dhcp.h"
#endif

#include "board.h"
#include "arch/lpc18xx_43xx_emac.h"

#include "arch/lpc_arch.h"
#include "arch/sys_arch.h"
#include "lpc_phy.h"

#if defined(lpc4337_m4)
#include "ciaaIO.h"
#endif


static struct netif educiaa_netif;

static void vSetupIFTask (void *pvParameters);
static void configurarHardware(void);
static void tcpip_init_done_signal(void *arg);

void msDelay(uint32_t ms)
{
	vTaskDelay((configTICK_RATE_HZ * ms) / 1000);
}

int main(void)
{
	configurarHardware();


	xTaskCreate(vSetupIFTask, (signed char *) "iniciar_programa",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Should never arrive here */
	return 1;
}

static void vSetupIFTask (void *pvParameters) {
	ip_addr_t ipaddr, netmask, gw;
	volatile s32_t tcpipdone = 0;
	uint32_t physts;
	static int prt_ip = 0;

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
	if (!netif_add(&educiaa_netif, &ipaddr, &netmask, &gw, NULL, lpc_enetif_init,
				   tcpip_input)) {
		LWIP_ASSERT("Net interface failed to initialize\r\n", 0);
	}
	netif_set_default(&educiaa_netif);
	netif_set_up(&educiaa_netif);

	NVIC_SetPriority(ETHERNET_IRQn, config_ETHERNET_INTERRUPT_PRIORITY);
	NVIC_EnableIRQ(ETHERNET_IRQn);

#if LWIP_DHCP
	dhcp_start(&educiaa_netif);
#endif

	  http_server_init();

	while (1) {
		physts = lpcPHYStsPoll();

		if (physts & PHY_LINK_CHANGED) {
			if (physts & PHY_LINK_CONNECTED) {
				Board_LED_Set(0, true);
				prt_ip = 0;


				if (physts & PHY_LINK_SPEED100) {

					Chip_ENET_SetSpeed(LPC_ETHERNET, 1);

					NETIF_INIT_SNMP(&educiaa_netif, snmp_ifType_ethernet_csmacd, 100000000);
				}
				else {

					Chip_ENET_SetSpeed(LPC_ETHERNET, 0);

					NETIF_INIT_SNMP(&educiaa_netif, snmp_ifType_ethernet_csmacd, 10000000);
				}
				if (physts & PHY_LINK_FULLDUPLX) {

					Chip_ENET_SetDuplex(LPC_ETHERNET, true);
				}
				else {
					Chip_ENET_SetDuplex(LPC_ETHERNET, false);
				}

				tcpip_callback_with_block((tcpip_callback_fn) netif_set_link_up,
										  (void *) &educiaa_netif, 1);
			}
			else {
				Board_LED_Set(0, false);
				tcpip_callback_with_block((tcpip_callback_fn) netif_set_link_down,
										  (void *) &educiaa_netif, 1);
			}

			DEBUGOUT("Link connect status: %d\r\n", ((physts & PHY_LINK_CONNECTED) != 0));

			/* Delay for link detection (250mS) */
			vTaskDelay(configTICK_RATE_HZ / 4);
		}

		/* Print IP address info */
		if (!prt_ip) {
			if (educiaa_netif.ip_addr.addr) {
				static char tmp_buff[16];
				DEBUGOUT("IP_ADDR    : %s\r\n", ipaddr_ntoa_r((const ip_addr_t *) &educiaa_netif.ip_addr, tmp_buff, 16));
				DEBUGOUT("NET_MASK   : %s\r\n", ipaddr_ntoa_r((const ip_addr_t *) &educiaa_netif.netmask, tmp_buff, 16));
				DEBUGOUT("GATEWAY_IP : %s\r\n", ipaddr_ntoa_r((const ip_addr_t *) &educiaa_netif.gw, tmp_buff, 16));
				prt_ip = 1;
			}
		}
	}
}

static void configurarHardware(void)
{
	SystemCoreClockUpdate();
	Board_Init();
#if defined(lpc4337_m4)
	ciaaIOInit();
#endif
	Board_LED_Set(0, false);
}

static void tcpip_init_done_signal(void *arg)
{
	*(s32_t *) arg = 1;
}
