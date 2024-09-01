#include "tinyos.h"
#include "kernel_pipe.h"
#include "util.h"
#include "kernel_streams.h"
#include "kernel_dev.h"
#include "kernel_cc.h"



typedef enum {
    UNBOUND,
    LISTENER,
    PEER
}socket_type;

typedef struct listener_socket{
    rlnode queue;
    CondVar req_available;
}listener_socket;


typedef struct peer_socket{
    PICB* write_pipe;
    PICB* read_pipe;
}peer_socket;



//████████████████████████████████████████████████████████████████████████████████████████████████████████████████

    typedef struct socket_control_block{
        uint ref_count;
        FCB* fcb;
        socket_type type;
        port_t port;
        
        union{
            listener_socket listener_s;
            peer_socket peer_s;
        };

    }SOCB;

//████████████████████████████████████████████████████████████████████████████████████████████████████████████████








//████████████████████████████████████████████████████████████████████████████████████████████████████████████████
    
    typedef struct connection_request_control_block{
	   int admitted;
       SOCB* peer;
       CondVar connected_cv;
       rlnode queue_node;
    }connection_request;
//████████████████████████████████████████████████████████████████████████████████████████████████████████████████
    


    SOCB* PORT_MAP[MAX_PORT + 1];




//____________________kernel_socket.c___________________________

Fid_t sys_Socket(port_t port);
int sys_Listen(Fid_t sock);
Fid_t sys_Accept(Fid_t lsock);
int sys_Connect(Fid_t sock, port_t port, timeout_t timeout);
int sys_ShutDown(Fid_t sock, shutdown_mode how);
int sys_open();
//______________________________________________________________



