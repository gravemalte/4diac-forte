/*******************************************************************************
 * Copyright (c) 2016, 2022 Johannes Messmer (admin@jomess.com), fortiss GmbH,
 *                          Jonathan Lainer
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Johannes Messmer - initial API and implementation and/or initial documentation
 *   Jose Cabral - Cleaning of namespaces
 *   Jonathan Lainer - Add method for deregistering Handles by ID.
 *******************************************************************************/

#include <algorithm>

#include "forte/io/mapper/io_mapper.h"
#include "forte/io/mapper/io_handle.h"
#include "forte/io/mapper/io_observer.h"
#include "forte/util/devlog.h"
#include "forte/util/criticalregion.h"

namespace forte::io {
  DEFINE_SINGLETON(IOMapper)

  IOMapper::IOMapper() = default;

  IOMapper::~IOMapper() = default;

  bool IOMapper::registerHandle(const std::string &paId, IOHandle &handle) {
    util::CCriticalRegion criticalRegion(mSyncMutex);

    auto it = std::find_if(mMapping.begin(), mMapping.end(), [&](const IOMapping &m) { return m.id == paId; });

    if (it != mMapping.end()) {
      auto &hVec = it->handlers;
      if (std::find(hVec.begin(), hVec.end(), &handle) != hVec.end()) {
        DEVLOG_WARNING("[IOMapper] Duplicated handle entry '%s'\n", paId.c_str());
        return false;
      }
      hVec.push_back(&handle);
    } else {
      mMapping.push_back(IOMapping{paId, {&handle}, {}});
      it = std::prev(mMapping.end());
    }

    DEVLOG_DEBUG("[IOMapper] Register handle %s\n", paId.c_str());

    if (!it->observers.empty()) {
      IOObserver *observer = it->observers.front();
      handle.onObserver(observer);
      observer->onHandle(&handle);
      DEVLOG_INFO("[IOMapper] Connected %s\n", paId.c_str());
    }

    return true;
  }

  void IOMapper::deregisterHandle(IOHandle &paHandle) {
    util::CCriticalRegion criticalRegion(mSyncMutex);

    std::erase_if(mMapping, [&](IOMapping &mapper) {
      auto it = std::find(mapper.handlers.begin(), mapper.handlers.end(), &paHandle);

      if (it != mapper.handlers.end()) {
        paHandle.dropObserver();
        for (auto *observer : mapper.observers) {
          observer->dropHandle();
        }

        DEVLOG_DEBUG("[IOMapper] Deregister handle %s\n", mapper.id.c_str());
        return true;
      }

      return false;
    });
  }

  void IOMapper::deregisterHandle(std::string const &paId) {
    util::CCriticalRegion criticalRegion(mSyncMutex);

    std::erase_if(mMapping, [&](IOMapping &mapper) {
      if (mapper.id == paId) {
        for (auto *handle : mapper.handlers) {
          handle->dropObserver();
          DEVLOG_INFO("[IOMapper] Disconnected %s (lost handle)\n", mapper.id.c_str());
        }
        for (auto *observer : mapper.observers) {
          DEVLOG_DEBUG("[IOMapper] Deregister handle %s\n", mapper.id.c_str());
          observer->dropHandle();
        }
        return true;
      }
      return false;
    });
  }

  IOHandle *IOMapper::getHandle(std::string const &paId) {
    util::CCriticalRegion criticalRegion(mSyncMutex);

    DEVLOG_DEBUG("[IOMapper] Retrieved handle %s\n", paId.c_str());
    for (auto mMapper : mMapping) {
      if (mMapper.id == paId) {
        return mMapper.handlers.at(0);
      }
    }
    return nullptr;
  }

  bool IOMapper::registerObserver(std::string const &paId, IOObserver *paObserver) {
    util::CCriticalRegion criticalRegion(mSyncMutex);

    auto it = std::find_if(mMapping.begin(), mMapping.end(), [&](const IOMapping &m) { return m.id == paId; });

    if (it == mMapping.end()) {
      mMapping.push_back({paId, {}, {paObserver}});
      DEVLOG_DEBUG("[IOMapper] Register observer %s (new mapping)\n", paId.c_str());
      return true;
    }

    auto &observers = it->observers;
    if (std::find(observers.begin(), observers.end(), paObserver) != observers.end()) {
      DEVLOG_WARNING("[IOMapper] Duplicated observer entry '%s'\n", paId.c_str());
      return false;
    }

    observers.push_back(paObserver);
    DEVLOG_DEBUG("[IOMapper] Register observer %s\n", paId.c_str());

    if (!it->handlers.empty()) {
      auto *handle = it->handlers.front();
      handle->onObserver(paObserver);
      paObserver->onHandle(handle);
      DEVLOG_INFO("[IOMapper] Connected %s\n", paId.c_str());
    }

    return true;
  }

  void IOMapper::deregisterObserver(IOObserver *paObserver) {
    util::CCriticalRegion criticalRegion(mSyncMutex);

    std::erase_if(mMapping, [&](IOMapping &mapper) {
      auto it = std::find(mapper.observers.begin(), mapper.observers.end(), paObserver);

      if (it != mapper.observers.end()) {
        for (auto *handle : mapper.handlers) {
          handle->dropObserver();
          paObserver->dropHandle();
          DEVLOG_INFO("[IOMapper] Disconnected %s (lost observer)\n", mapper.id.c_str());
        }

        DEVLOG_DEBUG("[IOMapper] Deregister observer %s\n", mapper.id.c_str());
        return true;
      }

      return false;
    });
  }

} // namespace forte::io
