#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "scheduler.h"

int idx = 0;
char arr[10000] = {0};
jmp_buf SCHEDULER;
FCB_ptr Current, Head = NULL;

int P,Q;
int task;
jmp_buf MAIN;
FCB_ptr Tail = NULL;
FCB_ptr block[5] = {NULL};
int small_times;
int mutex = 0;

int main(int argc,char *argv[])
{
    P = atoi(argv[1]);
    Q = atoi(argv[2]);
    task = atoi(argv[3]);
    small_times = atoi(argv[4]);
    //construct the circular linked list
    for(int i = 1;i <= 4;i++)
    {
        block[i] = (FCB_ptr)malloc(sizeof(FCB));
        block[i]->Name = i;
        block[i]->Next = block[i];
        block[i]->Previous = block[i];
        if(i == 1)
        {
            Head = block[i];
            Tail = block[i];
        }
        else
        {
            Tail->Next = block[i];
            block[i]->Next = Head;
            block[i]->Previous = Tail;
            Tail = block[i];
            Head->Previous = block[i];
        }
    }
    Current = block[4];
    //Initialize the stack frame
    if(setjmp(MAIN) == 0)
    {
        funct_5(1);
    }
    //context switch
    Scheduler();
    return 0;
}

void funct_1(int name)
{
    int i, j;
    // do the longjmp or setjmp
    if(setjmp(block[1]->Environment) == 0)
    {
        funct_5(2);
    }
    for(j = 1; j <= P; j++) // We call this for loop as ¡§Big loop¡¨ in the following description
    {
        // something else
        if(task == 2 && ((j - 1) % small_times) == 0 && j != 1)
        {
            mutex = 0;
            if(setjmp(block[1]->Environment) == 0)
            {
                longjmp(SCHEDULER,1);
            }
        }
        mutex = 1;
        for(i = 1; i <= Q; i++) // We call this for loop as ¡§Small loop¡¨ in the following description
        {
            sleep(1); // You should sleep for a second before append to arr
            arr[idx++] = '1';
        }
        //something else
        mutex = 0;
    }
    // do the longjmp or setjmp
    longjmp(SCHEDULER,-2);
}

void funct_2(int name)
{
    int i, j;
    // do the longjmp or setjmp
    if(setjmp(block[2]->Environment) == 0)
    {
        funct_5(3);
    }
    for(j = 1; j <= P; j++) // We call this for loop as ¡§Big loop¡¨ in the following description
    {
        // something else
        if(task == 2 && ((j - 1) % small_times) == 0 && j != 1)
        {
            mutex = 0;
            if(setjmp(block[2]->Environment) == 0)
            {
                longjmp(SCHEDULER,1);
            }
        }
        mutex = 2;
        for(i = 1; i <= Q; i++) // We call this for loop as ¡§Small loop¡¨ in the following description
        {
            sleep(1); // You should sleep for a second before append to arr
            arr[idx++] = '2';
        }
        //something else
        mutex = 0;
    }
    // do the longjmp or setjmp
    longjmp(SCHEDULER,-2);
}

void funct_3(int name)
{
    int i, j;
    // do the longjmp or setjmp
    if(setjmp(block[3]->Environment) == 0)
    {
        funct_5(4);
    }
    for(j = 1; j <= P; j++) // We call this for loop as ¡§Big loop¡¨ in the following description
    {
        // something else
        if(task == 2 && ((j - 1) % small_times) == 0 && j != 1)
        {
            mutex = 0;
            if(setjmp(block[3]->Environment) == 0)
            {
                longjmp(SCHEDULER,1);
            }
        }
        mutex = 3;
        for(i = 1; i <= Q; i++) // We call this for loop as ¡§Small loop¡¨ in the following description
        {
            sleep(1); // You should sleep for a second before append to arr
            arr[idx++] = '3';
        }
        //something else
        mutex = 0;
    }
    // do the longjmp or setjmp
    longjmp(SCHEDULER,-2);
}

void funct_4(int name)
{
    int i, j;
    // do the longjmp or setjmp
    if(setjmp(block[4]->Environment) == 0)
    {
        longjmp(MAIN,1);
    }
    for(j = 1; j <= P; j++) // We call this for loop as ¡§Big loop¡¨ in the following description
    {
        // something else
        if(task == 2 && ((j - 1) % small_times) == 0 && j != 1)
        {
            if(setjmp(block[4]->Environment) == 0)
            {
                longjmp(SCHEDULER,1);
            }
        }
        mutex = 4;
        for(i = 1; i <= Q; i++) // We call this for loop as ¡§Small loop¡¨ in the following description
        {
            sleep(1); // You should sleep for a second before append to arr
            arr[idx++] = '4';
        }
        //something else
        mutex = 0;
    }
    // do the longjmp or setjmp
    longjmp(SCHEDULER,-2);
}

void funct_5(int name)
{
    int a[10000]; // This line must not be changed.
    // call other functions
    if(name == 1)
    {
        funct_1(1);
    }
    else if(name == 2)
    {
        funct_2(2);
    }
    else if(name == 3)
    {
        funct_3(3);
    }
    else if(name == 4)
    {
        funct_4(4);
    }
}