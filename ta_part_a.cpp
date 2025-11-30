#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <cerrno>
#include <cstring>

using namespace std;

constexpr int MAX_EXAMS = 200;
constexpr int MAX_PATH = 512;
constexpr int RUBRIC_LINES = 5;


struct SharedData {
    // list of exam file paths
    char exams[MAX_EXAMS][MAX_PATH];
    int total_exams;
    int current_index; // which file index is currently loaded

    // loaded exam info
    char exam_filename[MAX_PATH];
    char exam_student[16]; 

    // status of each question in current exam:
    // 0 = unmarked, 1 = being_marked, 2 = marked
    int question_status[RUBRIC_LINES];

    // rubric letters (first character after comma for each of 5 lines)
    char rubric[RUBRIC_LINES];

    // finish flag (0 = run, 1 = finish)
    int finished;
};

