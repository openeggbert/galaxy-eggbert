#pragma once

/**
 * @file
 * @brief Declaration of a small integration-style world serialization smoke test.
 */

namespace GalaxyEggbert::Worlds
{
    /**
     * @brief Builds a tiny terrain sample, writes it to disk, reads it back, and verifies one block.
     *
     * The function is intended as a manual smoke test for basic world I/O flow.
     *
     * @return `0` when verification succeeds, non-zero otherwise.
     */
    int test_worlds();
}
