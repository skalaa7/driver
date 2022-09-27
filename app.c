#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "baza.h"
#define DELILAC 100 //for normalization
#define QUOT 10//for normalization
#define NUMOFVAR 3
#define NUMOFSLACK 3
#define ROWSIZE (NUMOFSLACK+1)
#define COLSIZE (NUMOFSLACK+NUMOFVAR+1)
#define MAX_PKT_SIZE (ROWSIZE*COLSIZE+1)
char loc0[]="/dev/xlnx,pivot";
char loc1[]="/dev/xlnx,bram";
int i;
int j;
int p;
void printmat(float wv[ROWSIZE][COLSIZE])
{
	for(int j=0;j<ROWSIZE; j++)
			{
				for(int i =0;i<COLSIZE;i++)
				{
					printf("%f,",wv[j][i]);
				}
				printf("\n");
			}
}
unsigned int float2uint32(float x)
{
    if(x>=0)
    {
        printf("%f,%u\n",x,(unsigned int)(x*2097152));
        return (unsigned int)(x*2097152);
    }
    else
    {
        printf("%f,%u\n",x,4294967295+(unsigned int)(x*2097152)+1);
        return 4294967295+(unsigned int)(x*2097152)+1;
    }
}
float uint2float(unsigned int x)
{
    if(x>=2147483648)
    {
        printf("%u,%f\n",x,((float)(x-4294967295))/2097152);
        return ((float)(x-4294967295))/2097152;
    }
    else
    {
        printf("%u,%f\n",x,((float)x)/2097152);
        return ((float)x)/2097152;
    }
}
void read_bram(unsigned int wvrow[ROWSIZE*COLSIZE+1])
{
	#ifdef MMAP
	// If memory map is defined send image directly via mmap
	int fd;
	int *p;
	fd = open(loc1, O_RDWR|O_NDELAY);
	if (fd < 0)
	{
		printf("Cannot open /dev/xlnx,bram for write\n");
		return -1;
	}
	p=(int*)mmap(0,5152*4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	memcpy(wvrow, p, MAX_PKT_SIZE);
	munmap(p, MAX_PKT_SIZE);
	close(fd);
	if (fd < 0)
	{
		printf("Cannot close /dev/xlnx,bram for write\n");
		return -1;
	}
	#else
	FILE *bram;
	if(bram == NULL)
	{
		printf("Cannot open /dev/xlnx,bram for read\n");
		return -1;
	}
	bram=fopen(loc1,"r")
	for(p=0;p<ROWSIZE*COLSIZE+1;p++)
	{
		
		fscanf(bram,"%u",wvrow[p]);
		
	}	
	fclose(bram);
	#endif
}
void write_bram(unsigned int wvrow[ROWSIZE*COLSIZE+1])
{
	#ifdef MMAP
	// If memory map is defined send image directly via mmap
	int fd;
	int *p;
	fd = open(loc1, O_RDWR|O_NDELAY);
	if (fd < 0)
	{
		printf("Cannot open /dev/xlnx,bram for write\n");
		return -1;
	}
	p=(int*)mmap(0,5152*4, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	memcpy(p, wvrow, MAX_PKT_SIZE);
	munmap(p, MAX_PKT_SIZE);
	close(fd);
	if (fd < 0)
	{
		printf("Cannot close /dev/xlnx,bram for write\n");
		return -1;
	}
	#else
	FILE *bram;
	if(bram == NULL)
	{
		printf("Cannot open /dev/xlnx,bram for write\n");
		return -1;
	}
	for(p=0;p<ROWSIZE*COLSIZE+1;p++)
	{
		bram=fopen(loc1,"w");
		fprintf(bram,"%d,%u\n",p,wvrow[p]);
		fclose(bram);
	}

	#endif;
}
int read_pivot(int *start,int *ready)
{
	FILE *pivot;
	pivot=fopen(loc0,"r");
	int start,ready;
	if(pivot == NULL)
	{
		printf("Cannot open /dev/xlnx,pivot for read\n");
		return -1;
	}
	
	fscanf(pivot,"%d %d",start,ready);
	fclose(pivot);
	return ready;
	
}
void write_pivot(int value)
{
	FILE *pivot;
	pivot=fopen(loc0,"w");
	if(pivot == NULL)
	{
		printf("Cannot open /dev/xlnx,pivot for write\n");
		return -1;
	}
	fprintf(pivot,"%d",value);
	fclose(pivot);
}

int main()
{
    printf("\nHeloo\n");
    float wv[ROWSIZE][COLSIZE];
	int wvrow[ROWSIZE*COLSIZE+1];
	int indeks=0;
	int ready=1;
	int start=0;
	//FILE *baza;
	//baza=fopen("baza.txt","r");
	for(int j=0;j<ROWSIZE; j++)
	{
		for(int i =0;i<COLSIZE;i++)
		{
			wv[j][i]=0;
		}
	}

        for(int j = 0; j < ROWSIZE; j++)
        {
            for(int i = 0; i< NUMOFVAR; i++)
            {
              wv[j][i]=baza[indeks];
			  indeks++;

            }
        }
		for(int j = 0;j< NUMOFSLACK;j++)
		{
			wv[j][COLSIZE-1]=baza[indeks];
			indeks++;
		}
	for(int j=0;j<ROWSIZE; j++)
	{
		for(int i =0;i<COLSIZE;i++)
		{
			wv[j][i]=wv[j][i]/QUOT;
		}
		//cout<<"wv: pre baze:"<<wv[j][COLSIZE-1]<<endl;
		wv[j][COLSIZE-1]=wv[j][COLSIZE-1]/DELILAC;

	}

	for(int j=0;j<ROWSIZE-1;j++)
	{
		{
			wv[j][NUMOFVAR+j]=1;
		}
	}
	for(int j=0;j<ROWSIZE; j++)
		{
			for(int i =0;i<COLSIZE;i++)
			{
				printf("%f,",wv[j][i]);
			}
			printf("\n");
		}
	for(int j=0;j<ROWSIZE; j++)
	{
		for(int i =0;i<COLSIZE;i++)
		{
			printf("%f,",wv[j][i]);
		}
		printf("\n");
	}

	
	write_pivot(0);
	read_pivot(&ready,&start);
	printf("Start=%d,ready=%d",start,ready);
	printf("Read from baza\n");
	//-----------------------------------------------------------------
    //CalculateSimplex
	print("Starting simplex algorithm\n");
    int pivotRow=0;
    int pivotCol=0;
    bool unbounded=false;
    bool optimality=false;
    float pivot;
    int count=0;
    int tempor=0;
    //u32 tempneg=0;
    float tempflo;
    //float temp;

    //while(!optimality)
    for(i=0;i<3;i++)
    {
    	//int tempwv;
    	count++;
        //checkOptimality(wv)
        optimality=true;
        for(int i=0;i<COLSIZE-1;i++)
        {
            if(wv[ROWSIZE-1][i]<0)
                optimality=false;
        }
        //findPivotCol(wv);
        float minnegval=wv[ROWSIZE-1][0];
        int loc=0;
        for(int i=1;i<COLSIZE-1;i++)
        {
            if(wv[ROWSIZE-1][i]<minnegval)
            {
                minnegval=wv[ROWSIZE-1][i];
                loc=i;
            }
        }
        pivotCol=loc;
        printf("pivotcol=%d\n",pivotCol);
        //isUnbounded(wv,pivotCol)
        unbounded=false;
        for(int j=0;j<ROWSIZE-1;j++)
        {
            if(wv[j][pivotCol]>0)
                unbounded=false;
        }
        if(unbounded)
        {
            break;
        }
        //findPivotRow(wv,pivotCol);
        float rat[ROWSIZE-1];
        for(int j=0;j<ROWSIZE-1;j++)
        {
            if(wv[j][pivotCol]>0)
            {
                rat[j]=wv[j][COLSIZE-1]/wv[j][pivotCol];
            }
            else
            {
                rat[j]=0;
            }
        }
        float minpozval=99999999;
        loc=0;
        for(int j=0;j<ROWSIZE-1;j++)
        {
            if(rat[j]>0)
            {
                if(rat[j]<minpozval)
                {
                    minpozval=rat[j];
                    loc=j;
                }
            }
        }
        pivotRow=loc;

        pivot=wv[pivotRow][pivotCol];
		//wv[pivotRow][pivotCol]=1/wv[pivotRow][pivotCol]; za softverski implementiran hardver u drajveru

        //printmat(wv);
        print("Sending to bram\n");
  p = 0;
  for(int j=0;j<COLSIZE;j++)
  {
		wvrow[p]=float2uint32(wv[pivotRow][j]);
		p++;
  }
  
  print("Sent pivot row\n");
  for (int i = 1; i < ROWSIZE; ++i)
  {
  	if(i!=pivotRow){
        for (int j = 0; j < COLSIZE; ++j)
        {
			wvrow[p]=float2uint32(wv[i][j]);
			p++;
		}
	}
  }
  printf("Sent matrix \n");
  wvrow[p]=float2uint32(pivotCol);
  
  
  
  printf("p=%d\n",p);
 
  printf("row=%d, col=%d\n",pivotRow,pivotCol);
  printf("pivot=%f\n",pivot);
  write_bram(wvrow);
  
  print("Sent to bram\n");


		write_pivot(1);///

		write_pivot(0);///
		
		while(!read_pivot());

		print("Pivot complete\n");

read_bram(wvrow);
  
  p=1;  //p=1 za softverski implementiran hardver u drajveru
for (int i = 0; i < ROWSIZE; ++i)
  {
  for (int j = 0; j < COLSIZE; ++j)
  {


	  
	  wv[i][j]=uint2float(wvrow[p]);
	  
  p++;
  }
  }
	printmat(wv);
	//printf("Counter1:%d\n",(int)Xil_In32(COUNTER_REG));
  }
   //Writing results
    if(unbounded)
    {
        print("Unbounded\n");
    }
    else
    {
        //solutions(wv);
        for(int i=0;i<NUMOFVAR; i++)
        {
            int count0 = 0;
            int index = 0;
            for(int j=0; j<ROWSIZE-1; j++)
            {
                if(wv[j][i]==0.0)
                {
                    count0 = count0+1;
                }
                else if(wv[j][i]==1)
                {
                    index = j;
                }
            }
            if(count0 == ROWSIZE - 2 )
				printf("variable%d: %f\n",(i+1),(DELILAC*wv[index][COLSIZE-1]));

            }
            else
            {
				printf("variable%d: %d\n",(i+1),0);
            }
        }



		printf("Optimal solution is %f\n",(DELILAC*QUOT*wv[ROWSIZE-1][COLSIZE-1]));

		printf("Number of iterations is %d\n",count);

   }

    return 0;
}