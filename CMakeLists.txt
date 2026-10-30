# origin-pack: STMicroelectronics::w25n01gvxx_part_drivers@2.2.0
# file-format: 1.0.0
project(STMicroelectronics_w25n01gvxx_part_drivers_2_2_0)
cmake_minimum_required(VERSION 3.30)
add_library(STMicroelectronics_w25n01gvxx_part_drivers_2_2_0 INTERFACE)

# List of all CMSIS properties that influence conditions for this package


# Device specific defined symbols








# Enable all components in this package
if(CMSIS_ENTIRE_STMicroelectronics_w25n01gvxx_part_drivers_2_2_0)
  list(APPEND CMSIS_COMPONENTS_LIST "Cvendor:STMicroelectronics#Cclass:Board Part#Cgroup:Memory#Csub:W25N01GVXX Core#Cversion:1.2.0")
endif()

# All conditions used by this package

# condition: STMicroelectronics.w25n01gvxx_part_drivers.2.2.0:W25N01GVXX CFG Condition
# description: STMicroelectronics configuration for W25N01GVXX part driver
set(STMicroelectronics.w25n01gvxx_part_drivers.2.2.0_W25N01GVXX_CFG_Condition "1")
message(DEBUG "CMSIS condition STMicroelectronics.w25n01gvxx_part_drivers.2.2.0_W25N01GVXX_CFG_Condition enabled")


# condition: STMicroelectronics.w25n01gvxx_part_drivers.2.2.0:W25N01GVXX Core Condition
# description: STMicroelectronics W25N01GVXX Core Driver
set(STMicroelectronics.w25n01gvxx_part_drivers.2.2.0_W25N01GVXX_Core_Condition "$<NOT:$<STREQUAL:$<FILTER:${CMSIS_COMPONENTS_LIST},INCLUDE,.*Cclass:Device#.*Cgroup:STM32 HAL#.*Csub:SPI(#.*|$)>,>>")
message(DEBUG "CMSIS condition STMicroelectronics.w25n01gvxx_part_drivers.2.2.0_W25N01GVXX_Core_Condition enabled")

# Files and components in this package
if("Cvendor:STMicroelectronics#Cclass:Board Part#Cgroup:Memory#Csub:W25N01GVXX Core#Cversion:1.2.0" IN_LIST CMSIS_COMPONENTS_LIST)  # Driver for W25N01GVXX FLASH Memory
  message(DEBUG "Using component Board_Part_Memory_W25N01GVXX_Core_1_2_0")
  target_compile_definitions(STMicroelectronics_w25n01gvxx_part_drivers_2_2_0 INTERFACE "$<${STMicroelectronics.w25n01gvxx_part_drivers.2.2.0_W25N01GVXX_Core_Condition}:-DCMSIS_USE_Board_Part_Memory_W25N01GVXX_Core_1_2_0=1>")
  target_include_directories(STMicroelectronics_w25n01gvxx_part_drivers_2_2_0 INTERFACE "$<${STMicroelectronics.w25n01gvxx_part_drivers.2.2.0_W25N01GVXX_Core_Condition}:${CMAKE_CURRENT_LIST_DIR}/.>")
  target_sources(STMicroelectronics_w25n01gvxx_part_drivers_2_2_0 INTERFACE "$<${STMicroelectronics.w25n01gvxx_part_drivers.2.2.0_W25N01GVXX_Core_Condition}:${CMAKE_CURRENT_LIST_DIR}/w25n01gvxx.c>")
endif()

