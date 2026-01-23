#ifndef RADAR_UTILS_H
#define RADAR_UTILS_H

#include "radar_types.h"

namespace radar {
    double calculateEnergy(const AudioFrame& frame);
} // namespace radar

#endif // RADAR_UTILS_H