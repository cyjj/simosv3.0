//
//  main.c
//  memory v1.0
//
//  Created by cy on 14-4-9.
//  Copyright (c) 2014年 cy. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include "simos.h"
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

#define opcodeShift 24
#define operandMask 0x00ffffff
#define agebitShift 1
#define agebitsMask 010




mType* Memory;

//=========================
//thread
//=========================
sem_t mutex;

pthread_t tid[1];

void * pagefault_handler(void *arg)
{
    sem_wait(&mutex);
    
    int i,maddr,pid ;
    pid = (intptr_t)arg;
    printf("Process %d in thread\n",pid);
    i=PCB[pid]->swapnum;
    
    maddr = PCB[pid]->pagetable[i]*pageSize;
    swap_in(pid, i , Memory+maddr);
    
    
    PCB[pid]->agebits[i] = PCB[pid]->agebits[i]|agebitsMask;
    PCB[pid]->dirtybit[i]=1;
    
    pagefault_complete(pid);
    sem_post(&mutex);
    return NULL;
}

//================
//memory
//================
int* free_list;

int framenum,os_framenum;

void init_mutex(sem_t mutex)
{
    sem_init(&mutex, 0,1);
    
}


void  size_memory()
{
    Memory = (mType *)malloc(memSize*sizeof(mType));
    
}


void initialize_memory()
{
    int i;
    add_timer(periodAgeScan,0,actAgeInterrupt,periodAgeScan);
    size_memory();
    framenum = ((memSize-1)/pageSize )+1;

    os_framenum=((OSmemSize-1)/pageSize)+1;
    
    free_list = (int *)malloc(framenum*4);
    for ( i=0; i<framenum; i++)
    {
        free_list[i]=1;
    }
    
    sem_init(&mutex,0,1);
}


int allocate_memory(pid,msize,numinstr)
{
    
    int i ,j = 0;
    

    for (i=os_framenum; i<framenum; i++)
    {
        if (free_list[i])
        {
            PCB[pid]->pagetable[0]=i;
            j=i;
            free_list[i]=0;
            pagetablesize[pid]=1;
            ocpframe++;
            break;
        }
        
    }
    if(PCB[pid]->pagetable[0]==0)
    {
        printf("There is no enough free frames for process %d",pid);
        PCB[pid]->swapnum=0;
        return (mPFault);
    }
    
  
    
  
    return (mNormal);
}



int compute_address(pid,offset)
int offset,pid ;
{
    int i,maddr ;
    i = (offset-1)/pageSize;
    if(PCB[pid]->pagetable[i]==0)
    {
        PCB[pid]->swapnum=i;
        int j,k=0,x;
        int minpid=2,minpage=0;
        
        printf ("Page fault handler is being activated for process %d.\n",pid);
        printf("the missing page is: %d\n",i );
        PCB[pid]->agebits[i]=0;
        for (j =os_framenum; j<framenum; j++)
        {
            if (free_list[j])
            {
                PCB[pid]->pagetable[i]=j;
                k=j;
                pagetablesize[pid]++;
                free_list[j]=0;
                
                
                printf("the frame allocated to the faulted page is %d",k);
                break;
            }
            
        }
        if (k==0)
        {
            printf("There is no enough frame now, search and swap out the least recent frame in memory");
            for (x=2; x<=currentPid; x++)
            {
                if (PCB[x]==NULL)
                {
                    minpid++;
                    continue;
                }
                else
                {
                    
                    for (j=0; j<pagetablesize[x]; j++)
                    {
                        if (PCB[x]->pagetable[j]==0)
                        {
                            continue;
                        }
                        if (PCB[x]->agebits[j]<PCB[minpid]->agebits[minpage])
                        {
                            minpid=x;
                            minpage=j;
                        }
                    }
                }
            }
            printf("The selected frame is %d",PCB[minpid]->pagetable[minpage]);
            if (PCB[minpid]->dirtybit[minpage])
            {
                swap_out(pid, minpage, Memory+(PCB[minpid]->pagetable[minpage])*pageSize);
            }
            PCB[pid]->pagetable[i]=PCB[minpid]->pagetable[minpage];
            PCB[minpid]->pagetable[minpage]=0;
            
            
        }

        return (mPFault);
    }
    else
    {
        PCB[pid]->agebits[i]= PCB[pid]->agebits[i]|agebitsMask;
        if(offset%pageSize !=0)
            maddr = (PCB[pid]->pagetable[i])*pageSize+(offset)%pageSize-1;
        else
            maddr = (PCB[pid]->pagetable[i])*pageSize+(pageSize-1);
        if(Observe)
        {
            printf("Process %d call compute address function,in frame %d offset: %d and the physical address:%d\n",pid,PCB[pid]->pagetable[i],offset,maddr);
        }
 
    }
        return(maddr);
}



//===============================================
//run time memory access operations
//===============================================

int check_data_address (offset )
int offset;
{

    if (offset>CPU.Mbound)
    {
        printf("Process %d access offset from base %d. Outside address space\n",CPU.Pid,(offset));
        return (mError);
    }
    else if (offset<= CPU.MDbase)
    {
        printf("Process %d data call access offset from base %d. In instruction region!\n",CPU.Pid,(offset));
        return (mError);
    }
    
    else
        return (mNormal);
}

int check_instruction_address(offset )
int offset;
{
    if(offset>CPU.Mbound)
    {
        printf("Process %d access offset from base %d. Outside address space\n",CPU.Pid,offset);
        return (mError);
    }
    else if (offset>CPU.MDbase)
    {
        printf("Process %d instruction call access offset from base %d. In data region!\n",CPU.Pid,offset);
        return (mError);
    }
    else
        return (mNormal);
}




int get_data(offset)
int offset ;
{
    int maddr,pid ;
    pid = CPU.Pid;
    offset+=CPU.MDbase;
    offset++;
    if(check_data_address(offset)==mError)
    {
        return (mError);
    }
    else
    {
        if ((maddr=compute_address(pid, offset))==mPFault)
        {
            return (mPFault);
        }
        
        CPU.MBR = Memory[maddr].mData;
        return (mNormal);
    }
    
}


int put_data(offset )
int offset;
{
    int maddr,pid,i;
    pid = CPU.Pid;
    offset+=CPU.MDbase;
    offset++;
    if(check_data_address(offset)== mError)
        return (mError);
    else
    {
        if ((maddr = compute_address(pid, offset))==mPFault)
        {
            return (mPFault);
        }
        
        Memory[maddr].mData= CPU.AC;
        i = (offset-1)/pageSize;
        PCB[pid]->dirtybit[i]=1;
        return (mNormal);
    }
}

int get_instruction (offset )
int offset;
{
    int maddr,pid,instr;
    pid = CPU.Pid;
    offset++;
    if (check_instruction_address(offset)==mError)
    {
        return (mError);
    }
    else
    {
        if ((maddr =compute_address(pid, offset))==mPFault)
        {
            return (mPFault);
        }

        
        instr = Memory[maddr].mInstr;
        CPU.IRopcode = instr >> opcodeShift;
        CPU.IRoperand = instr & operandMask;
        return (mNormal);
    }
}

//================================================
// load function
//================================================
int load_instruction(pid,offset ,opcode,operand)
int pid ,offset ,opcode, operand ;
{
    int ret;
    ret = load_instruction_swap(pid, offset, opcode, operand);
    if(ret==mError)
    {
        return (mError);
    }
    if(pid!=idlePid&&offset==(pageSize-1))
    {
        printf("swap the first page in");
        PCB[pid]->swapnum=0;
        int i,maddr;
        i=PCB[pid]->swapnum;
        
        maddr = PCB[pid]->pagetable[i]*pageSize;
        swap_in(pid, i , Memory+maddr);
        
        
        PCB[pid]->agebits[i] = PCB[pid]->agebits[i]|agebitsMask;
        PCB[pid]->dirtybit[i]=1;
        
        
    }
    
    return (mNormal);
}
//    int maddr;
//    offset++;
//    if(check_load_instruction_address (pid, offset) == mError)
//        return (mError);
//    else
//    {
//        opcode = opcode <<opcodeShift;
//        operand = operand & operandMask;
//        maddr= compute_swap_address(pid, offset);
//        Swap[maddr].mInstr = opcode | operand;
//        return (mNormal);
//        
//    }


int load_data (pid , offset, data)
int pid , offset;
float data;
{
    int ret;
    ret = load_data_swap(pid, offset, data);
    if (ret==mError)
    {
        return (mError);
    }
    if(pid==idlePid&&offset==0)
    {
         printf("swap the first page in");
        PCB[pid]->swapnum=0;
        int i,maddr;
        i=PCB[pid]->swapnum;
        
        maddr = PCB[pid]->pagetable[i]*pageSize;
        swap_in(pid, i , Memory+maddr);
        
        
        PCB[pid]->agebits[i] = PCB[pid]->agebits[i]|agebitsMask;
        PCB[pid]->dirtybit[i]=1;
        
    }
    else if (pid!=idlePid&&(offset+PCB[pid]->numInstr)==(pageSize-1))
    {
        
        printf("swap the first page in");
        PCB[pid]->swapnum=0;
        int i,maddr;
        i=PCB[pid]->swapnum;
        
        maddr = PCB[pid]->pagetable[i]*pageSize;
        swap_in(pid, i , Memory+maddr);
        
        
        PCB[pid]->agebits[i] = PCB[pid]->agebits[i]|agebitsMask;
        PCB[pid]->dirtybit[i]=1;
        
    }
    return (mNormal);
}
//    int maddr;
//    offset++;
//    offset+=PCB[pid]->MDbase;
//    if(check_load_data_address(pid, offset)== mError)
//        return (mError);
//    else
//    {
//        maddr = compute_swap_address(pid, offset);
//        Swap[maddr].mData = data;
//        printf("data in swap %.2f\n",data);
//        return (mNormal);
//    }





//================================================
// page fault handler and memory agescan
//================================================


void memory_agescan ()
{
    int i,j,maddr ;
    printf ("Scan and update age vectors for memory pages.\n");
    for (i=2; i<=currentPid; i++)
    {
        if (PCB[i]==NULL)
        {
            continue;
        }
        else
        {
            for (j=0; j<pagetablesize[i]; j++)
            {
                if (PCB[i]->pagetable[j]==0)
                {
                    continue;
                }
                
                PCB[i]->agebits[j] = PCB[i]->agebits[j]>>agebitShift;
                if (PCB[i]->agebits[j]==0&&PCB[i]->dirtybit[j]==0)
                {
                    free_list[PCB[i]->pagetable[j]]=1;    //when agebits are 0 and the frame is not dirty , it should be freed
                    PCB[i]->pagetable[j]=0;
                    ocpframe--;
                }
                else if (PCB[i]->agebits[j]==0&&PCB[i]->dirtybit[j]==1)
                {
                    maddr = PCB[i]->pagetable[j]*pageSize;
                    free_list[PCB[i]->pagetable[j]]=1;
                    PCB[i]->pagetable[j]=0;
                    ocpframe--;
                    swap_out(i,j,Memory+maddr);     //when the frame is dirty, we need to write back to swap space
                }

        }
        
        }
        
    }
    
}
void pagefault_complete(pid)
int pid;
{
    insert_doneWait_process (pid);
    set_interrupt (doneWaitInterrupt);
}



void init_pagefault_handler (pid)
int pid;
{

    pthread_create(&tid[0],NULL , pagefault_handler , (void*)(intptr_t)pid);
    pthread_join(tid[0], NULL);
    
    
}


int free_memory (pid )
int pid;
{
    int i;
    for (i =0; i<pagetablesize[pid]; i++)
    {
        free_list[PCB[pid]->pagetable[i]]=1;
    }
  
    return (mNormal);
}

//==================================================
// dump function
//==================================================


void dump_memory (pid )
int pid;
{
    int i,j ,k, maddr,count=0;
  
    for (i =0; i<pagetablesize[pid]; i++)
    {
        printf("Memory dump for process %d, page %d,frame %d: ",pid,i,PCB[pid]->pagetable[i]);
        maddr = PCB[pid]->pagetable[i]*pageSize;
        for (j=0; j<pageSize; j++)
        {
            count++;
            if (count<=PCB[pid]->MDbase)
            {
                printf("%x ",Memory[maddr+j].mInstr);
            }
            else if (count>PCB[pid]->MDbase&&count<=PCB[pid]->Mbound)
            {
                printf("%.2f ",Memory[maddr+j].mData);
            }
            else if (count>PCB[pid]->Mbound)
                break;
        }
        printf("\n");
    }
    
    j=0 ;
    k=0;
    int x[framenum-os_framenum];
    for (i=os_framenum; i<framenum; i++)
    {
        if (free_list[i])
        {
            x[j]=i;
            j++;
        }
    }
    printf("Free frame are: ");
    for (i=0; i<j; i++)
    {
        
        if (i!=j-1&&(x[i+1]-x[i])==1)
        {
            continue;
        }
        else if (i!=(j-1)&&(x[i+1]-x[i])!=1)
        {
            printf("%d ~ %d,",x[k],x[i]);
            k=i+1;
        }
        else if (i==(j-1)&&(x[i]-x[i-1])==1)
        {
            printf("%d ~ %d,",x[k],x[j-1]);
        }
        else if (i==(j-1)&&(x[i]-x[i-1])!=1)
        {
            printf("%d",x[j-1]);
        }
    }

    
//    printf("************ Instruction Memory Dump for Process %d\n", pid);
//    for (i=1;i<=PCB[pid]->numInstr;i++)
//    {
//        maddr = compute_address(pid, i);
//        printf("%x ",Memory[maddr].mInstr);
//    }
//    printf("\n");
//    
//    
//    printf ("************ Data Memory Dump for Process %d\n", pid);
//    for(i=1; i<=PCB[pid]->numData;i++)
//    {
//        j=i;
//        j+=PCB[pid]->MDbase;
//        maddr = compute_address(pid, j);
//        printf("%.2f",Memory[maddr].mData);
//    }
//    printf("\n");
}


