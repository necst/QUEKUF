#include <assert.h>
#include <stdio.h>
#include <chrono>

#include "../Design/DecoderMain.h"


void debugCorrection()
{
	bool syndrome[SYN_LEN] = {0, 0, 0,
							  1, 0, 1,
							  1, 0, 1};

	/*bool syndrome[SYN_LEN] = {1, 1, 1, 1,
							    0, 0, 0, 1,
							    0, 0, 0, 1,
							    0 ,0, 0, 0};*/

	/*bool syndrome[SYN_LEN] = {1, 1, 1, 1, 1,
							  0, 0, 0, 1, 1,
							  0, 0, 0, 0, 1,
							  0, 0 ,0, 0, 0,
							  0, 0 ,0, 0, 0};*/

	/*bool syndrome[SYN_LEN] = {0,0,0,0,0,0,0,0,
                                0,0,1,0,1,1,1,0,
                                0,0,1,1,0,0,1,1,
                                0,1,1,0,1,1,0,1,
                                0,1,0,1,0,0,1,0,
                                0,1,0,1,1,0,0,0,
                                0,0,1,0,0,0,0,0,
                                1,0,0,0,1,1,0,1};*/

    int64_t cc;

	int correctionArr[CORR_LEN] = {0};
	for(int j = 0; j < 1; j++)
	{
		bool correction[CORR_LEN] = {0};
		auto start=std::chrono::high_resolution_clock::now();
		decoderTop(syndrome, correction, &cc);
		auto stop=std::chrono::high_resolution_clock::now();
		auto duration=std::chrono::duration_cast<std::chrono::nanoseconds>(stop-start);
		for(int i = 0; i < CORR_LEN; ++i)
		{
			correctionArr[i] = correction[i];
		}
		printf("Time Taken: %ld\nClock Cycles Taken: %ld", duration.count(), cc);
	}

	printf("\nOur decoding: \n");
	for(int i = 0; i < CORR_LEN; ++i)
	{
		printf("%d ", correctionArr[i]);
	}
    printf("\n");
}


void correctionTest()
{
	FILE* f=fopen("/home/feder34/CLionProjects/qec-union-find/Scripts/Decoder_dataset.txt","r");

	int logicals[K][N] = {0};
	bool syndrome[SYN_LEN] = {0};
	bool correction[CORR_LEN] = {0};
	int correctionArr[CORR_LEN] = {0};
	int check[K];
	int bitstring[K];
    int accuracy=0;
    std::chrono::nanoseconds total=static_cast<std::chrono::nanoseconds>(0);



    fgetc(f); //first bracket
    fgetc(f); //second bracket
    for(int i=0; i<K && !feof(f); i++){

    	for(int j=0; j<N && !feof(f); j++){
    		logicals[i][j]=fgetc(f)-48;
            fgetc(f);//space or bracket
    	}

        fgetc(f);//end of line
        fgetc(f);//space
        fgetc(f);//bracket
    }

    while(!feof(f)){

        for(int i=0; i<SYN_LEN && !feof(f); i++){
            syndrome[i]=fgetc(f)-48;
            fgetc(f);//space
        }

        fgetc(f);//end of line
        fgetc(f);//first square bracket

        for(int i=0; i<K && !feof(f); i++){
        	check[i]=fgetc(f)-48;
        	fgetc(f); //space or bracket
        }

        fgetc(f);//end of line
        fgetc(f); //next square bracket
        int64_t cc;
        auto start=std::chrono::high_resolution_clock::now();
        decoderTop(syndrome, correction, &cc);
        auto stop=std::chrono::high_resolution_clock::now();
        auto duration=std::chrono::duration_cast<std::chrono::nanoseconds>(stop-start);
        total=total+duration;
        for(int i = 0; i < CORR_LEN; ++i)
		{
			correctionArr[i] = correction[i];
		}


        for(int i=0; i<K; i++){
        	bitstring[i]=0;

        	for(int j=0; j<CORR_LEN; j++)
        		bitstring[i]+=logicals[i][j]*correctionArr[j];

        	bitstring[i]=bitstring[i]%2;
        }


        if(check[0] == bitstring[0] && check[1] == bitstring[1])
        {
        	//printf("Correct!\n");
        	++accuracy;
        }
        else
        {
        	printf("\nOur decoding: \n");
			for(int i = 0; i < CORR_LEN; ++i)
			{
				printf("%d ", correctionArr[i]);
			}
			printf("\n");
        }

    }
	printf("\nDecoding test concluded with accuracy %f\nAverage running time: %f\n",(float)accuracy/169,(float)total.count()/169);

}

int main()
{
	correctionTest();
	return 0;
}
