/**
 * @file userspace_dma_driver_test.cpp
 * @author jwoeber@riegl.com
 * @brief Small test for zynqmp dma userspace driver library
 * @version 0.1
 * @date 2019-05-23
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include <string>
#include <iostream>
#include <chrono>
#include <thread>

#include "../lib/zynqmp_userspace_dma_driver.h"


using namespace std;

int main()
{
        int fd;
        printf("**********************************\n");
        printf("*******Userspace DMA TEST*********\n");

        // open drivers
        cout << "Open drivers" << endl;
        unique_ptr<DmaDriver> driver = getDmaDriver("uio0","udmabuf0","udmabuf1");
        if( !driver ){
                cout << "Opening drivers failed" << endl;
                return -1;
        }

        cout << "Buffer Size: " << driver->getBufferSize() << endl;



        //
        // Simple test with individual transfer request
        //
        {
                printf(" ****Individual transfer request test****\n", driver->getBufferSize()/2);

                // write pattern into buffer
                printf("Writing values into first %d Bytes of the Buffer \n", driver->getBufferSize()/2);
                for (size_t i = 0; i < driver->getBufferSize()/2; ++i)
                {
                        reinterpret_cast<uint8_t *>(driver->getBuffer())[i] = i % 128;
                }
                printf("Setting values of the last %d Bytes of the Buffer to 0 \n", driver->getBufferSize()/2);
                for (size_t i = 0; i < driver->getBufferSize()/2; ++i)
                {
                        reinterpret_cast<uint8_t *>(driver->getBuffer())[i+driver->getBufferSize()/2] = i % 128;
                }

                // configure dma
                DmaDriver::transferRequest request = { 
                        .srcAddr = driver->getBufferPhysAddr(),
                        .destAddr = driver->getBufferPhysAddr() + driver->getBufferSize()/2 ,
                        .transferLength = driver->getBufferSize()/2
                };  

                if( driver->configureDMA( {request} ) != 0){
                        cout << "Configuration failed" << endl;
                        return -1;
                }

                // start dma
                auto start_time = std::chrono::high_resolution_clock::now();
                driver->startDMA();
                while(driver->checkDMAStatus() == DmaDriver::XZDMA_BUSY) {}
                auto end_time = std::chrono::high_resolution_clock::now();

                if( driver->checkDMAStatus() != DmaDriver::XZDMA_IDLE){
                        cout << "Transfer failed" << endl;
                }
                cout << "Transfer succeeded in " << (end_time - start_time)/std::chrono::microseconds(1) << " us "<< endl;

                // check if data was transfered correctly
                printf("Checking last %d Bytes of the Buffer \n", driver->getBufferSize()/2);

                for (size_t i = 0; i < driver->getBufferSize()/2; ++i)
                {
                        if(reinterpret_cast<uint8_t *>(driver->getBuffer())[i+driver->getBufferSize()/2] != i % 128 ){
                                cout << "Error found at entry # " << i << endl;
                                return -1;
                        }
                }
                cout << "Data ok" << endl;
        }


        //
        // Test with 32 transfer requests
        //

        {
                printf(" ****32 transfer requests test****\n", driver->getBufferSize()/2);
                
                // write pattern into buffer
                printf("Writing values into first %d Bytes of the Buffer \n", driver->getBufferSize()/2);

                for (size_t i = 0; i < driver->getBufferSize()/2; ++i)
                {
                        reinterpret_cast<uint8_t *>(driver->getBuffer())[i] = i % 256;
                }
                printf("Setting values of the last %d Bytes of the Buffer to 0 \n", driver->getBufferSize()/2);
                for (size_t i = 0; i < driver->getBufferSize()/2; ++i)
                {
                        reinterpret_cast<uint8_t *>(driver->getBuffer())[i+driver->getBufferSize()/2] = i % 256;
                }

                // configure dma
                vector<DmaDriver::transferRequest> requests;
                for( int i = 0; i < 32; ++i ){
                        const unsigned int requestSize = (driver->getBufferSize() / 2) / 32;
                        DmaDriver::transferRequest request = { 
                                .srcAddr = driver->getBufferPhysAddr() + i*requestSize,
                                .destAddr = driver->getBufferPhysAddr() + driver->getBufferSize()/2 + i*requestSize,
                                .transferLength = requestSize
                        };  
                        requests.push_back(request);
                }
        
                if( driver->configureDMA( requests ) != 0){
                        cout << "Configuration failed" << endl;
                        return -1;
                }

                // start dma
                auto start_time = std::chrono::high_resolution_clock::now();
                driver->startDMA();
                while(driver->checkDMAStatus() == DmaDriver::XZDMA_BUSY) {}
                auto end_time = std::chrono::high_resolution_clock::now();

                if( driver->checkDMAStatus() != DmaDriver::XZDMA_IDLE){
                        cout << "Transfer failed" << endl;
                }
                cout << "Transfer succeeded in " << (end_time - start_time)/std::chrono::microseconds(1) << " us "<< endl;

                // check if data was transfered correctly
                printf("Checking last %d Bytes of the Buffer \n", driver->getBufferSize()/2);

                for (size_t i = 0; i < driver->getBufferSize()/2; ++i)
                {
                        if(reinterpret_cast<uint8_t *>(driver->getBuffer())[i+driver->getBufferSize()/2] != i % 256 ){
                                cout << "Error found at entry # " << i << endl;
                                return -1;
                        }
                }
                cout << "Data ok" << endl;
        }
}
