#include <inc/memlayout.h>
#include <kern/kheap.h>
#include <kern/memory_manager.h>

//2022: NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)

uint32 allocation_array[1000][2];

//void* kmalloc(unsigned int size)
//{
//	//TODO: [PROJECT 2026 - [1] Kernel Heap] kmalloc()
//	// Write your code here, remove the panic and write your code
//	kpanic_into_prompt("kmalloc() is not implemented yet...!!");
//
//	//NOTE: Allocation using NEXTFIT strategy
//	//NOTE: All kernel heap allocations are multiples of PAGE_SIZE (4KB)
//	//refer to the project presentation and documentation for details
//
//
//	//change this "return" according to your answer
//
//	return NULL;
//
//
//}
uint32 ptr_Next_fit = KERNEL_HEAP_START;

void* kmalloc(unsigned int size)
{
	if(size ==0){
		return NULL;
	}

	uint32 rounded_size = ROUNDUP(size,PAGE_SIZE);
	uint32 RequiredPages = rounded_size/PAGE_SIZE;
	uint32 count =0;
	uint32 start_address = 0;

	uint32 current = ptr_Next_fit;
	//wrap if needed at start
	if (current >= KERNEL_HEAP_MAX){
		current = KERNEL_HEAP_START;
	}
	uint32 start_point = current; // to detect full cycle

	int wrapped = 0;
	//keep searching until I explicitly decide to stop
	while(1){
		uint32 *ptr_page_table= NULL;
		struct Frame_Info* ptr_frame_info = get_frame_info(ptr_page_directory,(void*)current, &ptr_page_table);
		// Free page
		if(ptr_frame_info == NULL){
			if(count == 0)
			{
				start_address = current;
			}
			count++;

			if(count == RequiredPages)
			{
				for(uint32 i =start_address;i<start_address+(RequiredPages*PAGE_SIZE);i+= PAGE_SIZE)
				{
					struct Frame_Info *new_frame = NULL;
					int ret = allocate_frame(&new_frame);
					if( ret == E_NO_MEM)
					{
						cprintf("No enough memory for page itself!\n");
						// RollBack in case allocating the frame is failed
						for(uint32 j = start_address; j< i; j+=PAGE_SIZE )
						{
							unmap_frame(ptr_page_directory, (void*)j);
						}
						return NULL;
					}
					ret = map_frame(ptr_page_directory, new_frame,(void*)i, PERM_PRESENT | PERM_WRITEABLE);
					if(ret == E_NO_MEM)
					{
						cprintf("No enough memory for page table!\n");
						// RollBack in case mapping the frame is failed
						for(uint32 j = start_address; j< i; j+=PAGE_SIZE )
						{
							unmap_frame(ptr_page_directory, (void*)j);
						}
						return NULL;
					}
				}
				ptr_Next_fit = start_address + RequiredPages * PAGE_SIZE;
				// wrap ptr_Next_fit if it hits the max
                if(ptr_Next_fit >= KERNEL_HEAP_MAX){
                	ptr_Next_fit = KERNEL_HEAP_START;
				}

///////////////////////////////////////////////////
                // store allocation info
                for (int k = 0; k < 1000; k++)
                {
                    if (allocation_array[k][1] == 0) // free slot (size == 0)
                    {
                        allocation_array[k][0] = start_address;
                        allocation_array[k][1] = rounded_size;
                        break;
                    }
                }
///////////////////////////////////////////////////

				return (void*)start_address;
			}
		}
		else
		{
			count =0;
		}

		current += PAGE_SIZE;

		// wrap_around
		if(current >= KERNEL_HEAP_MAX)
		{
			current = KERNEL_HEAP_START;
			wrapped = 1;
		}

		// Detect full cycle
		if(current == start_point && wrapped)
		{
			return NULL;
		}
	}
}


// is the start address passed? handled both
void kfree(void* virtual_address)
{
	//TODO: [PROJECT 2026 - [2] Kernel Heap] kfree()
	// Write your code here, remove the panic and write your code
	//panic("kfree() is not implemented yet...!!");
	//you need to get the size of the given allocation using its address ????
	//refer to the project presentation and documentation for details
	if (virtual_address==NULL|| (uint32)virtual_address< KERNEL_HEAP_START|| (uint32)virtual_address>= KERNEL_HEAP_MAX){
		return;}

//	uint32 va= ROUNDDOWN((uint32)virtual_address, PAGE_SIZE);
	uint32 va= (uint32)virtual_address;

	int index=-1;
	for(int i=0; i<1000; i++){
		//if (allocation_array[i][0]==va){ index=i; break;}
	  if ((va>= allocation_array[i][0]&&va<allocation_array[i][0]+allocation_array[i][1])||
		  (va< allocation_array[i][0]+allocation_array[i][1]&&va> allocation_array[i][0]))
		{
		  //cprintf("found at i= %d va = %x \n", i,va);
		  index=i; break;}
	}

	if (index==-1){
		cprintf("\n virtual address %x wasn't allocated \n",va);
		return;}

	//uint32 *ptr_page_table = NULL;
	uint32 size= allocation_array[index][1];
	uint32 start = allocation_array[index][0];
	int no_pages= size/ PAGE_SIZE;

	for (int i=0; i<no_pages; i++){
    	//start+=PAGE_SIZE;
		unmap_frame(ptr_page_directory,(void*)start);
		start+=PAGE_SIZE;
	}

	allocation_array[index][0] = 0;
	allocation_array[index][1] = 0;
}

//TODO: [PROJECT 2026 - [4] Kernel Heap] kheap_physical_address()
	//panic("kheap_physical_address() is not implemented yet...!!");
	//return the physical address corresponding to given virtual_address
unsigned int kheap_physical_address(unsigned int virtual_address)
{
	uint32 physical_address;
	/* dir indexing w 10 then get pt then pt indexing w 10
	then get entry then get f no and mult entry by p size
	and add last 12 bits offset*/
    if (virtual_address >= KERNEL_HEAP_START&& virtual_address < KERNEL_HEAP_MAX)
    {
	//int DIR_index= virtual_address>>22;
	//int pt_index= virtual_address<<10;
	//pt_index= pt_index>>22;
	uint32* ptr_page_table=NULL;
	get_page_table(ptr_page_directory, (void*)virtual_address, &ptr_page_table);
	if (ptr_page_table!=NULL){
	uint32 pt_entry=ptr_page_table[PTX(virtual_address)];
	if ((pt_entry& PERM_PRESENT)!=0){
		int frame_number= pt_entry>>12;
		int offset= virtual_address<<20;
		offset=offset>>20 ;
		physical_address= frame_number* PAGE_SIZE +offset;
		return physical_address;}
	}
    }
    return 0;
    // or get frame info + to physical address neater
}


unsigned int kheap_virtual_address(unsigned int physical_address)
{
	uint32 offset = physical_address % PAGE_SIZE;
	for (uint32 va = KERNEL_HEAP_START; va < KERNEL_HEAP_MAX; va += PAGE_SIZE)
	{
		uint32 *ptr_page_table = NULL;
		struct Frame_Info *ptr_frame_info = get_frame_info(ptr_page_directory, (void*)va, &ptr_page_table);
		if (ptr_frame_info == NULL){
			continue;
		}
		 if (to_physical_address(ptr_frame_info) == ROUNDDOWN(physical_address, PAGE_SIZE))
		 {
			 return va + offset;
		 }
	}
	return 0;
}


//unsigned int kheap_virtual_address(unsigned int physical_address)
//{
//	//TODO: [PROJECT 2026 - [3] Kernel Heap] kheap_virtual_address()
//	// Write your code here, remove the panic and write your code
//	//panic("kheap_virtual_address() is not implemented yet...!!");
//	//return the virtual address corresponding to given physical_address
//	//refer to the project presentation and documentation for details
//	//change this "return" according to your answer
//	// a compare 2 frame infos?
//	//if ( physical_address>=0&& physical_address< 0xFFFFFFFF){
//	uint32 offset= physical_address- ROUNDDOWN(physical_address, PAGE_SIZE);
//	//uint32 va;
//	//int f_no= physical_address/PAGE_SIZE;
//	//ret frame info bas take pa
//	//rounded down to base?
//	struct Frame_Info* target = to_frame_info(physical_address);
//	if (target==NULL){return 0;}
//	struct Frame_Info* ptr;
//	uint32 *ptr_page_table;
//	for (uint32 i= KERNEL_HEAP_START; i< KERNEL_HEAP_MAX; i+=PAGE_SIZE ){
//		//get both the page table and the frame of the given virtual address
//		// ret frame info too but take va
//		ptr = get_frame_info(ptr_page_directory,(void*)i,&ptr_page_table);
//		//compare f info to f info
//		if (ptr==target){
//			return i+offset;
//		}
//	}
//	 return 0;
//}
