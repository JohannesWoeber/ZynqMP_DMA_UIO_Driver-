#include <string>
#include <vector>
#include <memory>

class DmaDriver{
public:
   virtual ~DmaDriver(){}

   virtual void* getBuffer() = 0;
   virtual unsigned int getBufferPhysAddr() = 0;
   virtual unsigned int getBufferSize() = 0;

   typedef struct {
      unsigned int srcAddr;
      unsigned int destAddr;
      unsigned int transferLength;
   } transferRequest;

   virtual int configureDMA(std::vector<transferRequest> requests) = 0;
   virtual int startDMA() = 0;

   typedef enum : unsigned int {
      XZDMA_IDLE = 0,		      /**< ZDMA is in Idle state */
	   XZDMA_PAUSE = 1,		   /**< Paused state */
	   XZDMA_BUSY = 2,		      /**< Busy state */
      XZDMA_ERROR = 3,		      /**< Busy state */
   } DMAStatus;
   virtual DMAStatus checkDMAStatus() = 0;
};

std::unique_ptr<DmaDriver> getDmaDriver(  std::string const& uioDmaDriverDevice = "uio0", 
                                          std::string const& udmaDataBufDevice = "udmabuf0",
                                          std::string const& udmaCfgBufDevice = "udmabuf1" );