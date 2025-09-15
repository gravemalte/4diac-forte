/*******************************************************************************
 * Copyright (c) 2015, 2023 fortiss GmbH, Johannes Kepler University Linz
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Martin Jobst - initial API and implementation and/or initial documentation
 *   Alois Zoitl  - upgraded to new FB memory layout
 *******************************************************************************/

#pragma once

#include "forte/stringid.h"
#include "forte/interfacespec.h"
#include "forte/typelib.h"

class CLuaEngine;

class CLuaBFBTypeEntry : public forte::core::CFBTypeEntry {
  private:
    const std::string cmLuaScriptAsString;
    SFBInterfaceSpec m_interfaceSpec;
    std::span<const forte::core::StringId> mVarInternalNames;

    CLuaBFBTypeEntry(forte::core::StringId typeNameId,
                     const std::string &paLuaScriptAsString,
                     SFBInterfaceSpec &interfaceSpec,
                     std::span<const forte::core::StringId> varInternalNames);

    ~CLuaBFBTypeEntry();

    static bool initInterfaceSpec(SFBInterfaceSpec &interfaceSpec, CLuaEngine *luaEngine, int index);
    static void deleteInterfaceSpec(SFBInterfaceSpec &interfaceSpec);
    // static bool
    // initInternalVarsInformation(SInternalVarsInformation &internalVarsInformation, CLuaEngine *luaEngine, int index);
    // static void deleteInternalVarsInformation(SInternalVarsInformation &internalVarsInformation);

  public:
    static CLuaBFBTypeEntry *createLuaFBTypeEntry(forte::core::StringId typeNameId,
                                                  const std::string &paLuaScriptAsString);

    CFunctionBlock *createFBInstance(forte::core::StringId paInstanceNameId, forte::core::CFBContainer &paContainer);

    const SFBInterfaceSpec &getInterfaceSpec() const {
      return m_interfaceSpec;
    }
};
