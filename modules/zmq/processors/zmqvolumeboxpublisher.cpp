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
    , volume_("volume") {

    addPort(volume_);

    zmq::context_t context(1);
    zmq::socket_t box_socket(context, ZMQ_PUB);
	box_socket.bind("tcp://*:12345");
	//std::string str = "inviwo";
    //zmq::message_t message(str.size());
    //memcpy(message.data(), str.data(), str.size());
	std::string str2 = "Hallo";
    zmq::message_t message2(str2.size());
    memcpy(message2.data(), str2.data(), str2.size());
		
    //box_socket.send(message, ZMQ_SNDMORE);
    bool rc = box_socket.send(message2);

    LogWarn(rc);
}

ZmqVolumeBoxProcessor::~ZmqVolumeBoxProcessor() {}

void ZmqVolumeBoxProcessor::process() {
}

void ZmqVolumeBoxProcessor::sendZMQ() { 
	/*std::string str = "Hallo";
    zmq::message_t message(str.size());
	memcpy(message.data(), str.data(), str.size());
    LogInfo(str.data());
    LogInfo(socket_.send(message));*/
}

void ZmqVolumeBoxProcessor::packMessage() {
}
}  // namespace inviwo
