
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"

//________________________________________________________
//                                                       |
// @brief Create a new thread in the current process.    |
//

    Tid_t sys_CreateThread(Task task, int argl, void* args)
    {


















	   return NOTHREAD;
    }
//________________________________________________________









//________________________________________________________
//                                                       |
//  @brief Return the Tid of the current thread.         |
//
    Tid_t sys_ThreadSelf()
     {


	     return (Tid_t) cur_thread();  // return (Tid_t) cur_thread()-> ptcbPointer;
     }

//_______________________________________________________





//________________________________________________________
//                                                       |
//  @brief Join the given thread.                        |
//
int sys_ThreadJoin(Tid_t tid, int* exitval)
{











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









	   return -1;
   }

//_______________________________________________________







//_______________________________________________________
//                                                      |
// @brief Terminate the current thread.                 |
//  
    void sys_ThreadExit(int exitval)
    {







    }
//_______________________________________________________
