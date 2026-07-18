# 2026-07-01T10:53:01.159036100
import vitis

client = vitis.create_client()
client.set_workspace(path="zynqberry_hello")

platform = client.get_component(name="platform")
status = platform.update_hw(hw_design = "D:\zynqberry_bits\vivado\zsys_wrapper.xsa")

status = platform.update_hw(hw_design = "D:\zynqberry_bits\vivado\zynqberry_clean\zsys_wrapper.xsa")

status = platform.build()

status = platform.build()

comp = client.get_component(name="hello_world")
comp.build()

status = platform.build()

comp.build()

vitis.dispose()

vitis.dispose()

