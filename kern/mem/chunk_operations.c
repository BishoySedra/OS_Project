/*
 * chunk_operations.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include <kern/trap/fault_handler.h>
#include <kern/disk/pagefile_manager.h>
#include "kheap.h"
#include "memory_manager.h"
#include <inc/queue.h>
#include <kern/tests/utilities.h>

//extern void inctst();

/******************************/
/*[1] RAM CHUNKS MANIPULATION */
/******************************/

//===============================
// 1) CUT-PASTE PAGES IN RAM:
//===============================
//This function should cut-paste the given number of pages from source_va to dest_va on the given page_directory
//	If the page table at any destination page in the range is not exist, it should create it
//	If ANY of the destination pages exists, deny the entire process and return -1. Otherwise, cut-paste the number of pages and return 0
//	ALL 12 permission bits of the destination should be TYPICAL to those of the source
//	The given addresses may be not aligned on 4 KB
int cut_paste_pages(uint32* page_directory, uint32 source_va, uint32 dest_va, uint32 num_of_pages)
{
	panic("cut_paste_pages() is not implemented yet...!!");

	return 0 ;
}

//===============================
// 2) COPY-PASTE RANGE IN RAM:
//===============================
//This function should copy-paste the given size from source_va to dest_va on the given page_directory
//	Ranges DO NOT overlapped.
//	If ANY of the destination pages exists with READ ONLY permission, deny the entire process and return -1.
//	If the page table at any destination page in the range is not exist, it should create it
//	If ANY of the destination pages doesn't exist, create it with the following permissions then copy.
//	Otherwise, just copy!
//		1. WRITABLE permission
//		2. USER/SUPERVISOR permission must be SAME as the one of the source
//	The given range(s) may be not aligned on 4 KB
int copy_paste_chunk(uint32* page_directory, uint32 source_va, uint32 dest_va, uint32 size)
{
	panic("copy_paste_chunk() is not implemented yet...!!");
	return 0;
}

//===============================
// 3) SHARE RANGE IN RAM:
//===============================
//This function should copy-paste the given size from source_va to dest_va on the given page_directory
//	Ranges DO NOT overlapped.
//	It should set the permissions of the second range by the given perms
//	If ANY of the destination pages exists, deny the entire process and return -1. Otherwise, share the required range and return 0
//	If the page table at any destination page in the range is not exist, it should create it
//	The given range(s) may be not aligned on 4 KB
int share_chunk(uint32* page_directory, uint32 source_va,uint32 dest_va, uint32 size, uint32 perms)
{
	panic("share_chunk() is not implemented yet...!!");
	return 0;
}

//===============================
// 4) ALLOCATE CHUNK IN RAM:
//===============================
//This function should allocate the given virtual range [<va>, <va> + <size>) in the given address space  <page_directory> with the given permissions <perms>.
//	If ANY of the destination pages exists, deny the entire process and return -1. Otherwise, allocate the required range and return 0
//	If the page table at any destination page in the range is not exist, it should create it
//	Allocation should be aligned on page boundary. However, the given range may be not aligned.
int allocate_chunk(uint32* page_directory, uint32 va, uint32 size, uint32 perms)
{
	panic("allocate_chunk() is not implemented yet...!!");
	return 0;
}

//=====================================
// 5) CALCULATE ALLOCATED SPACE IN RAM:
//=====================================
void calculate_allocated_space(uint32* page_directory, uint32 sva, uint32 eva, uint32 *num_tables, uint32 *num_pages)
{
	panic("calculate_allocated_space() is not implemented yet...!!");
}


//=====================================
// 6) CALCULATE REQUIRED FRAMES IN RAM:
//=====================================
//This function should calculate the required number of pages for allocating and mapping the given range [start va, start va + size) (either for the pages themselves or for the page tables required for mapping)
//	Pages and/or page tables that are already exist in the range SHOULD NOT be counted.
//	The given range(s) may be not aligned on 4 KB
uint32 calculate_required_frames(uint32* page_directory, uint32 sva, uint32 size)
{
	panic("calculate_required_frames() is not implemented yet...!!");
	return 0;
}

//=================================================================================//
//===========================END RAM CHUNKS MANIPULATION ==========================//
//=================================================================================//

/*******************************/
/*[2] USER CHUNKS MANIPULATION */
/*******************************/

//======================================================
/// functions used for USER HEAP (malloc, free, ...)
//======================================================

//=====================================
// 1) ALLOCATE USER MEMORY:
//=====================================
void allocate_user_mem(struct Env* e, uint32 virtual_address, uint32 size)
{
	//cprintf(" the sended address %d\n",virtual_address);
	/*=============================================================================*/
	//TODO: [PROJECT'23.MS2 - #10] [2] USER HEAP - allocate_user_mem() [Kernel Side]
	/*REMOVE THESE LINES BEFORE START CODING */
	//inctst();
	//return;
	/*======================================><=======================================*/

	// Write your code here, remove the panic and write your code
	//panic("allocate_user_mem() is not implemented yet...!!");


	/*logic of the function  :
	   1-set the number of pages to mark  and the start address
	   2-get the page table of this page
	   3-get the entry (page) from that page table
	   4-mark this page
	   5-update the the address to be the the base address of the next page*/

	// set the number of pages to marked and start address
	int num_of_pages_to_mark = (int)(ROUNDUP(size, PAGE_SIZE) / (uint32)PAGE_SIZE) ;
	uint32 base_address_of_each_page = virtual_address ;
    // mark and update address of the page
	for(int i=0 ;i<num_of_pages_to_mark ;i++ )
	{
		// get the page table
		uint32  *ptr_page_table =NULL ;   // may change t o **ptr_page_directory
		int result = get_page_table(e->env_page_directory , base_address_of_each_page , &ptr_page_table);
		if(result==TABLE_NOT_EXIST)
		{
			ptr_page_table =(uint32*) create_page_table(e->env_page_directory ,base_address_of_each_page);
		}
		// mark the page
		ptr_page_table[PTX(base_address_of_each_page)] =ptr_page_table[PTX(base_address_of_each_page)] | (PERM_MARKED);
		//update the address
		base_address_of_each_page+=PAGE_SIZE;
	}
}

//=====================================
// 2) FREE USER MEMORY:
//=====================================
int is_mapped(uint32 va ,uint32 * ptr_page_table)
{
	uint32 old_entry = ptr_page_table[PTX(va)] ;
	uint32 check_entry = ptr_page_table[PTX(va)] & (~PERM_PRESENT);
	if(old_entry == check_entry )
		return 0 ;
	return 1 ;
}
void free_user_mem(struct Env* e, uint32 virtual_address, uint32 size)
{
	/*==========================================================================*/
	//TODO: [PROJECT'23.MS2 - #12] [2] USER HEAP - free_user_mem() [Kernel Side]
	/*REMOVE THESE LINES BEFORE START CODING */
	//inctst();
	//return;
	/*==========================================================================*/

	//cprintf("Called free with address %x and size %d\n", virtual_address, size);
	// Write your code here, remove the panic and write your code
	//panic("free_user_mem() is not implemented yet...!!");
	//TODO: [PROJECT'23.MS2 - BONUS#2] [2] USER HEAP - free_user_mem() IN O(1): removing page from WS List instead of searching the entire list

	/* logic of the function
	 * 1- determine number of pages to work on  ---->done
	 * 2- unmark these pages in the page table  ----> done
	 * 3- remove the pages from working set (using invalidate)
	 * 4- remove these pages from page file (working set)
	 */

	// determine the number of pages to work on
	 int num_of_pages = (int)(ROUNDUP(size, PAGE_SIZE) / (uint32)PAGE_SIZE) ;
	 uint32 base_address_of_each_page = virtual_address ;
	 // unmark and update address of the page
	for(int i=0 ;i<num_of_pages ;i++ )
	{
		// get the page table
		uint32  *ptr_page_table =NULL ;
		int result = get_page_table(e->env_page_directory , base_address_of_each_page , &ptr_page_table);
		if(result==TABLE_IN_MEMORY)
		{
		// unmark the page
		ptr_page_table[PTX(base_address_of_each_page)] =ptr_page_table[PTX(base_address_of_each_page)] & (~PERM_MARKED);
		if(is_mapped(base_address_of_each_page ,ptr_page_table )==1)
			{
			// remove the page from the working set
			env_page_ws_invalidate( e,base_address_of_each_page);
			// remove the page from the page file
			pf_remove_env_page(e,base_address_of_each_page);
			//unmap
			unmap_frame(e->env_page_directory, base_address_of_each_page);
			}
		}
		//update the address
		base_address_of_each_page+=PAGE_SIZE;
	}
}

//=====================================
// 2) FREE USER MEMORY (BUFFERING):
//=====================================
void __free_user_mem_with_buffering(struct Env* e, uint32 virtual_address, uint32 size)
{
	// your code is here, remove the panic and write your code
	panic("__free_user_mem_with_buffering() is not implemented yet...!!");
}

//=====================================
// 3) MOVE USER MEMORY:
//=====================================
void move_user_mem(struct Env* e, uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
{
	//your code is here, remove the panic and write your code
	panic("move_user_mem() is not implemented yet...!!");

	// This function should move all pages from "src_virtual_address" to "dst_virtual_address"
	// with the given size
	// After finished, the src_virtual_address must no longer be accessed/exist in either page file
	// or main memory
}

//=================================================================================//
//========================== END USER CHUNKS MANIPULATION =========================//
//=================================================================================//

