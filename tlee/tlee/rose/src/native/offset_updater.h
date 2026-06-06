#pragma once
#include <string>

namespace offset_updater
{
    // Fetches the latest offsets from imtheo.lol and updates the variables in the Offsets namespace.
    // Returns true if successful, false otherwise.
    bool update_offsets();
}
