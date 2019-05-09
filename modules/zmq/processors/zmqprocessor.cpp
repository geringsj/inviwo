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

PropMapping::PropMapping(std::string address, std::string type, CompositeProperty* property,
                         CompositeProperty* mirror)
    : address(address), type(type), property(property), mirror(mirror) {}

void PropMapping::serialize(Serializer& s) const {
	s.serialize("address", address);
    s.serialize("type", type);
    s.serialize("property", property);
    s.serialize("mirror", mirror);
}

void PropMapping::deserialize(Deserializer& d) {
	d.deserialize("address", address);
    d.deserialize("type", type);
    d.deserializeAs<inviwo::Property>("property", property);
    d.deserializeAs<inviwo::Property>("mirror", mirror);
}

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
    , addParam_("addParam", "Add Parameter")
    , type_("property", "Property")
    , name_("name", "Name")
    , address_("address", "Address")
    , addParamButton_("add", "Add")
    , should_run_(true)
    , ctx_(1) {

    // Allow adding properties to the processor
    addProperty(addParam_);
    type_.addOption("Bool", "Bool");
    type_.addOption("Float", "Float");
    type_.addOption("Int", "Int");
    type_.addOption("IntVec2", "IntVec2");
    type_.addOption("FloatVec3", "FloatVec3");
    type_.addOption("Stereo Camera", "Stereo Camera");
    type_.setSelectedIndex(0);
    type_.setCurrentStateAsDefault();
    addParam_.addProperty(type_);
    addParam_.addProperty(name_);
    addParam_.addProperty(address_);
    addParamButton_.onChange([&]() { addSelectedProperty(); });
    addParam_.addProperty(addParamButton_);

    // Start the thread that listens to ZMQ messages
    thread_ = std::thread(&Zmq::receiveZMQ, this);
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
        std::string address_string =
            std::string(static_cast<char*>(address.data()), address.size());

        // Content Message:
        zmq::message_t message;
        zmq_socket.recv(&message);
        std::string message_string =
            std::string(static_cast<char*>(message.data()), message.size());

		if (address_string != "") {
            try {
                parseMessage(address_string,
                             json::parse(message_string));  // Parse all the messages and change the
                                                            // Mirrors accordingly
            } catch (...) {
                // Sometimes, the address and message contain incorrect values. This is a HACK to
                // catch them. It is not a clean solution and should be changed!
            }

            if (future_.wait_for(std::chrono::milliseconds(0)) ==
                std::future_status::ready) {  // If the UI is ready
                future_ =
                    dispatchFront([this, address_string, message_string]() {  // Go to the UI Thread
                        updateUI();                                           // Update the UI
                    });
            }
		}
    }

    zmq_socket.~socket_t();
    context.~context_t();
}

void Zmq::updateUI() {
    for (auto i = additionalProps.begin(); i != additionalProps.end(); ++i) {
        PropMapping* pm = *i;
        if (pm->type == "Stereo Camera") {
            // Update Left Eye
            dynamic_cast<FloatVec3Property*>(
                pm->property->getPropertyByIdentifier("lookFromL", true))
                ->set(dynamic_cast<FloatVec3Property*>(
                          pm->mirror->getPropertyByIdentifier("lookFromL", true))
                          ->get());
            dynamic_cast<FloatVec3Property*>(pm->property->getPropertyByIdentifier("lookToL", true))
                ->set(dynamic_cast<FloatVec3Property*>(
                          pm->mirror->getPropertyByIdentifier("lookToL", true))
                          ->get());
            dynamic_cast<FloatVec3Property*>(pm->property->getPropertyByIdentifier("lookUpL", true))
                ->set(dynamic_cast<FloatVec3Property*>(
                          pm->mirror->getPropertyByIdentifier("lookUpL", true))
                          ->get());
            // Update Right Eye
            dynamic_cast<FloatVec3Property*>(
                pm->property->getPropertyByIdentifier("lookFromR", true))
                ->set(dynamic_cast<FloatVec3Property*>(
                          pm->mirror->getPropertyByIdentifier("lookFromR", true))
                          ->get());
            dynamic_cast<FloatVec3Property*>(pm->property->getPropertyByIdentifier("lookToR", true))
                ->set(dynamic_cast<FloatVec3Property*>(
                          pm->mirror->getPropertyByIdentifier("lookToR", true))
                          ->get());
            dynamic_cast<FloatVec3Property*>(pm->property->getPropertyByIdentifier("lookUpR", true))
                ->set(dynamic_cast<FloatVec3Property*>(
                          pm->mirror->getPropertyByIdentifier("lookUpR", true))
                          ->get());
        } else if (pm->type == "Bool") {
            dynamic_cast<BoolProperty*>(pm->property->getPropertyByIdentifier("value"))->set(dynamic_cast<BoolProperty*>(pm->mirror->getPropertyByIdentifier("value"))->get());
        } else if (pm->type == "Float") {
            dynamic_cast<FloatProperty*>(pm->property->getPropertyByIdentifier("value"))
                ->set(dynamic_cast<FloatProperty*>(pm->mirror->getPropertyByIdentifier("value"))
                          ->get());
        } else if (pm->type == "Int") {
            dynamic_cast<IntProperty*>(pm->property->getPropertyByIdentifier("value"))
                ->set(dynamic_cast<IntProperty*>(pm->mirror->getPropertyByIdentifier("value"))
                          ->get());
        } else if (pm->type == "IntVec2") {
            dynamic_cast<IntVec2Property*>(pm->property->getPropertyByIdentifier("value"))
                ->set(dynamic_cast<IntVec2Property*>(pm->mirror->getPropertyByIdentifier("value"))
                          ->get());
        } else if (pm->type == "FloatVec3") {
            dynamic_cast<FloatVec3Property*>(pm->property->getPropertyByIdentifier("value"))
                ->set(dynamic_cast<FloatVec3Property*>(pm->mirror->getPropertyByIdentifier("value"))
                          ->get());
        }
    }
}

void Zmq::parseMessage(std::string address, json content) {
    for (auto i = additionalProps.begin(); i != additionalProps.end(); ++i) {
        PropMapping* pm = *i;
        if (pm->address == address) {
            if (pm->type == "Stereo Camera") {
                parseStereoCameraMessage(pm, content);
            } else if (pm->type == "Bool") {
                parseBoolMessage(pm, content);
            } else if (pm->type == "Float") {
                parseFloatMessage(pm, content);
            } else if (pm->type == "Int") {
                parseIntMessage(pm, content);
            } else if (pm->type == "IntVec2") {
                parseIntVec2Message(pm, content);
            } else if (pm->type == "FloatVec3") {
                parseFloatVec3Message(pm, content);
            }
        }
    }
}

void Zmq::parseStereoCameraMessage(PropMapping* prop, json content) {
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

    // Update Mirror
    dynamic_cast<FloatVec3Property*>(prop->mirror->getPropertyByIdentifier("lookFromL", true))
        ->set(fromL);
    dynamic_cast<FloatVec3Property*>(prop->mirror->getPropertyByIdentifier("lookToL", true))
        ->set(fromL + toL);
    dynamic_cast<FloatVec3Property*>(prop->mirror->getPropertyByIdentifier("lookUpL", true))
        ->set(upL);

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

    // Update Mirror
    dynamic_cast<FloatVec3Property*>(prop->mirror->getPropertyByIdentifier("lookFromR", true))
        ->set(fromR);
    dynamic_cast<FloatVec3Property*>(prop->mirror->getPropertyByIdentifier("lookToR", true))
        ->set(fromR + toR);
    dynamic_cast<FloatVec3Property*>(prop->mirror->getPropertyByIdentifier("lookUpR", true))
        ->set(upR);
}

void Zmq::parseBoolMessage(PropMapping* prop, json content) {
    bool value = content["value"];
    dynamic_cast<BoolProperty*>(prop->mirror->getPropertyByIdentifier("value"))->set(value);
}

void Zmq::parseFloatMessage(PropMapping* prop, json content) {
    float value = content["value"];
    dynamic_cast<FloatProperty*>(prop->mirror->getPropertyByIdentifier("value"))->set(value);
}

void Zmq::parseIntMessage(PropMapping* prop, json content) {
    int value = content["value"];
    dynamic_cast<IntProperty*>(prop->mirror->getPropertyByIdentifier("value"))->set(value);
}

void Zmq::parseIntVec2Message(PropMapping* prop, json content) {
    ivec2 value = ivec2(content["value"]["x"], content["value"]["y"]);
    dynamic_cast<IntVec2Property*>(prop->mirror->getPropertyByIdentifier("value"))->set(value);
}

void Zmq::parseFloatVec3Message(PropMapping* prop, json content) {
    vec3 value = vec3(content["value"]["x"], content["value"]["y"], content["value"]["z"]);
    dynamic_cast<FloatVec3Property*>(prop->mirror->getPropertyByIdentifier("value"))->set(value);
}

void Zmq::addSelectedProperty() {
    if (name_.get() != "" && address_.get() != "") {
        // Create a new Composite Property with the matching address
        CompositeProperty* newComp = new CompositeProperty(name_.get(), name_.get());
        CompositeProperty* newMirror = new CompositeProperty(name_.get(), name_.get());
        // Create the PropertyMapping for later modifications
        PropMapping* pm = new PropMapping(address_.get(), type_.get(), newComp, newMirror);

        // Add type-specific props
        std::string selectedType = type_.getSelectedDisplayName();
        if (selectedType == "Bool") {
            addBoolProperty(newComp, newMirror);
		} else if (selectedType == "Float") {
            addFloatProperty(newComp, newMirror);
        } else if (selectedType == "Int") {
            addIntProperty(newComp, newMirror);
        } else if (selectedType == "IntVec2") {
            addIntVec2Property(newComp, newMirror);
        } else if (selectedType == "FloatVec3") {
            addFloatVec3Property(newComp, newMirror);
        } else if (selectedType == "Stereo Camera") {
            addStereoCameraProperty(newComp, newMirror);
		}

        // Add the new Property
        additionalProps.push_back(pm);
        addProperty(pm->property.get());
        pm->property->setSerializationMode(PropertySerializationMode::All);
        pm->mirror->setSerializationMode(PropertySerializationMode::All);
    } else {
        LogWarn("Please Specify a Name and Adress for your new Property.")
    }
}

void Zmq::addBoolProperty(CompositeProperty* newComp, CompositeProperty* newMirror) {
    BoolProperty* newProp = new BoolProperty("value", "Value", false);
    newProp->setSerializationMode(PropertySerializationMode::All);
    BoolProperty* newMirrorProp = new BoolProperty("value", "Value", false);
    newMirrorProp->setSerializationMode(PropertySerializationMode::All);
    newComp->addProperty(newProp);
    newMirror->addProperty(newMirrorProp);
}

void Zmq::addFloatProperty(CompositeProperty* newComp, CompositeProperty* newMirror) {
    FloatProperty* newProp = new FloatProperty("value", "Value", 0.0, -10000.0, 10000.0);
    newProp->setSerializationMode(PropertySerializationMode::All);
    FloatProperty* newMirrorProp = new FloatProperty("value", "Value", 0.0, -10000.0, 10000.0);
    newMirrorProp->setSerializationMode(PropertySerializationMode::All);
    newComp->addProperty(newProp);
    newMirror->addProperty(newMirrorProp);
}

void Zmq::addIntProperty(CompositeProperty* newComp, CompositeProperty* newMirror) {
    IntProperty* newProp = new IntProperty("value", "Value", 0, -10000, 10000); 
    newProp->setSerializationMode(PropertySerializationMode::All);
	IntProperty* newMirrorProp = new IntProperty("value", "Value", 0, -10000, 10000);
    newMirrorProp->setSerializationMode(PropertySerializationMode::All);
    newComp->addProperty(newProp);
    newMirror->addProperty(newMirrorProp);
}

void Zmq::addIntVec2Property(CompositeProperty* newComp, CompositeProperty* newMirror) {
    IntVec2Property* newProp = new IntVec2Property("value", "Value", ivec2(0), -ivec2(10000), ivec2(10000));
    newProp->setSerializationMode(PropertySerializationMode::All);
    IntVec2Property* newMirrorProp = new IntVec2Property("value", "Value", ivec2(0), -ivec2(10000), ivec2(10000));
    newMirrorProp->setSerializationMode(PropertySerializationMode::All);
    newComp->addProperty(newProp);
    newMirror->addProperty(newMirrorProp);
}

void Zmq::addFloatVec3Property(CompositeProperty* newComp, CompositeProperty* newMirror) {
    FloatVec3Property* newProp = new FloatVec3Property("value", "Value", vec3(0.0f), -vec3(10000.0f), vec3(10000.0f), vec3(0.01f));
    newProp->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newMirrorProp = new FloatVec3Property("value", "Value", vec3(0.0f), -vec3(10000.0f), vec3(10000.0f), vec3(0.01f));
    newMirrorProp->setSerializationMode(PropertySerializationMode::All);
    newComp->addProperty(newProp);
    newMirror->addProperty(newMirrorProp);
}

void Zmq::addStereoCameraProperty(CompositeProperty* newComp, CompositeProperty* newMirror) {
    // Left Camera
    CompositeProperty* cameraL = new CompositeProperty("camparamsL", "Camera Parameters L");
    cameraL->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newFromPropL = new FloatVec3Property( "lookFromL", "Look From L", vec3(1.0f), -vec3(1000.0f), vec3(1000.0f), vec3(0.0001f), InvalidationLevel::Valid, PropertySemantics("Spherical"));
    newFromPropL->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newToPropL = new FloatVec3Property("lookToL", "Look to L", vec3(0.0f), -vec3(100.0f), vec3(100.0f), vec3(0.1f));
    newToPropL->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newUpPropL = new FloatVec3Property("lookUpL", "Look up L", vec3(0.0f, 1.0f, 0.0f), -vec3(100.0f), vec3(100.0f), vec3(0.1f));
    newUpPropL->setSerializationMode(PropertySerializationMode::All);
    cameraL->addProperty(newFromPropL);
    cameraL->addProperty(newToPropL);
    cameraL->addProperty(newUpPropL);
    newComp->addProperty(cameraL);
    // Left Camera Mirror
    CompositeProperty* cameraLMirror = new CompositeProperty("camparamsL", "Camera Parameters L");
    cameraLMirror->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newFromPropLMirror = new FloatVec3Property( "lookFromL", "Look From L", vec3(1.0f), -vec3(1000.0f), vec3(1000.0f), vec3(0.0001f), InvalidationLevel::Valid, PropertySemantics("Spherical"));
    newFromPropLMirror->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newToPropLMirror = new FloatVec3Property("lookToL", "Look to L", vec3(0.0f), -vec3(100.0f), vec3(100.0f), vec3(0.1f));
    newToPropLMirror->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newUpPropLMirror = new FloatVec3Property("lookUpL", "Look up L", vec3(0.0f, 1.0f, 0.0f), -vec3(100.0f), vec3(100.0f), vec3(0.1f));
    newUpPropLMirror->setSerializationMode(PropertySerializationMode::All);
    cameraLMirror->addProperty(newFromPropLMirror);
    cameraLMirror->addProperty(newToPropLMirror);
    cameraLMirror->addProperty(newUpPropLMirror);
    newMirror->addProperty(cameraLMirror);

    // Right Camera
    CompositeProperty* cameraR = new CompositeProperty("camparamsR", "Camera Parameters R");
    cameraR->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newFromPropR = new FloatVec3Property( "lookFromR", "Look From R", vec3(1.0f), -vec3(1000.0f), vec3(1000.0f), vec3(0.0001f), InvalidationLevel::Valid, PropertySemantics("Spherical"));
    newFromPropR->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newToPropR = new FloatVec3Property("lookToR", "Look to R", vec3(0.0f), -vec3(100.0f), vec3(100.0f), vec3(0.1f));
    newToPropR->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newUpPropR = new FloatVec3Property("lookUpR", "Look up R", vec3(0.0f, 1.0f, 0.0f), -vec3(100.0f), vec3(100.0f), vec3(0.1f));
    newUpPropR->setSerializationMode(PropertySerializationMode::All);
    cameraR->addProperty(newFromPropR);
    cameraR->addProperty(newToPropR);
    cameraR->addProperty(newUpPropR);
    newComp->addProperty(cameraR);
    // Right Camera Mirror
    CompositeProperty* cameraRMirror = new CompositeProperty("camparamsR", "Camera Parameters R");
    cameraRMirror->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newFromPropRMirror = new FloatVec3Property( "lookFromR", "Look From R", vec3(1.0f), -vec3(1000.0f), vec3(1000.0f), vec3(0.0001f), InvalidationLevel::Valid, PropertySemantics("Spherical"));
    newFromPropRMirror->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newToPropRMirror = new FloatVec3Property("lookToR", "Look to R", vec3(0.0f), -vec3(100.0f), vec3(100.0f), vec3(0.1f));
    newToPropRMirror->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newUpPropRMirror = new FloatVec3Property("lookUpR", "Look up R", vec3(0.0f, 1.0f, 0.0f), -vec3(100.0f), vec3(100.0f), vec3(0.1f));
    newUpPropRMirror->setSerializationMode(PropertySerializationMode::All);
    cameraRMirror->addProperty(newFromPropRMirror);
    cameraRMirror->addProperty(newToPropRMirror);
    cameraRMirror->addProperty(newUpPropRMirror);
    newMirror->addProperty(cameraRMirror);
}

void Zmq::serialize(Serializer& s) const {
    s.serialize("propMapping", additionalProps, "propmapping");
    Processor::serialize(s);
}

void Zmq::deserialize(Deserializer& d) {
    d.deserialize("propMapping", additionalProps, "propmapping");
    Processor::deserialize(d);
}
}  // namespace inviwo
