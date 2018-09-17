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

#include <modules/spout/processors/spoutprocessor.h>

#include <modules/opengl/texture/textureunit.h>
#include <modules/opengl/texture/textureutils.h>
#include <modules/opengl/image/imagegl.h>
#include <modules/opengl/shader/shaderutils.h>
#include <modules/opengl/inviwoopengl.h>

namespace inviwo {

const ProcessorInfo Spout::processorInfo_{
    "org.inviwo.Spout",       // Class identifier
    "Spout",                  // Display name
    "Image Operation",        // Category
    CodeState::Experimental,  // Code state
    Tags::GL,                 // Tags
};
const ProcessorInfo Spout::getProcessorInfo() const { return processorInfo_; }

Spout::Spout()
    : Processor()
    , inport_("inport")
    , dimensions_("dimensions", "Canvas Size", ivec2(256, 256), ivec2(128, 128), ivec2(4096, 4096),
                  ivec2(1, 1), InvalidationLevel::Valid)
    , inputSize_("inputSize", "Input Dimension Parameters")
    , widgetMetaData_{
          createMetaData<ProcessorWidgetMetaData>(ProcessorWidgetMetaData::CLASS_IDENTIFIER)} {
    addPort(inport_);
    addProperty(inputSize_);
    inport_.setOptional(true);

    dimensions_.setSerializationMode(PropertySerializationMode::None);
    dimensions_.onChange([this]() { widgetMetaData_->setDimensions(dimensions_.get()); });
    inputSize_.addProperty(dimensions_);

    inport_.onChange([&]() {
        sender_.CreateSender("inviwo_sender", inport_.getData()->getDimensions()[0],
                             inport_.getData()->getDimensions()[1]);
    });
}

Spout::~Spout() = default;

void Spout::process() {
    if (inport_.hasData()) {
        sender_.SendTexture(
            inport_.getData()->getColorLayer()->getRepresentation<LayerGL>()->getTexture()->getID(),
            GL_TEXTURE_2D, inport_.getData()->getDimensions()[0],
                            inport_.getData()->getDimensions()[1]);
    }
}
}  // namespace inviwo
