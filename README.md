# prepfil_dsn

Installation instructions:

REQUIREMENTS

- C++11-capable C++ compiler
- CMake (>= 2.8.8, https://cmake.org/download/)
- Google Protobuf (>= 2.0, https://github.com/google/protobuf)
- Boost (>= 1.32, only need Filesystem and Regex components,
  http://www.boost.org/users/download/)
- CUDA (optional, https://developer.nvidia.com/cuda-downloads)
- FFTW (required if CUDA is not available, http://www.fftw.org/download.html)
- OpenMP (optional, http://openmp.org/)


INSTALLATION

If compiling boost, use --with-libraries=filesystem when running bootstrap to
only compile the filesystem library.

To compile filterbank_utils, follows these steps:

$ git clone pollux:/home/lippuner/git/filterbank_utils.git/

$ cd filterbank_utils

$ mkdir build

$ cd build

$ cmake -DCMAKE_INSTALL_PREFIX=<path to install dir> ..
  
$ make -j8
  
$ make test
  
$ make install

*** Use cmake -DCUDA=No -DCMAKE_INSTALL_PREFIX=<path to install dir> .. ***
To Install WITHOUT CUDA!


add the following to your .bashrc
export PATH=/home/lippuner/local/bin:$PATH
export LD_LIBRARY_PATH=/home/lippuner/local/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/home/lippuner/local/lib64:$LD_LIBRARY_PATH
then (after sourcing new .bashrc, of course) delete everything in
the build directory and run cmake again



Added the following to my .bashrc at the end:

# Added for compiling prepfil with cmake.

# allow arbitrarily big core dumps
ulimit -c unlimited

export PATH=/sbin:$PATH
export SWT_GTK3=0

if [ -z "$LD_LIBRARY_PATH" ]; then
    export LD_LIBRARY_PATH=/home/lippuner/opt/dedisp/lib:$LD_LIBRARY_PATH
else
    export LD_LIBRARY_PATH=/home/lippuner/opt/dedisp/lib:$LD_LIBRARY_PATH
fi

export LD_LIBRARY_PATH=/home/lippuner/heimdall_install/lib:$LD_LIBRARY_PATH
export PATH=/home/lippuner/heimdall_install/bin:$PATH

export PATH=/home/lippuner/local/bin:$PATH
export LD_LIBRARY_PATH=/home/lippuner/local/lib:$LD_LIBRARY_PATH

export LD_LIBRARY_PATH=/home/lippuner/local/lib64:$LD_LIBRARY_PATH
export PKG_CONFIG_PATH=/home/lippuner/local/lib/pkgconfig:$PKG_CONFIG_PATH

export PATH=/home/lippuner/local/cuda-7.5/bin:$PATH
export LD_LIBRARY_PATH=/home/lippuner/local/cuda-7.5/lib64:$LD_LIBRARY_PATH






Installing prepfil on Ubuntu 16.04:

Need to build shared fftw libraries using:

./configure --prefix=$ASTROSOFT --enable-threads --enable-shared CFLAGS=-fPIC FFLAGS=-fPIC
./configure --prefix=$ASTROSOFT --enable-thread --enable-shared CFLAGS=-fPIC FFLAGS=-fPIC

Need protobuf-compiler and protobuf using:

sudo apt-get install protobuf-compiler
sudo apt-get install protobuf

Need Cuda 8 on Ubuntu 16.04

Need to set LD_LIBRARY_PATH correctly for CUDA in order to use the CUDA installation in bashrc.

Setting CMAKE_PREFIX_PATH for finding libraries required by prepfil/cmake:

export CMAKE_PREFIX_PATH=/some/path:/some/other/path



Copy .ssh/

Change in /home/pearlman/software/filterbank_utils/CMakeLists.txt (Line 43):

FROM:

set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-gencode arch=compute_52,code=sm_52)

TO:

set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-gencode arch=compute_52,code=sm_52; -gencode arch=compute_61,code=sm_61)




ON GEMINGA:

If you don't want to use the GPU for installing prepfil, add:

-DCUDA=No to the CMAKE command

  
  
Most recent instructions (Probably the best ones to follow now):
  
Installation Instructions (prepfil on Ubuntu 16.04):

First, we need to remake the double precision FFTW libraries. There is a bug in the pulsar software online installation script, here is the fix. This will build the double precision FFTW shared libraries and threads:

1. cd /usr/psr_software/fftw-3.3.4/
2. ./configure --prefix=$ASTROSOFT --enable-threads --enable-shared CFLAGS=-fPIC FFLAGS=-fPIC
3. make
4. make check
5. make install
6. make clean

Next, let's install prepfil:

6. Remove all the instances of "lippuner" from your .bashrc file. We will need to comment/delete those lines, which set the PATH and LD_LIBRARY_PATH variables from pollux. We need to use Cuda 8 on Ubuntu 16.04, not Cuda 7.5. To do this, add the following at the end of your .bashrc file:

export LD_LIBRARY_PATH=/usr/lib:/usr/lib/x86_64-linux-gnu:$PGPLOT_DIR:$ASTROSOFT/lib:$PRESTO/lib:$PRESTO/lib64:/usr/local/cuda/lib64:$LD_LIBRARY_PATH

7. cd /usr/psr_software
8. git clone https://(bitbucket_username)@bitbucket.org/jlippuner/filterbank_utils.git
9. cd filterbank_utils/

10. We need to update the GPU architecture in filterbank_utils. Change the following in /usr/psr_software/filterbank_utils/CMakeLists.txt (Line 43):

REPLACE:

set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-gencode arch=compute_52,code=sm_52)

TO:

set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-gencode arch=compute_52,code=sm_52; -gencode arch=compute_61,code=sm_61)

11. mkdir build/
12. cd build/
13. cmake -DCMAKE_INSTALL_PREFIX=/usr/psr_software/filterbank_utils/install ..
14. make -j8
15. make test
16. make install



In the future, we will need to make sure both single precision FFTW and double precision FFTW shared libraries are built. protobuf (libprotobuf-dev), protobuf-compiler (protobuf-compiler typo),  boost (libboost-all-dev), thrust(?), and cuda-8.0 will also need to be installed. Thrust is located in /usr/local/cuda-8.0/include/thrust

Also, another useful tip Jonas showed me is to set the CMAKE_PREFIX_PATH before running cmake to force cmake to look for libraries in specific directories:

export CMAKE_PREFIX_PATH=/some/path:/some/other/path
