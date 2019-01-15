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
	cprintf("1\n");
	struct proc *p = myproc();

	for (int i = 0; i < num_of_segments; ++i)
	{
		if(shm_table[i].id == id) {
			cprintf("error on open: segment %d is already exist.\n", id);
            release(&lk);
			return -1;
		}
	}

	shm_table[num_of_segments].id = id;
	shm_table[num_of_segments].owner = myproc()->pid;
	shm_table[num_of_segments].flags = flags;
	shm_table[num_of_segments].ref_count = 0;
	shm_table[num_of_segments].size = page_count;
	for (int i = 0; i < page_count; ++i)
	{
		
		// char* mem = 0;
		char* mem = kalloc();
		memset(mem, 0, PGSIZE);
  		shm_table[num_of_segments].frames[i] = V2P(mem);
	}

	num_of_segments++;

    cprintf("proc with pid: %d opened %d pages with flag %d\n", p->pid, page_count, flags);
    release(&lk);
	return 1;
}

void* shm_attach(int id)
{
    acquire(&lk);

	struct proc* p = myproc();
    int ref_cnt;
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
						cprintf("error on attach: permission denied.\n");
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

			shm_table[i].ref_count++;
            ref_cnt = shm_table[i].ref_count;

			int va = p->sz;
			for (int j = 0; j < shm_table[i].size; ++j)
			{
				mappages(p->pgdir, (void*)p->sz, PGSIZE, shm_table[i].frames[j], perm);
				p->sz += PGSIZE;
			}
            cprintf("proc with pid: %d attached to pages with id %d, current ref_cnt is:%d\n", p->pid, id, ref_cnt);	
            release(&lk);
			return (void*)va;
		}
	}

	cprintf("error on attach: segment %d not found\n", id);
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
			}
            cprintf("proc with pid: %d closed pages with id %d, current ref_cnt is:%d\n", p->pid, id, ref_cnt);
            release(&lk);
            return 1;
		}
	}

	cprintf("error on close: segment %d not found\n", id);
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




