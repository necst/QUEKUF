if [ "$#" -eq 0 ]
then
  echo "Usage: ./build.sh <code_length> <platform> <freq>"
  exit 1
fi
platform="xilinx_u55c_gen3x16_xdma_base_3"
code_length=$1
default_freq=350
source /opt/xilinx/xrt/setup.sh
source /home/xilinx/software/Vitis_HLS/2023.1/settings64.sh

sed -i "s/^#define D.*/#define D ${code_length}/" Design/Defines.h
sed -i "/#define[[:space:]]\+D[[:space:]]\+[0-9]\+/s/[0-9]\+/${code_length}/" HostCode/host.h
sed -i '/#define CBUILD/s/^/\/\//' Design/Controller.cpp


make all TARGET=hw PLATFORM=${platform} FREQ_MHZ=${default_freq}
