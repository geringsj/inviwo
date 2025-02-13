/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2013-2021 Inviwo Foundation
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

#include <modules/fontrendering/fontrenderingmoduledefine.h>
#include <modules/fontrendering/textrenderer.h>
#include <modules/fontrendering/properties/fontproperty.h>

#include <inviwo/core/ports/imageport.h>
#include <inviwo/core/processors/processor.h>
#include <inviwo/core/properties/optionproperty.h>
#include <inviwo/core/properties/stringproperty.h>
#include <inviwo/core/properties/ordinalproperty.h>
#include <inviwo/core/properties/boolproperty.h>
#include <inviwo/core/properties/buttonproperty.h>

#include <modules/opengl/shader/shader.h>
#include <modules/opengl/rendering/texturequadrenderer.h>

namespace inviwo {

class Image;
class ImageGL;

/** \docpage{org.inviwo.TextOverlayGL, Text Overlay}
 * ![](org.inviwo.TextOverlayGL.png?classIdentifier=org.inviwo.TextOverlayGL)
 *
 * Overlay text onto an image. The text can contain up to 99 place markers indicated by %1 to %99.
 * These markers will be replaced with the contents of the corresponding arg properties. A place
 * marker can occur multiple times and all occurences will be replaced with the same text.
 *
 * ### Inports
 *   * __Inport__ Input image (optional)
 *
 * ### Outports
 *   * __Outport__ Output image with overlayed text
 *
 * ### Properties
 *   * __Text__ Text to overlay. This text can contain place markers %1 to %99, which will be
 *              replaced with the optional argument properties
 *   * __Argument Properties__ texts used instead of place markers (optional, created with the "Add
 * Argument String" button)
 *   * __Font size__ Text size
 *   * __Position__ Where to put the text, relative position from 0 to 1
 *   * __Anchor__ What point of the text to put at "Position". relative from -1,1. 0 meas the image
 *     is centered on "Position".
 */

class IVW_MODULE_FONTRENDERING_API TextOverlayGL : public Processor {
public:
    TextOverlayGL();
    virtual ~TextOverlayGL() = default;

    virtual const ProcessorInfo getProcessorInfo() const override;
    static const ProcessorInfo processorInfo_;

    virtual void deserialize(Deserializer& d) override;

protected:
    virtual void process() override;

    /**
     * \brief returns the output string with place markers replaced by the corresponding strings of
     * the optional properties
     *
     * @return std::string output string with replaced place markers
     */
    std::string getString() const;

    void updateCache();

private:
    ImageInport inport_;
    ImageOutport outport_;

    BoolProperty enable_;
    StringProperty text_;
    FloatVec4Property color_;
    FontProperty font_;
    FloatVec2Property position_;
    IntVec2Property offset_;

    ButtonProperty
        addArgButton_;  //!< this button will add string properties to be used with place markers

    TextRenderer textRenderer_;
    std::size_t numArgs_;  //!< number of optional place marker properties

    const std::size_t maxNumArgs_ = 99;

    TextTextureObject textObject_;

    TextureQuadRenderer textureRenderer_;
};

}  // namespace inviwo
