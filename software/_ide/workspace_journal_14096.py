# 2026-06-29T16:36:21.718636100
import vitis

client = vitis.create_client()
client.set_workspace(path="zynqberry_hello")

vitis.dispose()

