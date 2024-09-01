#include <util.h>
#include <kernel_cc.h>



#define PIPE_BUFFER_SIZE 16384



/*

	████████████████████████████
	█		 				   █	██   █
	█		PIPE CONTROL 	   █ 		  █
	█	    	BLOCK   	   █		  █
	█					       █	 █	 █
	████████████████████████████		

*/


//████████████████████████████████████████████████████████████████████████████████████████████████████████████████

			typedef struct pipe_control_block {
	
					FCB *reader;
					FCB *writer;

					CondVar has_space;    /* For blocking writer if no space is available */
    				CondVar has_data;     /* For blocking reader until data are available */
	
			/*

		 			write, read position in buffer (it depends on your implementation of bounded buffer,
	 	 			i.e. alternatively pointers can be used) 

			*/

		 			int w_position, r_position;  
	

		 			char BUFFER[PIPE_BUFFER_SIZE];   /* bounded (cyclic) byte buffer */


		 			int writtenBytes;	/* Available bytes to read */
	

			} PICB;

//████████████████████████████████████████████████████████████████████████████████████████████████████████████████

  


	
//========================| PIPE OPERATIONS |============================
	


	int sys_Pipe(pipe_t* pipe);


	int pipe_write(void* pipecb_t, const char *buf, unsigned int n);

	int no_read(void* pipe, char *buf, unsigned int size);


	int pipe_read(void* pipecb_t, char *buf, unsigned int n);

	int no_write(void* pipe, const char *buf, unsigned int size);


	int pipe_writer_close(void* _pipecb);

	int pipe_reader_close(void* _pipecb);


	int open_unavailable();


//=======================================================================