if [ "$#" -eq 0 ]
then
  echo "Usage: ./build.sh <code_length> <platform> <freq> <grow_units> <peel_units>"
  exit 1
fi
code_length=$1
platform=$2
default_freq=$3
grow_units=$4
peel_units=$5
source /opt/xilinx/xrt/setup.sh
source /home/xilinx/software/Vitis_HLS/2023.1/settings64.sh

sed -i "s/^#define D.*/#define D ${code_length}/" src/Defines.h
sed -i "s/^#define GROWUNITCOUNT.*/#define GROWUNITCOUNT ${grow_units}/" src/Defines.h
sed -i "s/^#define PEELUNITCOUNT.*/#define PEELUNITCOUNT ${peel_units}/" src/Defines.h
sed -i "/#define[[:space:]]\+D[[:space:]]\+[0-9]\+/s/[0-9]\+/${code_length}/" HostCode/host.h
sed -i '/#define CBUILD/s/^/\/\//' src/Controller.cpp


make all TARGET=hw PLATFORM=${platform} FREQ_MHZ=${default_freq} JOBS=20
