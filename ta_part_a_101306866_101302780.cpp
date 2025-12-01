#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <ctime>
#include <cerrno>
#include <iomanip>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>

using namespace std;

constexpr int MAX_EXAMS = 100; //must run at least 20 exams
constexpr int Q = 5; //5 questions in each rubric

struct Shared {
    int totalExams;                 // number of exam files found
    int currentIndex;               // index of currently loaded exam
    vector<string> examPaths;       // list of exam file paths
    string loadedExam;              // path of currently loaded exam
    string loadedStudent;           // student id
    int questionStatus[Q];          // 0 = unmarked, 1 = being_marked, 2 = marked
    char rubric[Q];                 // one char per question
    int finished;                   // 0 running, 1 finish
};

static double rand_between(double lo, double hi) {
    return lo + (hi - lo) * (rand() / double(RAND_MAX));
}

// Reads rubric into shared memory
void loadRubricFile(const string &rubricPath, Shared *S) {
    ifstream in(rubricPath);
    if (!in.is_open()) {
        for (int i = 0; i < Q; ++i) S->rubric[i] = 'A' + i;
        return;
    }
    string line;
    int i = 0;
    while (i < Q && getline(in, line)) {
        auto p = line.find(',');
        if (p != string::npos) {
            size_t j = p + 1;
            while (j < line.size() && isspace((unsigned char)line[j])) ++j;
            S->rubric[i++] = (j < line.size() ? line[j] : '?');
        } else {
            S->rubric[i++] = '?';
        }
    }
    for (; i < Q; ++i) S->rubric[i] = 'A' + i;
}

// Write shared memory rubric back to file
void saveRubricFile(const string &rubricPath, Shared *S) {
    ofstream out(rubricPath, ios::trunc);
    if (!out.is_open()) {
        cerr << "pid " << getpid() << " warning: cannot write " << rubricPath << "\n";
        return;
    }
    for (int i = 0; i < Q; ++i) out << (i+1) << ", " << S->rubric[i] << "\n";
}

// Load current exam into shared memory
void loadCurrentExam(Shared *S) {
    if (S->currentIndex < 0 || S->currentIndex >= S->totalExams) {
        S->loadedExam.clear();
        S->loadedStudent = "0000";
        for (int i=0;i<Q;++i) S->questionStatus[i] = 0;
        return;
    }

    S->loadedExam = S->examPaths[S->currentIndex];

    ifstream fin(S->loadedExam);
    string first;
    if (!fin.is_open() || !getline(fin, first)) {
        S->loadedStudent = "0000";
    } else {
        string digits;
        for (char c : first) {
            if (isdigit((unsigned char)c)) digits.push_back(c);
            else if (!digits.empty()) break;
        }
        S->loadedStudent = digits.empty() ? "0000" : digits;
    }

    for (int i=0;i<Q;++i) S->questionStatus[i] = 0;
}

// TA process work
void ta_main(int ta_id, Shared *S, const string &rubricPath) {
    srand((unsigned)time(nullptr) ^ (getpid()<<8) ^ ta_id);

    while (true) {
        if (S->finished) {
            cout << "[TA " << ta_id << " pid " << getpid() << "] finishing (finished flag)\n";
            _exit(0);
        }

        cout << "[TA " << ta_id << " pid " << getpid() << "] Reviewing rubric for exam "
             << S->loadedExam << " (student " << S->loadedStudent << ")\n";

        for (int r = 0; r < Q; ++r) {
            double waitt = rand_between(0.5, 1.0);
            usleep(static_cast<useconds_t>(waitt * 1e6));

            if (rand() % 2 == 0) {
                char old = S->rubric[r];
                S->rubric[r] = static_cast<char>(S->rubric[r] + 1);
                cout << "[TA " << ta_id << " pid " << getpid() << "] Changed rubric Q" << (r+1)
                     << ": " << old << " -> " << S->rubric[r] << " (saving)\n";
                saveRubricFile(rubricPath, S);
            } else {
                cout << "[TA " << ta_id << " pid " << getpid() << "] No change Q" << (r+1)
                     << " (currently " << S->rubric[r] << ")\n";
            }
        }

        // Mark questions
        bool markedAny = false;
        while (true) {
            if (S->finished) break;

            int pick = -1;
            for (int q = 0; q < Q; ++q) {
                if (S->questionStatus[q] == 0) { pick = q; break; }
            }
            if (pick == -1) break;

            S->questionStatus[pick] = 1;
            double markt = rand_between(1.0, 2.0);
            cout << fixed << setprecision(2);
            cout << "[TA " << ta_id << " pid " << getpid() << "] Marking student " << S->loadedStudent
                 << " Q" << (pick+1) << " (will take " << markt << "s)\n";
            usleep(static_cast<useconds_t>(markt * 1e6));

            S->questionStatus[pick] = 2;
            markedAny = true;
            cout << "[TA " << ta_id << " pid " << getpid() << "] Finished student " << S->loadedStudent
                 << " Q" << (pick+1) << "\n";
        }

        if (!markedAny) usleep(100000);

        bool complete = true;
        for (int q = 0; q < Q; ++q) if (S->questionStatus[q] != 2) { complete = false; break; }

        if (complete) {
            cout << "[TA " << ta_id << " pid " << getpid() << "] Detected exam " << S->loadedExam
                 << " (student " << S->loadedStudent << ") complete.\n";

            if (S->currentIndex + 1 >= S->totalExams) {
                cout << "[TA " << ta_id << " pid " << getpid() << "] No more exams. Setting finished flag.\n";
                S->finished = 1;
                break;
            } else {
                S->currentIndex++;
                cout << "[TA " << ta_id << " pid " << getpid() << "] Loading next exam index "
                     << S->currentIndex << ": " << S->examPaths[S->currentIndex] << "\n";
                loadCurrentExam(S);
                cout << "[TA " << ta_id << " pid " << getpid() << "] Loaded next exam "
                     << S->loadedExam << " (student " << S->loadedStudent << ")\n";
                if (S->loadedStudent == "9999") {
                    cout << "[TA " << ta_id << " pid " << getpid() << "] Found student 9999. Setting finished.\n";
                    S->finished = 1;
                    break;
                }
            }
        }
        usleep(100000);
    }

    _exit(0);
}

int main(int argc, char **argv) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <num_TAs> <exams_dir> <rubric.txt>\n";
        return 1;
    }
    int n = atoi(argv[1]);
    if (n < 2) { cerr << "Please provide n >= 2\n"; return 1; }
    string examsDir = argv[2];
    string rubricPath = argv[3];

    Shared *S = (Shared*) mmap(nullptr, sizeof(Shared),
                               PROT_READ | PROT_WRITE,
                               MAP_SHARED | MAP_ANON, -1, 0);
    if (S == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    memset(S, 0, sizeof(Shared));

    // gather exam filenames
    DIR *d = opendir(examsDir.c_str());
    if (!d) { perror("opendir"); munmap(S, sizeof(Shared)); return 1; }
    vector<string> files;
    struct dirent *ent;
    while ((ent = readdir(d)) != nullptr) {
        string name = ent->d_name;
        if (name == "." || name == "..") continue;
        files.push_back(name);
    }
    closedir(d);
    sort(files.begin(), files.end());
    for (size_t i = 0; i < files.size() && i < MAX_EXAMS; ++i) {
        S->examPaths.push_back(examsDir + "/" + files[i]);
        S->totalExams++;
    }
    if (S->totalExams == 0) { cerr << "No exam files found\n"; munmap(S, sizeof(Shared)); return 1; }

    loadRubricFile(rubricPath, S);
    S->currentIndex = 0;
    loadCurrentExam(S);

    cout << "Parent: loaded " << S->totalExams << " exams. Starting with: " << S->loadedExam
         << " (student " << S->loadedStudent << ")\n";

    vector<pid_t> children;
    for (int i = 0; i < n; ++i) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); continue; }
        if (pid == 0) { ta_main(i+1, S, rubricPath); }
        else { children.push_back(pid); }
    }

    int status;
    while (!children.empty()) {
        pid_t p = wait(&status);
        if (p <= 0) break;
        children.erase(remove(children.begin(), children.end(), p), children.end());
    }

    munmap(S, sizeof(Shared));
    cout << "Parent: all TAs finished.\n";
    return 0;
}
