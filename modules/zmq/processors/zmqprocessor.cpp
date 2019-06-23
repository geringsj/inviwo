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

PropMapping::PropMapping(std::string address, CompositeProperty* property,
                         CompositeProperty* mirror)
    : address(address), type(PropertyType::none), property(property), mirror(mirror) {}

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

const ProcessorInfo ZmqReceiver::processorInfo_{
    "org.inviwo.Zmq",         // Class identifier
    "ZmqReceiver",            // Display name
    "Image Operation",        // Category
    CodeState::Experimental,  // Code state
    Tags::GL,                 // Tags
};

const ProcessorInfo ZmqReceiver::getProcessorInfo() const { return processorInfo_; }

ZmqReceiver::ZmqReceiver()
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
    type_.addOption("CameraProjection", "CameraProjection");
    type_.addOption("StereoCameraView", "StereoCameraView");
    type_.addOption("Transfer Function", "TransferFunction");
    type_.setSelectedIndex(0);
    type_.setCurrentStateAsDefault();
    addParam_.addProperty(type_);
    addParam_.addProperty(name_);
    addParam_.addProperty(address_);
    addParamButton_.onChange([&]() { addSelectedProperty(); });
    addParam_.addProperty(addParamButton_);

    // Start the thread that listens to ZMQ messages
    thread_ = std::thread(&ZmqReceiver::receiveZMQ, this);
}

ZmqReceiver::~ZmqReceiver() {
    should_run_ = false;
    thread_.join();
    thread_.~thread();
}

void ZmqReceiver::process() {}

/*
        Starting to listen to and dispatch ZMQ messages. REceives messages and triggers UI updates
   when ready. ZMQ thread, but also dispatching to UI thread. Time critical (delay & jitter)!
*/
void ZmqReceiver::receiveZMQ() {
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
        bool received = zmq_socket.recv(&address);
        std::string address_string =
            std::string(static_cast<char*>(address.data()), address.size());
        if (received) {
            // Content Message:
            zmq::message_t message;
            zmq_socket.recv(&message);
            std::string message_string =
                std::string(static_cast<char*>(message.data()), message.size());

            try {
                parseMessage(address_string,
                             json::parse(message_string));  // Parse all the messages and change the
                                                            // Mirrors accordingly
                if (future_.wait_for(std::chrono::milliseconds(0)) ==
                    std::future_status::ready) {  // If the UI is ready
                    future_ = dispatchFront(
                        [this, address_string, message_string]() {  // Go to the UI Thread
                            updateUI();                             // Update the UI
                        });
                }
            } catch (...) {
                // Sometimes, the address and message contain incorrect values. This is a
                // HACK to catch them. It is not a clean solution and should be changed!
            }
        }
    }

    zmq_socket.~socket_t();
    context.~context_t();
}

/*
        Updating the UI. This copies values from all mirror Props to all displayed Props and happens
   every time a frame has been drawn. UI thread. Time critical (jitter)!
*/
void ZmqReceiver::updateUI() {
    for (auto i = additionalProps.begin(); i != additionalProps.end(); ++i) {
        PropMapping* pm = *i;
        if (pm->type == PropertyType::boolVal) {
            dynamic_cast<BoolProperty*>(pm->property->getPropertyByIdentifier("value"))
                ->set(dynamic_cast<BoolProperty*>(pm->mirror->getPropertyByIdentifier("value"))
                          ->get());
        } else if (pm->type == PropertyType::intVal) {
            dynamic_cast<IntProperty*>(pm->property->getPropertyByIdentifier("value"))
                ->set(dynamic_cast<IntProperty*>(pm->mirror->getPropertyByIdentifier("value"))
                          ->get());
        } else if (pm->type == PropertyType::floatVal) {
            dynamic_cast<FloatProperty*>(pm->property->getPropertyByIdentifier("value"))
                ->set(dynamic_cast<FloatProperty*>(pm->mirror->getPropertyByIdentifier("value"))
                          ->get());
        } else if (pm->type == PropertyType::intVec2Val) {
            dynamic_cast<IntVec2Property*>(pm->property->getPropertyByIdentifier("value"))
                ->set(dynamic_cast<IntVec2Property*>(pm->mirror->getPropertyByIdentifier("value"))
                          ->get());
        } else if (pm->type == PropertyType::floatVec3Val) {
            dynamic_cast<FloatVec3Property*>(pm->property->getPropertyByIdentifier("value"))
                ->set(dynamic_cast<FloatVec3Property*>(pm->mirror->getPropertyByIdentifier("value"))
                          ->get());
        } else if (pm->type == PropertyType::stereoCameraVal) {
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
        } else if (pm->type == PropertyType::cameraProjectionVal) {

#define setProperty(PropertyType, SourceName, TargetName)                                         \
    dynamic_cast<##PropertyType*>(pm->property->getPropertyByIdentifier(#SourceName, true))       \
        ->set(                                                                                    \
            dynamic_cast<##PropertyType*>(pm->mirror->getPropertyByIdentifier(#TargetName, true)) \
                ->get())

            setProperty(FloatProperty, FOV, FOV);
            setProperty(FloatProperty, NearPlane, NearPlane);
            setProperty(FloatProperty, FarPlane, FarPlane);
            setProperty(FloatProperty, AspectRatio, AspectRatio);
            setProperty(IntVec2Property, CanvasSize, CanvasSize);
        } else if (pm->type == PropertyType::stereoCameraViewVal) {

            setProperty(FloatVec3Property, lookFromL, lookFromL);
            setProperty(FloatVec3Property, lookToL, lookToL);
            setProperty(FloatVec3Property, lookUpL, lookUpL);

            setProperty(FloatVec3Property, lookFromR, lookFromR);
            setProperty(FloatVec3Property, lookToR, lookToR);
            setProperty(FloatVec3Property, lookUpR, lookUpR);
        } else if (pm->type == PropertyType::transferFunctionVal) {
            setProperty(TransferFunctionProperty, transferFunction, transferFunction);
        }
    }
}

/*
        Parsing Messages from ZMQ. This might be a lot.
        ZMQ thread.
        Time critical (delay)!
*/
void ZmqReceiver::parseMessage(std::string address, json content) {
    for (auto i = additionalProps.begin(); i != additionalProps.end(); ++i) {
        PropMapping* pm = *i;
        if (pm->address == address) {
            switch (pm->type) {
                case PropertyType::boolVal:
                    parseBoolMessage(pm, content);
                    break;
                case PropertyType::intVal:
                    parseIntMessage(pm, content);
                    break;
                case PropertyType::floatVal:
                    parseFloatMessage(pm, content);
                    break;
                case PropertyType::intVec2Val:
                    parseIntVec2Message(pm, content);
                    break;
                case PropertyType::floatVec3Val:
                    parseFloatVec3Message(pm, content);
                    break;
                case PropertyType::stereoCameraVal:
                    parseStereoCameraMessage(pm, content);
                    break;
                case PropertyType::cameraProjectionVal:
                    parseCameraProjectionMessage(pm, content);
                    break;
                case PropertyType::stereoCameraViewVal:
                    parseStereoCameraViewMessage(pm, content);
                    break;
                case PropertyType::transferFunctionVal:
                    parseTransferFunctionMessage(pm, content);
                    break;
                default:
                    break;
            }
        }
    }
}

void ZmqReceiver::parseTransferFunctionMessage(PropMapping* prop, json content) {
    float maskMin = content["maskMin"];
    float maskMax = content["maskMax"];
    int type = content["type"];
    auto points = content["points"];

    TransferFunction tf = TransferFunction();

    for (auto it = points.begin(); it != points.end(); ++it) {
        json point = it.value();
        tf.add(point["pos"], vec4(point["rgba"]["x"], point["rgba"]["y"], point["rgba"]["z"],
                                       point["rgba"]["w"]));
    }

	tf.setMaskMax(maskMax);
    tf.setMaskMin(maskMin);

    // Update mirror
    dynamic_cast<TransferFunctionProperty*>(
        prop->mirror->getPropertyByIdentifier("transferFunction", true))
        ->set(tf);
}

void ZmqReceiver::parseStereoCameraMessage(PropMapping* prop, json content) {
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

void ZmqReceiver::parseStereoCameraViewMessage(PropMapping* prop, json content) {
    // Left Eye
    // Get camera props from unity

    vec3 fromL = vec3(content["leftEyeView"]["eyePos"]["x"], content["leftEyeView"]["eyePos"]["y"],
                      content["leftEyeView"]["eyePos"]["z"]);
    vec3 toL =
        vec3(content["leftEyeView"]["lookAtPos"]["x"], content["leftEyeView"]["lookAtPos"]["y"],
             content["leftEyeView"]["lookAtPos"]["z"]);
    vec3 upL =
        vec3(content["leftEyeView"]["camUpDir"]["x"], content["leftEyeView"]["camUpDir"]["y"],
             content["leftEyeView"]["camUpDir"]["z"]);

    // Update Mirror
    dynamic_cast<FloatVec3Property*>(prop->mirror->getPropertyByIdentifier("lookFromL", true))
        ->set(fromL);
    dynamic_cast<FloatVec3Property*>(prop->mirror->getPropertyByIdentifier("lookToL", true))
        ->set(toL);
    dynamic_cast<FloatVec3Property*>(prop->mirror->getPropertyByIdentifier("lookUpL", true))
        ->set(upL);

    // Right Eye
    // Get camera props from unity
    vec3 fromR =
        vec3(content["rightEyeView"]["eyePos"]["x"], content["rightEyeView"]["eyePos"]["y"],
             content["rightEyeView"]["eyePos"]["z"]);
    vec3 toR =
        vec3(content["rightEyeView"]["lookAtPos"]["x"], content["rightEyeView"]["lookAtPos"]["y"],
             content["rightEyeView"]["lookAtPos"]["z"]);
    vec3 upR =
        vec3(content["rightEyeView"]["camUpDir"]["x"], content["rightEyeView"]["camUpDir"]["y"],
             content["rightEyeView"]["camUpDir"]["z"]);

    // Update Mirror
    dynamic_cast<FloatVec3Property*>(prop->mirror->getPropertyByIdentifier("lookFromR", true))
        ->set(fromR);
    dynamic_cast<FloatVec3Property*>(prop->mirror->getPropertyByIdentifier("lookToR", true))
        ->set(toR);
    dynamic_cast<FloatVec3Property*>(prop->mirror->getPropertyByIdentifier("lookUpR", true))
        ->set(upR);
}

void ZmqReceiver::parseCameraProjectionMessage(PropMapping* prop, json content) {
    float fieldOfViewY_rad = content["fieldOfViewY_rad"];
    float nearClipPlane = content["nearClipPlane"];
    float farClipPlane = content["farClipPlane"];
    float aspect = content["aspect"];
    int pixelWidth = content["pixelWidth"];
    int pixelHeight = content["pixelHeight"];
    ivec2 imageDimensions = ivec2(pixelWidth, pixelHeight);

    dynamic_cast<FloatProperty*>(prop->mirror->getPropertyByIdentifier("FOV", true))
        ->set(glm::degrees(fieldOfViewY_rad));
    dynamic_cast<FloatProperty*>(prop->mirror->getPropertyByIdentifier("NearPlane", true))
        ->set(nearClipPlane);
    dynamic_cast<FloatProperty*>(prop->mirror->getPropertyByIdentifier("FarPlane", true))
        ->set(farClipPlane);
    dynamic_cast<FloatProperty*>(prop->mirror->getPropertyByIdentifier("AspectRatio", true))
        ->set(aspect);
    dynamic_cast<IntVec2Property*>(prop->mirror->getPropertyByIdentifier("CanvasSize", true))
        ->set(imageDimensions);
}

void ZmqReceiver::parseBoolMessage(PropMapping* prop, json content) {
    bool value = true;   
	if(content["value"] != "true") value = false;
    dynamic_cast<BoolProperty*>(prop->mirror->getPropertyByIdentifier("value"))->set(value);
}

void ZmqReceiver::parseFloatMessage(PropMapping* prop, json content) {
    float value = content["value"];
    dynamic_cast<FloatProperty*>(prop->mirror->getPropertyByIdentifier("value"))->set(value);
}

void ZmqReceiver::parseIntMessage(PropMapping* prop, json content) {
    int value = content["value"];
    dynamic_cast<IntProperty*>(prop->mirror->getPropertyByIdentifier("value"))->set(value);
}

void ZmqReceiver::parseIntVec2Message(PropMapping* prop, json content) {
    ivec2 value = ivec2(content["value"]["x"], content["value"]["y"]);
    dynamic_cast<IntVec2Property*>(prop->mirror->getPropertyByIdentifier("value"))->set(value);
}

void ZmqReceiver::parseFloatVec3Message(PropMapping* prop, json content) {
    vec3 value = vec3(content["value"]["x"], content["value"]["y"], content["value"]["z"]);
    dynamic_cast<FloatVec3Property*>(prop->mirror->getPropertyByIdentifier("value"))->set(value);
}

/*
        Add the selected Property type to the Processor.
        UI thread.
        This only happens on User Interaction and should thus not be time critical.
*/
void ZmqReceiver::addSelectedProperty() {
    if (name_.get() != "" && address_.get() != "") {
        // Create a new Composite Property with the matching address
        CompositeProperty* newComp = new CompositeProperty(name_.get(), name_.get());
        CompositeProperty* newMirror = new CompositeProperty(name_.get(), name_.get());
        // Create the PropertyMapping for later modifications
        PropMapping* pm = new PropMapping(address_.get(), newComp, newMirror);
        // Set the name and address values back to none.
        address_.set("");
        name_.set("");

        // Add type-specific props
        std::string selectedType = type_.getSelectedDisplayName();
        if (selectedType == "Bool") {
            pm->type = PropertyType::boolVal;
            addBoolProperty(newComp, newMirror);
        } else if (selectedType == "Float") {
            pm->type = PropertyType::floatVal;
            addFloatProperty(newComp, newMirror);
        } else if (selectedType == "Int") {
            pm->type = PropertyType::intVal;
            addIntProperty(newComp, newMirror);
        } else if (selectedType == "IntVec2") {
            pm->type = PropertyType::intVec2Val;
            addIntVec2Property(newComp, newMirror);
        } else if (selectedType == "FloatVec3") {
            pm->type = PropertyType::floatVec3Val;
            addFloatVec3Property(newComp, newMirror);
        } else if (selectedType == "Stereo Camera") {
            pm->type = PropertyType::stereoCameraVal;
            addStereoCameraProperty(newComp, newMirror);
        } else if (selectedType == "CameraProjection") {
            pm->type = PropertyType::cameraProjectionVal;
            addCameraProjectionProperty(newComp, newMirror);
        } else if (selectedType == "StereoCameraView") {
            pm->type = PropertyType::stereoCameraViewVal;
            addStereoCameraViewProperty(newComp, newMirror);
        } else if (selectedType == "TransferFunction") {
            pm->type = PropertyType::transferFunctionVal;
            addTransferFunctionProperty(newComp, newMirror);
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

void ZmqReceiver::addBoolProperty(CompositeProperty* newComp, CompositeProperty* newMirror) {
    BoolProperty* newProp = new BoolProperty("value", "Value", false);
    newProp->setSerializationMode(PropertySerializationMode::All);
    BoolProperty* newMirrorProp = new BoolProperty("value", "Value", false);
    newMirrorProp->setSerializationMode(PropertySerializationMode::All);
    newComp->addProperty(newProp);
    newMirror->addProperty(newMirrorProp);
}

void ZmqReceiver::addFloatProperty(CompositeProperty* newComp, CompositeProperty* newMirror) {
    FloatProperty* newProp = new FloatProperty("value", "Value", 0.0, -10000.0, 10000.0);
    newProp->setSerializationMode(PropertySerializationMode::All);
    FloatProperty* newMirrorProp = new FloatProperty("value", "Value", 0.0, -10000.0, 10000.0);
    newMirrorProp->setSerializationMode(PropertySerializationMode::All);
    newComp->addProperty(newProp);
    newMirror->addProperty(newMirrorProp);
}

void ZmqReceiver::addIntProperty(CompositeProperty* newComp, CompositeProperty* newMirror) {
    IntProperty* newProp = new IntProperty("value", "Value", 0, -10000, 10000);
    newProp->setSerializationMode(PropertySerializationMode::All);
    IntProperty* newMirrorProp = new IntProperty("value", "Value", 0, -10000, 10000);
    newMirrorProp->setSerializationMode(PropertySerializationMode::All);
    newComp->addProperty(newProp);
    newMirror->addProperty(newMirrorProp);
}

void ZmqReceiver::addIntVec2Property(CompositeProperty* newComp, CompositeProperty* newMirror) {
    IntVec2Property* newProp =
        new IntVec2Property("value", "Value", ivec2(0), -ivec2(10000), ivec2(10000));
    newProp->setSerializationMode(PropertySerializationMode::All);
    IntVec2Property* newMirrorProp =
        new IntVec2Property("value", "Value", ivec2(0), -ivec2(10000), ivec2(10000));
    newMirrorProp->setSerializationMode(PropertySerializationMode::All);
    newComp->addProperty(newProp);
    newMirror->addProperty(newMirrorProp);
}

void ZmqReceiver::addFloatVec3Property(CompositeProperty* newComp, CompositeProperty* newMirror) {
    FloatVec3Property* newProp = new FloatVec3Property(
        "value", "Value", vec3(0.0f), -vec3(10000.0f), vec3(10000.0f), vec3(0.00001f));
    newProp->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newMirrorProp = new FloatVec3Property(
        "value", "Value", vec3(0.0f), -vec3(10000.0f), vec3(10000.0f), vec3(0.00001f));
    newMirrorProp->setSerializationMode(PropertySerializationMode::All);
    newComp->addProperty(newProp);
    newMirror->addProperty(newMirrorProp);
}

void ZmqReceiver::addStereoCameraProperty(CompositeProperty* newComp,
                                          CompositeProperty* newMirror) {
    // Left Camera
    CompositeProperty* cameraL = new CompositeProperty("camparamsL", "Camera Parameters L");
    cameraL->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newFromPropL = new FloatVec3Property(
        "lookFromL", "Look From L", vec3(1.0f), -vec3(10000.0f), vec3(10000.0f), vec3(0.00001f),
        InvalidationLevel::Valid, PropertySemantics("Spherical"));
    newFromPropL->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newToPropL = new FloatVec3Property("lookToL", "Look to L", vec3(0.0f),
                                                          -vec3(10000.0f), vec3(10000.0f), vec3(0.00001f));
    newToPropL->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newUpPropL = new FloatVec3Property(
        "lookUpL", "Look up L", vec3(0.0f, 1.0f, 0.0f), -vec3(1000.0f), vec3(1000.0f), vec3(0.00001f));
    newUpPropL->setSerializationMode(PropertySerializationMode::All);
    cameraL->addProperty(newFromPropL);
    cameraL->addProperty(newToPropL);
    cameraL->addProperty(newUpPropL);
    newComp->addProperty(cameraL);
    // Left Camera Mirror
    CompositeProperty* cameraLMirror = new CompositeProperty("camparamsL", "Camera Parameters L");
    cameraLMirror->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newFromPropLMirror = new FloatVec3Property(
        "lookFromL", "Look From L", vec3(1.0f), -vec3(10000.0f), vec3(10000.0f), vec3(0.00001f),
        InvalidationLevel::Valid, PropertySemantics("Spherical"));
    newFromPropLMirror->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newToPropLMirror = new FloatVec3Property(
        "lookToL", "Look to L", vec3(0.0f), -vec3(10000.0f), vec3(10000.0f), vec3(0.00001f));
    newToPropLMirror->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newUpPropLMirror = new FloatVec3Property(
        "lookUpL", "Look up L", vec3(0.0f, 1.0f, 0.0f), -vec3(1000.0f), vec3(1000.0f), vec3(0.0001f));
    newUpPropLMirror->setSerializationMode(PropertySerializationMode::All);
    cameraLMirror->addProperty(newFromPropLMirror);
    cameraLMirror->addProperty(newToPropLMirror);
    cameraLMirror->addProperty(newUpPropLMirror);
    newMirror->addProperty(cameraLMirror);

    // Right Camera
    CompositeProperty* cameraR = new CompositeProperty("camparamsR", "Camera Parameters R");
    cameraR->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newFromPropR = new FloatVec3Property(
        "lookFromR", "Look From R", vec3(1.0f), -vec3(10000.0f), vec3(10000.0f), vec3(0.00001f),
        InvalidationLevel::Valid, PropertySemantics("Spherical"));
    newFromPropR->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newToPropR = new FloatVec3Property("lookToR", "Look to R", vec3(0.0f),
                                                          -vec3(10000.0f), vec3(10000.0f), vec3(0.00001f));
    newToPropR->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newUpPropR = new FloatVec3Property(
        "lookUpR", "Look up R", vec3(0.0f, 1.0f, 0.0f), -vec3(1000.0f), vec3(1000.0f), vec3(0.00001f));
    newUpPropR->setSerializationMode(PropertySerializationMode::All);
    cameraR->addProperty(newFromPropR);
    cameraR->addProperty(newToPropR);
    cameraR->addProperty(newUpPropR);
    newComp->addProperty(cameraR);
    // Right Camera Mirror
    CompositeProperty* cameraRMirror = new CompositeProperty("camparamsR", "Camera Parameters R");
    cameraRMirror->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newFromPropRMirror = new FloatVec3Property(
        "lookFromR", "Look From R", vec3(1.0f), -vec3(10000.0f), vec3(10000.0f), vec3(0.00001f),
        InvalidationLevel::Valid, PropertySemantics("Spherical"));
    newFromPropRMirror->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newToPropRMirror = new FloatVec3Property(
        "lookToR", "Look to R", vec3(0.0f), -vec3(10000.0f), vec3(10000.0f), vec3(0.00001f));
    newToPropRMirror->setSerializationMode(PropertySerializationMode::All);
    FloatVec3Property* newUpPropRMirror = new FloatVec3Property(
        "lookUpR", "Look up R", vec3(0.0f, 1.0f, 0.0f), -vec3(1000.0f), vec3(1000.0f), vec3(0.00001f));
    newUpPropRMirror->setSerializationMode(PropertySerializationMode::All);
    cameraRMirror->addProperty(newFromPropRMirror);
    cameraRMirror->addProperty(newToPropRMirror);
    cameraRMirror->addProperty(newUpPropRMirror);
    newMirror->addProperty(cameraRMirror);
}

void ZmqReceiver::addCameraProjectionProperty(CompositeProperty* newComp,
                                              CompositeProperty* newMirror) {
    FloatProperty* newPropAR =
        new FloatProperty("AspectRatio", "Aspect Ratio", 1.0, -10000.0, 10000.0);
    newPropAR->setSerializationMode(PropertySerializationMode::All);
    FloatProperty* newMirrorPropAR =
        new FloatProperty("AspectRatio", "Aspect Ratio", 1.0, -10000.0, 10000.0);
    newMirrorPropAR->setSerializationMode(PropertySerializationMode::All);
    newComp->addProperty(newPropAR);
    newMirror->addProperty(newMirrorPropAR);

    FloatProperty* newPropNP = new FloatProperty("NearPlane", "Near Plane", 0.5, -10000.0, 10000.0);
    newPropNP->setSerializationMode(PropertySerializationMode::All);
    FloatProperty* newMirrorPropNP =
        new FloatProperty("NearPlane", "Near Plane", 0.5, -10000.0, 10000.0);
    newMirrorPropNP->setSerializationMode(PropertySerializationMode::All);
    newComp->addProperty(newPropNP);
    newMirror->addProperty(newMirrorPropNP);

    FloatProperty* newPropFP = new FloatProperty("FarPlane", "Far Plane", 10.0, -10000.0, 10000.0);
    newPropFP->setSerializationMode(PropertySerializationMode::All);
    FloatProperty* newMirrorPropFP =
        new FloatProperty("FarPlane", "Far Plane", 10.0, -10000.0, 10000.0);
    newMirrorPropFP->setSerializationMode(PropertySerializationMode::All);
    newComp->addProperty(newPropFP);
    newMirror->addProperty(newMirrorPropFP);

    FloatProperty* newPropFOV = new FloatProperty("FOV", "FOV", 30.0, -10000.0, 10000.0);
    newPropFOV->setSerializationMode(PropertySerializationMode::All);
    FloatProperty* newMirrorPropFOV = new FloatProperty("FOV", "FOV", 30.0, -10000.0, 10000.0);
    newMirrorPropFOV->setSerializationMode(PropertySerializationMode::All);
    newComp->addProperty(newPropFOV);
    newMirror->addProperty(newMirrorPropFOV);

    IntVec2Property* newProp =
        new IntVec2Property("CanvasSize", "Canvas Size", ivec2(10), -ivec2(10000), ivec2(10000));
    newProp->setSerializationMode(PropertySerializationMode::All);
    IntVec2Property* newMirrorProp =
        new IntVec2Property("CanvasSize", "Canvas Size", ivec2(10), -ivec2(10000), ivec2(10000));
    newMirrorProp->setSerializationMode(PropertySerializationMode::All);
    newComp->addProperty(newProp);
    newMirror->addProperty(newMirrorProp);
}

void ZmqReceiver::addStereoCameraViewProperty(CompositeProperty* newComp,
                                              CompositeProperty* newMirror) {
    this->addStereoCameraProperty(newComp, newMirror);
}

void ZmqReceiver::addTransferFunctionProperty(CompositeProperty* newComp,
                                              CompositeProperty* newMirror) {
    TransferFunctionProperty* transferFunction =
        new TransferFunctionProperty("transferFunction", "Transfer Function");
    transferFunction->setSerializationMode(PropertySerializationMode::All);

    TransferFunctionProperty* transferFunctionMirror =
        new TransferFunctionProperty("transferFunction", "Transfer Function");
    transferFunction->setSerializationMode(PropertySerializationMode::All);

    newComp->addProperty(transferFunction);
    newMirror->addProperty(transferFunctionMirror);
}

/*
        Serialization Methods for the Processor. Calling the serialization of all PropMappings.
*/
void ZmqReceiver::serialize(Serializer& s) const {
    s.serialize("propMapping", additionalProps, "propmapping");
    Processor::serialize(s);
}

void ZmqReceiver::deserialize(Deserializer& d) {
    d.deserialize("propMapping", additionalProps, "propmapping");
    Processor::deserialize(d);
}
}  // namespace inviwo
