#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DELILAC 100 //for normalization
#define QUOT 10//for normalization
#define NUMOFVAR 3
#define NUMOFSLACK 3
#define ROWSIZE (NUMOFSLACK+1)
#define COLSIZE (NUMOFSLACK+NUMOFVAR+1)
#define CONVERSE 2097152
char loc0[]="/dev/xlnx,pivot";
char loc1[]="/dev/xlnx,bram";
int i;
int j;
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
void read_bram()
{
	FILE *bram;
	bram=fopen()
}
void write_hard()
{
	
}
void read_pivot(int *ready, int *start)
{
	FILE *pivot;
	pivot=fopen(loc0,'r');
	if(pivot == NULL)
	{
		printf("Cannot open /dev/xlnx,pivot for read\n");
		return -1;
	}
	
	fscanf(pivot,"%d",start);
	fscanf(pivot,"%d,ready);
	
}
void write_pivot()
{
	
}

int main()
{
    printf("\nHeloo\n");
    float wv[ROWSIZE][COLSIZE];
	int indeks=0;

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
              wv[j][i]=ulazdec[indeks];
			  indeks++;

            }
        }
		for(int j = 0;j< NUMOFSLACK;j++)
		{
			wv[j][COLSIZE-1]=ulazdec[indeks];
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
	Xil_Out32(START_REG,0);
	printf("Start:%d,Ready:%d\n",(int)Xil_In32(START_REG),(int)Xil_In32(READY_REG));////
	printf("Counter0:%d\n",(int)Xil_In32(COUNTER_REG));
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
    int p;
    int i;
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


        //printmat(wv);
        print("Sending to bram\n");
  p = 0;
  for(int j=0;j<COLSIZE;j++)
  {
	
		  Xil_Out32(XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR+p*4,(int) (wv[pivotRow][j]*CONVERSE));////

		  p++;
  }
  printf("p=%d\n",p);
  print("Sent pivot row\n");
  for (int i = 1; i < ROWSIZE; ++i)
  {
  	if(i!=pivotRow){
        for (int j = 0; j < COLSIZE; ++j)
        {
        
        	Xil_Out32(XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR+p*4,(int) (wv[i][j]*CONVERSE));////

 
		  p++;


	}
	}

  }
  printf("Sent matrix p=%d\n",p);

  Xil_Out32(XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR+p*4,(int) (pivotCol*CONVERSE));////
  
  printf("p=%d\n",p);
 
  printf("row=%d, col=%d\n",pivotRow,pivotCol);
  printf("pivot=%f\n",pivot);
  Xil_Out32(XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR,(int) (wv[pivotRow][0]*CONVERSE));////
  print("Sent to bram\n");


		Xil_Out32(START_REG,1);///

		Xil_Out32(START_REG,0);////

		print("Pivot complete\n");


  p=1;  

for (int i = 0; i < ROWSIZE; ++i)
  {
  for (int j = 0; j < COLSIZE; ++j)
  {


	  tempor=(int)Xil_In32(XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR+p*4);//readbram
	  wv[i][j]=(float)tempor;
	  wv[i][j]=wv[i][j]/CONVERSE;
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