#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <functional>
#include <cstddef>
#include "Resource.hpp"
#include "Task.hpp"

struct Instance {
    std::vector<Resource> resources;
    std::vector<Task> tasks;
    std::unordered_map<int, int> idToIndex;

    std::size_t resourceStructureSignature = 0;
    bool resourceStructureSignatureReady = false;

    std::size_t taskStructureSignature = 0;
    bool taskStructureSignatureReady = false;

    void buildIndex() {
        idToIndex.clear();
        for (size_t i = 0; i < tasks.size(); ++i) {
            idToIndex[tasks[i].id] = int(i);
        }
    }

    static std::size_t hashCombine(std::size_t seed, std::size_t value) {
        return seed ^ (value + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2));
    }

    void rebuildResourceStructureSignature() {
        std::size_t sig = hashCombine(1469598103934665603ull, resources.size());

        for (const auto& r : resources) {
            std::size_t resHash = hashCombine(std::hash<int>{}(r.id), r.skills.size());

            std::size_t skillsHash = 0;
            for (const auto& kv : r.skills) {
                std::size_t pairHash = std::hash<std::string>{}(kv.first);
                pairHash = hashCombine(pairHash, std::hash<int>{}(kv.second));
                skillsHash ^= hashCombine(pairHash, 0x517cc1b727220a95ull);
            }

            resHash = hashCombine(resHash, skillsHash);
            sig = hashCombine(sig, resHash);
        }

        resourceStructureSignature = sig;
        resourceStructureSignatureReady = true;
    }

    void rebuildTaskStructureSignature() {
        std::size_t sig = hashCombine(1099511628211ull, tasks.size());

        for (const auto& t : tasks) {
            std::size_t taskHash = hashCombine(std::hash<int>{}(t.id), std::hash<int>{}(t.duration));
            taskHash = hashCombine(taskHash, std::hash<std::string>{}(t.requirementKey()));
            taskHash = hashCombine(taskHash, std::hash<int>{}(t.totalRequiredLevel()));
            taskHash = hashCombine(taskHash, std::hash<int>{}(t.imopseIndex));

            std::size_t predHash = 0;
            for (int pid : t.predecessors) {
                predHash = hashCombine(predHash, std::hash<int>{}(pid));
            }

            std::size_t capHash = 0;
            for (int rid : t.capableResources) {
                capHash = hashCombine(capHash, std::hash<int>{}(rid));
            }

            taskHash = hashCombine(taskHash, predHash);
            taskHash = hashCombine(taskHash, capHash);

            sig = hashCombine(sig, taskHash);
        }

        taskStructureSignature = sig;
        taskStructureSignatureReady = true;
    }

    void rebuildStructureSignatures() {
        rebuildResourceStructureSignature();
        rebuildTaskStructureSignature();
    }
};