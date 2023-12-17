#include "sched.h"

#include <inc/assert.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/trap.h>
#include <kern/mem/kheap.h>
#include <kern/mem/memory_manager.h>
#include <kern/tests/utilities.h>
#include <kern/cmd/command_prompt.h>

//void on_clock_update_WS_time_stamps();
extern void cleanup_buffers(struct Env* e);
//================

//=================================================================================//
//============================== QUEUE FUNCTIONS ==================================//
//=================================================================================//

//================================
// [1] Initialize the given queue:
//================================
void init_queue(struct Env_Queue* queue)
{
	if(queue != NULL)
	{
		LIST_INIT(queue);
	}
}

//================================
// [2] Get queue size:
//================================
int queue_size(struct Env_Queue* queue)
{
	if(queue != NULL)
	{
		return LIST_SIZE(queue);
	}
	else
	{
		return 0;
	}
}

//====================================
// [3] Enqueue env in the given queue:
//====================================
void enqueue(struct Env_Queue* queue, struct Env* env)
{
	assert(queue != NULL)	;
	//cprintf("IN ENQUE with queue %x and ENV ID is %d\n", queue, env->env_id);
	if(env != NULL)
	{
		LIST_INSERT_HEAD(queue, env);
	}
}

//======================================
// [4] Dequeue env from the given queue:
//======================================
struct Env* dequeue(struct Env_Queue* queue)
{
	if (queue == NULL) return NULL;
	struct Env* envItem = LIST_LAST(queue);
	if (envItem != NULL)
	{
		LIST_REMOVE(queue, envItem);
	}
	return envItem;
}

//====================================
// [5] Remove env from the given queue:
//====================================
void remove_from_queue(struct Env_Queue* queue, struct Env* e)
{
	assert(queue != NULL)	;
	if (e != NULL)
	{
		LIST_REMOVE(queue, e);
	}
}

//========================================
// [6] Search by envID in the given queue:
//========================================
struct Env* find_env_in_queue(struct Env_Queue* queue, uint32 envID)
{
	if (queue == NULL) return NULL;

	struct Env * ptr_env=NULL;
	LIST_FOREACH(ptr_env, queue)
	{
		if(ptr_env->env_id == envID)
		{
			return ptr_env;
		}
	}
	return NULL;
}

//=====================================================================================//
//============================== SCHED Q'S FUNCTIONS ==================================//
//=====================================================================================//

//========================================
// [1] Delete all ready queues:
//========================================
void sched_delete_ready_queues()
{
#if USE_KHEAP
	if (env_ready_queues != NULL)
		kfree(env_ready_queues);
	if (quantums != NULL)
		kfree(quantums);
#endif
}

//=================================================
// [2] Insert the given Env in the 1st Ready Queue:
//=================================================
void sched_insert_ready0(struct Env* env)
{
	if(env != NULL)
	{
		env->env_status = ENV_READY ;
		//cprintf("\n2\n");
		enqueue(&env_ready_queues[env->priority_value], env);
		//enqueue(&(env_ready_queues[0]), env);
	}
}

//=================================================
// [3] Remove the given Env from the Ready Queue(s):
//=================================================
void sched_remove_ready(struct Env* env)
{
	if(env != NULL)
	{
		for (int i = 0 ; i < num_of_ready_queues ; i++)
		{
			struct Env * ptr_env = find_env_in_queue(&(env_ready_queues[i]), env->env_id);
			if (ptr_env != NULL)
			{
				LIST_REMOVE(&(env_ready_queues[i]), env);
				env->env_status = ENV_UNKNOWN;
				return;
			}
		}
	}
}


//=================================================
// [4] Insert the given Env in NEW Queue:
//=================================================
void sched_insert_new(struct Env* env)
{
	if(env != NULL)
	{
		env->env_status = ENV_NEW ;
		//cprintf("\n3\n");
		enqueue(&env_new_queue, env);
	}
}

//=================================================
// [5] Remove the given Env from NEW Queue:
//=================================================
void sched_remove_new(struct Env* env)
{
	if(env != NULL)
	{
		LIST_REMOVE(&env_new_queue, env) ;
		env->env_status = ENV_UNKNOWN;
	}
}

//=================================================
// [6] Insert the given Env in EXIT Queue:
//=================================================
void sched_insert_exit(struct Env* env)
{
	if(env != NULL)
	{
		if(isBufferingEnabled()) {cleanup_buffers(env);}
		env->env_status = ENV_EXIT ;
		//cprintf("\n4\n");
		enqueue(&env_exit_queue, env);
	}
}
//=================================================
// [7] Remove the given Env from EXIT Queue:
//=================================================
void sched_remove_exit(struct Env* env)
{
	if(env != NULL)
	{
		LIST_REMOVE(&env_exit_queue, env) ;
		env->env_status = ENV_UNKNOWN;
	}
}

//=================================================
// [8] Sched the given Env in NEW Queue:
//=================================================
void sched_new_env(struct Env* e)
{
	//add the given env to the scheduler NEW queue
	if (e!=NULL)
	{
		sched_insert_new(e);
	}
}


//=================================================
// [9] Run the given EnvID:
//=================================================
void sched_run_env(uint32 envId)
{
	struct Env* ptr_env=NULL;
	LIST_FOREACH(ptr_env, &env_new_queue)
	{
		if(ptr_env->env_id == envId)
		{
			sched_remove_new(ptr_env);
			sched_insert_ready0(ptr_env);
			/*2015*///if scheduler not run yet, then invoke it!
			if (scheduler_status == SCH_STOPPED)
			{
				fos_scheduler();
			}
			break;
		}
	}
	//	cprintf("ready queue:\n");
	//	LIST_FOREACH(ptr_env, &env_ready_queue)
	//	{
	//		cprintf("%s - %d\n", ptr_env->prog_name, ptr_env->env_id);
	//	}
}

//=================================================
// [10] Exit the given EnvID:
//=================================================
void sched_exit_env(uint32 envId)
{
	struct Env* ptr_env=NULL;
	int found = 0;
	if (!found)
	{
		LIST_FOREACH(ptr_env, &env_new_queue)
		{
			if(ptr_env->env_id == envId)
			{
				sched_remove_new(ptr_env);
				found = 1;
				//			return;
			}
		}
	}
	if (!found)
	{
		for (int i = 0 ; i < num_of_ready_queues ; i++)
		{
			if (!LIST_EMPTY(&(env_ready_queues[i])))
			{
				ptr_env=NULL;
				LIST_FOREACH(ptr_env, &(env_ready_queues[i]))
				{
					if(ptr_env->env_id == envId)
					{
						LIST_REMOVE(&(env_ready_queues[i]), ptr_env);
						found = 1;
						break;
					}
				}
			}
			if (found)
				break;
		}
	}
	if (!found)
	{
		if (curenv->env_id == envId)
		{
			ptr_env = curenv;
			found = 1;
		}
	}

	if (found)
	{
		sched_insert_exit(ptr_env);

		//If it's the curenv, then reinvoke the scheduler as there's no meaning to return back to an exited env
		if (curenv->env_id == envId)
		{
			curenv = NULL;
			fos_scheduler();
		}
	}
}


/*2015*/
//=================================================
// [11] KILL the given EnvID:
//=================================================
void sched_kill_env(uint32 envId)
{
	struct Env* ptr_env=NULL;
	int found = 0;
	if (!found)
	{
		LIST_FOREACH(ptr_env, &env_new_queue)
															{
			if(ptr_env->env_id == envId)
			{
				cprintf("killing[%d] %s from the NEW queue...", ptr_env->env_id, ptr_env->prog_name);
				sched_remove_new(ptr_env);
				env_free(ptr_env);
				cprintf("DONE\n");
				found = 1;
				//			return;
			}
															}
	}
	if (!found)
	{
		for (int i = 0 ; i < num_of_ready_queues ; i++)
		{
			if (!LIST_EMPTY(&(env_ready_queues[i])))
			{
				ptr_env=NULL;
				LIST_FOREACH(ptr_env, &(env_ready_queues[i]))
				{
					if(ptr_env->env_id == envId)
					{
						cprintf("killing[%d] %s from the READY queue #%d...", ptr_env->env_id, ptr_env->prog_name, i);
						LIST_REMOVE(&(env_ready_queues[i]), ptr_env);
						env_free(ptr_env);
						cprintf("DONE\n");
						found = 1;
						break;
						//return;
					}
				}
			}
			if (found)
				break;
		}
	}
	if (!found)
	{
		ptr_env=NULL;
		LIST_FOREACH(ptr_env, &env_exit_queue)
		{
			if(ptr_env->env_id == envId)
			{
				cprintf("killing[%d] %s from the EXIT queue...", ptr_env->env_id, ptr_env->prog_name);
				sched_remove_exit(ptr_env);
				env_free(ptr_env);
				cprintf("DONE\n");
				found = 1;
				//return;
			}
		}
	}

	if (!found)
	{
		if (curenv->env_id == envId)
		{
			ptr_env = curenv;
			assert(ptr_env->env_status == ENV_RUNNABLE);
			cprintf("killing a RUNNABLE environment [%d] %s...", ptr_env->env_id, ptr_env->prog_name);
			env_free(ptr_env);
			cprintf("DONE\n");
			found = 1;
			//If it's the curenv, then reset it and reinvoke the scheduler
			//as there's no meaning to return back to a killed env
			//lcr3(K_PHYSICAL_ADDRESS(ptr_page_directory));
			lcr3(phys_page_directory);
			curenv = NULL;
			fos_scheduler();
		}
	}
}

//=================================================
// [12] PRINT ALL Envs from all queues:
//=================================================
void sched_print_all()
{
	struct Env* ptr_env ;
	if (!LIST_EMPTY(&env_new_queue))
	{
		cprintf("\nThe processes in NEW queue are:\n");
		LIST_FOREACH(ptr_env, &env_new_queue)
		{
			cprintf("	[%d] %s\n", ptr_env->env_id, ptr_env->prog_name);
		}
	}
	else
	{
		cprintf("\nNo processes in NEW queue\n");
	}
	cprintf("================================================\n");
	for (int i = 0 ; i < num_of_ready_queues ; i++)
	{
		if (!LIST_EMPTY(&(env_ready_queues[i])))
		{
			cprintf("The processes in READY queue #%d are:\n", i);
			LIST_FOREACH(ptr_env, &(env_ready_queues[i]))
			{
				cprintf("	[%d] %s\n", ptr_env->env_id, ptr_env->prog_name);
			}
			cprintf("================================================\n");

		}
		else
		{
			//cprintf("No processes in READY queue #%d\n", i);
		}
		//cprintf("================================================\n");
	}
	if (!LIST_EMPTY(&env_exit_queue))
	{
		cprintf("The processes in EXIT queue are:\n");
		LIST_FOREACH(ptr_env, &env_exit_queue)
		{
			cprintf("	[%d] %s\n", ptr_env->env_id, ptr_env->prog_name);
		}
	}
	else
	{
		cprintf("No processes in EXIT queue\n");
	}
}


/*int total()
	{
		int x= 0 ;
		// change recent_cpu for ready processes
		for(int i=0 ;i<num_of_ready_queues;i++)
		{
			 struct Env *process;
			 LIST_FOREACH( process, &env_ready_queues[i])
			 {
				 if(process!=NULL)
				 {
					 cprintf("env_id = %d and in queue num =  %d \n" ,process->env_id ,i);
					 x++;
				 }
			 }
		}
		return x ;
	}*/
//=================================================
// [13] MOVE ALL NEW Envs into READY Q:
//=================================================
void sched_run_all()
{
	struct Env* ptr_env=NULL;
	//Suggested Solution
	int new_queue_size = LIST_SIZE(&env_new_queue);
	while(new_queue_size > 0)
	{
		ptr_env = dequeue(&env_new_queue);
		sched_insert_ready0(ptr_env); //### ask here ###//
		new_queue_size--;

	}

		/*LIST_FOREACH(ptr_env, &env_new_queue)
		{
			sched_remove_new(ptr_env);
			sched_insert_ready0(ptr_env);
		}*/
	/*2015*///if scheduler not run yet, then invoke it!
	if (scheduler_status == SCH_STOPPED)
		fos_scheduler();
}

//=================================================
// [14] KILL ALL Envs in the System:
//=================================================
void sched_kill_all()
{
	struct Env* ptr_env ;
	if (!LIST_EMPTY(&env_new_queue))
	{
		cprintf("\nKILLING the processes in the NEW queue...\n");
		LIST_FOREACH(ptr_env, &env_new_queue)
		{
			cprintf("	killing[%d] %s...", ptr_env->env_id, ptr_env->prog_name);
			sched_remove_new(ptr_env);
			env_free(ptr_env);
			cprintf("DONE\n");
		}
	}
	else
	{
		cprintf("No processes in NEW queue\n");
	}
	cprintf("================================================\n");
	for (int i = 0 ; i < num_of_ready_queues ; i++)
	{
		if (!LIST_EMPTY(&(env_ready_queues[i])))
		{
			cprintf("KILLING the processes in the READY queue #%d...\n", i);
			LIST_FOREACH(ptr_env, &(env_ready_queues[i]))
			{
				cprintf("	killing[%d] %s...", ptr_env->env_id, ptr_env->prog_name);
				LIST_REMOVE(&(env_ready_queues[i]), ptr_env);
				env_free(ptr_env);
				cprintf("DONE\n");
			}
		}
		else
		{
			cprintf("No processes in READY queue #%d\n",i);
		}
		cprintf("================================================\n");
	}

	if (!LIST_EMPTY(&env_exit_queue))
	{
		cprintf("KILLING the processes in the EXIT queue...\n");
		LIST_FOREACH(ptr_env, &env_exit_queue)
		{
			cprintf("	killing[%d] %s...", ptr_env->env_id, ptr_env->prog_name);
			sched_remove_exit(ptr_env);
			env_free(ptr_env);
			cprintf("DONE\n");
		}
	}
	else
	{
		cprintf("No processes in EXIT queue\n");
	}

	//reinvoke the scheduler since there're no env to return back to it
	curenv = NULL;
	fos_scheduler();
}

/*2018*/
//=================================================
// [14] EXIT ALL Ready Envs:
//=================================================
void sched_exit_all_ready_envs()
{
	struct Env* ptr_env=NULL;
	for (int i = 0 ; i < num_of_ready_queues ; i++)
	{
		if (!LIST_EMPTY(&(env_ready_queues[i])))
		{
			ptr_env=NULL;
			LIST_FOREACH(ptr_env, &(env_ready_queues[i]))
			{
				LIST_REMOVE(&(env_ready_queues[i]), ptr_env);
				sched_insert_exit(ptr_env);
			}
		}
	}
}

/*2023*/
/********* for BSD Priority Scheduler *************/
int64 timer_ticks()
{
	return ticks;
}
int env_get_nice(struct Env* e)
{
    //TODO: [PROJECT'23.MS3 - #3] [2] BSD SCHEDULER - env_get_nice
    //Your code is here
    return e->nice;
    //Comment the following line
    //panic("Not implemented yet");
    return 0;
}
void env_set_nice(struct Env* e, int nice_value)
{
    //TODO: [PROJECT'23.MS3 - #3] [2] BSD SCHEDULER - env_set_nice
    //Your code is here
    if (nice_value < -20)
    {
        nice_value = -20;
    }
    else if (nice_value > 20)
    {
        nice_value = 20;
    }

    e->nice = nice_value;



    //update recent_cpu
    fixed_point_t h1 = fix_scale(load_avg,2); // before divide
	fixed_point_t h2 = fix_add(h1 , fix_int(1));
	fixed_point_t y1 = fix_div(h1 ,h2); // before *
	fixed_point_t y2 = fix_mul(y1 ,e->recent_cpu);
	fixed_point_t result1 = fix_add(y2 , fix_int(e->nice));
	e->recent_cpu = result1;
    // update the priority of the env
     fixed_point_t r1 = fix_int(PRI_MAX);   // you need to check if the PRI_MAX is int
	 fixed_point_t x = fix_int(4);
	 fixed_point_t r2 =  fix_div(e->recent_cpu , x);
	 fixed_point_t x2  = fix_int(e->nice);
	 fixed_point_t r3 = fix_scale(x2 ,2);

	 fixed_point_t rs1 = fix_sub(r1 ,r2);
	 fixed_point_t result =fix_sub(rs1 ,r3);
	 //struct Env_Queue queue =env_ready_queues[curenv->priority_value]; //need to discuss
	 int priority = fix_trunc(result);
	 if(priority>num_of_ready_queues-1)
		 priority=num_of_ready_queues-1;
	 else if(priority<PRI_MIN)
		 priority=PRI_MIN;

	//cprintf("IN ### SET NICE #### Current Env ID is %d With old Priority %d New priority%d \n",e->env_id,e->priority_value,priority);
	//sched_print_all();
	//struct Env_Queue queue = //need to discuss
	//remove_from_queue(&env_ready_queues[e->priority_value] ,e);
	e->priority_value = priority;
	//enqueue(&env_ready_queues[priority], e);
	cprintf("envId = %d with nice_value = %d and priority = %d\n",e->env_id ,e->nice ,e->priority_value );

	//sched_print_all();


    //Comment the following line
//    panic("Not implemented yet");
}
int env_get_recent_cpu(struct Env* e)
{
    //TODO: [PROJECT'23.MS3 - #3] [2] BSD SCHEDULER - env_get_recent_cpu

	 return fix_round(fix_scale(e->recent_cpu,100));
//    panic("Not implemented yet");
     return 0;
}
int get_load_average()
{
	//TODO: [PROJECT'23.MS3 - #3] [2] BSD SCHEDULER - get_load_average
	//Your code is here
	//Comment the following line
    return fix_round(fix_scale(load_avg,100));
	//panic("Not implemented yet");
	return 0;
}
/********* for BSD Priority Scheduler *************/
//==================================================================================//
