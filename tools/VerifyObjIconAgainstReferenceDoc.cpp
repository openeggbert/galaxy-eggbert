// INFRA-003 follow-up (plan.md §7, `REMAKE-ANALYSIS.md` P0-2) -- closes the
// gap an external audit found 2026-07-22 in the already-"done" INFRA-003:
// P0-2 explicitly asks to "cross-check every GetObjIcon array/divisor
// against the reference doc PROGRAMMATICALLY (parse the reference tables,
// compare)". VerifyGetObjIcon.cpp is a regression lock whose expected
// values are transcribed FROM GEObjectIcons.cpp itself -- this tool is the
// missing independent half: it PARSES mobile-eggbert-reference/
// 08-animations.md's own §3.1/§3.2 tables directly (not hand-copied
// numbers) and cross-checks GetObjIcon()'s actual behavior against what
// that doc claims.
//
// The doc's "Frames" column means the raw MODULO BASE of the underlying
// per-type table (`kBulldozer[8]`, `kFollow1[26]`, etc.), not the number of
// visually distinct icon values -- several of these tables deliberately
// hold the SAME icon across 2+ adjacent array slots as an authoring choice
// (e.g. `kBulldozer = {66,66,67,67,66,66,65,65}`, only 4 visually distinct
// values across its own 8 real slots). A first draft of this tool tried
// pure black-box period/segment-counting with NO knowledge of the code at
// all, and got 6 of 24 rows wrong as a result -- confirmed by hand that
// this is a genuine, information-theoretic limit, not a bug in that
// approach: a repeated-value array and a genuinely-shorter array produce
// IDENTICAL output sequences, so no amount of black-box observation can
// tell them apart. Resolved (explicit user decision, 2026-07-22) by
// reading just the per-type DIVISOR from GEObjectIcons.cpp's own case
// bodies below (`kDivisors`) -- a small, structural fact analogous to a
// function's calling convention, not the expected animation DATA itself
// (which stays fully independent: the actual FRAME COUNT this tool
// compares against the doc is still measured live from GetObjIcon()'s own
// behavior, sampled at the right stride, never hand-copied).
//
// §3.1/§3.2 only (not §4, explosions) -- the doc's own §4 text explicitly
// says "which ObjectType triggers which explo1..8 table was not resolved
// in this pass", so there is no ObjectType->table mapping to cross-check
// against for explosions without inventing one here.
//
// One deliberate manual correction, not silent guessing: the doc's own
// "96 (follower, awake/homing)" row is a labeling artifact -- the real
// awake/homing table (`kFollow2`) is actually keyed by ObjectType97 in the
// code (the object's own `obj.type` field genuinely changes from 96 to 97
// on waking, confirmed in GEInteractionSystem.cpp's own wake-up transition
// logic), not still 96. Mapped explicitly below, not inferred.
#include "Game/GEObjectIcons.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <string>
#include <vector>

using GalaxyEggbert::ObjectType;
using namespace GalaxyEggbert::CNA;

namespace
{
    int checksRun = 0;
    int checksFailed = 0;

    void check(bool condition, const std::string& message)
    {
        ++checksRun;
        if (!condition)
        {
            ++checksFailed;
            std::cout << "FAIL: " << message << std::endl;
        }
    }

    struct DocRow
    {
        int objectType;
        int expectedFrames;
        std::string label;
    };

    // Parses every "| N (description) | F ... | ..." row from the doc's
    // §3.1/§3.2 tables (both share this exact column shape: ObjectType |
    // Frames | ...). Stops collecting once §4's own header line is seen,
    // so explosion-table rows (a different column shape entirely) are
    // never misparsed as ObjectType rows.
    std::vector<DocRow> ParseReferenceDoc(const std::string& path)
    {
        std::ifstream in(path);
        std::vector<DocRow> rows;
        if (!in)
        {
            return rows;
        }
        // Matches "| 2 (patrol enemy A) | 9 | 1.0 s | ..." and
        // "| 5 (treasure sparkle) | 22 (11-icon ping-pong) | ...":
        // group 1 = ObjectType number, group 2 = description,
        // group 3 = Frames number (the leading integer of that cell).
        const std::regex rowPattern(R"(^\|\s*(\d+)\s*\(([^)]*)\)\s*\|\s*(\d+))");
        std::string line;
        bool inSection3 = false;
        while (std::getline(in, line))
        {
            if (line.rfind("## 3.", 0) == 0)
            {
                inSection3 = true;
            }
            if (line.rfind("## 4.", 0) == 0)
            {
                break;
            }
            if (!inSection3)
            {
                continue;
            }
            std::smatch m;
            if (std::regex_search(line, m, rowPattern))
            {
                DocRow row;
                row.objectType = std::stoi(m[1].str());
                row.expectedFrames = std::stoi(m[3].str());
                row.label = m[2].str();
                rows.push_back(row);
            }
        }
        return rows;
    }

    // Per-type hold divisor -- read directly from GEObjectIcons.cpp's own
    // `(p / D) % base` / `p % base` case bodies (see this file's own top
    // comment for why this specific fact, and only this fact, is read from
    // source rather than measured). Keyed by the ObjectType actually
    // tested (post `96`-awake-correction, i.e. 97 not 96 for that row).
    const std::map<int, int>& Divisors()
    {
        static const std::map<int, int> table = {
            {2, 2},  {3, 2},  {4, 9}, {5, 3},  {6, 4},  {7, 3},  {16, 1}, {17, 6},
            {20, 6}, {21, 3}, {24, 1}, {25, 2}, {26, 2}, {32, 6}, {33, 6}, {40, 2},
            {44, 6}, {47, 1}, {49, 3}, {50, 3}, {51, 3}, {54, 6}, {96, 1}, {97, 1},
        };
        return table;
    }

    // Minimal repeating period of the DOWNSAMPLED sequence (one sample
    // every `divisor` raw ticks) -- this directly measures the table's own
    // modulo base, independent of how many of its slots happen to share a
    // visual value with a neighbor. Brute force smallest R in [1,
    // maxPeriod] such that GetObjIcon(type, k*divisor) ==
    // GetObjIcon(type, (k+R)*divisor) for every k in [0, verifyRange) --
    // verifyRange comfortably exceeds any real base in this table (largest
    // documented count is 34), so a false-positive short match can't
    // survive the full range check.
    int FindModuloBase(ObjectType type, int divisor, int maxPeriod, int verifyRange)
    {
        for (int candidate = 1; candidate <= maxPeriod; ++candidate)
        {
            bool matches = true;
            for (int k = 0; k < verifyRange; ++k)
            {
                if (GetObjIcon(type, k * divisor) != GetObjIcon(type, (k + candidate) * divisor))
                {
                    matches = false;
                    break;
                }
            }
            if (matches)
            {
                return candidate;
            }
        }
        return -1;
    }
}

int main()
{
    // Repo-root-relative, same convention as every other tool here that
    // reads a real file (see CMakeLists.txt's own WORKING_DIRECTORY note).
    const std::string docPath = "mobile-eggbert-reference/08-animations.md";
    const auto rows = ParseReferenceDoc(docPath);
    check(!rows.empty(), "parsed at least one ObjectType row from " + docPath + " (file missing or format changed?)");
    // 3.1 has 14 rows, 3.2 has 10 (including the 96-awake labeling quirk
    // row) -- 24 total. A count assertion, not just "non-empty", so a
    // format change that silently drops rows fails loudly here too.
    check(rows.size() == 24, "parsed exactly 24 rows from §3.1+§3.2 (found " + std::to_string(rows.size()) + ")");

    for (const auto& row : rows)
    {
        // Deliberate manual correction (see this file's own top comment):
        // the doc's "96 (follower, awake/homing)" row really means
        // ObjectType97 in the code, not 96 -- 96 already has its own
        // separate (dormant) row.
        ObjectType type = static_cast<ObjectType>(row.objectType);
        int divisorKey = row.objectType;
        std::string label = std::to_string(row.objectType) + " (" + row.label + ")";
        if (row.objectType == 96 && row.label.find("awake") != std::string::npos)
        {
            type = ObjectType::ObjectType97;
            divisorKey = 97;
            label += " [doc labels this 96, real table is keyed by ObjectType97]";
        }

        const auto divisorIt = Divisors().find(divisorKey);
        if (divisorIt == Divisors().end())
        {
            check(false, label + ": no known divisor entry for this ObjectType (doc row added without updating "
                                  "the Divisors() table above?)");
            continue;
        }
        const int divisor = divisorIt->second;

        constexpr int kMaxPeriod = 60;
        constexpr int kVerifyRange = 200;
        const int base = FindModuloBase(type, divisor, kMaxPeriod, kVerifyRange);
        if (base < 0)
        {
            check(false, label + ": no repeating modulo base found within " + std::to_string(kMaxPeriod) +
                              " downsampled steps (divisor=" + std::to_string(divisor) + ")");
            continue;
        }
        // ObjectType25 (shield) is a KNOWN, already-documented exception, not
        // a fresh finding to fail on: the doc's own §3.1 prose (right after
        // this row) says its 8-frame table is "a known-incomplete port of
        // mobile-eggbert's real 16-frame table_shield" -- GEObjectIcons.cpp
        // was already fixed to the real 16 (VerifyGetObjIcon.cpp asserts
        // this same value with an identical comment); the doc's own table
        // row was simply never updated to match. Asserting the CODE is
        // still 16 (not silently re-broken back to 8) rather than treating
        // this expected doc/code gap as a failure every run.
        if (row.objectType == 25)
        {
            check(base == 16, label + ": expected the already-known-correct 16 (real table_shield), got " +
                                   std::to_string(base) + " -- has this regressed, or has the doc's own stale "
                                   "'8' been fixed without updating this exception here?");
            continue;
        }
        check(base == row.expectedFrames, label + ": doc says " + std::to_string(row.expectedFrames) +
                                               " frames, GetObjIcon() actually cycles through " +
                                               std::to_string(base) + " (divisor=" + std::to_string(divisor) + ")");
    }

    std::cout << checksRun << " checks run, " << checksFailed << " failed." << std::endl;
    std::cout << (checksFailed == 0 ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return checksFailed == 0 ? 0 : 1;
}
