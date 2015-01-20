//
//  swap.c
//  simos v2.0
//
//  Created by cy on 14-4-15.
//  Copyright (c) 2014年 cy. All rights reserved.
//
#include <stdio.h>
#include <stdlib.h>
#include "simos.h"
#include <string.h>

#define opcodeShift 24
#define operandMask 0x00ffffff



//typedef union
//{
//    float mData;
//    int mInstr;
//}mType;


mType* Swap;

int* sfree_list;
int swap_framenum;



void size_swap_space()
{
    Swap = (mType *)malloc(swapSize*sizeof(mType));
}


void initialize_swap_space()
{
    int i;
    size_swap_space();
    swap_framenum = ((swapSize-1)/pageSize)+1;
    
    sfree_list = (int *)malloc(swap_framenum* sizeof(int));
    for (i = 0; i<swap_framenum; i++)
    {
        sfree_list[i]=1;
    }
}


int compute_swap_address(pid,offset)
int offset,pid ;
{
    int i,maddr;
    i = (offset-1)/pageSize;
    if(offset%pageSize !=0)
        maddr = (PCB[pid]->swaptable[i])*pageSize+(offset)%pageSize-1;
        else
            maddr = (PCB[pid]->swaptable[i])*pageSize+7;
            if(Observe)
            {
                printf("Process %d call compute address function in swap space,in frame %d offset: %d and the physical address:%d\n",pid,PCB[pid]->swaptable[i],offset,maddr);
            }
    return(maddr);
}


//=====================================================================
//allocate swap space and  load instructions and data to the space
//=====================================================================


int allocate_swap_space(pid,msize,numinstr)
{
    int size =0,i,j=0;
    size = ((msize-1)/pageSize)+1;
    PCB[pid]->pagetable= (int*)malloc(size*sizeof(int));
    PCB[pid]->swaptable=(int *)malloc(size*sizeof(int));
    PCB[pid]->agebits=(char*)malloc(size*sizeof(char));
    PCB[pid]->dirtybit=(char*)malloc(size*sizeof(char));
    for (i=0; i<swap_framenum; i++)
    {
        if (sfree_list[i])
        {
            if (j==size)
            {
                break;
            }
            PCB[pid]->pagetable[j]=0;
            PCB[pid]->dirtybit[j]=0;
            PCB[pid]->agebits[j]=0;
            PCB[pid]->swaptable[j]=i;
            sfree_list[i]=0;
            j++;
        }
    }
    if (j<size)
    {
        printf("No enough swap space %d for process %d\n",msize,pid);
        return (mError);
    }
    swaptablesize[pid]=j;

    PCB[pid]->Mbase=0;
    PCB[pid]->Mbound=msize;
    PCB[pid]->MDbase=numinstr;
    return (mNormal);
}

int check_load_data_address (pid,offset )
int offset,pid ;
{
    
    if (offset>PCB[pid]->Mbound)
    {
        printf("Process %d access offset from base %d. Outside address space\n",pid,(offset));
        return (mError);
    }
    else if (offset<PCB[pid]->MDbase)
    {
        printf("Process %d data load access offset from base %d. In instruction region!\n",pid,(offset));
        return (mError);
    }
    
    else
        return (mNormal);
}

int check_load_instruction_address(pid,offset )
int pid,offset;
{
    if(offset>PCB[pid]->Mbound)
    {
        printf("Process %d access offset from base %d. Outside address space\n",pid,offset);
        return (mError);
    }
    else if (offset>PCB[pid]->MDbase)
    {
        printf("Process %d instruction load access offset from base %d. In data region!\n",pid,offset);
        return (mError);
    }
    else
        return (mNormal);
}



int load_instruction_swap(pid,offset ,opcode,operand)
int pid ,offset ,opcode, operand ;
{
    int maddr;
    offset++;
    if(check_load_instruction_address (pid, offset) == mError)
        return (mError);
    else
    {
        opcode = opcode <<opcodeShift;
        operand = operand & operandMask;
        maddr= compute_swap_address(pid, offset);
        Swap[maddr].mInstr = opcode | operand;
        return (mNormal);
        
    }
}

int load_data_swap (pid , offset, data)
int pid , offset;
float data;
{
    int maddr;
    offset++;
    offset+=PCB[pid]->MDbase;
    if(check_load_data_address(pid, offset)== mError)
        return (mError);
    else
    {
        maddr = compute_swap_address(pid, offset);
        Swap[maddr].mData = data;
        printf("data in swap %.2f\n",data);
        return (mNormal);
    }
}



//=================================================
//run time page fault respond
//=================================================




//mType swap_in(pid,offset )
//int offset,pid ;
//{
//    int maddr;
//    maddr = compute_address(pid, offset );
//    return (Swap[maddr]);
//}
//

//==========================================
// swap in and out function
//==========================================


void swap_in(pid,pageNumber ,memory)
int pid,pageNumber;
mType* memory;
{
    int maddr1;
 

    maddr1 = (PCB[pid]->swaptable[pageNumber])*pageSize;
  //  memcpy(memory, Swap+maddr1*sizeof(mType), pageSize*sizeof(mType));
    memcpy(memory, Swap+maddr1, pageSize*sizeof(mType));
   // printf("%d",Swap+maddr*sizeof(mType));
   
}




void swap_out(pid,pageNumber,memory)
int pid,pageNumber;
mType* memory;
{
    memcpy(Swap+(PCB[pid]->swaptable[pageNumber])*pageSize*sizeof(mType), memory, pageSize*sizeof(mType));
}

//==========================================
// free and dump swap space function
//==========================================



int free_swap_space(pid )
int pid;
{
    int i;
    for (i = 0; i<swaptablesize[pid]; i++)
    {
        sfree_list[PCB[pid]->swaptable[i]]=1;
    }
    return (mNormal);
}



void dump_swap_space(pid )
int pid;
{
    int i,j,k,maddr,mark=0;
    for (i = 0; i<swaptablesize[pid]; i++)
    {
        maddr = PCB[pid]->swaptable[i]*pageSize;
        printf("Swap dump for process %d, page %d frame %d: ",pid,i,PCB[pid]->swaptable[i]);
            for (j=0; j<pageSize; j++)
            {
                mark++;
                if ((mark)<=PCB[pid]->MDbase)
                {
                    printf("%x ",Swap[maddr+j].mInstr);
                }
                else if((mark)>PCB[pid]->MDbase&&mark<=PCB[pid]->Mbound)
                {
                    printf("%.2f ",Swap[maddr+j].mData);
                }
                else if (mark>PCB[pid]->Mbound)
                
                break;
                
                
                
            }
            printf("\n");
        
        
    }
    j=0;
    k=0;
    int x[swap_framenum];
    for (i = 0; i<swap_framenum; i++)
    {
        if (sfree_list[i])
        {
            x[j]=i;
            j++;
        }
        
    }
    printf("Free swap space are: ");
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
    
    
    
}
//











