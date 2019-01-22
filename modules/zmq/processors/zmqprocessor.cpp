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

namespace inviwo {

const ProcessorInfo Zmq::processorInfo_{
    "org.inviwo.Zmq",       // Class identifier
    "Zmq",                  // Display name
    "Image Operation",        // Category
    CodeState::Experimental,  // Code state
    Tags::GL,                 // Tags
};
const ProcessorInfo Zmq::getProcessorInfo() const { return processorInfo_; }

Zmq::Zmq() 
	: Processor() 
	, ctx_(1)
	, camera_socket_(ctx_, ZMQ_SUB)
	, user_socket_(ctx_, ZMQ_SUB)
	, should_run_(true)
    , camParams_("cameraParameters", "Camera Parameters")
    , camera_pos_("camerapos", "Camera Pos", vec3(0.0), vec3(-1.0), vec3(1.0), vec3(0.001), InvalidationLevel::Valid)
    , camera_rot_("camerarot", "Camera Rot", vec4(0.0), vec4(-1.0), vec4(1.0), vec4(0.001), InvalidationLevel::Valid)
    , userParams_("userParameters", "User Parameters")
    , user_pos_("userpos", "User Pos", vec3(0.0), vec3(-1.0), vec3(1.0), vec3(0.001), InvalidationLevel::Valid)
    , user_rot_("userrot", "User Rot", vec4(0.0), vec4(-1.0), vec4(1.0), vec4(0.001), InvalidationLevel::Valid) {

	addProperty(camParams_);
	camParams_.addProperty(camera_pos_);
	camParams_.addProperty(camera_rot_);
    addProperty(userParams_);
	userParams_.addProperty(user_pos_);
	userParams_.addProperty(user_rot_);

    camera_socket_.setsockopt(ZMQ_IDENTITY, "Inviwo", 6);
    camera_socket_.setsockopt(ZMQ_SUBSCRIBE, "", 0);
    camera_socket_.connect("tcp://localhost:12345");
	
    user_socket_.setsockopt(ZMQ_IDENTITY, "Inviwo", 6);
    user_socket_.setsockopt(ZMQ_SUBSCRIBE, "", 0);
    user_socket_.connect("tcp://localhost:12346");

    receiveZMQ();
}

Zmq::~Zmq() { 
	should_run_ = false;
}

void Zmq::process() { receiveZMQ(); }

void Zmq::receiveZMQ() { 
    result_ = dispatchPool([this]() {
        camera_socket_.recv(&address_);
		camera_socket_.recv(&message_);
    	camera_address_string_ = std::string(static_cast<char*>(address_.data()), address_.size());
        j_camera = json::parse(std::string(static_cast<char*>(message_.data()), message_.size()));
        dispatchFront([this]() {
            if (camera_address_string_ == "camPos") {
                camera_pos_.set(vec3(j_camera["x"], j_camera["y"], j_camera["z"]));
			} else if (camera_address_string_ == "camRot") {
                camera_rot_.set(vec4(j_camera["x"], j_camera["y"], j_camera["z"], j_camera["w"]));
			}
        });

        user_socket_.recv(&address_);
		user_socket_.recv(&message_);
    	user_address_string_ = std::string(static_cast<char*>(address_.data()), address_.size());
        j_user = json::parse(std::string(static_cast<char*>(message_.data()), message_.size()));
        dispatchFront([this]() {
            if (user_address_string_ == "userPos") {
                user_pos_.set(vec3(j_user["x"], j_user["y"], j_user["z"]));
			} else if (user_address_string_ == "userRot") {
                user_rot_.set(vec4(j_user["x"], j_user["y"], j_user["z"], j_user["w"]));
			}
        });

		dispatchFront([this]() {
            invalidate(InvalidationLevel::InvalidOutput); 
		});
    });
}
}  // namespace inviwo
