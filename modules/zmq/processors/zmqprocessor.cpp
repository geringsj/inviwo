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

#include <modules/zmq/processors/zmqprocessor.h>
#include <inviwo/core/common/inviwoapplication.h>
#include <limits>

namespace inviwo {

const ProcessorInfo Zmq::processorInfo_{
    "org.inviwo.Zmq",         // Class identifier
    "Zmq",                    // Display name
    "Image Operation",        // Category
    CodeState::Experimental,  // Code state
    Tags::GL,                 // Tags
};
const ProcessorInfo Zmq::getProcessorInfo() const { return processorInfo_; }

Zmq::Zmq()
    : Processor()
    , ctx_(1)
    , should_run_(true)
    , addParam_("addParam", "Add Parameter")
    , addParamButton_("add", "Add")
    , camParamsL_("camparamsL", "Camera Parameters L")
    , camParamsR_("camparamsR", "Camera Parameters R")
    , cameraLFrom_("lookFromL", "Look From L", vec3(1.0f), -vec3(1000.0f), vec3(1000.0f),
                   vec3(0.0001f), InvalidationLevel::Valid, PropertySemantics("Spherical"))
    , cameraRFrom_("lookFromR", "Look From R", vec3(1.0f), -vec3(1000.0f), vec3(1000.0f),
                   vec3(0.0001f), InvalidationLevel::Valid, PropertySemantics("Spherical"))
    , cameraLTo_("lookToL", "Look to L", vec3(0.0f), -vec3(100.0f), vec3(100.0f), vec3(0.1f))
    , cameraRTo_("lookToR", "Look to R", vec3(0.0f), -vec3(100.0f), vec3(100.0f), vec3(0.1f))
    , cameraLUp_("lookUpL", "Look up L", vec3(0.0f, 1.0f, 0.0f), -vec3(100.0f), vec3(100.0f),
                 vec3(0.1f))
    , cameraRUp_("lookUpR", "Look up R", vec3(0.0f, 1.0f, 0.0f), -vec3(100.0f), vec3(100.0f),
                 vec3(0.1f)) {

	addProperty(addParam_);
    addParam_.addProperty(addParamButton_);

    addProperty(camParamsL_);
    camParamsL_.addProperty(cameraLFrom_);
    camParamsL_.addProperty(cameraLTo_);
    camParamsL_.addProperty(cameraLUp_);

    addProperty(camParamsR_);
    camParamsR_.addProperty(cameraRFrom_);
    camParamsR_.addProperty(cameraRTo_);
    camParamsR_.addProperty(cameraRUp_);

    cameraLFrom_.setReadOnly(true);
    cameraRFrom_.setReadOnly(true);
    cameraRFrom_.setReadOnly(true);
    cameraLTo_.setReadOnly(true);
    cameraRTo_.setReadOnly(true);
    cameraLUp_.setReadOnly(true);
    cameraRUp_.setReadOnly(true);

    thread_ = std::thread(&Zmq::receiveZMQ, this);

	addParamButton_.onChange([&]() {
        LogWarn("Test Add Button.");
	});
}

Zmq::~Zmq() {
    should_run_ = false;
    thread_.join();
    thread_.~thread();
}

void Zmq::process() {}

void Zmq::receiveZMQ() {
    zmq::context_t context(1);
    zmq::socket_t camera_socket = zmq::socket_t(context, ZMQ_SUB);
    camera_socket.setsockopt(ZMQ_IDENTITY, "Inviwo", 6);
    camera_socket.connect("tcp://localhost:12345");
    camera_socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);
    camera_socket.setsockopt(ZMQ_RCVTIMEO, 10);

    future_ = dispatchFront([this]() {});

    while (should_run_ == true) {
        // Address Message:
        zmq::message_t address;
        camera_socket.recv(&address);
        std::string address_string = std::string(static_cast<char*>(address.data()), address.size());

        // Content Message:
        zmq::message_t message;
        camera_socket.recv(&message);
        std::string message_string = std::string(static_cast<char*>(message.data()), message.size());

        if (future_.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
            future_ = dispatchFront([this, message_string]() {
                parseMessage(json::parse(message_string));
            });
        }
    }

	camera_socket.~socket_t();
	context.~context_t();
}

void Zmq::parseMessage(json content) {
    // Left Eye
    // Get camera props from unity
    vec3 fromL =
        vec3(content["camVecCamL"]["x"], content["camVecCamL"]["y"], content["camVecCamL"]["z"]);
    fromL.x = -fromL.x;
    vec3 toL =
        vec3(content["camFwdCamL"]["x"], content["camFwdCamL"]["y"], content["camFwdCamL"]["z"]);
    toL.x = -toL.x;
    vec3 upL =
        vec3(content["camUpCamL"]["x"], content["camUpCamL"]["y"], content["camUpCamL"]["z"]);
    upL.x = -upL.x;
    // Set inviwo arcball props
    cameraLFrom_.set(fromL);
    cameraLTo_.set(fromL + toL);
    cameraLUp_.set(upL);

    // Right Eye
    // Get camera props from unity
    vec3 fromR =
        vec3(content["camVecCamR"]["x"], content["camVecCamR"]["y"], content["camVecCamR"]["z"]);
    fromR.x = -fromR.x;
    vec3 toR =
        vec3(content["camFwdCamR"]["x"], content["camFwdCamR"]["y"], content["camFwdCamR"]["z"]);
    toR.x = -toR.x;
    vec3 upR =
        vec3(content["camUpCamR"]["x"], content["camUpCamR"]["y"], content["camUpCamR"]["z"]);
    upR.x = -upR.x;
    // Set inviwo arcball props
    cameraRFrom_.set(fromR);
    cameraRTo_.set(fromR + toR);
    cameraRUp_.set(upR);
}
}  // namespace inviwo
