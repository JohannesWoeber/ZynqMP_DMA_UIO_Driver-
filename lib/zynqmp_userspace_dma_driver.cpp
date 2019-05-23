/**
 * @file zynqmp_userspace_dma_driver.cpp
 * @author jwoeber@riegl.com
 * @brief zynqmp dma userspace driver
 * @version 0.1
 * @date 2019-05-23
 * 
 * @copyright Copyright (c) 2019
 * 
 */

#include "udmabuf.h"
#include "uio.h"

#include "xilinx_drivers/xzdma.h"

#include "zynqmp_userspace_dma_driver.h"

using namespace std;

/**
 * @brief activate the clock for the FPD DMAs
 * 
 * @return 0 if success 
 */
static int ActivateFpdDmaClk()
{
   const unsigned int FPD_DMA_REF_CTRL_ADDR = 0xFD1A00B8;
   const unsigned int CLKACT_MASK = 0x1000000;

   const unsigned int FPD_DMA_REF_CTRL_ADDR_PAGE = (FPD_DMA_REF_CTRL_ADDR & ~(sysconf(_SC_PAGE_SIZE) - 1));
   const unsigned int FPD_DMA_REF_CTRL_ADDR_PAGE_OFFSET = FPD_DMA_REF_CTRL_ADDR - FPD_DMA_REF_CTRL_ADDR_PAGE;

   int fd = open("/dev/mem", O_RDWR); /* read and write flags*/
   if (fd < 0)
   {
      printf("Can't open /dev/mem\n");
      return -1;
   }
   unsigned int *p = static_cast<unsigned int *>(mmap(0, getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, FPD_DMA_REF_CTRL_ADDR_PAGE)); /* read and write flags*/
   if (p == MAP_FAILED)
   {
      printf("Can't mmap ( %s )\n", strerror(errno));
      close(fd);
      return -2;
   }
   *(p + FPD_DMA_REF_CTRL_ADDR_PAGE_OFFSET / 4) |= 0x1000000;
   munmap(p, getpagesize());
   close(fd);
   return 0;
}

class DmaDriverImpl : public DmaDriver
{
public:
   virtual ~DmaDriverImpl() {}

   int map(string const &uioDmaDriverDevice,
           string const &udmaDataBufDevice,
           string const &udmaCfgBufDevice);

   virtual void *getBuffer() override;
   virtual unsigned int getBufferPhysAddr() override;
   virtual unsigned int getBufferSize() override;

   virtual int configureDMA(std::vector<transferRequest> requests) override;
   virtual int startDMA() override;

   virtual DmaDriver::DMAStatus checkDMAStatus() override;

private:
   udmabuf dataBuf;
   udmabuf cfgBuf;
   uio uio_dma;
   XZDma ZDma; /**<Instance of the ZDMA Device */
};

/**
 * @brief open and map the drivers 
 * 
 * @param uioDmaDriverDevice name of the uio dma device (f.e. "uio0") must be uio_pdrv_genirq driver device
 * @param udmaDataBufDevice name of dma buffer device (f.e. "udmabuf0") must be udmabuf driver device 
 * @param udmaCfgBufDevice name of dma cfg buffer device (f.e. "udmabuf1") must be udmabuf driver device 
 * @return int 
 */
int DmaDriverImpl::map(string const &uioDmaDriverDevice,
                       string const &udmaDataBufDevice,
                       string const &udmaCfgBufDevice)
{
   cout << "[ZynqMP Userspace DMA Driver] mapping data buffer ( " << udmaDataBufDevice << " )" << endl;
   if (dataBuf.map(udmaDataBufDevice) != 0)
   {
      std::cerr << "[ZynqMP Userspace DMA Driver] mapping data buffer ( " << udmaDataBufDevice << " ) failed" << std::endl;
      return -1;
   }
   cout << "[ZynqMP Userspace DMA Driver] mapping cfg buffer ( " << udmaCfgBufDevice << " )" << endl;
   if (cfgBuf.map(udmaCfgBufDevice) != 0)
   {
      std::cerr << "[ZynqMP Userspace DMA Driver] mapping cfg buffer ( " << udmaCfgBufDevice << " ) failed" << std::endl;
      return -1;
   }
   cout << "[ZynqMP Userspace DMA Driver] mapping uio device( " << uioDmaDriverDevice << " )" << endl;
   if (uio_dma.map(uioDmaDriverDevice) != 0)
   {
      std::cerr << "[ZynqMP Userspace DMA Driver] mapping uio device( " << uioDmaDriverDevice << " ) failed" << std::endl;
      return -1;
   }
   return 0;
}

void *DmaDriverImpl::getBuffer()
{
   return dataBuf.getAddr();
}

unsigned int DmaDriverImpl::getBufferPhysAddr()
{
   return dataBuf.physAddr();
}

unsigned int DmaDriverImpl::getBufferSize()
{
   return dataBuf.getSize();
}

int DmaDriverImpl::configureDMA(std::vector<transferRequest> requests)
{
   if(requests.size() > 32){
      cout << "max. Nbr. of transfer Requests = 32" << endl;
      return -10;
   }

   if (ActivateFpdDmaClk() != 0)
   {
      cout << "Could not activate Clock" << endl;
      return -1;
   }
   XZDma_Config Config =
      {
         0,
         (uintptr_t)uio_dma.getAddr(),
         0 /* 0 = GDMA, 1 = ADMA */
      };

   int Status;

   /*
	     * Initialize the ZDMA driver so that it's ready to use.
	     * Look up the configuration in the config table,
	     * then initialize it.
	     */

   std::cout << "Initializing ZDma" << std::endl;
   Status = XZDma_CfgInitialize(&ZDma, &Config, Config.BaseAddress);
   if (Status != XST_SUCCESS)
   {
      std::cout << "Initializing ZDma failed" << std::endl;
      return XST_FAILURE;
   }

   std::cout << "ZDma Seftest" << std::endl;
   /*
	     * Performs the self-test to check hardware build.
	     */
   Status = XZDma_SelfTest(&ZDma);
   if (Status != XST_SUCCESS)
   {
      std::cout << "ZDma Seftest failed" << std::endl;
      return XST_FAILURE;
   }
   std::cout << "ZDma Seftest succeeded" << std::endl;

   /* ZDMA has set in simple transfer of Normal mode */
   Status = XZDma_SetMode(&ZDma, TRUE, XZDMA_NORMAL_MODE);
   if (Status != XST_SUCCESS)
   {
      return XST_FAILURE;
   }

	/* Allocated memory starting address should be 64 bit aligned */
	XZDma_CreateBDList(&ZDma, XZDMA_LINEAR, (UINTPTR)cfgBuf.getAddr(), (UINTPTR)cfgBuf.physAddr() , 4096);

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
   if (/*Config->IsCacheCoherent*/ false)
   {
      Configure.SrcCache = 0xF;
      Configure.DstCache = 0xF;
   }
   Configure.SrcQos = 0;
   Configure.DstQos = 0;

   XZDma_SetChDataConfig(&ZDma, &Configure);
   /*
	   * Transfer elements
	   */
   XZDma_Transfer Data[32];
   for( int i = 0; i < requests.size(); ++i){
      Data[i].DstAddr = (UINTPTR)requests[i].destAddr;
      Data[i].DstCoherent = 1;
      Data[i].Pause = 0;
      Data[i].SrcAddr = (UINTPTR)requests[i].srcAddr;
      Data[i].SrcCoherent = 1;
      Data[i].Size = requests[i].transferLength; /* Size in bytes */
   }
  

   std::cout << "starting Transfer" << std::endl;
   XZDma_Start(&ZDma, Data, requests.size()); /* Initiates the data transfer */

   return 0;
}

int DmaDriverImpl::startDMA()
{
   XZDma_Enable(&ZDma);
   return 0;
}

DmaDriver::DMAStatus DmaDriverImpl::checkDMAStatus()
{
   return static_cast<DmaDriver::DMAStatus>(XZDma_ReadReg(ZDma.Config.BaseAddress,(XZDMA_CH_STS_OFFSET)) & (XZDMA_STS_ALL_MASK));
}

unique_ptr<DmaDriver> getDmaDriver(string const &uioDmaDriverDevice,
                                   string const &udmaDataBufDevice,
                                   string const &udmaCfgBufDevice)
{
   unique_ptr<DmaDriverImpl> driver(new DmaDriverImpl());
   if (driver->map(uioDmaDriverDevice, udmaDataBufDevice, udmaCfgBufDevice) != 0)
   {
      return nullptr;
   }
   return driver;
}