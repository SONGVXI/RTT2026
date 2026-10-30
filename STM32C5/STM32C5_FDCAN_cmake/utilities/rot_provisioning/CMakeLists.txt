# origin-pack: STMicroelectronics::rot_provisioning@2.0.1
# file-format: 1.0.0
project(STMicroelectronics_rot_provisioning_2_0_1)
cmake_minimum_required(VERSION 3.20)
add_library(STMicroelectronics_rot_provisioning_2_0_1 INTERFACE)

# List of all CMSIS properties that influence conditions for this package


# Device specific defined symbols








# Enable all components in this package
if(CMSIS_ENTIRE_STMicroelectronics_rot_provisioning_2_0_1)
  list(APPEND CMSIS_COMPONENTS_LIST "Cvendor:STMicroelectronics#Cclass:Security#Cgroup:RoT_Provisioning#Csub:Core#Cversion:1.0.1")
  list(APPEND CMSIS_COMPONENTS_LIST "Cvendor:STMicroelectronics#Cclass:Security#Cgroup:RoT_Provisioning#Csub:Helper#Cversion:1.0.1")
  list(APPEND CMSIS_COMPONENTS_LIST "Cvendor:STMicroelectronics#Cclass:Security#Cgroup:RoT_Provisioning#Csub:OEMiRoT#Cversion:1.0.1")
endif()

# All conditions used by this package


# Files and components in this package
if("Cvendor:STMicroelectronics#Cclass:Security#Cgroup:RoT_Provisioning#Csub:Core#Cversion:1.0.0" IN_LIST CMSIS_COMPONENTS_LIST)  # RoT Provisioning global documentation
  message(DEBUG "Using component Security_RoT_Provisioning_Core_1_0_1")
  target_compile_definitions(STMicroelectronics_rot_provisioning_2_0_1 INTERFACE "-DCMSIS_USE_Security_RoT_Provisioning_Core_1_0_1=1")
endif()

if("Cvendor:STMicroelectronics#Cclass:Security#Cgroup:RoT_Provisioning#Csub:Helper#Cversion:1.0.0" IN_LIST CMSIS_COMPONENTS_LIST)  # RoT Provisioning Helper component
  message(DEBUG "Using component Security_RoT_Provisioning_Helper_1_0_1")
  target_compile_definitions(STMicroelectronics_rot_provisioning_2_0_1 INTERFACE "-DCMSIS_USE_Security_RoT_Provisioning_Helper_1_0_1=1")
endif()

if("Cvendor:STMicroelectronics#Cclass:Security#Cgroup:RoT_Provisioning#Csub:OEMiRoT#Cversion:1.0.0" IN_LIST CMSIS_COMPONENTS_LIST)  # RoT Provisioning Helper component
  message(DEBUG "Using component Security_RoT_Provisioning_OEMiRoT_1_0_1")
  target_compile_definitions(STMicroelectronics_rot_provisioning_2_0_1 INTERFACE "-DCMSIS_USE_Security_RoT_Provisioning_OEMiRoT_1_0_1=1")
endif()

