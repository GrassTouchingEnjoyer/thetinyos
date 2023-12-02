
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "kernel_streams.h"
#include "kernel_cc.h"

//___________________________________________________________________________________________________________________ 
//                                                                                             | returns: thread ID | 
//                                                                                             | status: DONE       |
// @brief Create a new thread in the current process.                                          |____________________|
//

    Tid_t sys_CreateThread(Task task, int argl, void* args)
    {

        if(task != NULL)                                                                         // For if the task is NOT equal to NULL, 
        {                                                                                        // if it is not run the routine normally.

            PTCB* init_ptcb = NULL;                                                              // Initializing the new ptcb
            PCB* cur_proc = CURPROC;                                                             // current process is abducted      

            init_ptcb = ptcb_alloc(task, argl, args);                                            // Giving arguements to the new ptcb


            init_ptcb-> tcb= spawn_thread(cur_proc,start_many_threads);                          // Spawning the new thread in ptcb. 
            
            init_ptcb-> tcb-> tcb_ptr_ptcb = init_ptcb;                                          // TCB gets, as a new arguement for its
                                                                                                 // PTCB pointer, the initialized PTCB pointer.

            rlist_push_back(&cur_proc-> ptcb_list , &init_ptcb-> ptcb_list_node);                // Pushes to the back of current PCB's list,           
                                                                                                 // the initialized PTCB 'node'.

            cur_proc-> thread_count += 1;                                                        // Increment the current process, 
                                                                                                 // its thread count by one.
            wakeup(init_ptcb-> tcb);

            return (Tid_t) init_ptcb;                                                            // !RETURN IS HERE! 
    
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


	   return (Tid_t) cur_thread()-> tcb_ptr_ptcb;         // It calls the current thread to give its ID (PTCB pointer)


    }

//______________________________________________________________________________________________________________










//____________________________________________________________________________________________________________________________________________
//                                                                                                                   |status: DONE           |
//  @brief Join the given thread.                                                                                    |returns: integer status| 
//                                                                                                                   |_______________________|

int sys_ThreadJoin(Tid_t tid, int* exitval)
{   

    PTCB* wait_upon_ptcb = (PTCB*)tid;       // Tid was casted as a PTCB pointer, as it was given
                                            
    if (wait_upon_ptcb == NULL){return -1;}                                  
//____________________________________________________________________________|Check for if wait_upon_ptcb is NULL


    if (wait_upon_ptcb == cur_thread()-> tcb_ptr_ptcb){ return -1;}            
//____________________________________________________________________________|Check if the thread is calling itself 

   
    if (rlist_find(&CURPROC->ptcb_list,wait_upon_ptcb,NULL)==NULL){return -1;}
      
//____________________________________________________________________________|Check if the PTCB exists

   
   if(wait_upon_ptcb-> detached == 1){return -1;}

//____________________________________________________________________________| Check the thread's (detached) condition variable
   
    

    wait_upon_ptcb-> ref_count +=1;                     // increment threads that wait for (wait_upon_ptcb)



    while(wait_upon_ptcb->detached==0 && wait_upon_ptcb->exited==0){

        kernel_wait(&wait_upon_ptcb-> exit_cv,SCHED_USER);         // we must wait for if exited becomes 1 or detached becomes 1
    }

    if(wait_upon_ptcb->detached==1){                              // if after join the (wait_upon_ptcb) becomes 1, return -1
        minRefCount(wait_upon_ptcb);
        return -1;
    }
    

    if(exitval!=NULL){
        *exitval= wait_upon_ptcb->exitval;                        // saves the exitval to the given adress
    }


    minRefCount(wait_upon_ptcb);


	return 0;
}

//                                                                                                                                      |        
//______________________________________________________________________________________________________________________________________|












//____________________________________________________________________________________________________________________________________________
//                                                                                                                   |status: DONE           |
//  @brief Detach the given thread.                                                                                  |returns: integer status| 
//                                                                                                                   |_______________________|

   int sys_ThreadDetach(Tid_t tid)
   { 


        PTCB* detach_ptcb = (PTCB*)tid;


        if(detach_ptcb == NULL){return -1;}

//_______________________________________________________| if (detach_ptcb) does not exist 

        if(rlist_find(&CURPROC->ptcb_list,detach_ptcb,NULL)==NULL){return -1;}

//_____________________________________________________________________________| checks the thread/ptcb if it exists in its workload, good stuff!

        if(detach_ptcb-> detached == 1){return -1;}

//_______________________________________________________| if (detach_ptcb) is already detached

        if(detach_ptcb-> exited == 1){return -1;}    

//_______________________________________________________| if thread of ptcb it is exited

    detach_ptcb -> detached = 1;

    

    if(detach_ptcb-> ref_count>0){
        
        kernel_broadcast(&detach_ptcb-> exit_cv);
    
    }
    

    return 0;
	  
   }

//_____________________________________________________________________________________________________________________________________|










//____________________________________________________________________________________________________________________________________
//                                                                                                              |    status: DONE    |
// @brief Terminate the current thread.                                                                         |  returns: nothing  |
//                                                                                                              |____________________|
    void sys_ThreadExit(int exitval)
    {

        PCB* curproc = CURPROC; // Get te process.

        PTCB* ptcb_for_exit = cur_thread()-> tcb_ptr_ptcb; // Get the ptcb.

        ptcb_for_exit-> exited= 1; // Thread is now exited through ptcb.

        ptcb_for_exit-> exitval= exitval; // Set the exitval of ptcb.

//___________________________________________________________________________________________________

        kernel_broadcast(&ptcb_for_exit-> exit_cv); // Notify the waiting threads about the situation.

        curproc-> thread_count-= 1; // Decrease thread count.

//___________________________________________________________________________________________________

     if(curproc->thread_count==0)    // check if it is the last thread
    {
      if(get_pid(curproc)!=1)
      
      {

      /* Reparent any children of the exiting process to the 
         initial task */

      PCB* initpcb = get_pcb(1);
      while(!is_rlist_empty(& curproc->children_list)) {
        rlnode* child = rlist_pop_front(& curproc->children_list);
        child->pcb->parent = initpcb;
        rlist_push_front(& initpcb->children_list, child);
      }

      /* Add exited children to the initial task's exited list 
         and signal the initial task */

      if(!is_rlist_empty(& curproc->exited_list)) {
        rlist_append(& initpcb->exited_list, &curproc->exited_list);
        kernel_broadcast(& initpcb->child_exit);
      }

      /* Put me into my parent's exited list */

      rlist_push_front(& curproc->parent->exited_list, &curproc->exited_node);
      kernel_broadcast(& curproc->parent->child_exit);

    }

    assert(is_rlist_empty(& curproc->children_list));
    assert(is_rlist_empty(& curproc->exited_list));


    /* 
      Do all the other cleanup we want here, close files etc. 
    */

    /* Release the args data */

    if(curproc->args) {
      free(curproc->args);
      curproc->args = NULL;
    }

    /* Clean up FIDT */

    for(int i=0;i<MAX_FILEID;i++) {
      if(curproc->FIDT[i] != NULL) {
        FCB_decref(curproc->FIDT[i]);
        curproc->FIDT[i] = NULL;
      }
    }

    /* Disconnect my main_thread */
    curproc->main_thread = NULL;

    /* Now, mark the process as exited. */
    curproc->pstate = ZOMBIE;

  }

    /* Bye-bye cruel world */
    kernel_sleep(EXITED, SCHED_USER);
}
//_______________________________________________________________________________________________________________________|







//_________________________________________________________
//                                                        |
// this routine frees a PTCB for good looks and not pasta |

   void minRefCount(PTCB* ptcb){

        ptcb-> ref_count--;

        if(ptcb-> ref_count==0){
            rlist_remove(&ptcb->ptcb_list_node);
            free(ptcb);
        }
   }
//_______________________________________________________
