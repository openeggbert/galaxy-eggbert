#include <GalaxyEggbert/Game/ObjectModelTable.hpp>
#include <GalaxyEggbert/Game/BlockModels.hpp>

#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <filesystem>
#include <cstring>
#include <vector>

// Scripted verification of ObjectModelTable (plan.md BB3D-011): icons.txt parsing, lookups by
// sheet and icon, the billboard fallback for unlisted icons, rejected lines, and, given the
// directory of avatars3d/objects/ as the first argument, that every model listed in its
// index.txt has a manifest and a well-formed icons.txt. No graphics context needed.
int main(int argc, char** argv)
{
    using namespace GalaxyEggbert::Game;

    bool allOk = true;
    const auto check = [&allOk](bool cond, const std::string& what)
    {
        std::cout << (cond ? "PASS" : "FAIL") << ": " << what << std::endl;
        if (!cond) allOk = false;
    };

    {
        ObjectModelTable table;
        std::istringstream egg(
            "# egg: Blupi/oeuf.rd4\n"
            "# icon <sheet> <icon> <seconds> <mirror> <yaw degrees> <iou>\n"
            "icon element 21 0.0000 0 10 0.991\n"
            "\n"
            "icon element 24 0.0500 1 230 0.926\n"
            "icon element 48 0 2 0 1.000\n"
            "icon explo 70 0 0 0 0.9 64\n"
            "icon explo 67 0 0 0 0.99 144 -8 2\n");
        check(table.AddModel(3, egg), "a model's icons.txt with comments and a blank line parses");
        check(table.Size() == 5, "five icons listed");
        check(table.Find(ObjectTextureSource::Explo, 67)->offsetX == -8 &&
                  table.Find(ObjectTextureSource::Explo, 67)->offsetY == 2 &&
                  table.Find(ObjectTextureSource::Explo, 70)->offsetX == 0,
              "the sprite's offset in its cell, when given");
        check(table.Find(ObjectTextureSource::Explo, 70)->iconPixels == 64 &&
                  table.Find(ObjectTextureSource::Element, 21)->iconPixels == 0,
              "the icon's size in pixels, when given");
        const auto* pose = table.Find(ObjectTextureSource::Element, 24);
        check(pose != nullptr && pose->model == 3 && std::fabs(pose->seconds - 0.05f) < 1e-6f &&
                  pose->mirror == 1 && std::fabs(pose->yawDegrees - 230.0f) < 1e-4f,
              "element icon 24: model 3 at 0.05 s, mirrored left-right, yaw 230");
        check(table.Find(ObjectTextureSource::Element, 48)->mirror == 2, "element icon 48: upside down");
        check(table.Find(ObjectTextureSource::Element, 22) == nullptr,
              "an unlisted icon has no model (it stays a billboard)");
        check(table.Find(ObjectTextureSource::ObjectM, 21) == nullptr,
              "the same icon number on another sheet is another sprite");
        check(table.Find(ObjectTextureSource::Element, -1) == nullptr, "the invisible frame -1 has no model");

        std::istringstream later("icon element 21 0.1 0 0 0.9\nicon object-m 238 0 0 0 0.8\n");
        check(table.AddModel(5, later), "a second model parses");
        check(table.Find(ObjectTextureSource::Element, 21)->model == 5, "a later model replaces an icon");
        check(table.Find(ObjectTextureSource::ObjectM, 238) != nullptr, "object-m icons are looked up by sheet");
    }

    for (const char* bad : {"icon element 21 0.0", "icon nosheet 21 0 0 0 1", "icon element 21 0 3 0 1",
                            "icon element -3 0 0 0 1", "clip Stop", "icon element 21 -1 0 0 1"})
    {
        ObjectModelTable table;
        std::istringstream in(std::string("icon explo 5 0 0 0 1\n") + bad + "\n");
        std::string error;
        check(!table.AddModel(0, in, &error) && error == bad && table.Find(ObjectTextureSource::Explo, 5) != nullptr,
              std::string("rejects \"") + bad + "\" and keeps the lines before it");
    }

    ObjectTextureSource sheet{};
    check(ObjectModelTable::ParseSheet("blupi1", sheet) && sheet == ObjectTextureSource::Blupi1 &&
              ObjectModelTable::ParseSheet("explo", sheet) && sheet == ObjectTextureSource::Explo &&
              !ObjectModelTable::ParseSheet("Element", sheet),
          "sheet names are the PNGs' lower-case names");

    // The shipped models (avatars3d/objects/), when given.
    if (argc > 1)
    {
        const std::string dir = argv[1];
        std::ifstream index(dir + "/index.txt");
        check(index.good(), dir + "/index.txt exists");
        ObjectModelTable table;
        // Owner's upper-half-only saw: the baked animation poses must never bring
        // vertices back below the flat cut, and that cut must be closed from below.
        {
            const auto saw=std::filesystem::path(dir)/"saw378_scie";
            bool valid=std::filesystem::exists(saw/"upper-half.json");
            int parts=0,capVertices=0;
            for (const auto& file : std::filesystem::directory_iterator(saw))
            {
                const auto name=file.path().filename().string();
                if (!name.starts_with("saw_upper_") || !name.ends_with(".verts.bin")) continue;
                ++parts;
                std::ifstream in(file.path(),std::ios::binary);
                const std::vector<char> data((std::istreambuf_iterator<char>(in)),{});
                valid &= !data.empty() && data.size()%52==0;
                int partCaps=0;
                for (std::size_t at=0;at+52<=data.size();at+=52)
                {
                    float position[3],normal[3];
                    std::memcpy(position,data.data()+at,sizeof(position));
                    std::memcpy(normal,data.data()+at+12,sizeof(normal));
                    valid &= std::isfinite(position[1]) && position[1]>=0 && position[1]<.49f;
                    if (position[1]==0 && normal[1]<-.99f) { ++capVertices; ++partCaps; }
                }
                valid &= partCaps>0;
            }
            check(valid && parts==3 && capVertices>0,"all three saw poses keep only the upper half, with a closed flat base");
            for (const float time : {.05f,.25f,.3f})
            {
                int visible=0;
                for (const auto* name : {"saw_upper_0_0","saw_upper_1_0","saw_upper_2_0"})
                    visible+=IsVisibleSawPart(name,time);
                check(visible==1,"each animated saw icon draws exactly one clipped pose");
            }
        }
        int models = 0;
        for (std::string name; std::getline(index, name);)
        {
            if (name.empty() || name[0] == '#')
            {
                continue;
            }
            check(std::ifstream(dir + "/" + name + "/avatar.skinnedmodel.json").good(),
                  name + " has a SkinnedModelEXT manifest");
            std::ifstream icons(dir + "/" + name + "/icons.txt");
            const auto before = table.Size();
            std::string error;
            check(icons.good() && table.AddModel(models, icons, &error) && table.Size() > before,
                  name + "/icons.txt lists icons and parses" + (error.empty() ? "" : " (bad line: " + error + ")"));
            ++models;
        }
        const auto* unlit = table.Find(ObjectTextureSource::Element, 252);
        for (int icon : {253, 254, 255})
        {
            const auto* lit = table.Find(ObjectTextureSource::Element, icon);
            check(unlit && lit && unlit->model == lit->model && unlit->seconds == lit->seconds &&
                      unlit->mirror == lit->mirror && unlit->yawDegrees == lit->yawDegrees,
                  "dynamite " + std::to_string(icon) + ": same model pose as 252 (spark is a decal)");
        }
        check(models > 0, "index.txt lists models");
        std::cout << models << " models, " << table.Size() << " icons" << std::endl;
    }

    std::cout << (allOk ? "ALL PASS" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
