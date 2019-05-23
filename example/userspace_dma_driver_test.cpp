#include <string>
#include <iostream>
#include <chrono>

#include "../lib/zynqmp_userspace_dma_driver.h"


using namespace std;


int main()
{
        int fd;
        printf("**********************************\n");
        printf("*******Userspace DMA TEST*********\n");

        cout << "Open drivers" << endl;
        unique_ptr<DmaDriver> driver = getDmaDriver("uio0","udmabuf0","udmabuf1");
        if( !driver ){
                cout << "Opening drivers failed" << endl;
                return -1;
        }

        cout << "Buffer Size: " << driver->getBufferSize() << endl;

        // write values into udmabuf0
        printf("Writing values into first %d Bytes of the Buffer \n", driver->getBufferSize()/2);

        for (size_t i = 0; i < driver->getBufferSize()/2; ++i)
        {
                reinterpret_cast<uint8_t *>(driver->getBuffer())[i] = i % 256;
        }

        DmaDriver::transferRequest request = { 
                .srcAddr = driver->getBufferPhysAddr(),
                .destAddr = driver->getBufferPhysAddr() + driver->getBufferSize()/2,
                .transferLength = driver->getBufferSize()/2
        };
        driver->configureDMA( {request} );
        auto start_time = std::chrono::high_resolution_clock::now();


        driver->startDMA();

        while(driver->checkDMAStatus() == DmaDriver::XZDMA_BUSY) {}
        auto end_time = std::chrono::high_resolution_clock::now();

        if( driver->checkDMAStatus() != DmaDriver::XZDMA_IDLE){
                cout << "Transfer failed" << endl;
        }
        cout << "Transfer succeeded in " << (end_time - start_time)/std::chrono::microseconds(1) << " us "<< endl;

        // read values from udmabuf1
        printf("Reading from last %d Bytes of the Buffer \n", driver->getBufferSize()/2);

        for (size_t i = 0; i < driver->getBufferSize()/2; ++i)
        {
                printf("%d ", reinterpret_cast<uint8_t *>(driver->getBuffer())[i+driver->getBufferSize()/2]);
        }

}