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
               
        ptrRegs = (uintptr_t)mmap(NULL, 16384, PROT_READ | PROT_WRITE, MAP_SHARED, cdevice_fd, 0);
        if (ptrRegs == (uintptr_t)MAP_FAILED) {
                std::cerr << "UIO_mmap construction failed" << std::endl;
                return -2;
        }   
    }

    int startSimpleDMATransfer(unsigned long int srcAddr, unsigned long int destAddr, unsigned long int transferLength){
        
        unsigned int data = static_cast<unsigned int>(*((uint8_t *) ptrRegs));
        std::cout << "XZDMA_ERR_CTRL: " << data << std::endl;
        std::cout << "XZDMA_CH_CTRL0_OFFSET: " << XZDma_ReadReg(ptrRegs,XZDMA_CH_CTRL0_OFFSET) << std::endl;

        *((uint8_t *) ptrRegs + XZDMA_CH_CTRL0_OFFSET) = 1; 
        XZDma_WriteReg(ptrRegs,XZDMA_CH_CTRL0_OFFSET,1);

        std::cout << "XZDMA_CH_CTRL0_OFFSET: " << XZDma_ReadReg(ptrRegs,XZDMA_CH_CTRL0_OFFSET) << std::endl;

        *((uint8_t *) ptrRegs + XZDMA_CH_CTRL0_OFFSET) = 0; 
        XZDma_WriteReg(ptrRegs,XZDMA_CH_CTRL0_OFFSET,0); 
        
        std::cout << "XZDMA_CH_CTRL0_OFFSET: " << XZDma_ReadReg(ptrRegs,XZDMA_CH_CTRL0_OFFSET) << std::endl;

        XZDma_Config Config =
	    {
	    	0,
	    	ptrRegs,
	    	1 /* 0 = GDMA, 1 = ADMA */
	    };

        int Status;
        XZDma ZDma;		/**<Instance of the ZDMA Device */



        /*
	     * Initialize the ZDMA driver so that it's ready to use.
	     * Look up the configuration in the config table,
	     * then initialize it.
	     */
	
        std::cout << "Initializing ZDma" << std::endl;
        Status = XZDma_CfgInitialize(&ZDma, &Config, Config.BaseAddress);
	    if (Status != XST_SUCCESS) {
            std::cout << "Initializing ZDma failed" << std::endl;
	    	return XST_FAILURE;
	    }

        std::cout << "ZDma Seftest" << std::endl;
	    /*
	     * Performs the self-test to check hardware build.
	     */
	    Status = XZDma_SelfTest(&ZDma);
	    if (Status != XST_SUCCESS) {
            std::cout << "ZDma Seftest failed" << std::endl;
	    	return XST_FAILURE;
	    }

        std::cout << "XZDma_ReadReg: " << XZDma_ReadReg(ptrRegs, XZDMA_CH_STS_OFFSET) << std::endl;
    }

    ~UserspaceZynqMpDMA(){
        close(cdevice_fd);
    }
    private:
        const std::string cdevice;
        std::uintptr_t ptrRegs;
        int cdevice_fd;
};

UserspaceZynqMpDMA LpdDmaChan1("/dev/uio0");


int requestDmaTransfer(unsigned long int srcAddr, unsigned long int destAddr, unsigned long int transferLength){
    if( LpdDmaChan1.mmapRegs() == 0 ){
        LpdDmaChan1.startSimpleDMATransfer(srcAddr,destAddr,transferLength);
    }


    
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
