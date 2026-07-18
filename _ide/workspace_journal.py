# 2026-07-02T22:45:22.883127700
import vitis

client = vitis.create_client()
client.set_workspace(path="zynqberry_hello")

vitis.dispose()

