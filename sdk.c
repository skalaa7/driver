#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xil_io.h"

#define DELILAC 100 ///for normalization
#define QUOT 10//9.99965367///for normalization
#define NUMOFVAR 50
#define NUMOFSLACK 50
#define ROWSIZE (NUMOFSLACK+1)
#define COLSIZE (NUMOFSLACK+NUMOFVAR+1)
int main()
{
    init_platform();
	
    float wv[ROWSIZE][COLSIZE];
	int indeks=0;
	for(int j=0;j<ROWSIZE; j++)
	{
		for(int i =0;i<COLSIZE;i++)
		{
			wv[j][i]=0;
		}
	}
	
	if(baza.is_open())
    {
        for(int j = 0; j < ROWSIZE; j++)
        {
            for(int i = 0; i< NUMOFVAR; i++)
            {
              wv[j][i]=ulazhex[indeks];
			  indeks++;
              
            }
        }
		for(int j = 0;j< NUMOFSLACK;j++)
		{
			wv[j][COLSIZE-1]=ulazhex[indeks];
			indeks++;
		}
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
	
	//-----------------------------------------------------------------
    //CalculateSimplex 
    int pivotRow;
    int pivotCol;
    bool unbounded=false;
    bool optimality=false;
    float pivot;
    int count;
	
    while(!optimality)
    {
    	//count++;
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
        //isUnbounded(wv,pivotCol)
        unbounded=true;
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
     
    
        
        	float temp;
  p = 0; 
  //offset += sc_core::sc_time(11*takt, sc_core::SC_NS);
  for(int j=0;j<COLSIZE;j++)
  {
	 //baza >> temp;  //iz baze u temp
           //offset += sc_core::sc_time(1*takt, sc_core::SC_NS);///koliko taktova
           //std::cout<<(int)p<<std::endl;
          //write_bram(p++, (num_t) wv[pivotRow][j]);//write into bram 
		  Xil_Out32(XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR+p*4,(int) wv[pivotRow][j]);
		  p++;
  }
  for (int i = 0; i < ROWSIZE; ++i)
  {//write matrix A into bram
  	if(i!=pivotRow){
        for (int j = 0; j < COLSIZE; ++j)
        {
          //baza >> temp;  //iz baze u temp
           //offset += sc_core::sc_time(1*takt, sc_core::SC_NS);///koliko taktova
           //std::cout<<(int)p<<std::endl;
          //write_bram(p++, (num_t) wv[i][j]);//write into bram
		  Xil_Out32(XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR+p*4,(int) wv[i][j]);
		  p++;
          
	}
	}
	//cout<<"p="<<p<<endl;
  }
  //cout<<"p="<<p<<endl;
  //write_bram(p++,pivotRow);
  //offset += sc_core::sc_time(1*takt, sc_core::SC_NS);
  //write_bram(p,pivotCol);
  Xil_Out32(XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR+p*4,(int) pivotCol);
  //offset += sc_core::sc_time(1*takt, sc_core::SC_NS);
  printf("row=%d, col=%d",pivotRow,pivotCol);//cout<<"row="<<pivotRow<<", col="<<pivotCol<<endl;
  printf("%d\n",pivot);//cout<<pivot<<endl;
        
  print("Sent to bram\n");//cout<<"Sent to bram"<<endl;
	
        //write_hard(ADDR_CMD, 1);
		
        //cout<<"Starte hard"<<endl;
      
  
    int tempwv;
  p=0;  //READING FROM BRAM
  //offset += sc_core::sc_time(11*takt, sc_core::SC_NS);
for (int i = 0; i < ROWSIZE; ++i)
  {//write matrix A into bram
  for (int j = 0; j < COLSIZE; ++j)
  {
     
     //offset += sc_core::sc_time(1*takt, sc_core::SC_NS);
     //read_bram(p++, tempwv);//read from bram
	 tempw=(int)Xil_In32(XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR+p*4)
      wv[i][j]=(float)tempwv;    
	}
  }

  } 
   //Writing results
    if(unbounded)
    {
        print("Unbounded\n");//cout<<"Unbounded"<<endl;
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
            { //cout<<"wv:"<<wv[index][COLSIZE-1]<<endl;
               // cout<<"variable"<<i+1<<": "<<DELILAC*wv[index][COLSIZE-1]<<endl;
				print("variable%d: %f\n",(i+1),(DELILAC*wv[index][COLSIZE-1]));
               
            }
            else
            {
                //cout<<"wv:"<<wv[index][COLSIZE-1]<<endl;
                //cout<<"variable"<<i+1<<": "<<0<<endl;
				print("variable%d: %f\n",(i+1),0);
            }
        }

     
        //cout<<"pre optimal:"<<wv[ROWSIZE-1][COLSIZE-1]<<endl;
        //cout<<endl<<"Optimal solution is "<<DELILAC*QUOT*wv[ROWSIZE-1][COLSIZE-1]<<endl;
		print("Optimal solution is %f\n",(DELILAC*QUOT*wv[ROWSIZE-1][COLSIZE-1]));
        //cout<<"Time is "<<offset<<endl;
        //cout<<"Number of iterations is "<<count<<endl;
		print("Number of iterations is %d\n",count);
        /*int time_delay;
    	double t;
    	sscanf(offset.to_string().c_str(), "%d ns ", &time_delay);
    	t=time_delay*1.0/1000000000;
    	int ops=530452;
    	t = ops/t;
    	std::cout << "Number of operations per second is: " << t  << endl;*/
    }

    //return 0;

                       
}
    

    cleanup_platform();
    return 0;
}



//print("Hello World\n\r");
//Xil_Out32(0x4000_0000 + 8, i*4);
//Xil_Out32(XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR+i*4, i);
//printf("%d: %d\n",i,(int)Xil_In32(XPAR_AXI_BRAM_CTRL_0_S_AXI_BASEADDR+i*4));