
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"

//___________________________________________________________________________________________________________________ 
//                                                                                             | returns: thread ID | 
//                                                                                             | status: DONE       |
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

            return (Tid_t) init_ptcb;                                                            // !RETURN IS HERE! ////////////////////////////////////
    
        }

       else
       {     

	       return NOTHREAD;                                                                      // !RETURN IS HERE!
                                                                                                 // this option is for if there is no task
       }
    }
//                                                                                              |
//______________________________________________________________________________________________|













//_______________________________________________________________________________
//                                                       |  status: DONE        |
//  @brief Return the Tid of the current thread.         |  returns: thread ID  |
//                                                       |______________________|
    Tid_t sys_ThreadSelf()
    {


	   return (Tid_t) cur_thread()-> ptcbPointer;         // It calls the current thread to give its ID (PTCB pointer)


    }

//______________________________________________________________________________________________________________










//____________________________________________________________________________________________________________________________________________
//                                                                                                                   |status: DONE           |
//  @brief Join the given thread.                                                                                    |returns: integer status| 
//                                                                                                                   |_______________________|

int sys_ThreadJoin(Tid_t tid, int* exitval)
{

    PTCB wait_upon_ptcb = (PTCB*)tid;  // Tid was casted as a PTCB pointer, as it was given




    switch(wait_upon_ptcb){                                                               
//_________________________________________|switch case
   
    case (NULL):                                   
        return -1;
        break;

//_________________________________________|Check for if wait_upon_ptcb is NULL

    
    case (cur_thread()->wait_upon_ptcb):            
        return -1;
        break;

//__________________________________________________________________|Check if the thread is calling itself 
   
    case (rlist_find(&CURPROC->ptcb_list,wait_upon_ptcb,NULL)==NULL):   
        return -1;
        break;

//__________________________________________________________________|Check if the PTCB exists
   
   case(c-> detached == 1):             
        return -1
        break;

//_________________________________________| Check the thread's (detached) condition variable
   
    }

    wait_upon_ptcb-> refcount+ =1;                      // increment threads that wait for (wait_upon_ptcb)



    while(wait_upon_ptcb->detached==0 && wait_upon_ptcb->exited==0){

        kernel_wait(&ptcb->exit_cv,SCHED_USER);         // we must wait for if exited becomes 1 or detached becomes 1
    }

    if(wait_upon_ptcb->detached==1){                    // if after join the (wait_upon_ptcb) becomes 1, return -1
        minRefCount(wait_upon_ptcb);
        return -1;
    }
    

    if(exitval!=NULL){
        *exitval= wait_upon_ptcb->exitval;              // saves the exitval to the given adress
    }


    minRefCount(wait_upon_ptcb);


	return 0;
}
//                                                                                                                                      |        
//______________________________________________________________________________________________________________________________________|












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

//_______________________________________________________|










//_______________________________________________________
//                                                      |
// @brief Terminate the current thread.                 |
//  
    void sys_ThreadExit(int exitval)
    {



        PTCB* ptcb_for_exit = cur_thread()-


    }
//_______________________________________________________









//_________________________________________________________
//                                                        |
// this routine frees a PTCB for good looks and not pasta |

   void minRefCount(PTCB* ptcb){

        ptcb-> refcount--;

        if(refcount==0){
            rlist_remove(&ptcb->ptcb_list_node);
            free(ptcb);
        }
   }
//_______________________________________________________
