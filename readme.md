# ZynqMP DMA Userspace Driver
This userspace driver allows the configuration of a [ZynqMp DMA](https://www.xilinx.com/support/documentation/user_guides/ug1085-zynq-ultrascale-trm.pdf#page=520). This is done by configuring the DMA with a source 
and destination address (physical) and a transfer length andstarting the DMA. Up to 32 requests can be configured
and started at once ( via the sg functionality of the DMA ).Furthermore a DMA Buffer can be requested from this 
library via the getBuffer() method. The physical address ofthat buffer is available via the getBufferPhysAddr()
command.

The DMA must be accessible via a uio_pdrv_genirq driver.Furthermore two buffers (one for the data buffer and
one for the sg list) must also be accessible via the udmabufdriver. The devices are opened via the getDmaDriver()
method and closed once the object is destroyed.

## Device Tree Example: 

 ```c
 udmabuf@0 {
 	compatible = "ikwzm,udmabuf-0.10.a";
 	device-name = "udmabuf0";
 	minor-number = <0>;
 	size = <0x80000>;	//512kB 
 };
 udmabuf@1 {
 	compatible = "ikwzm,udmabuf-0.10.a";
 	device-name = "udmabuf1";
 	minor-number = <1>;
 	size = <0x1000>;	//4kB ( 32 * 128 Byte SG List entries )
 };
 /delete-node/ &fpd_dma_chan8;
&amba {
	uio_fpd_dma_chan8: uio_fpd_dma_chan8@fd570000 {
		clocks = <&clk 19>, <&clk 31>;
		clock-names = "clk_main", "clk_apb";
		compatible = "generic-uio";
		reg = <0x0 0xfd570000 0x0 0x1000>;
		interrupt-parent = <&gic>;
		interrupts = <0 131 4>;
		xlnx,bus-width = <128>;
		#stream-id-cells = <1>;
		iommus = <&smmu 0x14ef>;
		power-domains = <&pd_gdma>;
};
 ```