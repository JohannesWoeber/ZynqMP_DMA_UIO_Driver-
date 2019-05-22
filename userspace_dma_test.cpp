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
#include <unistd.h>

#include <string>
#include <iostream>
#include <chrono>
#include <thread>

#include "xzdma.h"

const unsigned int BUFFERSIZE = 1024;


static int ActivateFpdDmaClk(){
        const unsigned int FPD_DMA_REF_CTRL_ADDR = 0xFD1A00B8;
        const unsigned int CLKACT_MASK = 0x1000000;

        const unsigned int FPD_DMA_REF_CTRL_ADDR_PAGE = (FPD_DMA_REF_CTRL_ADDR & ~(sysconf(_SC_PAGE_SIZE) - 1));
        const unsigned int FPD_DMA_REF_CTRL_ADDR_PAGE_OFFSET = FPD_DMA_REF_CTRL_ADDR - FPD_DMA_REF_CTRL_ADDR_PAGE;

        int fd = open("/dev/mem", O_RDWR); /* read and write flags*/
        if(fd < 0)
        {
                printf("Can't open /dev/mem\n");
                return 1;
        }
        unsigned int * p  = static_cast<unsigned int *>(mmap(0, getpagesize() , PROT_READ | PROT_WRITE, MAP_SHARED, fd, FPD_DMA_REF_CTRL_ADDR_PAGE)); /* read and write flags*/
        if(p == MAP_FAILED)
        {
                printf("Can't mmap ( %s )\n",strerror(errno));
                close(fd);
                return 1;
        }
        *(p + FPD_DMA_REF_CTRL_ADDR_PAGE_OFFSET/4) |= 0x1000000;
        munmap(p, getpagesize());
        close(fd);
        return 0;
}

class UserspaceZynqMpDMA{
    public:
    UserspaceZynqMpDMA(std::string const& cdevice) : cdevice(cdevice) {}
    int mmapRegs(){
        if( ActivateFpdDmaClk() != 0){
                return -3;
        }

        cdevice_fd = open(cdevice.c_str(), O_RDWR);
        if (cdevice_fd < 0){
            std::cerr << "opening " << cdevice << " failed " << std::endl;
            return -1;

        }
               
        ptrRegs = (uintptr_t)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, cdevice_fd, 0);
        if (ptrRegs == (uintptr_t)MAP_FAILED) {
                std::cerr << "UIO_mmap construction failed" << std::endl;
                return -2;
        }   
        return 0;
    }

    int startSimpleDMATransfer(unsigned long int srcAddr, unsigned long int destAddr, unsigned long int transferLength){
        XZDma_Config Config =
	    {
	    	0,
	    	ptrRegs,
	    	0 /* 0 = GDMA, 1 = ADMA */
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
        std::cout << "ZDma Seftest succeeded" << std::endl;

        /* ZDMA has set in simple transfer of Normal mode */
	    Status = XZDma_SetMode(&ZDma, FALSE, XZDMA_NORMAL_MODE);
	    if (Status != XST_SUCCESS) {
		    return XST_FAILURE;
	    }

        /* Configuration settings */
        XZDma_DataConfig Configure; /* Configuration values */
	    Configure.OverFetch = 1;
	    Configure.SrcIssue = 0x1F;
	    Configure.SrcBurstType = XZDMA_INCR_BURST;
	    Configure.SrcBurstLen = 0xF;
	    Configure.DstBurstType = XZDMA_INCR_BURST;
	    Configure.DstBurstLen = 0xF;
	    Configure.SrcCache = 0x2;
	    Configure.DstCache = 0x2;
	    if (/*Config->IsCacheCoherent*/ false) {
	    	Configure.SrcCache = 0xF;
	    	Configure.DstCache = 0xF;
	    }
	    Configure.SrcQos = 0;
	    Configure.DstQos = 0;

	    XZDma_SetChDataConfig(&ZDma, &Configure);
	    /*
	     * Transfer elements
	     */
        XZDma_Transfer Data;
	    Data.DstAddr = (UINTPTR)destAddr;
	    Data.DstCoherent = 1;
	    Data.Pause = 0;
	    Data.SrcAddr = (UINTPTR)srcAddr;
	    Data.SrcCoherent = 1;
	    Data.Size = transferLength; /* Size in bytes */

        std::cout << "starting Transfer" << std::endl;
	    XZDma_Start(&ZDma, &Data, 1); /* Initiates the data transfer */
        std::this_thread::sleep_for(std::chrono::seconds(3));    
        return 0;
    }

    ~UserspaceZynqMpDMA(){
        close(cdevice_fd);
    }
    private:
        const std::string cdevice;
        std::uintptr_t ptrRegs;
        int cdevice_fd;
};

UserspaceZynqMpDMA LpdDmaChan1("/dev/uio1");


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
