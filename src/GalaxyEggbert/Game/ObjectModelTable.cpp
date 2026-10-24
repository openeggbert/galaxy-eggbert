#include "ObjectModelTable.hpp"

#include <sstream>

namespace GalaxyEggbert::Game
{
    bool ObjectModelTable::ParseSheet(const std::string& name, ObjectTextureSource& sheet)
    {
        if (name == "element") sheet = ObjectTextureSource::Element;
        else if (name == "object-m") sheet = ObjectTextureSource::ObjectM;
        else if (name == "explo") sheet = ObjectTextureSource::Explo;
        else if (name == "blupi") sheet = ObjectTextureSource::Blupi;
        else if (name == "blupi1") sheet = ObjectTextureSource::Blupi1;
        else return false;
        return true;
    }

    std::uint32_t ObjectModelTable::Key(ObjectTextureSource sheet, int icon) noexcept
    {
        return (static_cast<std::uint32_t>(sheet) << 16) | (static_cast<std::uint32_t>(icon) & 0xFFFFu);
    }

    bool ObjectModelTable::AddModel(int model, std::istream& iconsTxt, std::string* error)
    {
        std::string line;
        while (std::getline(iconsTxt, line))
        {
            const auto hash = line.find('#');
            std::istringstream in(line.substr(0, hash));
            std::string kind;
            if (!(in >> kind))
            {
                continue;
            }
            std::string sheetName;
            int icon = -1;
            int mirrored = 0;
            ObjectModelPose pose;
            pose.model = model;
            ObjectTextureSource sheet{};
            if (kind != "icon" || !(in >> sheetName >> icon >> pose.seconds >> mirrored >> pose.yawDegrees) ||
                !ParseSheet(sheetName, sheet) || icon < 0 || pose.seconds < 0.0f || (mirrored != 0 && mirrored != 1))
            {
                if (error != nullptr)
                {
                    *error = line;
                }
                return false;
            }
            pose.mirrored = mirrored == 1;
            poses_[Key(sheet, icon)] = pose;
        }
        return true;
    }

    const ObjectModelPose* ObjectModelTable::Find(ObjectTextureSource sheet, int icon) const
    {
        if (icon < 0)
        {
            return nullptr;
        }
        const auto it = poses_.find(Key(sheet, icon));
        return it == poses_.end() ? nullptr : &it->second;
    }
}
