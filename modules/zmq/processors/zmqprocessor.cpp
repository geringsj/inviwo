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
#include <inviwo/core/network/networklock.h>
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
    , camera_socket_(ctx_, ZMQ_SUB)
    , should_run_(true)
    , camParams_("camparams", "Camera Parameters")
    , distance_("distance", "Distance", 20.0f, 0.0f, 1000.0f, 0.1f, InvalidationLevel::Valid)
    , cameraLFrom_("lookFromL", "Look From L", vec3(1.0f), -vec3(1000.0f), vec3(1000.0f), vec3(0.0001f),
                  InvalidationLevel::Valid, PropertySemantics("Spherical"))
    , cameraRFrom_("lookFromR", "Look From R", vec3(1.0f), -vec3(1000.0f), vec3(1000.0f), vec3(0.0001f),
                   InvalidationLevel::Valid, PropertySemantics("Spherical"))
    , cameraTo_("lookTo", "Look to", vec3(0.0f), -vec3(100.0f), vec3(100.0f), vec3(0.1f))
    , cameraUp_("lookUp", "Look up", vec3(0.0f, 1.0f, 0.0f), -vec3(100.0f), vec3(100.0f), vec3(0.1f)) {

    addProperty(camParams_);
    camParams_.addProperty(distance_);
    camParams_.addProperty(cameraLFrom_);
    camParams_.addProperty(cameraRFrom_);
    camParams_.addProperty(cameraTo_);
    camParams_.addProperty(cameraUp_);
    cameraLFrom_.setReadOnly(true);
    cameraRFrom_.setReadOnly(true);
    cameraRFrom_.setReadOnly(true);
    cameraTo_.setReadOnly(true);
    cameraUp_.setReadOnly(true);

    camera_socket_.setsockopt(ZMQ_IDENTITY, "Inviwo", 6);
    camera_socket_.setsockopt(ZMQ_SUBSCRIBE, "", 0);
    camera_socket_.connect("tcp://localhost:12345");

    receiveZMQ();
}

Zmq::~Zmq() { should_run_ = false; }

void Zmq::process() {
}

void Zmq::receiveZMQ() {
    dispatchPool([this]() {
        while (should_run_) {
            camera_socket_.recv(&address1_);
            camera_socket_.recv(&message1_);
            camera_socket_.recv(&address2_);
            camera_socket_.recv(&message2_);
        
			if (future.wait_for(0) == std::future_status::ready) {
				future_ = dispatchFront([this]() {
					parseMessage(
						json::parse(std::string(static_cast<char*>(message1_.data()), message1_.size())),
							std::string(static_cast<char*>(address1_.data()), address1_.size()));
					parseMessage(
						json::parse(std::string(static_cast<char*>(message2_.data()), message2_.size())),
							std::string(static_cast<char*>(address2_.data()), address2_.size()));
					invalidate(InvalidationLevel::InvalidOutput);
				});
            }
        }
    });
}

void Zmq::parseMessage(json j_camera, std::string camera_address) {
    if (camera_address == "camVecCamL") {
        vec3 cartesian = vec3(j_camera["x"], j_camera["y"], j_camera["z"]);
        cameraLFrom_.set(distance_.get() * convertPosition(cartesian));
    } else if (camera_address == "camVecCamR") {
        vec3 cartesian = vec3(j_camera["x"], j_camera["y"], j_camera["z"]);
        cameraRFrom_.set(distance_.get() * convertPosition(cartesian));
    }
}

vec3 Zmq::convertPosition(vec3 cartesian) {
    float r = sqrt(pow(cartesian.x, 2.0) + pow(cartesian.y, 2.0) + pow(cartesian.z, 2.0));
    float phi = acos(cartesian.z / r);
    float theta = atan(cartesian.y / cartesian.x);
    vec3 spherical = vec3(r, phi, theta);
    if (cartesian.x <= 0) {
        spherical.y = M_PI + spherical.y;
    } else {
        spherical.y = M_PI - spherical.y;
    }
    float x = spherical.x * sin(spherical.y) * cos(spherical.z);
    float y = spherical.x * sin(spherical.y) * sin(spherical.z);
    float z = spherical.x * cos(spherical.y);
    return (vec3(x, y, z));
}
}  // namespace inviwo
