#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "sharedm.h"

void print(char* str)
{
	write(1, str, strlen(str));
	write(1, "\n", 1);
}

void test_1()
{
	shm_open(1, 1, ONLY_CHILD_CAN_ATTACH);
	shm_open(2, 1, ONLY_CHILD_CAN_ATTACH);

	if(fork())
	{
		wait();

		char* t1 = shm_attach(1);
		print(t1);
		shm_close(1);

		char* t2 = shm_attach(2);
		print(t2);
		shm_close(2);
	}
	else
	{
		char* t1 = shm_attach(1);
		strcpy(t1, "first segment.");

		char* t2 = shm_attach(2);
		strcpy(t2, "second segment.");
	}
}

void test_2()
{
	shm_open(3, 1, ONLY_CHILD_CAN_ATTACH);

	if(fork())
	{
		char* t1 = shm_attach(3);
		strcpy(t1, "M is written.");
		wait();
		shm_close(3);
	}
	else
	{
		char* t1 = shm_attach(3);
		while(t1[0] != 'M');
		print(t1);
		shm_close(3);
	}
}

void test_3()
{
	shm_open(4, 1, ONLY_CHILD_CAN_ATTACH | ONLY_OWNER_WRITE);

	if(fork())
	{
		wait();
		char* t1 = shm_attach(4);
		print(t1);
		strcpy(t1, "message from parent.");
		print(t1);
	}
	else
	{
		char* t1 = shm_attach(4);
		strcpy(t1, "message from child.");
	}
}

void test_4()
{
	shm_open(5, 1, ONLY_CHILD_CAN_ATTACH | ONLY_OWNER_WRITE);

	if(fork())
	{
		char* t1 = shm_attach(5);
		strcpy(t1, "message from parent.");
		wait();
		shm_close(5);
	}
	else
	{
		char* t1 = shm_attach(5);
		while(t1[0] != 'm');
		print(t1);
		shm_close(5);
	}
}

void test_5()
{
	shm_open(6, 1, ONLY_CHILD_CAN_ATTACH);
	char* t1 = shm_attach(6);

	if(fork())
	{
		wait();
		print(t1);
	}
	else
	{
		strcpy(t1, "message from child.");
	}

	shm_close(6);
}

void test_6()
{
	shm_open(6, 1, ONLY_CHILD_CAN_ATTACH | ONLY_OWNER_WRITE);
	char* t1 = shm_attach(6);

	if(fork())
	{
		wait();
	}
	else
	{
		strcpy(t1, "message from child.");
	}

	shm_close(6);
}

void test_7()
{
	shm_open(7, 1, ONLY_CHILD_CAN_ATTACH);

	if(fork())
	{
		wait();
		shm_attach(7);
	}
	else
	{
		if(fork())
		{
			wait();
			shm_attach(7);
		}
		else
		{
			shm_attach(7);
		}
	}
}

void run_test(int test_number)
{
	switch(test_number) 
	{
		case 1:
			test_1();
			break;
		case 2:
			test_2();
			break;
		case 3:
			test_3();
			break;
		case 4:
			test_4();
			break;
		case 5:
			test_5();
			break;
		case 6:
			test_6();
			break;
		case 7:
			test_7();
			break;
		default:
			print("usage: sort <test_number>");
	}
}

int main(int argc, char *argv[])
{
	int test_number = argv[1][0] - '0';
	run_test(test_number);
	exit();
}

