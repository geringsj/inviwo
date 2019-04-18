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
    , type_("property", "Property")
    , name_("name", "Name")
    , address_("address", "Address")
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
    type_.addOption("Float", "Float");
    type_.addOption("Int", "Int");
    type_.setSelectedIndex(0);
    type_.setCurrentStateAsDefault();
    addParam_.addProperty(type_);
    addParam_.addProperty(name_);
    addParam_.addProperty(address_);
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

    addParamButton_.onChange([&]() { addSelectedProperty(); });
}

Zmq::~Zmq() {
    should_run_ = false;
    thread_.join();
    thread_.~thread();
}

void Zmq::process() {}

void Zmq::receiveZMQ() {
    zmq::context_t context(1);
    zmq::socket_t zmq_socket = zmq::socket_t(context, ZMQ_SUB);
    zmq_socket.setsockopt(ZMQ_IDENTITY, "Inviwo", 6);
    zmq_socket.connect("tcp://localhost:12345");
    zmq_socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);
    zmq_socket.setsockopt(ZMQ_RCVTIMEO, 10);

    future_ = dispatchFront([this]() {});

    while (should_run_ == true) {
        // Address Message:
        zmq::message_t address;
        zmq_socket.recv(&address);
        std::string address_string = std::string(static_cast<char*>(address.data()), address.size());

        // Content Message:
        zmq::message_t message;
        zmq_socket.recv(&message);
        std::string message_string =
            std::string(static_cast<char*>(message.data()), message.size());

        if (future_.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
            future_ = dispatchFront( [this, message_string]() { parseMessage(json::parse(message_string)); });
        }
    }

    zmq_socket.~socket_t();
    context.~context_t();
}

void Zmq::parseMessage(std::string address, json content) {
    LogWarn(address);
    LogWarn(content);
    //if (address)
}

void Zmq::parseStereoCameraMessage(json content) {
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

void Zmq::addSelectedProperty() {
    if (name_.get() != "" && address_.get() != "") {
		// Create a new Composite Property with the matching address
		CompositeProperty* newComp = new CompositeProperty(name_.get(),name_.get());
		// Create the PropertyMapping for later modifications
    	PropMapping pm = PropMapping();
    	pm.address = address_.get();
    	pm.type = type_.get();
    	pm.property = newComp;

		// Add type-specific props
		std::string selectedType = type_.getSelectedDisplayName();
    	if (selectedType == "Float") {
    	    addFloatProperty(newComp, &pm);
    	} else if (selectedType == "Int") {
    	    addIntProperty(newComp, &pm);
		}

		// Add the new Property
    	additionalProps.push_back(pm);
    	addProperty(pm.property);
    } else {
        LogWarn("Please Specify a Name and Adress for your new Property.")
	}
}

void Zmq::addFloatProperty(CompositeProperty* newComp, PropMapping* pm) { 
    newComp->addProperty(new FloatProperty("value", "Value", 0.0, -10000.0, 10000.0));
    pm->mirror = new FloatProperty("value", "Value", 0.0, -10000.0, 10000.0);
}

void Zmq::addIntProperty(CompositeProperty* newComp, PropMapping* pm) { 
	newComp->addProperty(new IntProperty("value", "Value", 0, -10000, 10000));
	pm->mirror = new IntProperty("value", "Value", 0, -10000, 10000);
}
}  // namespace inviwo
