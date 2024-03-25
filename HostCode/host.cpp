#include "host.h"

float startExecution(cl::CommandQueue& q, cl::Kernel& decoderUF, cl::Buffer& syn, cl::Buffer& corrOut, cl::Buffer& totalClocks)
{
	cl_int err;
	OCL_CHECK(err, err = decoderUF.setArg(0, syn));
	OCL_CHECK(err, err = decoderUF.setArg(1, corrOut));
    OCL_CHECK(err, err = decoderUF.setArg(2, totalClocks));


    // Data will be migrated to kernel space
    q.enqueueMigrateMemObjects({syn}, 0); /*0 means from host*/
	q.finish();

	auto start=std::chrono::high_resolution_clock::now();

	//Launch the Kernel
	q.enqueueTask(decoderUF);
	q.finish();
	
	auto stop=std::chrono::high_resolution_clock::now();

    auto duration=std::chrono::duration_cast<std::chrono::nanoseconds>(stop-start);

    printf("Operation concluded in %f nanoseconds\n", (float)duration.count());
	
	
	//Data from Kernel to Host
	q.enqueueMigrateMemObjects({corrOut, totalClocks},CL_MIGRATE_MEM_OBJECT_HOST);
	q.finish();
	
	return (float)duration.count();
}

int main(int argc, char* argv[]){
	
	//TARGET_DEVICE macro needs to be passed from gcc command line
	if(argc != 3) {
		std::cout << "Usage: " << argv[0] <<" <xclbin> <dataset path>" << std::endl;
		return EXIT_FAILURE;
	}

    FILE* f = fopen(argv[2], "r");
    
    double frequency = 350000000;

    int accuracy = 0;

	std::vector<uint8_t, aligned_allocator<uint8_t>> syndrome_in(SYN_LEN);
	std::vector<uint8_t, aligned_allocator<uint8_t>> correction_out(CORR_LEN);
    std::vector<uint64_t, aligned_allocator<uint64_t>> totalClocks(1);
    int logicals[K][CORR_LEN] = {0};
    int check[K] = {0};
    int bitstring[K] = {0};

    fgetc(f); //first bracket
    fgetc(f); //second bracket
    for(int i=0; i<K && !feof(f); i++){

        for(int j=0; j<CORR_LEN && !feof(f); j++){
            logicals[i][j]=fgetc(f)-48;
            fgetc(f);//space or bracket
        }

        fgetc(f);//end of line
        fgetc(f);//space
        fgetc(f);//bracket
    }


	double decodeAVG = 0;
	double clocksAVG = 0;


	size_t syndrome_size = sizeof(bool) * SYN_LEN;
	size_t correction_out_size = sizeof(bool) * CORR_LEN;
    size_t totalClocks_size = sizeof(int64_t);


	
    

/*
================================================================================================================================
	OPENCL STUFF
================================================================================================================================
*/

   	std::string binaryFile = argv[1];

    
    cl_int err;
    cl::Context context;
    cl::Kernel decoderUF;
    cl::CommandQueue q;

    auto devices = xcl::get_xil_devices();

    auto fileBuf = xcl::read_binary_file(binaryFile);

    cl::Program::Binaries bins{{fileBuf.data(), fileBuf.size()}};

    bool valid_device = false;

    for (unsigned int i = 0; i < devices.size(); i++) {
        auto device = devices[i];
        // Creating Context and Command Queue for selected Device
        OCL_CHECK(err, context = cl::Context(device, nullptr, nullptr, nullptr, &err));
        OCL_CHECK(err, q = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
        std::cout << "Trying to program device[" << i << "]: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        cl::Program program(context, {device}, bins, nullptr, &err);
        if (err != CL_SUCCESS) {
            std::cout << "Failed to program device[" << i << "] with xclbin file!\n";
        } else {
            std::cout << "Device[" << i << "]: program successful!\n";
            OCL_CHECK(err, decoderUF= cl::Kernel(program, "decoderTop", &err));
            valid_device = true;
            break; // we break because we found a valid device
        }
    }
    if (!valid_device) {
        std::cout << "Failed to program any device found, exit!\n";
        exit(EXIT_FAILURE);
    }

    cl::Buffer buffer_syn(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, syndrome_size, syndrome_in.data());
    cl::Buffer correction_out_buf(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, correction_out_size, correction_out.data());
    cl::Buffer totalClocks_Buf(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, totalClocks_size, totalClocks.data());

/*
================================================================================================================================
	MY STUFF
================================================================================================================================
*/

	float clocksAVGArr[200] = {0}; 

	int totalDecodes = 0;

    while(!feof(f))
    {
        for (int i = 0; i < SYN_LEN && !feof(f); i++) {
            syndrome_in[i] = fgetc(f) - 48;
            fgetc(f);//space
        }

        fgetc(f);//end of line
        fgetc(f);//first square bracket

        for (int i = 0; i < K && !feof(f); i++) {
            check[i] = fgetc(f) - 48;
            fgetc(f); //space or bracket
        }

        fgetc(f);//end of line
        fgetc(f); //next square bracket

        decodeAVG += startExecution(q, decoderUF, buffer_syn, correction_out_buf, totalClocks_Buf);
	clocksAVG += totalClocks[0];
	clocksAVGArr[totalDecodes] = totalClocks[0];

        for (int i = 0; i < K; i++) {
            bitstring[i] = 0;

            for (int j = 0; j < CORR_LEN; j++)
                bitstring[i] += logicals[i][j] * correction_out[j];

            bitstring[i] = bitstring[i] % 2;
        }

        if (check[0] == bitstring[0] && check[1] == bitstring[1]) {
            accuracy++;
        }
		totalDecodes++;
    }

	float distanceSum = 0;
	float mean = (float)clocksAVG/totalDecodes;
//Standard Deviation Computation
	for(int i = 0; i < totalDecodes; i++)
	{
		distanceSum += (mean - clocksAVGArr[i])*(mean - clocksAVGArr[i]);
	}
	
	float SD = sqrt(distanceSum / totalDecodes);
	
		
	printf("Decode AVG: %lf\n", (double)(clocksAVG*1000000.0)/(totalDecodes*frequency));
    	printf("Clock Cycles AVG: %f\n", (double)clocksAVG/totalDecodes);
	printf("Standard Deviation; %f\n", SD);
	printf("Correct/Total_Decodes: %d / %d\n", accuracy, totalDecodes); 

	return 0;
}

