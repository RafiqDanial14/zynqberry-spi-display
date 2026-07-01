# 2026-06-25T22:39:12.069463400
import vitis

client = vitis.create_client()
client.set_workspace(path="zynqberry_hello")

platform = client.get_component(name="platform")
status = platform.build()

comp = client.get_component(name="hello_world")
comp.build()

vitis.dispose()

