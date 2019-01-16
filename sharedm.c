#include "types.h"
#include "defs.h"
#include "mmu.h"
#include "param.h"
#include "proc.h"
#include "spinlock.h"
#include "memlayout.h"
#include "sharedm.h"


#define MAX_SHARED_PAGES 10
#define MAX_SEGMENT 20

struct _segment {
	uint id;
	uint owner;
	uint flags;
	uint ref_count;
	uint size;
	uint frames[MAX_SHARED_PAGES];
};

struct spinlock lk;

int init_flag = 0;

char* mem_c = 0;

struct _segment shm_table[MAX_SEGMENT] = {0};
uint num_of_segments = 0;

int shm_open(int id, int page_count, int flags) 
{
    if(!init_flag)
    {
        initlock(&lk, "dummy");    
        init_flag = 1;
    }
    acquire(&lk);
	
	struct proc *p = myproc();
	char* mem = 0;

	// if(!mem_c)
	// {
	// 	mem_c = kalloc();
	// 	if(!mem_c) {
	// 		cprintf("error on kalocc!\n");
	// 	}
	// 	memset(mem_c, 0, PGSIZE);
	// 	// cprintf("kernel new: %s\n", mem_c);
	// }
	
	// mappages(p->pgdir, (void*)p->sz, PGSIZE, V2P(mem_c), PTE_W|PTE_U);
	// p->sz += PGSIZE;
	// cprintf("kernel end: %s\n", mem_c);
	// release(&lk);
	// return p->sz - PGSIZE;

	for (int i = 0; i < num_of_segments; ++i)
	{
		if(shm_table[i].id == id) {
			cprintf("Log: error on open: segment %d is already exist.\n", id);
            release(&lk);
			return -1;
		}
	}

	shm_table[num_of_segments].id = id;
	shm_table[num_of_segments].owner = p->pid;
	shm_table[num_of_segments].flags = flags;
	shm_table[num_of_segments].ref_count = 0;
	shm_table[num_of_segments].size = page_count;
	
	for (int i = 0; i < page_count; ++i)
	{
		mem = kalloc();
		if(!mem)
		{
			cprintf("Log: error on open: out of memory.\n");
            release(&lk);
			return -1;
		}
		memset(mem, 0, PGSIZE);
  		shm_table[num_of_segments].frames[i] = V2P(mem);
	}

	num_of_segments++;

    cprintf("Log: proc with pid: %d opened %d pages with flag %d\n", p->pid, page_count, flags);
    release(&lk);
	return 1;
}

void* shm_attach(int id)
{
    acquire(&lk);
	struct proc* p = myproc();
	for (int i = 0; i < num_of_segments; ++i)
	{
		if(shm_table[i].id == id)
		{
			int perm = PTE_U;
			if(shm_table[i].flags & ONLY_CHILD_CAN_ATTACH)
			{
				// should check permission
				if(p->pid == shm_table[i].owner)
				{
					// grant
					perm |= PTE_W;
				}
				else
				{
					if(!p->parent || p->parent->pid != shm_table[i].owner)
					{
						// denied
						cprintf("Log: proc with pid: %d error on attach: permission denied.\n", p->pid);
                        release(&lk);						
                        return 0;
					}
					else
					{
						// grant
						if(! (shm_table[i].flags & ONLY_OWNER_WRITE))
							perm |= PTE_W;
					}
				}
			}
			else
			{
				// has not this flag
				perm |= PTE_W;
			}

			shm_table[i].ref_count++;
			int va = p->sz;
			for (int j = 0; j < shm_table[i].size; ++j)
			{
				if(mappages(p->pgdir, (void*)(p->sz), PGSIZE, shm_table[i].frames[j], perm) != 0)
				{
					cprintf("Log: failed to map.\n");	
				}
				p->sz += PGSIZE;
			}
            cprintf("Log: proc with pid: %d attached to pages with id %d, current ref_cnt is:%d\n", p->pid, id, shm_table[i].ref_count);	
            release(&lk);
			return (void*)va;
		}
	}

	cprintf("Log: proc with pid: %d error on attach: segment %d not found\n", p->pid, id);
    release(&lk);
	return 0;
}

int shm_close(int id)
{
    acquire(&lk);

    struct proc* p = myproc();
    int ref_cnt;
	for (int i = 0; i < num_of_segments; ++i)
	{
		if(shm_table[i].id == id)
		{
			shm_table[i].ref_count--;
            ref_cnt = shm_table[i].ref_count;
			if(shm_table[i].ref_count == 0) 
			{
				shm_table[i].id = -1;
				for (int j = 0; j < shm_table[i].size; ++j)
				{
					// kfree((char*)shm_table[i].frames[j]);
				}
			}
            cprintf("Log: proc with pid: %d closed pages with id %d, current ref_cnt is:%d\n", p->pid, id, ref_cnt);
            release(&lk);
            return 1;
		}
	}

	cprintf("Log: error on close: segment %d not found\n", id);
    release(&lk);
	return -1;
}

int is_shared_memory(uint pa)
{
	for (int i = 0; i < num_of_segments; ++i)
	{
		for (int j = 0; j < shm_table[i].size; ++j)
		{
			if(shm_table[i].frames[j] == pa)
				return 1;
		}
	}

	return 0;
}


int is_shared_memory_with_inc(uint pa)
{
	for (int i = 0; i < num_of_segments; ++i)
	{
		for (int j = 0; j < shm_table[i].size; ++j)
		{
			if(shm_table[i].frames[j] == pa) {
				shm_table[i].ref_count++;
				return 1;
			}
		}
	}

	return 0;
}

int is_only_child_can_attach(uint pa)
{
	for (int i = 0; i < num_of_segments; ++i)
		for (int j = 0; j < shm_table[i].size; ++j)
			if(shm_table[i].frames[j] == pa)
				return !!(shm_table[i].flags & ONLY_CHILD_CAN_ATTACH);

	return 0;
}

int is_only_owner_can_write(uint pa)
{
	for (int i = 0; i < num_of_segments; ++i)
		for (int j = 0; j < shm_table[i].size; ++j)
			if(shm_table[i].frames[j] == pa)
				return !!(shm_table[i].flags & ONLY_OWNER_WRITE);

	return 0;
}

int get_owner_of_segment(uint pa)
{
	for (int i = 0; i < num_of_segments; ++i)
		for (int j = 0; j < shm_table[i].size; ++j)
			if(shm_table[i].frames[j] == pa)
				return shm_table[i].owner;

	return 0;
}


