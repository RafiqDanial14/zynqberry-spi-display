# 2026-05-27T19:43:29.476616900
import vitis

client = vitis.create_client()
client.set_workspace(path="zynqberry_hello")

status = client.add_platform_repos(platform=["d:\zynqberrydemo\vivado"])

status = client.add_platform_repos(platform=["d:\zynqberrydemo\vivado"])

status = client.add_platform_repos(platform=["d:\zynqberrydemo\vivado"])

platform = client.create_platform_component(name = "platform",hw_design = "D:\zynqberrydemo\vivado\zsys_wrapper.xsa",os = "standalone",cpu = "ps7_cortexa9_0",domain_name = "standalone_ps7_cortexa9_0")

comp = client.create_app_component(name="hello_world",platform = "$COMPONENT_LOCATION/../platform/export/platform/platform.xpfm",domain = "standalone_ps7_cortexa9_0",template = "hello_world")

platform = client.get_component(name="platform")
status = platform.build()

comp = client.get_component(name="hello_world")
comp.build()

status = platform.update_hw(hw_design = "D:\zynqberrydemo\vivado\zsys_wrapper.xsa")

status = platform.build()

status = platform.build()

comp.build()

status = platform.build()

comp.build()

