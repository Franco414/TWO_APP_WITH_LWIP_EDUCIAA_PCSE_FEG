/*------------------------------------------------------------------------------
*---------------------------DEMOSTRACION_UDP_ECHO_SERVER------------------------
------------------------------------------------------------------------------- */
#include "board.h"
#include "driver_enet_8720.h"
#include "udpecho.h"

#if defined(lpc4337_m4)
#include "ciaaIO.h"
#endif


static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();
	Board_Init();

#if defined(lpc4337_m4)
	ciaaIOInit();
#endif
	Board_LED_Set(0, false);
}



int main(void)
{
	prvSetupHardware();

	iniciar_udp_server();

	vTaskStartScheduler();

	return 1;
}
