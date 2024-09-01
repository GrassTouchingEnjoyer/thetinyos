
#include "tinyos.h"
#include "kernel_socket.h"


int socket_write(void* socket, const char *buf, unsigned int n);
int socket_read(void* socket, char *buf, unsigned int n);
int socket_close(void* socket);



//________________-create a socket-_________________

SOCB* create_socket(){

	SOCB* init_socket = (SOCB*) xmalloc(sizeof(SOCB));

	init_socket -> ref_count = 1;
    init_socket -> fcb = NULL;
    init_socket -> type = UNBOUND;
    init_socket -> port = NOPORT;

    return init_socket;

}

//__________________________________________________





//________________-_-SOCKET OPS-_-__________________

static file_ops sock_ops = {
  .Open = sys_open,
  .Read = socket_read,
  .Write = socket_write,
  .Close = socket_close
};

//__________________________________________________

/*--------------------------------SO FAR WE...-------------------------------------   

⠀⠀⠀⠀ (\__/)⠀⠀⠀⠀⠀
⠀⠀⠀⠀ (•ㅅ•)⠀⠀		
⠀＿  ノヽ⠀ノ＼＿__    Made a function for a socket to be created, PLUS 
/⠀️⠀ Y⠀⌒Ｙ⌒⠀Ｙヽ  \   we made the O P E R A T I O N S for the sockets.
(⠀️⠀️⠀️(三ヽ人⠀⠀/⠀ ⠀) 
|⠀️⠀️⠀️ﾉ⠀¯¯\⠀￣￣ヽ ノ
ヽ＿＿＿⠀⠀＞､＿_／
⠀⠀⠀｜(⠀王⠀)〈⠀⠀    
⠀⠀⠀/⠀ﾐ`——彡⠀\     

___________________________________________________________________________________*/






//____________________-SYS_SOCKET-__________________________________-

Fid_t sys_Socket(port_t port)
{

	Fid_t Fd;
	FCB *fcb;



	if(port< NOPORT||port>MAX_PORT){return NOFILE;}

	


		if(FCB_reserve(1,&Fd,&fcb))
		{

			SOCB* init_socket = create_socket();


			init_socket -> port = port;
    		init_socket -> fcb = fcb;

    		fcb -> streamobj = init_socket;
    		fcb -> streamfunc = &sock_ops;

    		return Fd;
		}



	return NOFILE;
}

//_________________________________________________________________











//____________________________________________________-SYS_LISTEN-___________________________________________________________-


int sys_Listen(Fid_t sock)
{


// CHECK IF SOCK IS DIRTY!!!!-------------------------


	if(sock<0 || sock>MAX_FILEID){return -1;}

//----------------------------------------------------





//-------------------------------------------------------

	FCB* sock_fcb=get_fcb(sock); // go GET that sock!!!!

//-------------------------------------------------------






//CHECK IF SOCK EVEN EXISTS---------------------------
	

	if(sock_fcb == NULL){return -1;}

//----------------------------------------------------
	


//-----------------------------------------------------------------------------------------------------------------------

	SOCB* listener = (SOCB*)sock_fcb->streamobj; // set listener the stream object of sock after checking that it exists

//-----------------------------------------------------------------------------------------------------------------------	




// Every CHECK!!!!!!----------------------------------


	if(listener == NULL ){return -1;}

	if(listener->port == NOPORT){return -1;}

	if (PORT_MAP[listener->port] != NULL){return -1;}

	if (listener->port > MAX_PORT){return -1;}

	if(listener->type > UNBOUND){return -1;}

//---------------------------------------------------






//-----------------------------------------------------------------------------------------------------------------------	

/*				   INSTALL SOCKET TO THE PORT!!!
__________      	             						  	  							______
|   _____|  		  								     								  ______|    |
|   |_____ 			  								  									 |_______    |
|   _____|	  */PORT_MAP[listener->port]=listener;/*	 _______|    |
|   |_____  		   								   									 |_______    |
|________|             		 							       							   |___|
*/





//-------------------------------------------------------------------

//              SOCKET IS NOW A LISTENER SOCKET

				listener->type= LISTENER; 
				
//-------------------------------------------------------------------





//-------------------------------------------------------------------
// 			   INITIALIZING LISTENER


			   rlnode_init(&listener->listener_s.queue,NULL); 
				 
			   listener->listener_s.req_available= COND_INIT;



			   return 0; //    ᕙ(•̀ ᗜ •́ )ᕗ  
//-------------------------------------------------------------------


//-----------------------------------------------------------------------------------------------------------------------	


}
//__________________________________________________________________________________________________________________________









//___________________________________________________-SYS_ACCEPT-_______________________________________________________________-


Fid_t sys_Accept(Fid_t lsock)
{

//_____CHECK_lsock_VALIDITY__________________

	if(lsock < 0){return NOFILE;}

	if(lsock > MAX_FILEID-1){return NOFILE;}
//___________________________________________



	FCB* temp_fcb= get_fcb(lsock);


//___CHECK_IF_LEGAL__________________________

	if(temp_fcb == NULL){return NOFILE;}

//___________________________________________



	SOCB* listener_sock = (SOCB*)temp_fcb->streamobj;



//___CHECK_listener_VALIDITY_______________________________

	if(listener_sock == NULL){return NOFILE;}

	if(listener_sock->type != LISTENER){return NOFILE;}

	if(temp_fcb->streamfunc != &sock_ops){ return NOFILE; }
//__________________________________________________________


	listener_sock->ref_count ++; // increase refcount so that it won't be deleted

	

//___PLEASE_WAIT_UNTIL_SOMETHING_COMES_UP____________________________________________________________________

	while(is_rlist_empty(&listener_sock->listener_s.queue) && PORT_MAP[listener_sock->port]==listener_sock){
		kernel_wait(&listener_sock->listener_s.req_available,SCHED_USER);
	}
//___________________________________________________________________________________________________________


	if(PORT_MAP[listener_sock->port] != listener_sock ){return NOFILE;}



//_____________________________________________________________________________________________

//  PUSH REQUEST => LISTENER'S LIST

	connection_request* request= rlist_pop_front(&listener_sock->listener_s.queue)->obj;
	


//__CREATE SOCKET FOR PEAR-TO-PEAR CONNECTION__________________________________________________

	Fid_t fid= sys_Socket(listener_sock->port);
    FCB* fcb= get_fcb(fid);
//_____________________________________________________________________________________________



//___CHECK_VALIDITY_OF_FILE____________________________________________________________________


	if(fcb == NULL){return NOFILE;}

//_________________________________________________BUILDING_PEER_CONNECTION_______________________________________________________


	SOCB* peer_one= (SOCB*)fcb->streamobj;
	SOCB* peer_two= request-> peer;

	peer_one->type= PEER;
	peer_two->type= PEER;

	PICB *first_pipe= init_pipe();	//INITIALIZE PIPE
	PICB *second_pipe= init_pipe();	//INITIALIZE PIPE

	first_pipe->reader= peer_one->fcb;
	first_pipe->writer= peer_two->fcb;

	second_pipe->reader= peer_two->fcb;
	second_pipe->writer= peer_one->fcb;

	peer_one->peer_s.read_pipe= first_pipe;
	peer_one->peer_s.write_pipe=  second_pipe;
	
	peer_two->peer_s.write_pipe=  first_pipe;
	peer_two->peer_s.read_pipe= second_pipe;

	request->admitted= 1;

	kernel_signal(&request->connected_cv);

	listener_sock->ref_count--;

	return	fid;

}

//__________________________________________________________________________________________________________________________________







//________________________________________-SYS_CONNECT-____________________________________________________________-

int sys_Connect(Fid_t sock, port_t port, timeout_t timeout)
{


// DO ALL CHECKS HERE

//____________________CHECKS_________________________ 
 
 
 if((sock<0) || sock > (MAX_FILEID-1)){ return -1; }

//__________________________________________________


  FCB* tmp = get_fcb(sock);




//____________________CHECKS_________________________ 

  
  if(tmp == NULL){return -1;}

//__________________________________________________





  SOCB* sock_req = (SOCB*)tmp->streamobj;





//____________________CHECKS________________________

  if(port<0){ return -1; }


  if(port>MAX_PORT){ return -1; } 


  if(PORT_MAP[port] == NULL) { return -1; }


  if(sock_req->type != UNBOUND){ return -1; }

//__________________________________________________




// MAKE REQUEST


  connection_request* request = (connection_request*)xmalloc(sizeof(connection_request));
  request->peer = sock_req;
  request->connected_cv = COND_INIT;

//____________________________________________________________________________________________________


// WAIT

  request->admitted = 0;
//_________________________




  sock_req->ref_count++;



// CHECK IF LISTENER SOCK IS ATTACHED TO PORT

  if(PORT_MAP[port]->type!=LISTENER){return -1;}  



  //ADD REQ TO LISTENER'S LIST

  rlnode_init(&request->queue_node, request);
  rlist_push_back(&PORT_MAP[port]->listener_s.queue, &request->queue_node);




  //SIGNAL A WAITER

  kernel_signal(&PORT_MAP[port]->listener_s.req_available);

  

 //WAKE UP FROM TIME OUT IF NOT SERVED
  kernel_timedwait(&request->connected_cv,SCHED_USER,timeout);



  sock_req->ref_count--;


  	if(request->admitted == 1){ return 0; }
  		
  		else {return -1;}
}


//_________________________________________________________________________________________________________________






/*
⣿⣿⣿⣿⣿⣿⣿⣷⣿⣿⣿⣿⣿⣿⣿⣷⣷⣶⣶⣷⣷⣷⣶⣶⣶⣶⣶⣶⣶⣶⣦⣄⠻⣶⣶⣶⣶⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣬⠻⣿⣿⡿⣿⡿⠿⠛⣻⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⡈⠶⠖⠛⣋⣉⣉⣭⣤⣤⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣿⣿⠋⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠿⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⠘⣛⣉⣭⣥⣴⣶⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⡿⠁⢲⣿⣿⣿⣿⣿⣿⣿⣿⡟⠁⠔⠒⠈⣉⡉⠛⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣇⠹⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣉⣉⣩⣥⣤⣤⠆⣰⣿⣿⣿⣿⣿⣿⣿⣿⣋⣤⣤⣶⢾⣿⣿⣦⣄⠀⠈⠻⢿⣿⣿⣿⣿⣿⣿⣿⣿⡆⢻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⡟⢰⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠋⠀⠀⢈⣈⡀⠙⠛⠷⢦⡀⠀⡹⠋⠛⢻⣿⣿⣿⣿⡇⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⠟⠛⢿⠇⢻⣿⣿⣿⣿⣿⣿⣿⡿⠛⠀⠀⠠⠖⠛⠋⠛⠳⠀⠀⠀⠙⠆⠀⡾⠀⠀⠻⠿⢿⣿⡇⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⡟⠀⣴⣄⡀⢾⣿⣿⣿⣿⣿⣿⣿⣿⡇⢀⣀⣤⡙⠂⠀⠀⡄⠀⠀⠀⠀⣴⣾⣧⠀⠀⠀⠀⠀⠀⠃⠘⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⡇⠀⣾⣿⣿⣦⣹⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣭⣀⠀⠐⠃⠀⢀⣠⣼⣿⣿⣿⠀⠀⠀⠀⠀⢤⠀⢠⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⡇⠘⠋⠀⢠⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠅⣠⣼⣿⣿⣿⣿⣿⣿⣦⠀⠀⠀⠀⠀⠀⡾⠿⠿⠿⠿⠿⠟⠿⠿⡿⡿⡿
⣿⣿⡀⣀⠀⢻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⢹⣦⣤⣤⣦⡄⠉⠉⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⢸⣿⣧⣼⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⢡⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⢠⡀⠀⠹⣿⠇⣸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡻⢃⣾⡿⠿⢿⣿⣿⣿⠙⠻⣿⠟⢻⠇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠈⠓⡄⠀⢀⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣧⠘⠏⠀⢀⣀⣙⡛⠋⠀⠀⠈⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠇⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣤⣤⣤⣭⠀⠀⠀⢀⣠⣆⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⣇⠀⠀⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⢩⣿⡟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⢀⠁⠀⠇⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠋⠹⠿⠿⠛⠋⠉⠉⠁⠀⠈⠙⠋⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⢸⠀⠀⢤⢸⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠟⡁⢠⣤⣤⣄⣤⣤⣤⣀⣀⣀⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⣬⡾⠓⠀⣸⣿⣿⣿⣿⣿⡟⢿⣿⣿⣿⣿⣿⡏⢀⠄⣸⣿⣿⣿⣿⣿⠟⠉⠉⠉⠉⠀⠀⢀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⡯⠐⣡⣾⣿⣿⣿⣿⣿⣿⠻⣎⠻⣿⣿⣿⣿⣇⢘⣠⣹⣿⣿⣿⣿⣧⣀⡀⣀⣀⠀⢀⠀⣼⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⢁⣼⣿⣿⣿⣿⣿⣿⣿⣿⣧⡙⢷⣽⣿⣮⡻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⡟⠀⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣭⢻⣿⣿⣮⣻⣿⣿⣿⣿⣿⣿⣿⡿⠟⠛⠙⠙⠃⢰⣌⠣⡀⣄⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣾⣿⣿⣿⡟⠈⠙⠛⠿⠿⠿⠋⠀⠀⠀⠀⠀⢀⣄⣹⣇⠱⠘⣿⣷⣦⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠻⣿⣿⣿⠇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣾⠟⢻⣿⠀⡆⢸⡇⢿⣿⣷⣄⡀⠀⠀⠀⠀⠀⠀
*/
//___________________________________________-SYS_SHUTDOWN-________________________________________________________________________-

int sys_ShutDown(Fid_t sock, shutdown_mode how)
{
	

	if(sock<0 || sock>MAX_FILEID-1){return -1;} // check validity of sock


	FCB* sock_sht = get_fcb(sock);


	if(sock_sht == NULL){return -1;} // check if NULL



	SOCB* sock_strobj = (SOCB*)sock_sht->streamobj;	// get streamobj which related to the two pipes



	if(sock_strobj == NULL){return -1;} // if communication devices don't exist then Shutdown is not important



	if(sock_strobj->type != PEER){return -1;} // there must be a connection to Shutdown if not then return -1
	



	
//      based on (how) mode Delete the following pipe



//------------------------------------------------------------------
	switch(how) {
//------------------------------------------------------------------

		case SHUTDOWN_READ:

			pipe_reader_close(sock_strobj->peer_s.read_pipe);
			if(sock_strobj->peer_s.write_pipe!=NULL){return -1;}
			

			break;

//------------------------------------------------------------------

		case SHUTDOWN_WRITE:

			pipe_writer_close(sock_strobj->peer_s.write_pipe);
			if(sock_strobj->peer_s.write_pipe!=NULL){return -1;}
			

			break;

//------------------------------------------------------------------

		case SHUTDOWN_BOTH:


			pipe_reader_close(sock_strobj->peer_s.read_pipe);
			if(sock_strobj->peer_s.write_pipe!=NULL){return -1;}

			
			pipe_writer_close(sock_strobj->peer_s.write_pipe);
			if(sock_strobj->peer_s.write_pipe!=NULL){return -1;}

			
			break;

//------------------------------------------------------------------

		default:
			return -1;

//------------------------------------------------------------------
	}//-------------------------------------------END OF SWITCH-CASE	

return 0;
}
//____________________________________________________________________________________________________________________________________












//__________________________________________________________________

int socket_write(void* socket, const char* buf, unsigned int n){
  

  SOCB* socket_wrtr = (SOCB*)socket;



 if(socket_wrtr == NULL){return -1;}

 if(socket_wrtr->type != PEER){return -1;}


 return pipe_write(socket_wrtr->peer_s.write_pipe, buf, n);
}
//__________________________________________________________________







//__________________________________________________________________

int socket_read(void* socket, char *buf, unsigned int n){
   
  SOCB* socket_rdr = (SOCB*)socket;


  if(socket_rdr == NULL){return -1;}


  if (socket_rdr->type != PEER){return -1;}


  return pipe_read(socket_rdr->peer_s.read_pipe,buf,n);
}
//__________________________________________________________________










/*
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠛⠛⠛⠻⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠿⠿⠟⠿⣿⠟⠁⠀⠀⠀⠀⠀⠀⠉⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠟⠛⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢰⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠟⠛⠉⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢘⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⠟⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣶⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢹⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⠟⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣠⣤⣾⣷⣿⣿⣿⣿⣧⡀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⠟⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣇⠀⠀⠀⠀⠀⠀⠀⢰⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠘⠿⠿⠿⠿⠿⠿⢿⣿⣿⣿⣿⣿⠟⠀⠀⠀⠀⠀⠀⠀⠘⠻⢿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠙⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿
⣿⣿⣦⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠉⠙⠿⣿⣿⣿⣿⣿⣿
⣿⣿⣿⣿⣶⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠘⢿⣿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣷⣦⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀   ⠀⠀socket_close⠀⠀⠀     ⠈⢿⣿⣿⣿
⣿⣿⣿⣿⣿⣿⣿⣿⣿⣧⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⣿⣿⣿
___________________________________________________________________________________________________________________________________
*/

int socket_close(void* socket){


SOCB* socket_clsd= (SOCB*) socket;	


//--------------------------------------------------------------------------------
//  Check if it already closed, if it is not close it based on type



	if(socket_clsd == NULL){ return -1; }

//--------------------------------------------------------------------------------
 



//--------------------------------------------------------------------------------
// IF it is peer we MUST close the pipes before closing the socket
  	 
  	 if(socket_clsd->type == PEER)
  	 {
  	 
  	  pipe_reader_close(socket_clsd->peer_s.read_pipe);  
  	  socket_clsd->peer_s.read_pipe = NULL;


  	  pipe_writer_close(socket_clsd->peer_s.write_pipe);
  	  socket_clsd->peer_s.write_pipe = NULL;


  	  socket_clsd->ref_count--;
 	
	 }
//--------------------------------------------------------------------------------





//---------------------------------------------------------------------------------------------
// IF it is LISTENER we must delete socket from port table and wake up all the waiting requests


  	 else if(socket_clsd->type ==  LISTENER)
  	  {

      	PORT_MAP[socket_clsd->port] = NULL;
 
      	kernel_broadcast(&socket_clsd->listener_s.req_available);  
     
        socket_clsd->ref_count--;
		
	  }
//--------------------------------------------------------------------------------
      		




// IF it is UNBOUND we must decrease ref count------------------------------------


     else if(socket_clsd->type == UNBOUND){ socket_clsd->ref_count--; }

//--------------------------------------------------------------------------------







// else return error--------------------------------------------------------------
  	  				
  	 else{return -1;}

//--------------------------------------------------------------------------------






//---IF THE SOCKET'S REF COUNT BECOMES ZERO IT MUST BE GOODBYED-----


 		if(socket_clsd->ref_count == 0){free(socket_clsd);}	

 		return 0;

}

//______________________________________________________________________________________________________________









/*⠀⠀			sus_open

  ⠀⠀⠀⠀⢀⣤⠤⠤⠤⠤⠤⠤⠤⠤⠤⠤⢤⣤⣀⣀⡀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⢀⡼⠋⠀⣀⠄⡂⠍⣀⣒⣒⠂⠀⠬⠤⠤⠬⠍⠉⠝⠲⣄⡀⠀⠀
⠀⠀⠀⢀⡾⠁⠀⠊⢔⠕⠈⣀⣀⡀⠈⠆⠀⠀⠀⡍⠁⠀⠁⢂⠀⠈⣷⠀⠀
⠀⠀⣠⣾⠥⠀⠀⣠⢠⣞⣿⣿⣿⣉⠳⣄⠀⠀⣀⣤⣶⣶⣶⡄⠀⠀⣘⢦⡀
⢀⡞⡍⣠⠞⢋⡛⠶⠤⣤⠴⠚⠀⠈⠙⠁⠀⠀⢹⡏⠁⠀⣀⣠⠤⢤⡕⠱⣷
⠘⡇⠇⣯⠤⢾⡙⠲⢤⣀⡀⠤⠀⢲⡖⣂⣀⠀⠀⢙⣶⣄⠈⠉⣸⡄⠠⣠⡿
⠀⠹⣜⡪⠀⠈⢷⣦⣬⣏⠉⠛⠲⣮⣧⣁⣀⣀⠶⠞⢁⣀⣨⢶⢿⣧⠉⡼⠁
⠀⠀⠈⢷⡀⠀⠀⠳⣌⡟⠻⠷⣶⣧⣀⣀⣹⣉⣉⣿⣉⣉⣇⣼⣾⣿⠀⡇⠀
⠀⠀⠀⠈⢳⡄⠀⠀⠘⠳⣄⡀⡼⠈⠉⠛⡿⠿⠿⡿⠿⣿⢿⣿⣿⡇⠀⡇⠀
⠀⠀⠀⠀⠀⠙⢦⣕⠠⣒⠌⡙⠓⠶⠤⣤⣧⣀⣸⣇⣴⣧⠾⠾⠋⠀⠀⡇⠀
⠀⠀⠀⠀⠀⠀⠀⠈⠙⠶⣭⣒⠩⠖⢠⣤⠄⠀⠀⠀⠀⠀⠠⠔⠁⡰⠀⣧⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠛⠲⢤⣀⣀⠉⠉⠀⠀⠀⠀⠀⠁⠀⣠⠏⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠉⠉⠛⠒⠲⠶⠤⠴⠒⠚⠁⠀⠀
*/

//________-SYS_OPEN-________________________________________________-

int sys_open()
{
	return -1;
}

//__________________________________________________________________