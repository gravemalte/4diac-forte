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

#pragma once

#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sockhand.h>

#include "comlayer.h"
#include "datatype.h"
#include "fdselecthand.h"
#include "gensockhand.h"
#include "parameterParser.h"

namespace forte::com_infra {

class CCANComLayer : public CComLayer {
public:
  typedef FORTE_SOCKET_TYPE TCANSocketHandle;

  CCANComLayer(CComLayer *paUpperLayer, CBaseCommFB *paComFB);
  ~CCANComLayer() override;

  EComResponse sendData(void *paData, unsigned int paSize) override;
  EComResponse recvData(const void *paData, unsigned int paSize) override;

  EComResponse openConnection(char *paLayerParameter) override;

  EComResponse processInterrupt() override;

  void closeConnection() override;

private:
  struct SCANParameters {
    std::string interfaceName;
    TForteInt32 baudRate;
    TForteUInt32 CANId;
  };

  enum ECANCommunicationParameter {
    eInterface = 0,
    eBaudrate = 1, // not used yet
    eCANId = 2,
    eCANComParamterAmount = 3
  };

  CFDSelectHandler::TFileDescriptor mCANSocket =
      CFDSelectHandler::scmInvalidFileDescriptor;

  struct sockaddr_can mAddr;
  struct ifreq mIfr;
  struct can_frame mFrame;

  SCANParameters setupCANInternals(CParameterParser &parser);
};

} // namespace forte::com_infra
