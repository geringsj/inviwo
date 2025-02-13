/*********************************************************************************
 *
 * Inviwo - Interactive Visualization Workshop
 *
 * Copyright (c) 2021 Inviwo Foundation
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

#include <modules/brushingandlinking/datastructures/brushingaction.h>

#include <iostream>
#include <mutex>
#include <algorithm>

namespace inviwo {

const BrushingTarget BrushingTarget::Row("row");
const BrushingTarget BrushingTarget::Column("column");

std::ostream& operator<<(std::ostream& os, BrushingTarget bt) {
    os << bt.getString();
    return os;
}

inline std::string_view BrushingTarget::findOrAdd(std::string_view target) {
    static std::mutex mutex;
    static std::vector<std::unique_ptr<const std::string>> targets{};
    std::scoped_lock lock{mutex};
    const auto it =
        std::find_if(targets.begin(), targets.end(),
                     [&](const std::unique_ptr<const std::string>& ptr) { return *ptr == target; });
    if (it == targets.end()) {
        return std::string_view{*targets.emplace_back(std::make_unique<const std::string>(target))};
    } else {
        return std::string_view{**it};
    }
}

}  // namespace inviwo
