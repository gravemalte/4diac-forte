# *******************************************************************************
# *******************************************************************************
# Copyright (c) 2025 Malte Grave, LIT CPS Johannes Kepler Universität,
#                                 github.com/gravemalte
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0.
#
# SPDX-License-Identifier: EPL-2.0
# 
# Contributors:
#     Malte Grave - initial API and implementation and/or initial documentation
# *******************************************************************************/
# *******************************************************************************/
#############################################################################
# CAN Com Layer (currently only usable on Linux)
#############################################################################

function(CHECK_IF_KERNEL_MODULE_IS_LOADED)
  execute_process(
    COMMAND lsmod
    COMMAND grep -wE "^(can|can_raw|vcan|can_dev)"
    COMMAND awk "{print $1}"
    COMMAND tr "\n" " "
    OUTPUT_VARIABLE CAN_MODULES_FOUND
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  message(STATUS "Found CAN modules: ${CAN_MODULES_FOUND}")
    if(NOT "${CAN_MODULES_FOUND}" MATCHES "can")
        message(FATAL_ERROR "can module is missing!")
    endif()
    
    if(NOT "${CAN_MODULES_FOUND}" MATCHES "vcan")
        message(FATAL_ERROR "vcan module is missing!")
    endif()

    if(NOT "${CAN_MODULES_FOUND}" MATCHES "can_raw")
        message(FATAL_ERROR "can_raw module is missing!")
    endif()

    if(NOT "${CAN_MODULES_FOUND}" MATCHES "can_dev")
        message(FATAL_ERROR "can_dev module is missing!")
    endif()
endfunction()


forte_add_network_layer(CAN OFF "can" CCANComLayer CANLayer "Enable the CAN Commuication Layer")
if(FORTE_COM_CAN)
  CHECK_IF_KERNEL_MODULE_IS_LOADED()

  forte_add_include_directories(${CMAKE_CURRENT_SOURCE_DIR})  
endif(FORTE_COM_CAN)
