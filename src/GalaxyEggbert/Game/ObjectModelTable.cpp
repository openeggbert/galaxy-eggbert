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
            ObjectModelPose pose;
            pose.model = model;
            ObjectTextureSource sheet{};
            if (kind != "icon" || !(in >> sheetName >> icon >> pose.seconds >> pose.mirror >> pose.yawDegrees) ||
                !ParseSheet(sheetName, sheet) || icon < 0 || pose.seconds < 0.0f || pose.mirror < 0 ||
                pose.mirror > 2)
            {
                if (error != nullptr)
                {
                    *error = line;
                }
                return false;
            }
            float iou = 0.0f;
            int pixels = 0;
            if (in >> iou && in >> pixels && pixels > 0)
            {
                pose.iconPixels = pixels;
                int dx = 0;
                int dy = 0;
                if (in >> dx >> dy)
                {
                    pose.offsetX = dx;
                    pose.offsetY = dy;
                }
            }
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
