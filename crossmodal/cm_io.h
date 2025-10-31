/**
 * @file cm_io.h
 */

#ifndef MELVIN_CROSSMODAL_CM_IO_H
#define MELVIN_CROSSMODAL_CM_IO_H

#include <string>
#include "cm_grounder.h"

namespace melvin {
namespace crossmodal {

class CMIO {
public:
    static bool LoadVisionMap(const std::string& path, CMGrounder& g);
    static bool LoadAudioMap(const std::string& path, CMGrounder& g);
    static bool LoadMotorMap(const std::string& path, CMGrounder& g);
    static bool ExportBindingsTSV(const std::string& path, const CMBindings& b);
};

} // namespace crossmodal
} // namespace melvin

#endif // MELVIN_CROSSMODAL_CM_IO_H


