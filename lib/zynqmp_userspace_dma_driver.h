/**
 * @file zynqmp_userspace_dma_driver.h
 * @author jwoeber@riegl.com
 * @brief zynqmp dma userspace driver
 * @version 0.1
 * @date 2019-05-23
 * 
 * @copyright Copyright (c) 2019
 * 
 * This userspace driver allows the configuration of a ZynqMp DMA. This is done by configuring the DMA with a source 
 * and destination address (physical) and a transfer length and starting the DMA. Up to 32 requests can be configured
 * and started at once ( via the sg functionality of the DMA ). Furthermore a DMA Buffer can be requested from this 
 * library via the getBuffer() method. The physical address of that buffer is available via the getBufferPhysAddr()
 * command.
 * 
 * The DMA must be accessible via a uio_pdrv_genirq driver. Furthermore two buffers (one for the data buffer and
 * one for the sg list) must also be accessible via the udmabuf driver. The devices are opened via the getDmaDriver()
 * method and closed once the object is destroyed.
 * 
 * Device Tree Example: 
 * udmabuf@0 {
 *		compatible = "ikwzm,udmabuf-0.10.a";
 *		device-name = "udmabuf0";
 *		minor-number = <0>;
 *		size = <0x80000>;	//512kB 
 *	};
 *	udmabuf@1 {
 *		compatible = "ikwzm,udmabuf-0.10.a";
 *		device-name = "udmabuf1";
 *		minor-number = <1>;
 *		size = <0x1000>;	//4kB ( 32 * 128 Byte SG List entries )
 *	};
 * /delete-node/ &fpd_dma_chan8;
 *  &amba {
 *		uio_fpd_dma_chan8: uio_fpd_dma_chan8@fd570000 {
 *			clocks = <&clk 19>, <&clk 31>;
 *			clock-names = "clk_main", "clk_apb";
 *			compatible = "generic-uio";
 *			reg = <0x0 0xfd570000 0x0 0x1000>;
 *			interrupt-parent = <&gic>;
 *			interrupts = <0 131 4>;
 *			xlnx,bus-width = <128>;
 *			#stream-id-cells = <1>;
 *			iommus = <&smmu 0x14ef>;
 *			power-domains = <&pd_gdma>;
 *	 };
 *
 * 
 */

#include <string>
#include <vector>
#include <memory>

class DmaDriver{
public:
   virtual ~DmaDriver(){}

   /**
    * @brief Get the pointer to the virtual address of the DMA buffer
    * @return void* 
    */
   virtual void* getBuffer() = 0;
   /**
    * @brief Get physical address of the DMA buffer
    * @return void* 
    */
   virtual unsigned int getBufferPhysAddr() = 0;
   /**
    * @brief Get the size of the DMA buffer
    * @return void* 
    */
   virtual unsigned int getBufferSize() = 0;


   typedef struct {
      unsigned int srcAddr;         //physical
      unsigned int destAddr;        //physical
      unsigned int transferLength;
   } transferRequest;

   /**
    * @brief configure the dma 
    * 
    * @param requests list of up to 32 transfer request the dma should process
    * @return 0 if success
    */
   virtual int configureDMA(std::vector<transferRequest> requests) = 0;

   /**
    * @brief start the dma
    * 
    * @return 0 if success
    */
   virtual int startDMA() = 0;

   typedef enum : unsigned int {
      XZDMA_IDLE = 0,		      /**< ZDMA is in Idle state */
	   XZDMA_PAUSE = 1,		      /**< Paused state */
	   XZDMA_BUSY = 2,		      /**< Busy state */
      XZDMA_ERROR = 3,		      /**< Busy state */
   } DMAStatus;
   /**
    * @brief check the state of the dma (only possible after configuration)
    * 
    * @return DMAStatus 
    */
   virtual DMAStatus checkDMAStatus() = 0;
};

/**
 * @brief get a instance of the dma driver 
 * 
 * @param uioDmaDriverDevice name of the uio dma device (f.e. "uio0") must be uio_pdrv_genirq driver device
 * @param udmaDataBufDevice name of dma buffer device (f.e. "udmabuf0") must be udmabuf driver device 
 * @param udmaCfgBufDevice name of dma cfg buffer device (f.e. "udmabuf1") must be udmabuf driver device 
 * @return std::unique_ptr<DmaDriver> 
 */
std::unique_ptr<DmaDriver> getDmaDriver(  std::string const& uioDmaDriverDevice = "uio0", 
                                          std::string const& udmaDataBufDevice = "udmabuf0",
                                          std::string const& udmaCfgBufDevice = "udmabuf1" );