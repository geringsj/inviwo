/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2016-2021 Inviwo Foundation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *********************************************************************************/
#pragma once

#include <modules/animation/animationmoduledefine.h>

#include <inviwo/core/util/timer.h>
#include <inviwo/core/common/inviwoapplication.h>
#include <inviwo/core/properties/propertyowner.h>
#include <inviwo/core/properties/fileproperty.h>
#include <inviwo/core/properties/optionproperty.h>

#include <modules/animation/demo/democontrollerobserver.h>

namespace inviwo {

namespace animation {

/**
 * The AnimationController is responsible for evaluating the demo and keeping track of the
 * demo time and state.
 */
class IVW_MODULE_ANIMATION_API DemoController : public DemoControllerObservable,
                                                public PropertyOwner {
public:
    DemoController(InviwoApplication* app = InviwoApplication::getPtr());
    virtual ~DemoController();

    enum Offset { None, First, Previous, Next, Last, Reload };

    void onChangeSelection(Offset offset);
    void setFileOptions();

    void setFolder(const std::string& path);

    virtual InviwoApplication* getInviwoApplication() override;

protected:
    void loadWorkspaceApp(const std::string& fileName);

    InviwoApplication* app_;
    FileProperty demoFolder_;
    OptionPropertyInt demoFile_;

    bool updateWorkspace_ = true;
};

}  // namespace animation

}  // namespace inviwo
