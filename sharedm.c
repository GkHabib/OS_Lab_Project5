#include "types.h"
#include "defs.h"
#include "mmu.h"
#include "param.h"
#include "proc.h"

#include "memlayout.h"


#define MAX_SHARED_PAGES 255
#define MAX_SEGMENT 20


#define ONLY_OWNER_WRITE 2
#define ONLY_CHILD_CAN_ATTACH 4



struct _segment {
	uint id;
	uint owner;
	uint flags;
	uint ref_count;
	uint size;
	uint frames[MAX_SHARED_PAGES];
};


struct _segment shm_table[MAX_SEGMENT] = {0};
uint num_of_segments = 0;

int shm_open(int id, int page_count, int flags) 
{
	struct proc *p = myproc();

	for (int i = 0; i < num_of_segments; ++i)
	{
		if(shm_table[i].id == id) {
			cprintf("error on open: segment %d is already exist.\n", id);
			return -1;
		}
	}

	shm_table[num_of_segments].id = id;
	shm_table[num_of_segments].owner = myproc()->pid;
	shm_table[num_of_segments].flags = flags;
	shm_table[num_of_segments].ref_count = 1;
	shm_table[num_of_segments].size = page_count;
	for (int i = 0; i < page_count; ++i)
	{
		char* mem = kalloc();
		memset(mem, 0, PGSIZE);
  		mappages(p->pgdir, (void*)p->sz, PGSIZE, V2P(mem), PTE_W|PTE_U);
  		p->sz += PGSIZE;
  		shm_table[num_of_segments].frames[i] = V2P(mem);
	}

	num_of_segments++;

    cprintf("proc with pid: %d opened %d pages with flag %d\n", p->pid, page_count, flags);
	return 1;
}

void* shm_attach(int id)
{
	struct proc* p = myproc();
    int ref_cnt;
	for (int i = 0; i < num_of_segments; ++i)
	{
		if(shm_table[i].id == id)
		{
			if(shm_table[i].flags & ONLY_CHILD_CAN_ATTACH)
			{
				// should check permission
				if(!p->parent || p->parent->pid != shm_table[i].owner)
				{
					cprintf("error on attach: permission denied.\n");
					return 0;
				}
			}

			int perm;
			if(shm_table[i].flags & ONLY_OWNER_WRITE)
				perm = PTE_U;
			else
				perm = PTE_U | PTE_W;

			shm_table[i].ref_count++;
            ref_cnt = shm_table[i].ref_count;

			int va = p->sz;
			for (int j = 0; j < shm_table[i].size; ++j)
			{
				mappages(p->pgdir, (void*)p->sz, PGSIZE, shm_table[i].frames[j], perm);
				p->sz += PGSIZE;
			}
            cprintf("proc with pid: %d attached to pages with id %d, current ref_cnt is:%d\n", p->pid, id, ref_cnt);	
			return (void*)va;
		}
	}

	cprintf("error on attach: segment %d not found\n", id);
	return 0;
}

int shm_close(int id)
{
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
			}
            cprintf("proc with pid: %d closed pages with id %d, current ref_cnt is:%d\n", p->pid, id, ref_cnt);
            return 1;
		}
	}

	cprintf("error on close: segment %d not found\n", id);
	return -1;
}



