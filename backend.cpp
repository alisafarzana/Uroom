#include <algorithm>
#include <map>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <initializer_list>
#include <utility>
using namespace std;

template <typename T, size_t Capacity>
class FixedList {
private:
    T items[Capacity];
    size_t count = 0;

public:
    using value_type = T;
    using reference = T&;
    using const_reference = const T&;
    using size_type = size_t;

    FixedList() = default;

    FixedList(initializer_list<T> values) {
        for (const T& value : values) push_back(value);
    }

    void push_back(const T& value) {
        if (count >= Capacity) throw runtime_error("FixedList capacity exceeded.");
        items[count++] = value;
    }

    void push_back(T&& value) {
        if (count >= Capacity) throw runtime_error("FixedList capacity exceeded.");
        items[count++] = move(value);
    }

    void pop_back() {
        if (count == 0) throw runtime_error("Cannot remove from an empty FixedList.");
        --count;
    }

    reference front() { return items[0]; }
    const_reference front() const { return items[0]; }
    reference operator[](size_t index) { return items[index]; }
    const_reference operator[](size_t index) const { return items[index]; }
    T* begin() { return items; }
    const T* begin() const { return items; }
    T* end() { return items + count; }
    const T* end() const { return items + count; }
    size_t size() const { return count; }
    bool empty() const { return count == 0; }
};

//room record
struct Room { string id, name, location; int capacity; };

//rank scoring
struct Ranked {
    int distanceScore;
    double score;
    Room room;
    int extra;
    FixedList<string, 8> slots;
};

//minHeap comparator
struct MinCmp {
    bool operator()(
        const Ranked& a,
        const Ranked& b
    ) const {
        //First priority: the location nearest to the selected starting location.
        if (a.distanceScore != b.distanceScore) {
            return a.distanceScore > b.distanceScore;
        }

        // Second priority: the lowest total score within the same location rank.
        if (a.score != b.score) {
            return a.score > b.score;
        }

        // Final tie-breaker: lower room ID comes first.
        return a.room.id > b.room.id;
    }
};

//CSV parsing
FixedList<string, 64> splitCsvLine(
    const string& line
) {
    FixedList<string, 64> columns;
    string value;
    bool insideQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char character = line[i];

        if (character == '"') {
            if (
                insideQuotes
                && i + 1 < line.size()
                && line[i + 1] == '"'
            ) {
                value += '"';
                ++i;
            } else {
                insideQuotes = !insideQuotes;
            }
        } else if (
            character == ','
            && !insideQuotes
        ) {
            columns.push_back(value);
            value.clear();
        } else if (character != '\r') {
            value += character;
        }
    }

    columns.push_back(value);

    return columns;
}

string csvEscape(const string& value) {
    string escaped;
    bool needsQuotes = false;

    for (char character : value) {
        if (
            character == ','
            || character == '"'
            || character == '\n'
            || character == '\r'
        ) {
            needsQuotes = true;
        }

        if (character == '"') {
            escaped += "\"\"";
        } else if (
            character != '\n'
            && character != '\r'
        ) {
            escaped += character;
        }
    }

    if (needsQuotes) {
        return "\"" + escaped + "\"";
    }

    return escaped;
}

//graph
struct BenchmarkPair {
    std::string testCaseId;

    int datasetSize = 0;

    double baselineOperations = 0.0;
    double optimizedOperations = 0.0;

    double baselineTime = 0.0;
    double optimizedTime = 0.0;

    double baselineMemory = 0.0;
    double optimizedMemory = 0.0;

    bool hasBaseline = false;
    bool hasOptimized = false;
};

//room records
FixedList<Room, 14> rooms() {
    return {
        {"R001","Vortex 1","Nadi@UTP",20}, {"R002","Vortex 2","Nadi@UTP",12},
        {"R003","Vault 1","Nadi@UTP",30}, {"R004","Vault 2","Nadi@UTP",20},
        {"R005","Elite Oasis","Nadi@UTP",25},
        {"R006","Discussion Room A","Information Resource Centre",10},
        {"R007","Discussion Room B","Information Resource Centre",16},
        {"R008","Collaborative Room","Information Resource Centre",24},
        {"R009","Seminar Room 1","Chancellor Complex",40},
        {"R010","Meeting Room 2","Chancellor Complex",18},
        {"R011","Study Pod A","Village 5",6}, {"R012","Study Pod B","Village 5",8},
        {"R013","Learning Space 1","Block B",15}, {"R014","Learning Space 2","Block B",28}
    };
}

//HASH MAP for location priority orders
unordered_map<string, FixedList<string, 12>> locationPriorityOrders() {
    return {
        {
            "Information Resource Centre",
            {
                "Information Resource Centre",
                "Chancellor Complex",
                "Pocket D",
                "Block B",
                "Village 7",
                "Village 1",
                "Village 2",
                "Village 3",
                "Village 4",
                "Nadi@UTP",
                "Village 6",
                "Village 5"
            }
        },
        {
            "Chancellor Complex",
            {
                "Chancellor Complex",
                "Information Resource Centre",
                "Pocket D",
                "Block B",
                "Village 7",
                "Village 1",
                "Village 2",
                "Village 3",
                "Village 4",
                "Nadi@UTP",
                "Village 6",
                "Village 5"
            }
        },
        {
            "Pocket D",
            {
                "Pocket D",
                "Chancellor Complex",
                "Information Resource Centre",
                "Block B",
                "Village 7",
                "Village 1",
                "Village 2",
                "Village 3",
                "Village 4",
                "Nadi@UTP",
                "Village 6",
                "Village 5"
            }
        },
        {
            "Block B",
            {
                "Block B",
                "Chancellor Complex",
                "Information Resource Centre",
                "Pocket D",
                "Village 7",
                "Village 1",
                "Village 2",
                "Village 3",
                "Village 4",
                "Nadi@UTP",
                "Village 6",
                "Village 5"
            }
        },
        {
            "Village 7",
            {
                "Village 7",
                "Block B",
                "Chancellor Complex",
                "Information Resource Centre",
                "Pocket D",
                "Village 1",
                "Village 2",
                "Village 3",
                "Village 4",
                "Nadi@UTP",
                "Village 6",
                "Village 5"
            }
        },
        {
            "Village 1",
            {
                "Village 1",
                "Village 2",
                "Chancellor Complex",
                "Information Resource Centre",
                "Block B",
                "Nadi@UTP",
                "Village 6",
                "Village 3",
                "Village 4",
                "Pocket D",
                "Village 5",
                "Village 7"
            }
        },
        {
            "Village 2",
            {
                "Village 2",
                "Village 1",
                "Nadi@UTP",
                "Village 3",
                "Village 4",
                "Pocket D",
                "Village 6",
                "Village 5",
                "Chancellor Complex",
                "Information Resource Centre",
                "Block B",
                "Village 7"
            }
        },
        {
            "Village 3",
            {
                "Village 3",
                "Village 4",
                "Pocket D",
                "Village 5",
                "Nadi@UTP",
                "Village 6",
                "Village 2",
                "Village 1",
                "Chancellor Complex",
                "Information Resource Centre",
                "Block B",
                "Village 7"
            }
        },
        {
            "Village 4",
            {
                "Village 4",
                "Nadi@UTP",
                "Village 6",
                "Village 3",
                "Pocket D",
                "Village 5",
                "Village 2",
                "Village 1",
                "Chancellor Complex",
                "Information Resource Centre",
                "Block B",
                "Village 7"
            }
        },
        {
            "Village 5",
            {
                "Village 5",
                "Village 3",
                "Pocket D",
                "Village 4",
                "Nadi@UTP",
                "Village 6",
                "Village 2",
                "Village 1",
                "Chancellor Complex",
                "Information Resource Centre",
                "Block B",
                "Village 7"
            }
        },
        {
            "Village 6",
            {
                "Village 6",
                "Nadi@UTP",
                "Village 4",
                "Village 3",
                "Village 2",
                "Pocket D",
                "Village 5",
                "Village 1",
                "Chancellor Complex",
                "Information Resource Centre",
                "Block B",
                "Village 7"
            }
        },
        {
            "Nadi@UTP",
            {
                "Nadi@UTP",
                "Village 6",
                "Village 4",
                "Village 3",
                "Village 2",
                "Pocket D",
                "Village 5",
                "Village 1",
                "Chancellor Complex",
                "Information Resource Centre",
                "Block B",
                "Village 7"
            }
        }
    };
}

// distance score based on location priority orders
int getDistanceScore(
    const string& startingLocation,
    const string& roomLocation
) {
    // No starting location means distance should not affect the total score.
    if (startingLocation.empty()) {
        return 0;
    }

    static const auto orders = locationPriorityOrders();

    auto orderIterator = orders.find(startingLocation);

    //Unknown starting locations receive a score worse than all 12 known ranks.
    if (orderIterator == orders.end()) {
        return 13;
    }

    const FixedList<string, 12>& order = orderIterator->second;

    auto locationIterator = find(
        order.begin(),
        order.end(),
        roomLocation
    );

    // Unknown room locations also receive the worst fallback score.
    if (locationIterator == order.end()) {
        return 13;
    }

    // Convert the zero-based list position to a user-friendly score from 1 to 12.
    return static_cast<int>(
        distance(order.begin(), locationIterator)
    ) + 1; //index + 1 to convert to 1-based score
}

FixedList<string, 64> split(const string& s, char delimiter) {
    FixedList<string, 64> values; string item; stringstream stream(s);
    while (getline(stream, item, delimiter)) values.push_back(item);
    return values;
}

template <size_t Capacity>
string join(const FixedList<string, Capacity>& values, const string& separator) {
    string output;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i) output += separator;
        output += values[i];
    }
    return output;
}


FixedList<string, 8> slots(const Room& room) {
    if (room.location == "Information Resource Centre") {
        return {"10:00-12:00","12:00-14:00","14:00-16:00","16:00-18:00","18:00-20:00","20:00-22:00"};
    }
    return {"08:00-10:00","10:00-12:00","12:00-14:00","14:00-16:00","16:00-18:00","18:00-20:00","20:00-22:00"};
}

template <size_t Capacity>
bool contains(const FixedList<string, Capacity>& values, const string& target) {
    return find(values.begin(), values.end(), target) != values.end();
}

//hashing function for stable pseudo-randomness
unsigned long long stableHash(const string& value) {
    unsigned long long hash = 1469598103934665603ULL;
    for (unsigned char character : value) {
        hash ^= character;
        hash *= 1099511628211ULL;
    }
    return hash;
}

//hash set of saved bookings from CSV file
unordered_set<string> savedBookings(const string& filePath) {
    unordered_set<string> saved; ifstream file(filePath); string line;
    while (getline(file, line)) {
        auto parts = splitCsvLine(line);
        if (parts.size() >= 3 && parts[0] != "room_id") saved.insert(parts[0] + "|" + parts[1] + "|" + parts[2]);
    }
    return saved;
}

FixedList<string, 8> freeSlots(const Room& room, const string& day, const unordered_set<string>& saved) {
    auto all = slots(room);
    unordered_set<string> used;
    auto state = stableHash(room.id + "|" + day);
    int count = 1 + state % min<size_t>(3, all.size());
    while ((int)used.size() < count) {
        state = state * 6364136223846793005ULL + 1ULL;
        used.insert(all[state % all.size()]);
    }
    FixedList<string, 8> free;
    for (const auto& slot : all) {
        if (!used.count(slot) && !saved.count(room.id + "|" + day + "|" + slot)) free.push_back(slot);
    }
    return free;
}

string label(const string& slot) {
    auto parts = split(slot, '-');
    auto formatTime = [](const string& time) {
        auto values = split(time, ':');
        int hour = stoi(values[0]);
        string suffix = hour < 12 ? "AM" : "PM";
        hour %= 12;
        if (!hour) hour = 12;
        return to_string(hour) + ":" + values[1] + " " + suffix;
    };
    return formatTime(parts[0]) + "-" + formatTime(parts[1]);
}

void runSearch(char** argv) {
    int pax = stoi(argv[2]);
    string day = argv[3], wanted = argv[4], start = argv[5], preferred = argv[6], file = argv[7];
    auto allRooms = rooms();
    auto saved = savedBookings(file);
    FixedList<Room, 14> candidates;
    for (const auto& room : allRooms) if (preferred.empty() || room.location == preferred) candidates.push_back(room);

    //priority queue keeps the best location priority and score at the top.
    priority_queue<Ranked, FixedList<Ranked, 14>, MinCmp> heap;
    for (const auto& room : candidates) {
        if (room.capacity < pax) continue;
        auto free = freeSlots(room, day, saved);
        if (!wanted.empty() && (!contains(slots(room), wanted) || !contains(free, wanted))) continue;
        if (wanted.empty() && free.empty()) continue;

        //calculate capacity difference and distance score for ranking
        int extra = room.capacity - pax;
        int distanceScore = getDistanceScore(start, room.location);

        // A lower distance score produces a lower and therefore better total score.
        // Extra capacity increases the score, while more free slots slightly reduce it.
        double score =
            extra * 2
            + distanceScore
            - 0.5 * min<size_t>(free.size(), 4);

        heap.push({distanceScore, score, room, extra, free});
    }

    cout << "META|" << candidates.size() << "|" << allRooms.size() << "\n";
    int rank = 1;
    while (!heap.empty()) {
        auto result = heap.top(); heap.pop();
        FixedList<string, 3> labels;
        for (size_t i = 0; i < min<size_t>(3, result.slots.size()); ++i) labels.push_back(label(result.slots[i]));
        string available = wanted.empty() ? join(labels, ", ") + (result.slots.size() > 3 ? " +" : "") : label(wanted);
        cout << fixed << setprecision(2)
             << "RESULT|" << rank++ << "|" << result.room.id << "|" << result.room.name << "|"
             << result.room.location << "|" << result.room.capacity << "|" << result.extra << "|"
             << result.distanceScore << "|" << available << "|" << result.score << "|"
             << join(result.slots, ",") << "\n";
    }
}

void runSchedule(char** argv) {
    string id = argv[2], day = argv[3], file = argv[4];
    auto allRooms = rooms(); auto saved = savedBookings(file);
    auto iterator = find_if(allRooms.begin(), allRooms.end(), [&](const Room& room){ return room.id == id; });
    if (iterator == allRooms.end()) throw runtime_error("Room not found.");
    auto free = freeSlots(*iterator, day, saved);
    for (const auto& slot : slots(*iterator)) {
        cout << "SLOT|" << slot << "|" << (contains(free, slot) ? "Available" : "Booked") << "|" << label(slot) << "\n";
    }
}

void runBook(char** argv) {
    string id = argv[2], day = argv[3], selected = argv[4], filePath = argv[5];
    string name = argv[6], email = argv[7], title = argv[8], description = argv[9];
    auto allRooms = rooms(); auto saved = savedBookings(filePath);
    auto iterator = find_if(allRooms.begin(), allRooms.end(), [&](const Room& room) { return room.id == id; });
    if (iterator == allRooms.end()) throw runtime_error("Room not found.");
    if (!contains(slots(*iterator), selected)) throw runtime_error("That slot is not offered for this facility.");
    if (!contains(freeSlots(*iterator, day, saved), selected)) throw runtime_error("That slot is no longer available.");
    if (name.empty() || email.empty() || title.empty() || description.empty()) throw runtime_error("All student booking details are required.");
    ifstream existingFile(filePath);
    bool needsHeader = !existingFile.good() || existingFile.peek() == ifstream::traits_type::eof();
    existingFile.close();
    ofstream fileOut(filePath, ios::app);
    if (!fileOut.is_open()) throw runtime_error("Unable to open bookings.csv.");
    if (needsHeader) fileOut << "room_id,booking_date,time_slot,name,email,title,description\n";
    fileOut << csvEscape(id) << "," << csvEscape(day) << "," << csvEscape(selected) << ","
            << csvEscape(name) << "," << csvEscape(email) << "," << csvEscape(title) << ","
            << csvEscape(description) << "\n";

    cout << "OK|" << iterator->name << " booked for " << day << ", " << label(selected) << ".\n";
}


void runBenchmark(
    const string& csvPath
) {
    ifstream file(csvPath);

    if (!file.is_open()) {
        throw runtime_error(
            "Unable to open comparison dataset: "
            + csvPath
        );
    }

    map<string, BenchmarkPair> comparisons;

    string line;

    // Skip the CSV header.
    getline(file, line);

    while (getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        FixedList<string, 64> columns =
            splitCsvLine(line);

        /*
        Column positions in the CSV:

        0  record_id
        1  test_case_id
        2  approach
        3  dataset_size_rooms
        4  number_of_pax
        5  booking_date
        6  preferred_time_slot
        7  starting_location
        8  preferred_location
        9  candidate_rooms
        10 matching_rooms
        11 estimated_operations
        12 execution_time_ms
        13 memory_usage_kb

        Additional CSV columns may remain after column 13,
        but they are not required by the dashboard.
        */

        if (columns.size() < 14) {
            cerr
                << "Skipped incomplete CSV row: "
                << line
                << endl;

            continue;
        }

        try {
            string testCaseId = columns[1];
            string approach = columns[2];

            BenchmarkPair& comparison =
                comparisons[testCaseId];

            comparison.testCaseId =
                testCaseId;

            comparison.datasetSize =
                stoi(columns[3]);

            double operations =
                stod(columns[11]);

            double executionTime =
                stod(columns[12]);

            double memoryUsage =
                stod(columns[13]);

            if (approach == "Baseline") {
                comparison.baselineOperations =
                    operations;

                comparison.baselineTime =
                    executionTime;

                comparison.baselineMemory =
                    memoryUsage;

                comparison.hasBaseline = true;
            }

            else if (approach == "Optimized") {
                comparison.optimizedOperations =
                    operations;

                comparison.optimizedTime =
                    executionTime;

                comparison.optimizedMemory =
                    memoryUsage;

                comparison.hasOptimized = true;
            }
        }

        catch (const invalid_argument&) {
            cerr
                << "Skipped row containing invalid data: "
                << line
                << endl;
        }

        catch (const out_of_range&) {
            cerr
                << "Skipped row containing a number "
                   "that is too large: "
                << line
                << endl;
        }
    }

    file.close();

    FixedList<BenchmarkPair, 1000> completeComparisons;

    for (const auto& entry : comparisons) {
        const BenchmarkPair& comparison =
            entry.second;

        if (
            comparison.hasBaseline
            && comparison.hasOptimized
        ) {
            completeComparisons.push_back(
                comparison
            );
        }
    }

    sort(
        completeComparisons.begin(),
        completeComparisons.end(),
        [](
            const BenchmarkPair& first,
            const BenchmarkPair& second
        ) {
            return (
                first.datasetSize
                < second.datasetSize
            );
        }
    );

    if (completeComparisons.empty()) {
        throw runtime_error(
            "No complete baseline and optimized "
            "comparison pairs were found."
        );
    }

    for (
        const BenchmarkPair& comparison
        : completeComparisons
    ) {
        cout
            << fixed
            << setprecision(3)

            << "BENCH|"

            << comparison.datasetSize
            << "|"

            << comparison.baselineOperations
            << "|"

            << comparison.optimizedOperations
            << "|"

            << comparison.baselineTime
            << "|"

            << comparison.optimizedTime
            << "|"

            << comparison.baselineMemory
            << "|"

            << comparison.optimizedMemory

            << "\n";
    }
}

int main(int argc, char** argv) {
    try {
        if (argc < 2) throw runtime_error("Missing mode.");
        string mode = argv[1];
        if (mode == "search" && argc >= 8) runSearch(argv);
        else if (mode == "schedule" && argc >= 5) runSchedule(argv);
        else if (mode == "book" && argc >= 10) runBook(argv);
        else if (mode == "benchmark") {
            if (argc < 3) {
                throw runtime_error(
                    "Comparison CSV file path is required."
                );
            }

            runBenchmark(argv[2]);
        }
        else throw runtime_error("Invalid arguments.");
        return 0;
    } catch (const exception& error) {
        cerr << error.what() << "\n";
        return 1;
    }
}
