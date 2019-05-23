
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
#include <poll.h>

#include <string>
#include <iostream>
#include <vector>
#include <memory>

class uio
{
public:
        int map(std::string const &device)
        {
                deviceName = device;

                int tfd;
                if ((tfd = open(("/sys/class/uio/" + deviceName + "/maps/map0/size").c_str(), O_RDONLY)) != -1)
                {
                        char attr[1024];
                        read(tfd, attr, 1024);
                        sscanf(attr, "%x", &size);
                        close(tfd);
                }
                else
                {
                        std::cerr << "Could not open " << ("/sys/class/uio/" + deviceName + "/map0/size") << std::endl;
                        return -1;
                }
                std::cout << "[ZynqMP Userspace DMA Driver] " << deviceName << " Size: " << size << std::endl;

                if ((tfd = open(("/sys/class/uio/" + deviceName + "/maps/map0/addr").c_str(), O_RDONLY)) != -1)
                {
                        char attr[1024];
                        read(tfd, attr, 1024);
                        sscanf(attr, "%x", &physAddr);
                        close(tfd);
                }
                else
                {
                        std::cerr << "Could not open " << ("/sys/class/uio/" + deviceName + "/map0/addr") << std::endl;
                        return -2;
                }
                std::cout << "[ZynqMP Userspace DMA Driver] "  << deviceName << " PhysAddr: " << std::hex << physAddr << std::dec << std::endl;

                fd = open(("/dev/" + device).c_str(), O_RDWR | O_SYNC);
                if (fd != -1)
                {
                        buf = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                        if (buf == MAP_FAILED)
                        {
                                std::cerr << "Can't mmap " << ("/dev/" + deviceName) << " ( " << strerror(errno) << " ) " << std::endl;
                                close(fd);
                        }
                }
                else
                {
                        std::cerr << "Could not open " << ("/dev/" + device) << std::endl;
                        return -3;
                }
                return 0;
        }

        void *getAddr()
        {
                return buf;
        };
        unsigned int getSize()
        {
                return size;
        };

        ~uio()
        {
                munmap(buf, getpagesize());
                buf = nullptr;
                close(fd);
        }

private:
        int fd;
        void *buf = nullptr;
        std::string deviceName;
        unsigned int size;
        unsigned int physAddr;
};
