
#include "udpecho.h"
#include "lwipopts.h"
#include "lwip/opt.h"

#if LWIP_NETCONN

#include "lwip/sys.h"
#include "lwip/api.h"


static struct netconn *conn;
static struct netbuf *buf;
static ip_addr_t *addr;
static unsigned short port;
/*-----------------------------------------------------------------------------------*/
static void udpecho_thread(void *arg)
{
  err_t err, recv_err;

  LWIP_UNUSED_ARG(arg);

  conn = netconn_new(NETCONN_UDP);
  if (conn!= NULL)
  {
    err = netconn_bind(conn, IP_ADDR_ANY, 1701);
    if (err == ERR_OK)
    {
      while (1)
      {
        recv_err = netconn_recv(conn, &buf);

        if (recv_err == ERR_OK)
        {
        void *dato;

        int lenghtDato;
        int contador=0;
        netbuf_data(buf, (void**)&dato, &lenghtDato);
        for(int i=0;i<4;i++){
        char cadenaBuscar[]="LED2";
        if(((char*)dato)[i]==cadenaBuscar[contador])contador++;
        else contador=0;
        }
        if(contador==4){
        	Board_LED_Toggle(4);
        }
          addr = netbuf_fromaddr(buf);
          port = netbuf_fromport(buf);
          netconn_connect(conn, addr, port);
          buf->addr.addr = 0;
          netconn_send(conn,buf);
          netbuf_delete(buf);
        }
      }
    }
    else
    {
      netconn_delete(conn);
    }
  }
}

/*-----------------------------------------------------------------------------------*/


void udpecho_init(void)
{
  sys_thread_new("udpecho_thread", udpecho_thread, NULL, DEFAULT_THREAD_STACKSIZE,DEFAULT_THREAD_PRIO );
}

/*-----------------------------------------------------------------------------------*/

#endif
