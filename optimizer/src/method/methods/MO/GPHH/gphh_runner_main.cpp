#ifdef GPHH_RUNNER

#include <iostream>
#include <memory>
#include <random>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <string>
#include <cctype>
#include <locale>
#include <vector>
#include <sstream>
#include "io/DefParser.hpp"
#include "scheduler/Scheduler.hpp"
#include "rules/GPTreeRule.hpp"
#include "rules/GPTreeResRule.hpp"
#include "gp/GPTree.hpp"
#include "gp/TreeEA.hpp"
#include "gp/Features.hpp"
#include "gp/Precompute.hpp"
#include "gp/Normalization.hpp"


static double g_weight = 0.5;

extern bool g_trace;
static bool g_useNormalization = true;

static bool g_normFromArgs = false;
static bool g_normAsked = false;

static bool printStartSched = false;
static bool printFinalSched = true;
static bool printHist = false;
static bool printTreeExpr = false;

static std::string toLower(std::string s) {
    for (auto& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

static void printSchedule(const Instance& I, const ScheduleResult& res, const char* title) {
    std::cout << "\n== " << title << " ==\n";
    for (const auto& t : I.tasks) {
        std::cout << "T" << t.id << "  start=" << t.start << "  finish=" << t.finish << "  res:";
        for (size_t i = 0; i < t.assignedResources.size(); ++i) {
            if (i) std::cout << ",";
            std::cout << t.assignedResources[i];
        }
        std::cout << "\n";
    }
    std::cout << "Makespan: " << res.makespan << "\n";
    std::cout << "Total cost: " << std::fixed << std::setprecision(2) << res.totalCost << "\n";
}

static std::string canonicalRule(std::string in) {
    in = toLower(in);
    if (in == "1" || in == "random" || in == "r") return "random";
    if (in == "2" || in == "avail-gap" || in == "a") return "avail-gap";
    if (in == "3" || in == "work" || in == "w") return "work";
    if (in == "4" || in == "est+dur" || in == "e") return "est+dur";
    if (in == "5" || in == "cheapxdur" || in == "cx") return "cheapxdur";
    if (in == "6" || in == "cheapps+est" || in == "cheap-per-skill+est" || in == "cpe")
        return "cheap-per-skill+est";
    return "random";
}

static GPTree makeTreeByRule(std::mt19937& rng, int maxDepthSeed, const std::string& ruleName) {
    const std::string r = toLower(ruleName);
    if (r == "random")                  return GPTree::RandomTreeMS(rng, maxDepthSeed);
    if (r == "avail-gap")               return GPTree::Make_AVAIL_minus_REQ();
    if (r == "work")                    return GPTree::Make_REQ_times_DUR();
    if (r == "est+dur")                 return GPTree::Make_EST_plus_DUR();
    if (r == "cheapxdur")               return GPTree::Make_CHEAPxDUR();
    if (r == "cheap-per-skill+est")     return GPTree::Make_CHEAP_PER_SKILL_plus_EST();
    return GPTree::RandomTreeMS(rng, maxDepthSeed);
}

static bool loadFloatVector(const std::string& path, std::vector<float>& out) {
    out.clear();
    std::ifstream f(path);
    if (!f) return false;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        float v;
        if (!(iss >> v)) return false;
        out.push_back(v);
    }
    return true;
}

static bool loadIntVector(const std::string& path, std::vector<int>& out) {
    out.clear();
    std::ifstream f(path);
    if (!f) return false;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        int v;
        if (!(iss >> v)) return false;
        out.push_back(v);
    }
    return true;
}

static void askNormalizationIfNeeded() {
    if (g_normAsked) return;

    if (g_normFromArgs) {
        g_normAsked = true;
        return;
    }

    std::cout << "\nFitness normalization (min-max to [0,1]):\n";
    std::cout << "  [1] Enable (default)\n";
    std::cout << "  [2] Disable (use raw makespan and cost)\n";
    std::cout << "Select option (Enter=1): ";

    std::string line;
    std::getline(std::cin, line);

    if (!line.empty()) {
        char c = line[0];
        if (c == '2') {
            g_useNormalization = false;
        }
        else {
            g_useNormalization = true;
        }
    }
    g_normAsked = true;
}

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    std::string path;
    std::string rule;
    bool     seedOverride = false;
    uint64_t seedCLI = 0;
    int      depthCLI = -1;
    std::string taPath;
    std::string keysPath;
    bool useEA = false;
    size_t eaPop = 50;
    size_t eaGen = 2000;
    double eaPC = 0.6;
    double eaMp = 0.3;
    double eaMs = 0.02;
    int    eaMaxD = 8;
    int    eaTour = 2;
    size_t eaElite = 0;
    std::string startRule = "random";

    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        if (s.rfind("--rule=", 0) == 0) { rule = s.substr(7); continue; }
        if (s.rfind("--def=", 0) == 0) { path = s.substr(6); continue; }
        if (s == "--def" && i + 1 < argc) { path = argv[++i]; continue; }
        if (path.empty() && s.size() >= 4) {
            auto tail = toLower(s.substr(s.size() - 4));
            if (tail == ".def") { path = s; continue; }
        }
        if (s == "--trace") { g_trace = true; continue; }
        if (s == "--quiet") {
            g_trace = false;
            printStartSched = false;
            printFinalSched = false;
            printHist = false;
            printTreeExpr = false;
            continue;
        }
        if (s == "--print-start") { printStartSched = true; continue; }
        if (s == "--no-print-final") { printFinalSched = false; continue; }
        if (s == "--hist") { printHist = true; continue; }
        if (s == "--tree") { printTreeExpr = true; continue; }
        if (s == "--no-norm" || s == "--no-normalize") {
            g_useNormalization = false;
            g_normFromArgs = true;
            continue;
        }
        if (s.rfind("--norm=", 0) == 0) {
            auto val = toLower(s.substr(7));
            if (val == "0" || val == "no" || val == "nie") {
                g_useNormalization = false;
            }
            else {
                g_useNormalization = true;
            }
            g_normFromArgs = true;
            continue;
        }

        if (s.rfind("--seed=", 0) == 0) { seedCLI = std::stoull(s.substr(7)); seedOverride = true; continue; }
        if (s.rfind("--depth=", 0) == 0) { depthCLI = std::stoi(s.substr(8)); continue; }
        if (s.rfind("--w=", 0) == 0) {
            double w = std::stod(s.substr(4));
            if (w < 0.0) w = 0.0;
            if (w > 1.0) w = 1.0;
            g_weight = w;
            continue;
        }
        if (s.rfind("--ta=", 0) == 0) { taPath = s.substr(5); continue; }
        if (s.rfind("--keys=", 0) == 0) { keysPath = s.substr(7); continue; }
        if (s == "--evolve") { useEA = true; continue; }
        if (s.rfind("--pop=", 0) == 0) { eaPop = (size_t)std::stoul(s.substr(6)); continue; }
        if (s.rfind("--gen=", 0) == 0) { eaGen = (size_t)std::stoul(s.substr(6)); continue; }
        if (s.rfind("--pc=", 0) == 0) { eaPC = std::stod(s.substr(5)); continue; }
        if (s.rfind("--mp=", 0) == 0) { eaMp = std::stod(s.substr(5)); continue; }
        if (s.rfind("--ms=", 0) == 0) { eaMs = std::stod(s.substr(5)); continue; }
        if (s.rfind("--maxdepth=", 0) == 0) { eaMaxD = std::stoi(s.substr(11)); continue; }
        if (s.rfind("--tour=", 0) == 0) { eaTour = std::stoi(s.substr(7));  continue; }
        if (s.rfind("--elite=", 0) == 0) { eaElite = (size_t)std::stoul(s.substr(8)); continue; }
        if (s.rfind("--start=", 0) == 0) { startRule = canonicalRule(s.substr(8)); continue; }
    }

    while (path.empty()) {
        std::cout << "Enter path to .def file: ";
        std::string tmp;
        std::getline(std::cin, tmp);
        if (!tmp.empty()) path = tmp;
        if (path.size() < 4 || toLower(path.substr(path.size() - 4)) != ".def") {
            std::cout << "This does not look like a .def file. Try again.\n";
            path.clear();
        }
    }

    std::filesystem::path defPath(path);
    if (!std::filesystem::exists(defPath)) {
        std::cerr << "Failed to load: " << defPath.string() << "\n";
        return 1;
    }
    defPath = std::filesystem::weakly_canonical(defPath);
    const std::string pathStr = defPath.string();
    std::cout << "DEF path: " << pathStr << "\n";

    if (!useEA) {
        std::cout << "\nSelect mode:\n"
            "  [1] Evolve trees (EA)\n"
            "  [2] No evolution\n"
            "Select option (Enter=1): ";
        std::string in;
        std::getline(std::cin, in);
        if (in.empty()) in = "1";
        for (auto& c : in) c = (char)std::tolower((unsigned char)c);
        useEA = (in == "1" || in == "e" || in == "ea" || in == "evolve");

        askNormalizationIfNeeded();

        if (useEA) {
            std::cout << "\nEA parameters (Enter = keep default):\n";
            std::string s;
            std::cout << "  Population [current " << eaPop << "]: ";
            std::getline(std::cin, s); if (!s.empty()) eaPop = (size_t)std::stoul(s);
            std::cout << "  Generations [current " << eaGen << "]: ";
            s.clear(); std::getline(std::cin, s); if (!s.empty()) eaGen = (size_t)std::stoul(s);
            std::cout << "  pCrossover [current " << eaPC << "]: ";
            s.clear(); std::getline(std::cin, s); if (!s.empty()) eaPC = std::stod(s);
            std::cout << "  pMutParam (parameter mutation) [current " << eaMp << "]: ";
            s.clear(); std::getline(std::cin, s); if (!s.empty()) eaMp = std::stod(s);
            std::cout << "  pMutStruct (structure mutation) [current " << eaMs << "]: ";
            s.clear(); std::getline(std::cin, s); if (!s.empty()) eaMs = std::stod(s);
            std::cout << "  maxDepth [current " << eaMaxD << "]: ";
            s.clear(); std::getline(std::cin, s); if (!s.empty()) eaMaxD = std::stoi(s);

            std::cout << "\nEA baseline start rule:\n"
                "  [1] random\n"
                "  [2] avail-gap\n"
                "  [3] work\n"
                "  [4] est+dur\n"
                "  [5] cheapxdur\n"
                "  [6] cheapps+est\n"
                "Enter option/name (Enter=1): ";
            {
                std::string in2;
                std::getline(std::cin, in2);
                if (in2.empty()) in2 = "1";
                startRule = canonicalRule(in2);
            }
        }
    }
    askNormalizationIfNeeded();

    if (!useEA && rule.empty()) {
        std::cout << "\nSelect GP rule:\n"
            "  [1] random\n"
            "  [2] avail-gap\n"
            "  [3] work\n"
            "  [4] est+dur\n"
            "  [5] cheapxdur\n"
            "  [6] cheapps+est\n"
            "Enter option or name (Enter=1): ";
        std::string in;
        std::getline(std::cin, in);
        if (in.empty()) in = "1";
        rule = canonicalRule(in);
    }

    if (!useEA)
        std::cout << "Selected GP rule (--rule=" << rule << ")\n";
    else
        std::cout << "Tryb: EVOLVE TREES (pop=" << eaPop
        << ", gen=" << eaGen
        << ", pc=" << eaPC
        << ", mp=" << eaMp
        << ", ms=" << eaMs
        << ", maxDepth=" << eaMaxD
        << ", w=" << g_weight
        << "), start=" << startRule
        << "\n";


    Instance I;
    if (!DefParser::parseFile(pathStr, I)) {
        std::cerr << "Nie udalo sie wczytac: " << pathStr << "\n";
        return 1;
    }

    gp::CPMPrecalc cpm;
    if (!gp::buildCPM(I, cpm)) {
        std::cerr << "CPM precompute failed (czy instancja to DAG?)\n";
    }
    gp::setCPMPrecalc(&cpm);
    gp::initFeatureScaling(I);

    uint64_t seed = seedOverride ? seedCLI
        : (uint64_t)std::chrono::steady_clock::now().time_since_epoch().count();

    int maxDepthSeed = (depthCLI >= 1) ? depthCLI : 3;
    std::mt19937 rng{ (unsigned)seed };

    GPTree tree;
    GPTree startTreeTask;
    GPTree startTreeRes;
    GPTree startTree;

    int    makespan_start_json = -1;
    double cost_start_json = 0.0;


    std::vector<double> hist;
    std::vector<double> histBest;
    std::vector<double> histAvg;
    std::vector<double> histWorst;

    ScheduleResult res{};
    bool haveRes = false;

    Instance IstartSnap;
    bool haveStartSnap = false;
    Instance Igen0Snap;
    bool haveGen0Snap = false;
    int    makespan_gen0_json = -1;
    double cost_gen0_json = 0.0;
    GP_Individual gen0BestInd;

    GP_Individual bestInd;

    if (!useEA) {
        tree = makeTreeByRule(rng, maxDepthSeed, canonicalRule(rule));
        std::cout << "[seed=" << seed << "]\n";
        if (printTreeExpr) std::cout << "\nExpression (GP static): " << tree.toString() << "\n";
    }
    else {
        GPEA_Params P;
        P.popSize = eaPop;
        P.generations = eaGen;
        P.pCrossover = eaPC;
        P.pMutParam = eaMp;
        P.pMutStruct = eaMs;
        P.maxDepth = eaMaxD;
        P.tournamentK = eaTour;
        P.eliteCount = eaElite;
        P.seed = seed;
        P.weight = g_weight;
        P.useNormalization = g_useNormalization;

        startTreeTask = makeTreeByRule(rng, maxDepthSeed, startRule);
        startTreeRes = GPTree::RandomTreeRES(rng, maxDepthSeed);
        startTree = startTreeTask;

        std::cout << "[EA] Baseline start rule (tasks): " << startRule;
        if (printTreeExpr) {
            std::cout << "\n[EA] StartTreeTask: " << startTreeTask.toString()
                << "\n[EA] StartTreeRes : " << startTreeRes.toString();
        }
        std::cout << "\n";

        {
            Instance Istart = I;
            GPTreeResRule startResRule(startTreeRes);
            auto resStart = Scheduler::withResources(Istart, GPTreeRule(startTreeTask), &startResRule);

            if (printStartSched)
                printSchedule(..., "START schedule (seed trees)");

            makespan_start_json = resStart.makespan;
            cost_start_json = resStart.totalCost;
            IstartSnap = Istart; haveStartSnap = true;
        }

        TreeEA ea(I, P);

        ea.setSeedTrees(startTreeTask, startTreeRes);

        bestInd = ea.run();

        if (ea.hasBestGen0()) {
            gen0BestInd = ea.getBestGen0();

            Instance Igen0 = I;
            GPTreeResRule resRuleGen0(gen0BestInd.resTree);
            auto resGen0 = Scheduler::withResources(Igen0, GPTreeRule(gen0BestInd.taskTree), &resRuleGen0);

            makespan_gen0_json = resGen0.makespan;
            cost_gen0_json = resGen0.totalCost;
            Igen0Snap = Igen0;
            haveGen0Snap = true;
        }

        GPTree taskTreeFinal = bestInd.taskTree;
        GPTree resTreeFinal = bestInd.resTree;

        histBest = ea.getHistBest();
        histAvg = ea.getHistAvg();
        histWorst = ea.getHistWorst();
        hist = histBest;

        if (printHist) {
            std::cout << "\n== Historia fitnessu (gen: best / avg / worst) ==\n";
            for (size_t i = 0; i < histBest.size(); ++i) {
                std::cout << "gen " << i << " : "
                    << std::fixed << std::setprecision(6)
                    << histBest[i] << " / "
                    << (i < histAvg.size() ? histAvg[i] : NAN) << " / "
                    << (i < histWorst.size() ? histWorst[i] : NAN) << "\n";
            }
        }

        Instance Ifinal = I;
        GPTreeResRule resRuleFinal(resTreeFinal);
        auto resFinal = Scheduler::withResources(
            Ifinal,
            GPTreeRule(taskTreeFinal),
            &resRuleFinal
        );

        if (printFinalSched)
            printSchedule(..., "FINAL schedule (after evolution)");

        tree = taskTreeFinal;

        std::cout << "[seed=" << seed << "]\n";
        std::cout << "EA best (fitness=" << std::fixed << std::setprecision(6) << bestInd.fitness
            << ", ms=" << bestInd.makespan
            << ", cost=" << std::fixed << std::setprecision(2) << bestInd.cost
            << ", msNorm=" << bestInd.msNorm
            << ", costNorm=" << bestInd.costNorm
            << ")\n";
        if (printTreeExpr) std::cout << "Best GP tree: " << tree.toString() << "\n";

        res = resFinal;
        I = Ifinal;
        haveRes = true;
    }

    std::vector<int>   forced;  bool forcedOk = false;
    std::vector<float> pkeys;   bool keysOk = false;
    if (!taPath.empty()) {
        forcedOk = loadIntVector(taPath, forced);
        if (!forcedOk)
            std::cerr << "[warn] Failed to load --ta=" << taPath << "\n";
        else if ((int)forced.size() != (int)I.tasks.size())
            std::cerr << "[warn] --ta size (" << forced.size()
            << ") != number of tasks (" << I.tasks.size() << ")\n";
    }
    if (!keysPath.empty()) {
        keysOk = loadFloatVector(keysPath, pkeys);
        if (!keysOk)
            std::cerr << "[warn] Failed to load --keys=" << taPath << "\n";
        else if ((int)pkeys.size() != (int)I.tasks.size())
            std::cerr << "[warn] --keys size (" << forced.size()
            << ") != number of tasks (" << I.tasks.size() << ")\n";
    }
    if (forcedOk) Scheduler::setForcedResources(&forced);
    if (keysOk)   Scheduler::setPriorityKeys(&pkeys);
    if (!haveRes) {
        gp::setCPMPrecalc(&cpm);
        res = Scheduler::withResources(I, GPTreeRule(tree));
        haveRes = true;
    }

    Scheduler::setForcedResources(nullptr);
    Scheduler::setPriorityKeys(nullptr);

    const int    ms = res.makespan;
    const double cost = res.totalCost;

    const ImopseBounds B = compute_imopse_bounds(I);
    auto [msNorm, costNorm] = imopse_minmax_normalize(ms, cost, B);

    const double w = g_weight;
    const double fitnessNorm = w * msNorm + (1.0 - w) * costNorm;
    const double fitnessRaw = w * ms + (1.0 - w) * cost;
    const double fitness = g_useNormalization ? fitnessNorm : fitnessRaw;

    std::cout << "Makespan: " << ms << "\n";
    std::cout << "Total cost: " << std::fixed << std::setprecision(2) << cost << "\n";
    std::cout << "Bounds  ms=[" << B.ms_min << "," << B.ms_max << "],  cost=["
        << std::fixed << std::setprecision(2) << B.cost_min << ","
        << std::fixed << std::setprecision(2) << B.cost_max << "]\n";
    std::cout << "Normalized  ms=" << std::setprecision(6) << msNorm
        << "  cost=" << costNorm << "\n";

    if (g_useNormalization) {
        std::cout << "Fitness (uzywany, w*msNorm + (1-w)*costNorm, w=" << w << "): "
            << std::setprecision(6) << fitnessNorm << "\n";
        std::cout << "FitnessRaw (w*ms + (1-w)*cost): "
            << std::setprecision(6) << fitnessRaw << "\n";
    }
    else {
        std::cout << "FitnessRaw (uzywany, w*ms + (1-w)*cost, w=" << w << "): "
            << std::setprecision(6) << fitnessRaw << "\n";
        std::cout << "FitnessNorm (nieuzywany, w*msNorm + (1-w)*costNorm): "
            << std::setprecision(6) << fitnessNorm << "\n";
    }

    const std::filesystem::path outDir = R"(C:\Users\awesd\source\repos\MSRCPSP_DR)";
    std::error_code ec;
    std::filesystem::create_directories(outDir, ec);

    std::string tag = useEA ? "evolved" : canonicalRule(rule);
    if (tag.empty()) tag = "rule";
    std::string base = std::string("out_") + std::to_string(seed) + "_" + tag;
    std::filesystem::path jsonPath = outDir / (base + ".json");

    std::ofstream jf(jsonPath.string());
    jf.imbue(std::locale::classic());
    jf << "{\n";
    jf << "  \"instance\": " << std::quoted(pathStr) << ",\n";
    jf << "  \"seed\": " << seed << ",\n";
    jf << "  \"mode\": " << (useEA ? "\"evolve\"" : "\"single\"") << ",\n";
    jf << "  \"weight\": " << std::fixed << std::setprecision(6) << g_weight << ",\n";
    jf << "  \"makespan\": " << ms << ",\n";
    jf << "  \"totalCost\": " << std::fixed << std::setprecision(2) << cost << ",\n";
    jf << "  \"msMin\": " << B.ms_min << ", \"msMax\": " << B.ms_max << ",\n";
    jf << "  \"costMin\": " << std::fixed << std::setprecision(2) << B.cost_min
        << ", \"costMax\": " << std::fixed << std::setprecision(2) << B.cost_max << ",\n";
    jf << "  \"msNorm\": " << std::fixed << std::setprecision(6) << msNorm
        << ", \"costNorm\": " << std::fixed << std::setprecision(6) << costNorm << ",\n";
    jf << "  \"fitness\": " << std::fixed << std::setprecision(6) << fitness << ",\n";

    if (useEA) {
        jf << "  \"startRule\": " << std::quoted(startRule) << ",\n";
        jf << "  \"start\": {"
            << "\"makespan\": " << makespan_start_json
            << ", \"totalCost\": " << std::fixed << std::setprecision(2) << cost_start_json
            << "},\n";

        if (haveGen0Snap) {
            jf << "  \"gen0\": {"
                << "\"fitness\": " << std::fixed << std::setprecision(6) << gen0BestInd.fitness
                << ", \"makespan\": " << makespan_gen0_json
                << ", \"totalCost\": " << std::fixed << std::setprecision(2) << cost_gen0_json
                << "},\n";
        }

        auto dumpArr = [&](const char* name, const std::vector<double>& v) {
            jf << "  \"" << name << "\": [";
            for (size_t i = 0; i < v.size(); ++i) {
                if (i) jf << ",";
                jf << std::fixed << std::setprecision(6) << v[i];
            }
            jf << "],\n";
            };
        dumpArr("fitnessBest", histBest);
        dumpArr("fitnessAvg", histAvg);
        dumpArr("fitnessWorst", histWorst);
        dumpArr("fitnessHistory", hist);

    }
    else {
        jf << "  \"rule\": " << std::quoted(canonicalRule(rule)) << ",\n";
    }

    if (useEA) {
        jf << "  \"ea\": {"
            << "\"pop\":" << eaPop << ",\"gen\":" << eaGen
            << ",\"pc\":" << eaPC << ",\"mp\":" << eaMp
            << ",\"ms\":" << eaMs << ",\"maxDepth\":" << eaMaxD << "},\n";
    }

    jf << "  \"gaHooks\": {\n";
    jf << "    \"forcedProvided\": " << (taPath.empty() ? "false" : "true") << ",\n";
    jf << "    \"keysProvided\": " << (keysPath.empty() ? "false" : "true") << "\n";
    jf << "  },\n";

    if (useEA) {
        jf << "  \"startTree\": " << startTree.toJson() << ",\n";
    }
    if (useEA && haveStartSnap) {
        jf << "  \"startTasks\": [\n";
        for (size_t i = 0; i < IstartSnap.tasks.size(); ++i) {
            const auto& t = IstartSnap.tasks[i];
            if (i) jf << ",\n";
            jf << "    {\"id\":" << t.id
                << ",\"start\":" << t.start
                << ",\"finish\":" << t.finish
                << ",\"duration\":" << t.duration
                << ",\"reqSkill\":" << std::quoted(t.reqSkill)
                << ",\"reqLevel\":" << t.reqLevel
                << ",\"resources\":[";
            for (size_t k = 0; k < t.assignedResources.size(); ++k) {
                if (k) jf << ",";
                jf << t.assignedResources[k];
            }
            jf << "]}";
        }
        jf << "\n  ],\n";
    }

    if (useEA && haveGen0Snap) {
        jf << "  \"gen0Tasks\": [\n";
        for (size_t i = 0; i < Igen0Snap.tasks.size(); ++i) {
            const auto& t = Igen0Snap.tasks[i];
            if (i) jf << ",\n";
            jf << "    {\"id\":" << t.id
                << ",\"start\":" << t.start
                << ",\"finish\":" << t.finish
                << ",\"duration\":" << t.duration
                << ",\"reqSkill\":" << std::quoted(t.reqSkill)
                << ",\"reqLevel\":" << t.reqLevel
                << ",\"resources\":[";
            for (size_t k = 0; k < t.assignedResources.size(); ++k) {
                if (k) jf << ",";
                jf << t.assignedResources[k];
            }
            jf << "]}";
        }
        jf << "\n  ],\n";
    }

    if (useEA) {
        jf << "  \"treeTask\": " << bestInd.taskTree.toJson() << ",\n";
        jf << "  \"treeRes\": " << bestInd.resTree.toJson() << ",\n";

        jf << "  \"startTreeTask\": " << startTreeTask.toJson() << ",\n";
        jf << "  \"startTreeRes\": " << startTreeRes.toJson() << ",\n";
    }
    else {
        jf << "  \"treeTask\": " << tree.toJson() << ",\n";
        jf << "  \"treeRes\": " << tree.toJson() << ",\n";
    }
    jf << "  \"tasks\": [\n";
    for (size_t i = 0; i < I.tasks.size(); ++i) {
        const auto& t = I.tasks[i];
        if (i) jf << ",\n";
        jf << "    {\"id\":" << t.id
            << ",\"start\":" << t.start
            << ",\"finish\":" << t.finish
            << ",\"duration\":" << t.duration
            << ",\"reqSkill\":" << std::quoted(t.reqSkill)
            << ",\"reqLevel\":" << t.reqLevel
            << ",\"resources\":[";
        for (size_t k = 0; k < t.assignedResources.size(); ++k) {
            if (k) jf << ",";
            jf << t.assignedResources[k];
        }
        jf << "]}";
    }
    jf << "\n  ]\n";
    jf << "}\n";
    jf.close();

    std::cout << "JSON saved: " << jsonPath.string() << "\n";
    return 0;
}

#endif
