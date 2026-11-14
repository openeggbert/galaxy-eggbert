#include "PlatformFiles.hpp"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>

// The browser side (EM_JS: the bodies are JavaScript).
EM_JS(int, galaxy_offer_download, (const char* file, const char* name), {
    try {
        var data = FS.readFile(UTF8ToString(file));
        var url = URL.createObjectURL(new Blob([data], {type: 'application/octet-stream'}));
        var link = document.createElement('a');
        link.href = url;
        link.download = UTF8ToString(name);
        document.body.appendChild(link);
        link.click();
        link.remove();
        setTimeout(function() { URL.revokeObjectURL(url); }, 10000);
        return 1;
    } catch (e) {
        console.error('Galaxy Eggbert: export failed', e);
        return 0;
    }
});

EM_JS(void, galaxy_request_upload, (const char* directory, const char* extension), {
    var dir = UTF8ToString(directory);
    var ext = UTF8ToString(extension);
    var input = document.createElement('input');
    input.type = 'file';
    input.accept = ext;
    input.onchange = function() {
        var file = input.files && input.files[0];
        if (!file) {
            return;
        }
        var reader = new FileReader();
        reader.onload = function() {
            var name = file.name.replace(/[^A-Za-z0-9._-]/g, '_');
            if (name.slice(-ext.length).toLowerCase() !== ext.toLowerCase()) {
                name += ext;
            }
            try {
                FS.mkdirTree(dir);
            } catch (e) {
            }
            // A file of that name already there stays: name-2.ext, name-3.ext ...
            var stem = name.slice(0, name.length - ext.length);
            for (var n = 2; FS.analyzePath(dir + '/' + name).exists && n < 10000; ++n) {
                name = stem + '-' + n + ext;
            }
            FS.writeFile(dir + '/' + name, new Uint8Array(reader.result));
            Module['galaxyUploadDone'] = 1;
            console.log('Galaxy Eggbert: imported ' + dir + '/' + name);
        };
        reader.readAsArrayBuffer(file);
    };
    input.click();
});

EM_JS(int, galaxy_consume_upload_done, (), {
    var done = Module['galaxyUploadDone'] | 0;
    Module['galaxyUploadDone'] = 0;
    return done;
});
#endif

namespace GalaxyEggbert::Editor
{
#if defined(__EMSCRIPTEN__)
    bool OfferDownload(const std::filesystem::path& file, const std::string& name)
    {
        return galaxy_offer_download(file.string().c_str(), name.c_str()) != 0;
    }

    bool RequestUpload(const std::filesystem::path& directory, const std::string& extension)
    {
        galaxy_request_upload(directory.string().c_str(), extension.c_str());
        return true;
    }

    bool ConsumeUploadDone()
    {
        return galaxy_consume_upload_done() != 0;
    }
#else
    bool OfferDownload(const std::filesystem::path&, const std::string&)
    {
        return false;
    }

    bool RequestUpload(const std::filesystem::path&, const std::string&)
    {
        return false;
    }

    bool ConsumeUploadDone()
    {
        return false;
    }
#endif
}

namespace GalaxyEggbert::Editor
{
    bool FileUploadAvailable() noexcept
    {
#if defined(__EMSCRIPTEN__)
        return true;
#else
        return false;
#endif
    }
}
