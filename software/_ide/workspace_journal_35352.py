# 2026-05-31T17:08:12.873517500
import vitis

client = vitis.create_client()
client.set_workspace(path="zynqberry_hello")

platform = client.get_component(name="platform")
status = platform.update_hw(hw_design = "D:\zynqberrydemo\vivado\zsys_wrapper_new.xsa")

status = platform.build()

comp = client.get_component(name="hello_world")
comp.build()

comp = client.create_app_component(name="peripheral_tests",platform = "$COMPONENT_LOCATION/../platform/export/platform/platform.xpfm",domain = "standalone_ps7_cortexa9_0",template = "peripheral_tests")

status = platform.build()

comp = client.get_component(name="peripheral_tests")
comp.build()

status = platform.build()

comp = client.get_component(name="hello_world")
comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.update_hw(hw_design = "D:\zynqberrydemo\vivado\zsys_wrapper_dc.xsa")

status = platform.build()

status = platform.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

status = platform.update_hw(hw_design = "D:\zynqberrydemo\vivado\zsys_wrapper_ver2.xsa")

status = platform.update_hw(hw_design = "D:\zynqberrydemo\vivado\zsys_wrapper_v3.xsa")

status = platform.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

vitis.dispose()

