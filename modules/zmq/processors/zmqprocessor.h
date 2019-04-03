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

#ifndef IVW_ZMQ_H
#define IVW_ZMQ_H

#include <modules/zmq/zmqmoduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/processors/processor.h>
#include <inviwo/core/properties/ordinalproperty.h>
#include <inviwo/core/properties/cameraproperty.h>
#include <inviwo/core/properties/compositeproperty.h>
#include <zmq.hpp>
#include <thread>
#include "../ext/json.hpp"

using json = nlohmann::json;

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
class IVW_MODULE_ZMQ_API Zmq : public Processor {
public:
    Zmq();
    virtual ~Zmq();

    virtual void process() override;

    virtual const ProcessorInfo getProcessorInfo() const override;
    static const ProcessorInfo processorInfo_;

public:
private:
    CompositeProperty camParamsL_;
    CompositeProperty camParamsR_;
    FloatVec3Property cameraLFrom_;
    FloatVec3Property cameraRFrom_;
    FloatVec3Property cameraLTo_;
    FloatVec3Property cameraRTo_;
    FloatVec3Property cameraLUp_;
    FloatVec3Property cameraRUp_;

    std::atomic<bool> should_run_;
    std::future<void> future_;

    void receiveZMQ();
    void parseMessage(json content);
    std::thread thread_;

    zmq::context_t ctx_;
};

}  // namespace inviwo

#endif  // IVW_ZMQ_H
