# 2026-08-18T14:28:32.525130700
import vitis

client = vitis.create_client()
client.set_workspace(path="software")

platform = client.get_component(name="platform")
status = platform.build()

comp = client.get_component(name="hello_world")
comp.build()

vitis.dispose()

vitis.dispose()

