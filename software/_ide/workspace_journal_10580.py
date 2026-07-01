# 2026-06-18T04:39:37.817010800
import vitis

client = vitis.create_client()
client.set_workspace(path="zynqberry_hello")

vitis.dispose()

