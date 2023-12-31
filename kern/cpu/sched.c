#include "sched.h"

#include <inc/assert.h>
#include<inc/fixed_point.h>    // we added it
#include <kern/proc/user_environment.h>
#include <kern/trap/trap.h>
#include <kern/mem/kheap.h>
#include <kern/mem/memory_manager.h>
#include <kern/tests/utilities.h>
#include <kern/cmd/command_prompt.h>



uint32 isSchedMethodRR(){if(scheduler_method == SCH_RR) return 1; return 0;}
uint32 isSchedMethodMLFQ(){if(scheduler_method == SCH_MLFQ) return 1; return 0;}
uint32 isSchedMethodBSD(){if(scheduler_method == SCH_BSD) return 1; return 0;}

//===================================================================================//
//============================ SCHEDULER FUNCTIONS ==================================//
//===================================================================================//

//===================================
// [1] Default Scheduler Initializer:
//===================================
void sched_init()
{
	old_pf_counter = 0;

	sched_init_RR(INIT_QUANTUM_IN_MS);

	init_queue(&env_new_queue);
	init_queue(&env_exit_queue);
	scheduler_status = SCH_STOPPED;
}

//=========================
// [2] Main FOS Scheduler:
//=========================
void
fos_scheduler(void)
{
	chk1();
	scheduler_status = SCH_STARTED;

	//This variable should be set to the next environment to be run (if any)
	struct Env* next_env = NULL;

	if (scheduler_method == SCH_RR)
	{
		// Implement simple round-robin scheduling.
		// Pick next environment from the ready queue,
		// and switch to such environment if found.
		// It's OK to choose the previously running env if no other env
		// is runnable.0
		//If the curenv is still exist, then insert it again in the ready queue
		if (curenv != NULL)
		{
			//cprintf("\n1\n");
			enqueue(&(env_ready_queues[0]), curenv);
		}

		//Pick the next environment from the ready queue
		next_env = dequeue(&(env_ready_queues[0]));

		//Reset the quantum
		//2017: Reset the value of CNT0 for the next clock interval
		kclock_set_quantum(quantums[0]);
		//uint16 cnt0 = kclock_read_cnt0_latch() ;
		//cprintf("CLOCK INTERRUPT AFTER RESET: Counter0 Value = %d\n", cnt0 );


	}
	else if (scheduler_method == SCH_MLFQ)
	{
		next_env = fos_scheduler_MLFQ();
	}
	else if (scheduler_method == SCH_BSD)
	{
		next_env = fos_scheduler_BSD();
	}
	//temporarily set the curenv by the next env JUST for checking the scheduler
	//Then: reset it again
	struct Env* old_curenv = curenv;
	curenv = next_env ;
	chk2(next_env) ;
	curenv = old_curenv;


	if(next_env != NULL)
	{
		//		cprintf("\nScheduler select program '%s' [%d]... counter = %d\n", next_env->prog_name, next_env->env_id, kclock_read_cnt0());
		//		cprintf("Q0 = %d, Q1 = %d, Q2 = %d, Q3 = %d\n", queue_size(&(env_ready_queues[0])), queue_size(&(env_ready_queues[1])), queue_size(&(env_ready_queues[2])), queue_size(&(env_ready_queues[3])));
		env_run(next_env);
	}
	else
	{
		/*2015*///No more envs... curenv doesn't exist any more! return back to command prompt
		curenv = NULL;
		//lcr3(K_PHYSICAL_ADDRESS(ptr_page_directory));
		lcr3(phys_page_directory);

		//cprintf("SP = %x\n", read_esp());

		scheduler_status = SCH_STOPPED;
		//cprintf("[sched] no envs - nothing more to do!\n");
		while (1)
			run_command_prompt(NULL);

	}
}

//=============================
// [3] Initialize RR Scheduler:
//=============================
void sched_init_RR(uint8 quantum)
{

	// Create 1 ready queue for the RR
	num_of_ready_queues = 1;
#if USE_KHEAP
	sched_delete_ready_queues();
	env_ready_queues = kmalloc(sizeof(struct Env_Queue));
	quantums = kmalloc(num_of_ready_queues * sizeof(uint8)) ;
#endif
	quantums[0] = quantum;
	kclock_set_quantum(quantums[0]);
	init_queue(&(env_ready_queues[0]));

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_RR;
	//=========================================
	//=========================================
}

//===============================
// [4] Initialize MLFQ Scheduler:
//===============================
void sched_init_MLFQ(uint8 numOfLevels, uint8 *quantumOfEachLevel)
{
#if USE_KHEAP
	//=========================================
	//DON'T CHANGE THESE LINES=================
	sched_delete_ready_queues();
	//=========================================
	//=========================================

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_MLFQ;
	//=========================================
	//=========================================
#endif
}

//===============================
// [5] Initialize BSD Scheduler:
//===============================
void sched_init_BSD(uint8 numOfLevels, uint8 quantum)
{
	num_of_ready_queues=numOfLevels;
#if USE_KHEAP
	sched_delete_ready_queues();
	env_ready_queues = kmalloc(num_of_ready_queues*sizeof(struct Env_Queue));
	quantums = kmalloc(sizeof(uint8));
#endif
	quantums[0]=quantum;
	kclock_set_quantum (quantums[0]);
	load_avg = fix_int(0);
    for(int i=0 ;i<num_of_ready_queues ;i++)
    {
        init_queue(&(env_ready_queues[i]));
    }

    //=========================================
    //DON'T CHANGE THESE LINES=================
    scheduler_status = SCH_STOPPED;
    scheduler_method = SCH_BSD;
    //=========================================
    //=========================================
}
//=========================
// [6] MLFQ Scheduler:
//=========================
struct Env* fos_scheduler_MLFQ()
{
	panic("not implemented");
	return NULL;
}

//=========================
// [7] BSD Scheduler:
//=========================
struct Env* fos_scheduler_BSD()
{
	//TODO: [PROJECT'23.MS3 - #5] [2] BSD SCHEDULER - fos_scheduler_BSD

	/*cprintf("\nBefore\n");
	sched_print_all();*/

	if (curenv != NULL)
	{
		enqueue(&(env_ready_queues[curenv->priority_value]), curenv);
	}

	/*cprintf("\nAfter\n");
	sched_print_all();
	*/
	struct Env *required_env=NULL;
    for(int i=num_of_ready_queues-1 ;i>=0 ;i--)
    {
        int size = queue_size(&(env_ready_queues[i]));
        if(size>0)
        {
        	required_env = dequeue(&env_ready_queues[i]);
            break;
        }
    }
    kclock_set_quantum(quantums[0]);
    /*if(required_env != NULL)
    	cprintf("\nThe next env =%x\n",required_env->env_id);
    */

    // if there is no any other envs in system, then load average will = 0
    if(required_env == NULL )
    {
    	load_avg = fix_int(0);
    }

    return required_env;
}
//========================================
// [8] Clock Interrupt Handler
//	  (Automatically Called Every Quantum)
//========================================

int num_of_ready_processes()
{
	int total_num=0;

	for(int i=0 ;i<num_of_ready_queues ;i++)
	{
		int num_of_processes_in_queue = queue_size(&(env_ready_queues[i]));
		total_num+=num_of_processes_in_queue;
	}

	// the running does not exist in ready_queues, so we should check about it
	if(curenv!=NULL)
	{
		total_num+=1;
	}

	return total_num ;
}

void change_recent_cpu()
{
	// change recent_cpu for ready processes
	for(int i=0 ;i<num_of_ready_queues;i++)
	{
		 struct Env *process;
		 LIST_FOREACH( process, &env_ready_queues[i])
		 {
			// change recent_cpu
			fixed_point_t r1 = fix_scale(load_avg,2); // before divide
			fixed_point_t r2 = fix_add(r1 , fix_int(1));
			fixed_point_t x1 = fix_div(r1 ,r2); // before *
			fixed_point_t x2 = fix_mul(x1 ,process->recent_cpu);
			fixed_point_t result = fix_add(x2 , fix_int(process->nice));
			process->recent_cpu =result ;
		 }
	}

	if(curenv != NULL)
	{
		// change recent_cpu for running process
		fixed_point_t r1 = fix_scale(load_avg,2); // before divide
		fixed_point_t r2 =fix_add(r1 ,fix_int(1));// r1 + ;
		fixed_point_t x1 = fix_div(r1 ,r2); // before *
		fixed_point_t x2 = fix_mul(x1 ,curenv->recent_cpu);
		fixed_point_t result = fix_add(x2 , fix_int(curenv->nice));
		curenv->recent_cpu = result;
	}

}

void change_priority()
{
	// change priority for ready processes
	for(int i=0 ;i<num_of_ready_queues;i++)
	{
		 struct Env *process;
		 int size = queue_size(&(env_ready_queues[i]));

		 while(size > 0)
		 {
			 process = dequeue(&env_ready_queues[i]);
			 fixed_point_t r1 = fix_int(PRI_MAX);   // you need to check if the PRI_MAX is int
			 fixed_point_t x = fix_int(4);
			 fixed_point_t r2 =  fix_div(process->recent_cpu , x);
			 fixed_point_t x2  = fix_int(process->nice);
			 fixed_point_t r3 = fix_scale(x2 ,2);

			 fixed_point_t rs1 = fix_sub(r1 ,r2);
			 fixed_point_t result =fix_sub(rs1 ,r3);

			 int priority = fix_trunc(result);
			 if(priority>num_of_ready_queues-1)
				 priority=num_of_ready_queues-1;
			 else if(priority<PRI_MIN)
				 priority=PRI_MIN;

			process->priority_value = priority;
			enqueue(&env_ready_queues[process->priority_value], process);
			size--;
		 }
	}
	// change the priority for the running env
	if(curenv != NULL)
	{
		fixed_point_t r1 = fix_int(PRI_MAX);   // you need to check if the PRI_MAX is int
		fixed_point_t x = fix_int(4);
		fixed_point_t r2 =  fix_div(curenv->recent_cpu , x);
		fixed_point_t x2  = fix_int(curenv->nice);
		fixed_point_t r3 = fix_scale(x2 ,2);

		fixed_point_t rs1 = fix_sub(r1 ,r2);
		fixed_point_t result =fix_sub(rs1 ,r3);

		int priority = fix_trunc(result);
		if(priority>num_of_ready_queues-1)
		{
			priority=num_of_ready_queues-1;
		}

		else if(priority<PRI_MIN)
		{
			priority=PRI_MIN;
		}

		curenv->priority_value = priority;
	}

}
void change_load_avg()  //equation = (59/60)*load_avg + (1/60) *ready_processes
{
	fixed_point_t n1 =fix_int(59);
	fixed_point_t n2 =fix_int(60);
	fixed_point_t r1 =fix_div(n1,n2);  //(59/60)
	fixed_point_t r2=fix_mul(r1 , load_avg);  // before pluss

	fixed_point_t n3 =fix_int(1);
	fixed_point_t n4 =fix_int(60);
	fixed_point_t r3 =fix_div(n3,n4);  //(1/60)

	int ready_process = num_of_ready_processes();

	fixed_point_t r4 = fix_scale(r3 , ready_process); // after plus
	load_avg = fix_add(r2 , r4); // result of equation  (updated load_avg)
}
int milli_seconds = 0, ourticks = 0;

void clock_interrupt_handler()
{

	//TODO: [PROJECT'23.MS3 - #5] [2] BSD SCHEDULER - Your code is here
	if(isSchedMethodBSD())
	{
		ourticks++;
		milli_seconds += quantums[0];

		//[a] increment the recent_cpu for running process each tick by 1
		fixed_point_t f1 = fix_int(1);
		curenv->recent_cpu = fix_add(curenv->recent_cpu , f1);

		// [b] at each second
		if(milli_seconds >= 1000)
		{
			milli_seconds = milli_seconds - 1000;
			//[1-b] change load average
			change_load_avg();
			//[2-b] change recent_cpu for all ready_processes & running one
			change_recent_cpu();
		}
		//[c]change priority for all envs at each 4 ticks
		if(ourticks == 4)
		{
			ourticks = 0;
			change_priority();
		}
	}

	/********DON'T CHANGE THIS LINE***********/
	ticks++;
	if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_TIME_APPROX))
	{
		update_WS_time_stamps();
	}
	//cprintf("Clock Handler\n") ;
	fos_scheduler();
	/*****************************************/
}

//===================================================================
// [9] Update LRU Timestamp of WS Elements
//	  (Automatically Called Every Quantum in case of LRU Time Approx)
//===================================================================
void update_WS_time_stamps()
{
	struct Env *curr_env_ptr = curenv;

	if(curr_env_ptr != NULL)
	{
		struct WorkingSetElement* wse ;
		{
			int i ;
#if USE_KHEAP
			LIST_FOREACH(wse, &(curr_env_ptr->page_WS_list))
			{
#else
			for (i = 0 ; i < (curr_env_ptr->page_WS_max_size); i++)
			{
				wse = &(curr_env_ptr->ptr_pageWorkingSet[i]);
				if( wse->empty == 1)
					continue;
#endif
				//update the time if the page was referenced
				uint32 page_va = wse->virtual_address ;
				uint32 perm = pt_get_page_permissions(curr_env_ptr->env_page_directory, page_va) ;
				uint32 oldTimeStamp = wse->time_stamp;

				if (perm & PERM_USED)
				{
					wse->time_stamp = (oldTimeStamp>>2) | 0x80000000;
					pt_set_page_permissions(curr_env_ptr->env_page_directory, page_va, 0 , PERM_USED) ;
				}
				else
				{
					wse->time_stamp = (oldTimeStamp>>2);
				}
			}
		}

		{
			int t ;
			for (t = 0 ; t < __TWS_MAX_SIZE; t++)
			{
				if( curr_env_ptr->__ptr_tws[t].empty != 1)
				{
					//update the time if the page was referenced
					uint32 table_va = curr_env_ptr->__ptr_tws[t].virtual_address;
					uint32 oldTimeStamp = curr_env_ptr->__ptr_tws[t].time_stamp;

					if (pd_is_table_used(curr_env_ptr->env_page_directory, table_va))
					{
						curr_env_ptr->__ptr_tws[t].time_stamp = (oldTimeStamp>>2) | 0x80000000;
						pd_set_table_unused(curr_env_ptr->env_page_directory, table_va);
					}
					else
					{
						curr_env_ptr->__ptr_tws[t].time_stamp = (oldTimeStamp>>2);
					}
				}
			}
		}
	}
}

