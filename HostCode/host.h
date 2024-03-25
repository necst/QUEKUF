#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <random>
#include <unistd.h>
#include <assert.h>
#include <chrono>
#include <math.h>


#include "common/xcl2.hpp"

#define K 2
#define D 8
#define SYN_LEN D*D
#define CORR_LEN 2*D*D
#define DATA_SIZE 4098
#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1
#define TESTSIZE 5

int main(int argc, char** argv);
