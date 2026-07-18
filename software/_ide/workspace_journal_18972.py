# 2026-07-03T15:16:45.856250
import vitis

client = vitis.create_client()
client.set_workspace(path="software")

platform = client.get_component(name="platform")
status = platform.update_hw(hw_design = "D:\zynqberry_bits\vivado\zynqberry_clean\zsys_wrapper.xsa")

status = platform.build()

vitis.dispose()

