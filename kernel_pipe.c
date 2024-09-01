
#include "tinyos.h"
#include <stdlib.h>
#include "kernel_pipe.h"
#include "kernel_dev.h"
#include "kernel_streams.h"







//_________________________________________________________________________________

	PICB* init_pipe(){


		PICB* init_pipe= (PICB*) malloc(sizeof(PICB));

		init_pipe->has_space= COND_INIT;
		init_pipe->has_data= COND_INIT;
		init_pipe->w_position= 0;
		init_pipe->r_position= 0;
		init_pipe->reader= NULL;
		init_pipe->writer= NULL;
		init_pipe->writtenBytes= 0;

		return init_pipe;

}
//         ᕙ(•̀‸•́‶)ᕗ  pipe made here, BUT IT LACKS BULK
//_________________________________________________________________________________














//_____________________________SET_OF_OPERATIONS_IN_USE___________________________


		file_ops reader_file_ops={

			.Open = open_unavailable,

			.Read = pipe_read,  

			.Write = no_write,

			.Close = pipe_reader_close
		};


		file_ops writer_file_ops={

			.Open = open_unavailable,

			.Read = no_read,

			.Write = pipe_write,

			.Close = pipe_writer_close
		};

/*-----------------------------------MANUAL-------------------------------------   

		ᕙ(•̀ ᗜ •́ )ᕗ  these bad boys are pretty useful.
 				

 		ᕦ(ಠ‿‿ಠ)ᕥ    yeah, it is not supposed to use OPEN so we put NULL so
 				    that the programmer that messes can have a good time. :D

 		ᕙ(•̀ ᗜ •́ )ᕗ  Read is not used in writer so NO reading for him, but 
 					  FOR READER no write for him that's that.
⠀⠀⠀⠀ (\__/)⠀⠀⠀⠀⠀
⠀⠀⠀⠀ (•ㅅ•)⠀⠀		
⠀＿  ノヽ⠀ノ＼＿__   AND for the close is...close...
/⠀️⠀ Y⠀⌒Ｙ⌒⠀Ｙヽ  \  They are POINTER-to-FUNCTION
(⠀️⠀️⠀️(三ヽ人⠀⠀/⠀ ⠀) 
|⠀️⠀️⠀️ﾉ⠀¯¯\⠀￣￣ヽ ノ
ヽ＿＿＿⠀⠀＞､＿_／
⠀⠀⠀｜(⠀王⠀)〈⠀⠀    
⠀⠀⠀/⠀ﾐ`——彡⠀\     

_________________________________________________________________________________*/

















//_______________________________________________________________________________________________
//															|returns: integer/status of function|
//															|___________________________________|

int sys_Pipe(pipe_t* pipe){


	Fid_t fd[2];
	FCB* fcb[2];


	if(!FCB_reserve(2,fd,fcb)){ return -1; } //    Reserve two (fid_t) for reader and writer 

  
//  assign reader and writer 


	pipe->read = fd[0]; //    (read)   The read end of the pipe 
	pipe->write= fd[1]; //    (write)  The write end of the pipe 


//  allocate the new pipe

	PICB* new_Pipe = init_pipe(); 

	new_Pipe->reader= fcb[0];
	new_Pipe->writer= fcb[1];


	fcb[0]->streamobj= new_Pipe;  // it gives stream object a pipe object          
	fcb[1]->streamobj= new_Pipe;	

	fcb[0]->streamfunc=&reader_file_ops; // it takes what it was given in pipe file ops
	fcb[1]->streamfunc=&writer_file_ops;  
	
	return 0;

}
/*--------------------------------------MANUAL------------------------------------------   

		ᕙ(•̀ ᗜ •́ )ᕗ  Do not sumbit to the horrors and fears of the unknown
 				

 		ᕦ(ಠ‿‿ಠ)ᕥ   This is actually pretty chill...
 				    

⠀⠀⠀⠀ (\__/)⠀⠀⠀⠀⠀
⠀⠀⠀⠀ (•ㅅ•)⠀⠀		
⠀＿  ノヽ⠀ノ＼＿__   kernel_streams.h & kernel_dev.h
/⠀️⠀ Y⠀⌒Ｙ⌒⠀Ｙヽ  \  		give great explanations
(⠀️⠀️⠀️(三ヽ人⠀⠀/⠀ ⠀) 
|⠀️⠀️⠀️ﾉ⠀¯¯\⠀￣￣ヽ ノ
ヽ＿＿＿⠀⠀＞､＿_／
⠀⠀⠀｜(⠀王⠀)〈⠀⠀    
⠀⠀⠀/⠀ﾐ`——彡⠀\     

___________________________________________________________________________________________*/












/*___FIB_RESERVE_______________________________________________________________________________


					
					open = -1
				   write = -1
						|
___FIDT____ 		____|____
|		  |			|		|
|  r:FCB  |-----> R |  FCB	|￣￣￣￣￣￣|		  _________
|---------|			|_______|		   |______/-->|       |
|  w:FCB  |										  | PIPE  |
|_________|										  |  CB   |
	|								   |--------->|_______|
	|				_________          |
	|＿＿＿＿/---> W |		|		   |
				    |  FCB	|__________|
					|_______|
						|
						|
					read = -1
					open = -1
	


___________________both_file_descriptors_end up_in_the_same_pipe_________________________________*/















//_______________________________________________________________________________________________________________________________________
//																									|returns: integer/status of function|
//																									|___________________________________|

int pipe_write(void* pipecb_t, const char *buf, unsigned int n){


	PICB* pipe= (PICB*) pipecb_t; 														// we turn it into a variable 


	if(pipe==NULL || pipe->reader==NULL || pipe->writer== NULL) { return -1; }			// we check the existance of pipe,reader,writer


	
	int bytes_To_Write= 0;			/* bytes to write */

	while(pipe->writtenBytes == PIPE_BUFFER_SIZE ){										// if the buffer is full we it waits for free space	
		kernel_broadcast(&pipe->has_data);
		kernel_wait(&pipe->has_space , SCHED_PIPE);				
	}
	
// --------------------------------------------------------------------------------

	int index= 0;															


	if(n > (PIPE_BUFFER_SIZE - pipe->writtenBytes)){  bytes_To_Write= PIPE_BUFFER_SIZE - pipe->writtenBytes;  }
	

		else{ bytes_To_Write= n; }


	while(index < bytes_To_Write)
	{

		pipe->BUFFER[pipe->w_position]= buf[index];  index++;


		pipe->writtenBytes++;

		pipe->w_position++;

		
		if(pipe->w_position >= PIPE_BUFFER_SIZE){  pipe->w_position= pipe->w_position - PIPE_BUFFER_SIZE;  }
		

	}
	
	kernel_broadcast(&pipe->has_data); 	

	return bytes_To_Write;
}
//_______________________________________________________________________________________________________________________________________













//_______________________________________________________________________________________________________________________________________
//																									|returns: integer/status of function|
//																									|___________________________________|

int pipe_read(void* pipecb_t, char *buf, unsigned int n)
{

	PICB* pipe= (PICB*) pipecb_t;										// we turn it into a variable 




	if( pipe->writer == NULL && pipe->writtenBytes==0){ return 0; }		// if writer doesn't exist and writtenBytes is 0 then, job is done

	if(pipe==NULL || pipe->reader==NULL ){ return -1; }					// checks for pipe,reader existance





	while(pipe->writtenBytes == 0){
		
		kernel_broadcast(&pipe->has_space);
		kernel_wait(&pipe->has_data, SCHED_PIPE);
	}


	if( pipe->writer == NULL && pipe->writtenBytes==0){ return 0; }


// --------------------------------------------------------------------------------

	int bytes_To_Read= 0, index =0;




	if(n>pipe->writtenBytes){  bytes_To_Read= pipe->writtenBytes;  }
	
		else{  bytes_To_Read= n;  }
	



	while( index < bytes_To_Read ){

		buf[index]= pipe->BUFFER[pipe->r_position];  index++;


		pipe->r_position++;


		pipe->writtenBytes--;


		if(pipe->r_position>=PIPE_BUFFER_SIZE){  pipe->r_position= pipe->r_position- PIPE_BUFFER_SIZE;  }
	}

	kernel_broadcast(&pipe->has_space); 	

	return bytes_To_Read;
}
//_______________________________________________________________________________________________________________________________________














//_______________________________________________________________________________________________________________________________________
//																									|returns: integer/status of function|
//																									|___________________________________|


int pipe_reader_close(void* _pipecb)
{

	PICB* pipe= (PICB*) _pipecb;


	if(pipe == NULL || pipe->reader== NULL){ return -1; }

	
	pipe->reader= NULL;


	if(pipe->writer == NULL){ free(pipe); }


	else{ kernel_broadcast(&pipe->has_space); }
	

	return 0;
}
//_______________________________________________________________________________________________________________________________________
















//_______________________________________________________________________________________________________________________________________
//																									|returns: integer/status of function|
//																									|___________________________________|


int pipe_writer_close(void* _pipecb){

	PICB* pipe= (PICB*) _pipecb;


	if(pipe == NULL || pipe->writer== NULL){ return -1; } 


	pipe->writer= NULL;


	if(pipe->reader == NULL){ free(pipe); }


	else{ kernel_broadcast(&pipe->has_data); }


	return 0;
}
//_______________________________________________________________________________________________________________________________________













//_________________________________________________________________________________

	int no_read(void* pipe, char *buf, unsigned int size){ return -1; }


	int no_write(void* pipe, const char *buf, unsigned int size){ return -1; }
//_________________________________________________________________________________









//__________________________________UPDATABLE______________________________________

	int open_unavailable(){ return -1; }

//_________________________________________________________________________________








