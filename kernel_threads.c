
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"

//___________________________________________________________________________________________________________________ status: ONGOING
//                                                                                             | returns: thread ID |
// @brief Create a new thread in the current process.                                          |____________________|
//

    Tid_t sys_CreateThread(Task task, int argl, void* args)
    {

        assert(task != NULL);                                                                    // For smooth debugging experience

        if(task != NULL)                                                                         // For if the task is NOT equal to NULL, 
        {                                                                                        // if it is not run the routine normally.

            PTCB* init_ptcb;                                                                     // Initializing the new ptcb
            PCB* cur_proc = CURPROC;                                                             // current process is abducted      

            init_ptcb = ptcb_malloc(task, argl, args);                                           // Giving arguements to the new ptcb

            rlist_push_back(&cur_proc->ptcb_list , init_ptcb->ptcb_list_node);                   // Pushes to the back of current PCB's list,           
                                                                                                 // the initialized PTCB 'node'.
            
            init_ptcb-> tcb-> tcb_ptr_ptcb = init_ptcb;                                          // TCB gets, as a new arguement for its
                                                                                                 // PTCB pointer, the initialized PTCB pointer.

            cur_proc-> thread_count+ = 1;                                                        // Increment the current process, 
                                                                                                 // its thread count by one.
            wakeup(init_ptcb-> tcb);

            return (Tid_t) init_ptcb->tcb;                                                       // !RETURN IS HERE! ////////////////////////////////////
    
        }

       else
       {     

	       return NOTHREAD;                                                                      // !RETURN IS HERE!
                                                                                                 // this option is for if there is no task
       }
    }
//                                                                                              |
//______________________________________________________________________________________________|









//________________________________________________________
//                                                       |  status: done
//  @brief Return the Tid of the current thread.         |
//
    Tid_t sys_ThreadSelf()
    {


	   return (Tid_t) cur_thread()-> ptcbPointer;


    }

//_______________________________________________________





//________________________________________________________
//                                                       |
//  @brief Join the given thread.                        |
//
int sys_ThreadJoin(Tid_t tid, int* exitval)
{




/* 
                   █  █ █▀▀ █▀▀█ █▀▀ 
                   █▀▀█ █▀▀ █▄▄▀ █▀▀ 
                   ▀  ▀ ▀▀▀ ▀ ▀▀ ▀▀▀
*/





	return -1;
}
//_______________________________________________________








//_______________________________________________________
//                                                      |
//   @brief Detach the given thread.                    |
//
// 
   int sys_ThreadDetach(Tid_t tid)
   { 



/* 
                   █  █ █▀▀ █▀▀█ █▀▀ 
                   █▀▀█ █▀▀ █▄▄▀ █▀▀ 
                   ▀  ▀ ▀▀▀ ▀ ▀▀ ▀▀▀
*/





	   return -1;
   }

//_______________________________________________________







//_______________________________________________________
//                                                      |
// @brief Terminate the current thread.                 |
//  
    void sys_ThreadExit(int exitval)
    {



/* 
                   █  █ █▀▀ █▀▀█ █▀▀ 
                   █▀▀█ █▀▀ █▄▄▀ █▀▀ 
                   ▀  ▀ ▀▀▀ ▀ ▀▀ ▀▀▀
*/



    }
//_______________________________________________________
