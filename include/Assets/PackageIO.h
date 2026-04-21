#pragma once
#ifndef MIST_PACKAGE_IO_H
#define MIST_PACKAGE_IO_H

#include <string>

// `.mistpkg` archive — single JSON file that bundles a scene and all
// of its referenced assets (textures + materials) as base64 blobs.
// Chosen over zlib/zip for zero new dependencies; the single-file
// shape still delivers the portability story (move the .mistpkg
// between machines, reopen, everything resolves).
//
// Layout:
//   {
//     "version": "1.0",
//     "scene":   { ... entire scene JSON inline ... },
//     "assets": {
//       "textures/foo.png":      "<base64>",
//       "materials/bar.mistmat": "<base64>"
//     }
//   }
//
// On Import we extract assets to a temp dir under $TMPDIR and rewrite
// the scene's asset paths to the temp dir before loading, so the
// existing asset loaders work unchanged.

namespace Mist::Assets {

struct PackageIO {
    // Export: reads `scenePath` (.mist), walks all material refs +
    // their texture refs, embeds them as base64 in the output
    // `pkgPath` (.mistpkg). Returns true on success.
    static bool Export(const std::string& scenePath, const std::string& pkgPath);

    // Import: reads `pkgPath`, extracts every asset entry into a
    // fresh temp directory, then writes the patched scene.mist into
    // the same dir. Writes the resolved scene path into `outScenePath`.
    // Returns true on success.
    static bool Import(const std::string& pkgPath, std::string& outScenePath);
};

} // namespace Mist::Assets

#endif // MIST_PACKAGE_IO_H
