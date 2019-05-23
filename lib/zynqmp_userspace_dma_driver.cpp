#include "udmabuf.h"
#include "uio.h"

#include "xilinx_drivers/xzdma.h"

#include "zynqmp_userspace_dma_driver.h"

using namespace std;

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
      return 1;
   }
   unsigned int *p = static_cast<unsigned int *>(mmap(0, getpagesize(), PROT_READ | PROT_WRITE, MAP_SHARED, fd, FPD_DMA_REF_CTRL_ADDR_PAGE)); /* read and write flags*/
   if (p == MAP_FAILED)
   {
      printf("Can't mmap ( %s )\n", strerror(errno));
      close(fd);
      return 1;
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
};

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
   XZDma ZDma; /**<Instance of the ZDMA Device */

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
   Status = XZDma_SetMode(&ZDma, FALSE, XZDMA_NORMAL_MODE);
   if (Status != XST_SUCCESS)
   {
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
   XZDma_Transfer Data;
   Data.DstAddr = (UINTPTR)requests[0].destAddr;
   Data.DstCoherent = 1;
   Data.Pause = 0;
   Data.SrcAddr = (UINTPTR)requests[0].srcAddr;
   Data.SrcCoherent = 1;
   Data.Size = requests[0].transferLength; /* Size in bytes */

   std::cout << "starting Transfer" << std::endl;
   XZDma_Start(&ZDma, &Data, 1); /* Initiates the data transfer */

   return 0;
}

int DmaDriverImpl::startDMA()
{
   return 0;
}

DmaDriver::DMAStatus DmaDriverImpl::checkDMAStatus()
{
   return DmaDriver::XZDMA_UNAVAILABLE;
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