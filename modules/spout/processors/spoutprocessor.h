/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2013-2018 Inviwo Foundation
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

#ifndef IVW_SPOUT_H
#define IVW_SPOUT_H

#include <modules/spout/spoutmoduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/processors/processor.h>
#include <inviwo/core/properties/optionproperty.h>
#include <inviwo/core/properties/ordinalproperty.h>
#include <inviwo/core/properties/buttonproperty.h>
#include <inviwo/core/properties/stringproperty.h>
#include <inviwo/core/ports/imageport.h>
#include <modules/opengl/shader/shader.h>
#include "../ext/Spout.h"
#include <inviwo/core/metadata/processorwidgetmetadata.h>
#include <inviwo/core/properties/compositeproperty.h>

namespace inviwo {

/** \docpage{org.inviwo.Spout, Spout}
 * ![](org.inviwo.Spout.png?classIdentifier=org.inviwo.Spout)
 * Uses Spout to share the image with other Applications.
 *
 * ### Inports
 *   * __ImageInport__ Input image.
 *
 * ### Properties
 */

/**
 * \brief Shares the image with other Spout Applications.
 *
 */
class IVW_MODULE_SPOUT_API Spout : public Processor {
public:
    Spout();
    virtual ~Spout();

    virtual void process() override;

    virtual const ProcessorInfo getProcessorInfo() const override;
    static const ProcessorInfo processorInfo_;

	void setCanvasSize(ivec2);
    ivec2 getCanvasSize() const;

    bool getUseCustomDimensions() const;
    ivec2 getCustomDimensions() const;

public:
    ImageInport inport_;
    IntVec2Property dimensions_;
    CompositeProperty inputSize_;
    BoolProperty enableCustomInputDimensions_;
    IntVec2Property customInputDimensions_;
    StringProperty senderName_;

private:
    void sizeChanged();
    void nameChanged();

	SpoutSender sender_;
    ivec2 previousImageSize_;
};

}  // namespace inviwo

#endif  // IVW_SPOUT_H
