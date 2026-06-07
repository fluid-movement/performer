#pragma once

#include "Config.h"
#include "Types.h"
#include "Serialize.h"
#include "Routing.h"
#include "FileDefs.h"
#include "core/utils/StringUtils.h"


// NOTE: CurveTrack defines its own FillMode enum (Variation/Invert/NextPattern/None)
// which is NOT the same as the FillMode in Note/Arp/Stochastic (Gates/Condition/NextPattern/None).
// Do NOT consolidate the two enums.
class BaseTrack {
public:
    static constexpr size_t NameLength = FileHeader::NameLength;

    // trackName
    const char *name() const { return _name; }
    void setName(const char *name) {
        StringUtils::copy(_name, name, sizeof(_name));
    }
    
    protected:
        char _name[NameLength + 1];
};