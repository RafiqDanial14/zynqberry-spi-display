# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "D:\\Vitis_Projects\\zynqberry_hello\\platform\\zynq_fsbl\\zynq_fsbl_bsp\\include\\diskio.h"
  "D:\\Vitis_Projects\\zynqberry_hello\\platform\\zynq_fsbl\\zynq_fsbl_bsp\\include\\ff.h"
  "D:\\Vitis_Projects\\zynqberry_hello\\platform\\zynq_fsbl\\zynq_fsbl_bsp\\include\\ffconf.h"
  "D:\\Vitis_Projects\\zynqberry_hello\\platform\\zynq_fsbl\\zynq_fsbl_bsp\\include\\sleep.h"
  "D:\\Vitis_Projects\\zynqberry_hello\\platform\\zynq_fsbl\\zynq_fsbl_bsp\\include\\xilffs.h"
  "D:\\Vitis_Projects\\zynqberry_hello\\platform\\zynq_fsbl\\zynq_fsbl_bsp\\include\\xilffs_config.h"
  "D:\\Vitis_Projects\\zynqberry_hello\\platform\\zynq_fsbl\\zynq_fsbl_bsp\\include\\xilrsa.h"
  "D:\\Vitis_Projects\\zynqberry_hello\\platform\\zynq_fsbl\\zynq_fsbl_bsp\\include\\xiltimer.h"
  "D:\\Vitis_Projects\\zynqberry_hello\\platform\\zynq_fsbl\\zynq_fsbl_bsp\\include\\xtimer_config.h"
  "D:\\Vitis_Projects\\zynqberry_hello\\platform\\zynq_fsbl\\zynq_fsbl_bsp\\lib\\libxilffs.a"
  "D:\\Vitis_Projects\\zynqberry_hello\\platform\\zynq_fsbl\\zynq_fsbl_bsp\\lib\\libxilrsa.a"
  "D:\\Vitis_Projects\\zynqberry_hello\\platform\\zynq_fsbl\\zynq_fsbl_bsp\\lib\\libxiltimer.a"
  )
endif()
