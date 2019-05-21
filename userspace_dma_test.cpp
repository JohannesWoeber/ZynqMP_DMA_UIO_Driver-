#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <string>
#include <iostream>

#include "xzdma_hw.h"
#include "xzdma.h"
const unsigned int BUFFERSIZE = 1024;


class UserspaceZynqMpDMA{
    public:
    UserspaceZynqMpDMA(std::string const& cdevice) : cdevice(cdevice) {}
    int mmapRegs(){
        cdevice_fd = open(cdevice.c_str(), O_RDWR);
        if (cdevice_fd < 0){
            std::cerr << "opening " << cdevice << " failed " << std::endl;
            return -1;

        }
               
        ptrRegs = mmap(NULL, 2**12, PROT_READ | PROT_WRITE, MAP_SHARED, cdevice_fd, 0);
        if (ptrRegs == MAP_FAILED) {
                std::cerr << "UIO_mmap construction failed" << std::endl;
                return -2;
        }   
    }

    int startSimpleDMATransfer(unsigned long int srcAddr, unsigned long int destAddr, unsigned long int transferLength){
        XZDma_ReadReg(ptrRegs, XZDMA_CH_STS_OFFSET)
    }

    ~UserspaceZynqMpDMA(){
        close(cdevice_fd);
    }
    private:
        const std::string cdevice;
        void *ptrRegs;
        int cdevice_fd;
};

UserspaceZynqMpDMA LpdDmaChan1("/dev/uio0");


int requestDmaTransfer(unsigned long int srcAddr, unsigned long int destAddr, unsigned long int transferLength){
    LpdDmaChan1.mmapRegs();
    return 0;
}


int main()
{
        int fd;
        printf("**********************************\n");
        printf("*******Userspace DMA TEST*******\n");

        // write values into udmabuf0
        printf("write values into udmabuf0\n");

        if ((fd  = open("/dev/udmabuf0", O_RDWR | O_SYNC)) != -1) {
          void* buf = mmap(NULL, BUFFERSIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
          for( size_t i = 0; i < BUFFERSIZE; ++i){
                reinterpret_cast<uint8_t*>(buf)[i] = i%256;
          }
          close(fd);
        }
        
        // request transfer from buf0 to buf1 from dmaengine-proxy driver
        printf("request transfer from buf0 to buf1 from dmaengine-proxy driver \n");
        unsigned int udmabuf0_phys_addr = 0;
        if ((fd  = open("/sys/class/udmabuf/udmabuf0/phys_addr", O_RDONLY)) != -1) {
                char attr[1024];
                read(fd, attr, 1024);
                sscanf(attr, "%x", &udmabuf0_phys_addr);
                close(fd);
        }
        printf("udmabuf0 phys addr = %x \n", udmabuf0_phys_addr);

        unsigned int udmabuf1_phys_addr = 1;
        if ((fd  = open("/sys/class/udmabuf/udmabuf1/phys_addr", O_RDONLY)) != -1) {
                char attr[1024];
                read(fd, attr, 1024);
                sscanf(attr, "%x", &udmabuf1_phys_addr);
                close(fd);
        }
        printf("udmabuf1 phys addr = %x \n", udmabuf1_phys_addr);

        requestDmaTransfer(udmabuf0_phys_addr, udmabuf1_phys_addr, BUFFERSIZE);

        // read values from udmabuf1
        printf("read values from udmabuf1\n");

        if ((fd  = open("/dev/udmabuf1", O_RDWR | O_SYNC)) != -1) {
          void* buf = mmap(NULL, BUFFERSIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
          for( size_t i = 0; i < BUFFERSIZE; ++i){
                printf("%d ",reinterpret_cast<uint8_t*>(buf)[i]);
          }
          close(fd);
        }


}
