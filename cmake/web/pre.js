// pre.js - executed before the Emscripten Module is initialized.
// Mounts IDBFS at /save so that IsolatedStorage writes (SpeedyBlupi save file)
// persist across page reloads via the browser's IndexedDB.
//
// Virtual filesystem layout at runtime (GalaxyEggbertCNA):
//   /Content/backgrounds  - preloaded read-only game backgrounds
//   /Content/icons        - preloaded read-only game icons
//   /Content/sounds       - preloaded read-only game sounds
//   /worlds3d             - preloaded read-only .vwr level files
//   /textures3d           - preloaded read-only galaxy-eggbert-owned textures
//   /avatars3d            - preloaded read-only third-person character model
//   /save                 - IDBFS (persistent; SaveData's savedata.txt lives here)
//
// TODO: call FS.syncfs(false, cb) periodically or on exit to flush IDBFS
//       writes back to IndexedDB.  Currently data is written to the in-memory
//       layer but may not survive a hard reload unless sync is called.

Module['preRun'] = Module['preRun'] || [];
Module['preRun'].push(function () {
    FS.mkdir('/save');
    FS.mount(IDBFS, {}, '/save');

    // Synchronise IDBFS from the persistent store (populate=true means
    // "load existing data into the in-memory VFS before the game starts").
    FS.syncfs(true, function (err) {
        if (err) {
            console.warn('IDBFS preRun sync failed:', err);
        }
    });
});

// Flush IDBFS to IndexedDB when the page is about to unload so that the
// last write of SpeedyBlupi save data is not lost.
window.addEventListener('beforeunload', function () {
    if (typeof FS !== 'undefined' && typeof IDBFS !== 'undefined') {
        FS.syncfs(false, function (err) {
            if (err) {
                console.warn('IDBFS beforeunload sync failed:', err);
            }
        });
    }
});
