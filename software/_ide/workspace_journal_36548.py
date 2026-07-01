# 2026-06-11T22:26:05.793351900
import vitis

client = vitis.create_client()
client.set_workspace(path="zynqberry_hello")

vitis.dispose()

