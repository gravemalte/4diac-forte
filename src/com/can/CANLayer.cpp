/*******************************************************************************
 * Copyright (c) 2025 Malte Grave, LIT CPS Johannes Kepler Universität,
 *                                 github.com/gravemalte
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *  Malte Grave - initial API and implementation and/or initial documentation
 *******************************************************************************/

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <linux/can.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

#include "CANLayer.h"
#include "basecommfb.h"
#include "comtypes.h"
#include "datatype.h"
#include "devlog.h"
#include "forte_any.h"
#include "forte_array.h"

using namespace forte::com_infra;

CCANComLayer::CCANComLayer(CComLayer *paUpperLayer, CBaseCommFB *paComFB)
    : CComLayer(paUpperLayer, paComFB) {}

EComResponse CCANComLayer::openConnection(char *paLayerParameter) {
  EComResponse eRetVal = e_InitInvalidId;
  CParameterParser parser(paLayerParameter, ';', eCANComParamterAmount);
  if (eCANComParamterAmount != parser.parseParameters()) {
    DEVLOG_ERROR("[CAN Layer] Parsing of the parameters failed.\n");
    return eRetVal;
  }

  SCANParameters can_params = setupCANInternals(parser);

  switch (mFb->getComServiceType()) {
  case e_Server:
    DEVLOG_ERROR("[CAN Layer] the layer only supports the PUB/SUB "
                 "pattern. Please use a Publisher block to send data.");
    eRetVal = e_InitTerminated;
    break;
  case e_Client:
    DEVLOG_ERROR("[CAN Layer] the layer only supports the PUB/SUB "
                 "pattern. Please use a Subscribe block to receive data.");
    eRetVal = e_InitTerminated;
    break;
  case e_Publisher:
    // is handled via sendData
    break;
  case e_Subscriber:
    // is handled via recvData
    break;
  }

  DEVLOG_DEBUG("[CAN Layer] Opening connection\n");

  CFDSelectHandler::TFileDescriptor fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);

  if (CFDSelectHandler::scmInvalidFileDescriptor == fd) {
    DEVLOG_ERROR("[CAN Layer] Socket can't be created: %s\n", strerror(errno));
    return e_InitInvalidId;
  }

  std::strncpy(mIfr.ifr_name, can_params.interfaceName.c_str(), IFNAMSIZ - 1);
  mIfr.ifr_name[IFNAMSIZ - 1] =
      '\0'; // Ensure that null termination is added if input is too long.
  if (-1 == ioctl(fd, SIOCGIFINDEX, &mIfr)) {
    DEVLOG_ERROR("[CAN Layer] Socket cannot be controlled. %s\n",
                 strerror(errno));
    close(fd);
    return eRetVal;
  }

  mCANSocket = fd;
  mAddr.can_family = AF_CAN;
  mAddr.can_ifindex = mIfr.ifr_ifindex;

  if (-1 == bind(mCANSocket, (struct sockaddr *)&mAddr, sizeof(mAddr))) {
    DEVLOG_ERROR("[CAN Layer] Binding of the socket unsuccessful: %s\n",
                 strerror(errno));
    close(mCANSocket);
    return eRetVal;
  }

  this->getExtEvHandler<CFDSelectHandler>().addComCallback(mCANSocket, this);
  DEVLOG_DEBUG("[CAN Layer] Callback handler added.\n");

  mFrame.can_id = can_params.CANId;

  eRetVal = e_InitOk;
  return eRetVal;
}

CCANComLayer::~CCANComLayer() { closeConnection(); }

void CCANComLayer::closeConnection() {
  if (mCANSocket != CFDSelectHandler::scmInvalidFileDescriptor) {
    DEVLOG_DEBUG("[CAN Layer] Closing socket\n");
    close(mCANSocket);
    DEVLOG_DEBUG("[CAN Layer] Unregister layer\n");
    this->getExtEvHandler<CFDSelectHandler>().removeComCallback(mCANSocket);
  }
}

EComResponse CCANComLayer::sendData(void *paData, unsigned int) {
  auto *raw_data = static_cast<CIEC_ARRAY *>(
      &(static_cast<CIEC_ANY **>(paData)[0]->unwrap()));

  size_t size = raw_data->getToStringBufferSize();
  DEVLOG_DEBUG("[CAN Layer] Size of the array is: %u.\n", size);
  std::vector<char> buffer(size);

  int nLength = raw_data->toString(buffer.data(), size);
  if (nLength <= 0) {
    DEVLOG_ERROR("[CAN Layer] Copying of the raw data failed.");
    return e_ProcessDataSendFailed;
  }

  size_t offset = 0;
  while (offset < size) {

    mFrame.len = static_cast<uint8_t>(
        (size - offset > CAN_MAX_DLEN) ? CAN_MAX_DLEN : (size - offset));

    // Clear old data and copy new chunk
    std::memset(mFrame.data, 0, CAN_MAX_DLEN);
    std::memcpy(mFrame.data, buffer.data() + offset, mFrame.len);

    offset += mFrame.len;

    DEVLOG_DEBUG("[CAN Layer] Sending data ...\n");
    if (write(mCANSocket, &mFrame, sizeof(struct can_frame)) < 0) {
      DEVLOG_ERROR("CAN send failed: %s\n", strerror(errno));
      return e_ProcessDataSendFailed;
    }
  }

  return e_ProcessDataOk;
}

EComResponse CCANComLayer::recvData(const void *paData, unsigned int paSize) {
  return e_ProcessDataOk;
}

EComResponse CCANComLayer::processInterrupt() { return e_ProcessDataOk; }

CCANComLayer::SCANParameters
CCANComLayer::setupCANInternals(CParameterParser &parser) {
  SCANParameters can_params;

  can_params.interfaceName =
      std::string(parser[eInterface], strlen(parser[eInterface]));

  can_params.baudRate = static_cast<TForteInt32>(
      forte::core::util::strtoul(parser[eBaudrate], nullptr, 10));
  can_params.CANId = static_cast<TForteUInt32>(
      forte::core::util::strtoul(parser[eCANId], nullptr, 10));

  DEVLOG_DEBUG(
      "[CAN Layer] Using interface: %s with Baudrate %u and CAN-ID: %u\n",
      can_params.interfaceName.c_str(), can_params.baudRate, can_params.CANId);

  return can_params;
}