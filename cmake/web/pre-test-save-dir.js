// pre-test-save-dir.js - executed before the Emscripten Module is initialized.
// Used only by VerifyGESaveData's web build: creates the /save MEMFS
// directory GESaveData's kSavePath ("/save/savedata.txt" under
// __EMSCRIPTEN__) writes into. Unlike the real app's pre.js, this does NOT
// mount IDBFS -- the test runs and exits within one Node process, so plain
// in-memory MEMFS is enough; there is nothing to persist across reloads.

Module['preRun'] = Module['preRun'] || [];
Module['preRun'].push(function () {
    FS.mkdir('/save');
});
