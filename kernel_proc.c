
#include <assert.h>
#include "kernel_cc.h"
#include "kernel_proc.h"
#include "kernel_sched.h"
#include "kernel_streams.h"


/* 
 The process table and related system calls:
 - Exec
 - Exit
 - WaitPid
 - GetPid
 - GetPPid

 */






///* The process table */_________________________________________________________________________

PCB PT[MAX_PROC];
unsigned int process_count;

PCB* get_pcb(Pid_t pid)
{
  return PT[pid].pstate==FREE ? NULL : &PT[pid];
}
//________________________________________________________________________________________________






//________________________________________________________________________________________________

Pid_t get_pid(PCB* pcb)
{
  return pcb==NULL ? NOPROC : pcb-PT;
}
//________________________________________________________________________________________________





///* Initialize a PCB */___________________________________________________________________________

static inline void initialize_PCB(PCB* pcb)
{
  pcb->pstate = FREE;
  pcb->argl = 0;
  pcb->args = NULL;

  for(int i=0;i<MAX_FILEID;i++)
    pcb->FIDT[i] = NULL;

  rlnode_init(& pcb->children_list, NULL);
  rlnode_init(& pcb->exited_list, NULL);
  rlnode_init(& pcb->children_node, pcb);
  rlnode_init(& pcb->exited_node, pcb);
  pcb->child_exit = COND_INIT;
}
//________________________________________________________________________________________________





//________________________________________________________________________________________________

      static PCB* pcb_freelist;
//________________________________________________________________________________________________






//________________________________________________________________________________________________

void initialize_processes()
{
  /* initialize the PCBs */
  for(Pid_t p=0; p<MAX_PROC; p++) {
    initialize_PCB(&PT[p]);
  }

  /* use the parent field to build a free list */
  PCB* pcbiter;
  pcb_freelist = NULL;
  for(pcbiter = PT+MAX_PROC; pcbiter!=PT; ) {
    --pcbiter;
    pcbiter->parent = pcb_freelist;
    pcb_freelist = pcbiter;
  }

  process_count = 0;

  /* Execute a null "idle" process */
  if(Exec(NULL,0,NULL)!=0)
    FATAL("The scheduler process does not have pid==0");
}
//________________________________________________________________________________________________





//*Must be called with kernel_mutex held*/_______________________________________________________

PCB* acquire_PCB()
{
  PCB* pcb = NULL;

  if(pcb_freelist != NULL) {
    pcb = pcb_freelist;
    pcb->pstate = ALIVE;
    pcb_freelist = pcb_freelist->parent;
    process_count++;
  }

  return pcb;
}
//________________________________________________________________________________________________







//________________________________________________________________________________________________









//*Must be called with kernel_mutex held*/_______________________________________________________

void release_PCB(PCB* pcb)
{
  pcb->pstate = FREE;
  pcb->parent = pcb_freelist;
  pcb_freelist = pcb;
  process_count--;
}
//________________________________________________________________________________________________






//________________________________________________________________________________________________| status: under construction

void start_many_threads(){

  int   exitval;

  PTCB* ptcb = cur_thread()-> tcb_ptr_ptcb;

  Task  call = ptcb-> task;

  int argl = ptcb-> argl; 
  void* args = ptcb-> args;

  exitval = call(argl,args);
  ThreadExit(exitval);

}
//________________________________________________________________________________________________


      






//________________!!! Process creation !!!________________________________________________________
//                                                        |
//    This function is provided as an argument to spawn,  |
//    to execute the main thread of a process.            |
//________________________________________________________|

void start_main_thread() 
{
  int exitval;

  Task call =  CURPROC->main_task;
  int argl = CURPROC->argl;
  void* args = CURPROC->args;

  exitval = call(argl,args);
  Exit(exitval);
}
//________________________________________________________________________________________________








///* System call */_______________________________________________________________________________

Pid_t sys_GetPid()
{
  return get_pid(CURPROC);
}
//________________________________________________________________________________________________





//________________________________________________________________________________________________

Pid_t sys_GetPPid()
{
  return get_pid(CURPROC->parent);
}
//________________________________________________________________________________________________






//________________________________________________________________________________________________

static void cleanup_zombie(PCB* pcb, int* status)
{
  if(status != NULL)
    *status = pcb->exitval;

  rlist_remove(& pcb->children_node);
  rlist_remove(& pcb->exited_node);

  release_PCB(pcb);
}
//________________________________________________________________________________________________






//________________________________________________________________________________________________

static Pid_t wait_for_specific_child(Pid_t cpid, int* status)
{

  /* Legality checks */
  if((cpid<0) || (cpid>=MAX_PROC)) {
    cpid = NOPROC;
    goto finish;
  }

  PCB* parent = CURPROC;
  PCB* child = get_pcb(cpid);
  if( child == NULL || child->parent != parent)
  {
    cpid = NOPROC;
    goto finish;
  }

  /* Ok, child is a legal child of mine. Wait for it to exit. */
  while(child->pstate == ALIVE)
    kernel_wait(& parent->child_exit, SCHED_USER);
  
  cleanup_zombie(child, status);
  
finish:
  return cpid;
}
//________________________________________________________________________________________________






//________________________________________________________________________________________________

static Pid_t wait_for_any_child(int* status)
{
  Pid_t cpid;

  PCB* parent = CURPROC;

  /* !!! Make sure I have children !!! */

  int no_children, has_exited;
  while(1) {
    no_children = is_rlist_empty(& parent->children_list);
    if( no_children ) break;

    has_exited = ! is_rlist_empty(& parent->exited_list);
    if( has_exited ) break;

    kernel_wait(& parent->child_exit, SCHED_USER);    
  }

  if(no_children)
    return NOPROC;

  PCB* child = parent->exited_list.next->pcb;
  assert(child->pstate == ZOMBIE);
  cpid = get_pid(child);
  cleanup_zombie(child, status);

  return cpid;
}
//________________________________________________________________________________________________





//________________________________________________________________________________________________

   Pid_t sys_WaitChild(Pid_t cpid, int* status)
  {
     /* Wait for specific child. */
     if(cpid != NOPROC) {
       return wait_for_specific_child(cpid, status);
     }
     /* Wait for any child */
     else {
       return wait_for_any_child(status);
     }

  }
//________________________________________________________________________________________________







///*System call to create a new process */________________________________________________________

Pid_t sys_Exec(Task call, int argl, void* args)
{
  PCB *curproc, *newproc;

  PTCB *init_ptcb;
  
  /* The new process PCB */
  newproc = acquire_PCB();

  if(newproc == NULL) goto finish;  /* We have run out of PIDs! */

  if(get_pid(newproc)<=1) {
    /* Processes with pid<=1 (the scheduler and the init process) 
       are parentless and are treated specially. */
    newproc->parent = NULL;
  }
  else
  {
    /* Inherit parent */
    curproc = CURPROC;

    /* Add new process to the parent's child list */
    newproc->parent = curproc;
    rlist_push_front(& curproc->children_list, & newproc->children_node);

    /* Inherit file streams from parent */
    for(int i=0; i<MAX_FILEID; i++) {
       newproc->FIDT[i] = curproc->FIDT[i];
       if(newproc->FIDT[i])
          FCB_incref(newproc->FIDT[i]);
    }
  }


  /* Set the main thread's function */
  newproc->main_task = call;

  /* Copy the arguments to new storage, owned by the new process */
  newproc->argl = argl;
  if(args!=NULL) {
    newproc->args = malloc(argl);
    memcpy(newproc->args, args, argl);
  }
  else
    newproc->args=NULL;
//_________________________________________________________________________________________________________________________________________

      /* _________________________________________________________________________________________
         | Create and wake up the thread for the main function. This must be the last thing      |
         | we do, because once we wakeup the new thread it may run! so we need to have finished  |
         | the initialization of the PCB.                                                        |
         |_______________________________________________________________________________________|
                                        |                 |
                                        | status  :  done |                                     
                                        |_________________|                                      */
  

    rlnode_init(&newproc->ptcb_list,NULL);                                         // Initializing PCB thread queue                    


    if(call != NULL) {                                       
      
      init_ptcb = ptcb_alloc(newproc->main_task,newproc->argl,newproc->args);  // Allocating memory to the new PTCB and giving it arguements

      rlist_push_back(&newproc->ptcb_list, &init_ptcb->ptcb_list_node);        // Pushing to the back tof the queue the new PTCB

      init_ptcb-> tcb = spawn_thread(newproc,start_main_thread);               // Creating a thread and crown it as the main thread

      newproc-> main_thread = init_ptcb-> tcb;                                 // The thread pointer of PCB points to the thread in PTCB
      init_ptcb-> tcb-> tcb_ptr_ptcb = init_ptcb;                              // The PTCB pointer in TCB, gets the new PTCB, to point to

      newproc-> thread_count = 1;                                              // We increment the thread count by one

      wakeup(init_ptcb->tcb);                                                  // wake up main thread to be executed

                                                              /* 

                                                              | PAST CODE:

                                                              |  newproc->main_thread = spawn_thread(newproc, start_main_thread);
                                                              |  wakeup(newproc->main_thread); 

                                                              */
    }
//_________________________________________________________________________________________________________________________________________

finish:
  return get_pid(newproc);
}
//________________________________________________________________________________________________|








//____________________________________________________________________________________________________________________________________________

void sys_Exit(int exitval)
{

  PCB *curproc = CURPROC;  /* cache for efficiency */


  if(get_pid(curproc)==1) { /* Here, we must check that we are not the init task. If we are, we must wait until all child processes exit. */

    while(sys_WaitChild(NOPROC,NULL)!=NOPROC);

  } 

  curproc->exitval = exitval; /* First, store the exit status */

  sys_ThreadExit(exitval); // new routine call
}
//_____________________________________________________________________________________________________________________________________________





//_______________________________________________________________________TASK MANAGER____________________________________________________________________________________

int procinfo_read(void* info,  char *buffer, unsigned int size)
{

  procinfo_cb* Proc_info = (procinfo_cb*) info;
  
  if(Proc_info == NULL){ return 0; }                         // Check Proc_info validity

  if(Proc_info->pcb_cursor > MAX_PROC-1){ return 0; }        // Check if cursor is over the MAX_PCB limit


  PCB* Cur_pcb = &PT[Proc_info->pcb_cursor];                 // Look for Proc_info in Process Table with + the value of Proc_info cursor   


  while(Cur_pcb == NULL || Cur_pcb->pstate == FREE)          // Look for available PCBs in Process Table 
  {
    Proc_info->pcb_cursor++;                                 // in while keep adding +1 to cursor to keep searching
    Cur_pcb = &PT[Proc_info->pcb_cursor];                    
  }  

                                                             // IF YOU DO take every value and SEND IT to Proc_info struct

//-------------------passing all the values to Proc_info------------------------------

  Proc_info->current_info->pid = get_pid(Cur_pcb);
  Proc_info->current_info->ppid = get_pid(Cur_pcb->parent);

  Proc_info->current_info->alive = (Cur_pcb->pstate == ALIVE);

  Proc_info->current_info->thread_count = Cur_pcb->thread_count;

  Proc_info->current_info->main_task = Cur_pcb->main_task;
  Proc_info->current_info->argl = Cur_pcb->argl;

  memcpy(Proc_info->current_info->args, (char*)&Cur_pcb->args,PROCINFO_MAX_ARGS_SIZE);

//--------------------------------------------------------------------------------------


  int unexpected_total_size;                                          // just making sure

  if (size > sizeof(procinfo)){unexpected_total_size = sizeof(procinfo);} 

    else {unexpected_total_size = size;}


  memcpy(buffer,(char*)Proc_info->current_info,unexpected_total_size); // we give as arguements the buffer, we cast as char pointer the Proc_info->current_info who we want to give the info from 
                                                                       // unexpected_total_size is the sizeof(procinfo) we just check if it it unexpextedly larger that sizeof(procinfo).
  
  Proc_info->pcb_cursor++;                                             // next in line

  return unexpected_total_size;
}
//____________________________________________________________________________________________________________________________________________________________________________________________________







//________________________________

int procinfo_close(void* info) {

  if(info == NULL){return NOFILE;}

  free(info);

  return 0;
}
//_______________________________






//_________________________________
static file_ops procinfo_file_ops = {
  .Open = NULL,
  .Read = procinfo_read,
  .Write = noo_write,
  .Close = procinfo_close
};
//_________________________________






//__________________________________________________________________________________________________________________________________

Fid_t sys_OpenInfo()
{
  Fid_t Fd;
  FCB* fcb;

  if (FCB_reserve(1,&Fd,&fcb) == 0){return NOFILE;}

  procinfo_cb* Proc_info = (procinfo_cb*) xmalloc(sizeof(procinfo_cb)); // initialize  procinfo control block 
  Proc_info->current_info = (procinfo*) xmalloc(sizeof(procinfo));      // point where to take current info from
  Proc_info->pcb_cursor = 0;  
  fcb->streamobj = Proc_info;           // for the new fcb's streamobj is proc info
  fcb->streamfunc = &procinfo_file_ops; // file operations

  return Fd;
}
//__________________________________________________________________________________________________________________________________


/*
typedef struct procinfo
{
  Pid_t pid;      The pid of the process. 
  Pid_t ppid;     The parent pid of the process.

                   This is equal to NOPROC for parentless procs.
  
  int alive;        Non-zero if process is alive, zero if process is zombie.
  
  unsigned long thread_count;  Current no of threads. 
  
  Task main_task;  The main task of the process. 
  
  int argl;        Argument length of main task. 

            Note that this is the
            real argument length, not just the length of the @c args field, which is
            limited at PROCINFO_MAX_ARGS_SIZE. 

  char args[PROCINFO_MAX_ARGS_SIZE]; 
  
} procinfo;
*/



//__________________________________________________________________
int noo_write(){

  return 0;
}
//__________________________________________________________________