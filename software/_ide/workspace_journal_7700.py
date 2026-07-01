# 2026-06-29T18:33:16.664129800
import vitis

client = vitis.create_client()
client.set_workspace(path="zynqberry_hello")

platform = client.get_component(name="platform")
status = platform.build()

comp = client.get_component(name="hello_world")
comp.build()

