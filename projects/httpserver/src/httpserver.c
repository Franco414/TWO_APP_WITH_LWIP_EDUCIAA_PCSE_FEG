/*
* modulo HTTP_SERVER
*/

#include "lwip/opt.h"

#if LWIP_NETCONN

#include "lwip/sys.h"
#include "lwip/api.h"

static struct netconn *conn;
static struct netbuf *buf;
static ip_addr_t *addr;
static unsigned short port;


const static char http_code404_hdr[] = "HTTP/1.1 404 Not Found.r\nContent-type: text/html\r\n\r\n";
const static char http_html_hdr[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";
const static char http_404_html[] = "<html><head><title>Congrats!</title></head><body><h1>Pagina no encontrada</h1><p>ingrese una nueva pagina</body></html>";
const static char http_index_html[] = "<html><head><title>Congrats!</title></head><body><h1>Pagina de inicio de mi http web serve en EDU-CIAA!</h1><p>implementado con LWIP + RTOS + NETCONN_API.</body></html>";
const static char http_LEDAON_html[] = "<html><head><title>Congrats!</title></head><body><h1>Estado del led 1 en la educiaa</h1><p>encendido.</body></html>";
const static char http_LEDAOFF_html[] = "<html><head><title>Congrats!</title></head><body><h1>Estado del led 1 en la educiaa</h1><p>apagado.</body></html>";

/*-----------------------------------------------------------------------------------*/
static void
http_server_netconn_serve(struct netconn *conn)
{
	struct netbuf *inbuf;
	char *datoRecivido;
	u16_t longDato;
	err_t err;

	err = netconn_recv(conn, &inbuf);

	if (err == ERR_OK) {
		netbuf_data(inbuf, (void**)&datoRecivido, &longDato);

		if (strncmp((char const *)datoRecivido,"GET /home",9)==0){
			netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);
			netconn_write(conn, http_index_html, sizeof(http_index_html)-1, NETCONN_NOCOPY);
		}else{
			if(strncmp((char const *)datoRecivido,"GET /LED1_ON",12)==0){
				Board_LED_Set(3, true);
				netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);
				netconn_write(conn, http_LEDAON_html, sizeof(http_LEDAON_html)-1, NETCONN_NOCOPY);
			}else{
				if(strncmp((char const *)datoRecivido,"GET /LED1_OFF",13)==0){
					Board_LED_Set(3, false);
					netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_NOCOPY);
					netconn_write(conn, http_LEDAOFF_html, sizeof(http_LEDAOFF_html)-1, NETCONN_NOCOPY);
				}else{
					netconn_write(conn, http_code404_hdr, sizeof(http_code404_hdr)-1, NETCONN_NOCOPY);
					netconn_write(conn, http_404_html, sizeof(http_404_html)-1, NETCONN_NOCOPY);
				}
			}
		}
	}

	netconn_close(conn);
	netbuf_delete(inbuf);
}
static void 
httpserver_thread(void *arg)
{
	LWIP_UNUSED_ARG(arg);
	struct netconn *conn, *connEntrante;
	err_t accept_err;

	conn = netconn_new(NETCONN_TCP);

	if (conn!= NULL)
	{
		if (netconn_bind(conn, NULL, 80)== ERR_OK)
		{
			netconn_listen(conn);
			while( true )
			{
				accept_err = netconn_accept(conn, &connEntrante);
				if(accept_err == ERR_OK)
				{
					http_server_netconn_serve(connEntrante);
					netconn_delete(connEntrante);
				}
			}
		}
	}
}
/*-----------------------------------------------------------------------------------*/

void http_server_init(void)
{
  sys_thread_new("udpecho_thread", httpserver_thread, NULL, DEFAULT_THREAD_STACKSIZE,DEFAULT_THREAD_PRIO );
}

/*-----------------------------------------------------------------------------------*/

#endif /* LWIP_NETCONN */
