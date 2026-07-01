# 2026-06-23T04:16:54.079396100
import vitis

client = vitis.create_client()
client.set_workspace(path="zynqberry_hello")

platform = client.get_component(name="platform")
status = platform.update_hw(hw_design = "D:\zynqberry_spi1_test\zynqberry_spi1_test\zsys_wrapper_spi1.xsa")

status = platform.build()

vitis.dispose()

vitis.dispose()

