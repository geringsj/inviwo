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

#include <modules/zmq/processors/zmqvolumeboxpublisher.h>
#include <inviwo/core/common/inviwoapplication.h>
#include <inviwo/core/network/networklock.h>
#include <limits>
#include <Windows.h>

namespace inviwo {

const ProcessorInfo ZmqVolumeBoxProcessor::processorInfo_{
    "org.inviwo.ZmqVolumeBoxProcessor",         // Class identifier
    "ZmqVolumeBoxProcessor",                    // Display name
    "Image Operation",        // Category
    CodeState::Experimental,  // Code state
    Tags::GL,                 // Tags
};
const ProcessorInfo ZmqVolumeBoxProcessor::getProcessorInfo() const { return processorInfo_; }

ZmqVolumeBoxProcessor::ZmqVolumeBoxProcessor()
    : Processor()
    , volume_("volume") 
	, context(zmq::context_t(1))
	, box_socket(zmq::socket_t(context, ZMQ_PUB)) {

    addPort(volume_);

	box_socket.bind("tcp://*:12345");
}

ZmqVolumeBoxProcessor::~ZmqVolumeBoxProcessor() {}

void ZmqVolumeBoxProcessor::process() { 
	sendZMQ();
}

void ZmqVolumeBoxProcessor::sendZMQ() { 
	std::string str2 = "hallo";
    zmq::message_t message = zmq::message_t(str2.size());
    memcpy(message.data(), str2.data(), str2.size());
	box_socket.send(message);
}

void ZmqVolumeBoxProcessor::packMessage() {
}
}  // namespace inviwo
