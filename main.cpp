#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <random>
#include <string>
#include <iomanip>

using namespace std;
using namespace std::chrono;

struct Job {
    int id;
    vector<int> processing_times;
    int total_time;
};

int calculateMakespan(const vector<Job>& jobs, const vector<int>& sequence, int m) {
    if (sequence.empty()) return 0;

    int n = sequence.size();
    vector<vector<int>> completion_times(n, vector<int>(m, 0));

    completion_times[0][0] = jobs[sequence[0]].processing_times[0];
    for (int j = 1; j < m; ++j) {
        completion_times[0][j] = completion_times[0][j-1] + jobs[sequence[0]].processing_times[j];
    }

    for (int i = 1; i < n; ++i) {
        completion_times[i][0] = completion_times[i-1][0] + jobs[sequence[i]].processing_times[0];
        for (int j = 1; j < m; ++j) {
            completion_times[i][j] = max(completion_times[i-1][j], completion_times[i][j-1]) + jobs[sequence[i]].processing_times[j];
        }
    }

    return completion_times[n-1][m-1];
}

void printGanttChart(int n, int m, const vector<Job>& jobs,
                     const vector<vector<int>>& start_times,
                     const vector<vector<int>>& end_times,
                     const vector<int>& sequence) {
    cout << "\nUPROSZCZONY WYKRES GANTTA (Wizualizacja zajetosci maszyn):" << endl;
    for (int j = 0; j < m; j++) {
        cout << "Maszyna " << j << ": ";
        int current_pos = 0;
        for (int k : sequence) {
            while (current_pos < start_times[k][j]) {
                cout << "-";
                current_pos++;
            }
            for (int t = 0; t < jobs[k].processing_times[j]; t++) {
                cout << (char)('A' + k);
                current_pos++;
            }
        }
        cout << " (Koniec: " << end_times[sequence.back()][j] << ")" << endl;
    }
}

// Algorytm NEH z pelnym raportem i zapisem do CSV
void nehAlgorithm(vector<Job> jobs, int m, const string& filename) {
    auto start_time = high_resolution_clock::now();

    sort(jobs.begin(), jobs.end(), [](const Job& a, const Job& b) {
        return a.total_time > b.total_time;
    });

    vector<int> current_sequence;

    for (int i = 0; i < jobs.size(); ++i) {
        int best_cmax = 1e9;
        int best_position = -1;
        vector<int> best_sequence;

        for (int pos = 0; pos <= current_sequence.size(); ++pos) {
            vector<int> temp_sequence = current_sequence;
            temp_sequence.insert(temp_sequence.begin() + pos, i);

            int current_cmax = calculateMakespan(jobs, temp_sequence, m);

            if (current_cmax < best_cmax) {
                best_cmax = current_cmax;
                best_position = pos;
                best_sequence = temp_sequence;
            }
        }
        current_sequence = best_sequence;
    }

    auto end_time = high_resolution_clock::now();
    double duration = duration_cast<microseconds>(end_time - start_time).count() / 1e6;

    int n = jobs.size();
    vector<vector<int>> start_times(n, vector<int>(m, 0));
    vector<vector<int>> end_times(n, vector<int>(m, 0));

    end_times[current_sequence[0]][0] = jobs[current_sequence[0]].processing_times[0];
    start_times[current_sequence[0]][0] = 0;

    for (int j = 1; j < m; ++j) {
        start_times[current_sequence[0]][j] = end_times[current_sequence[0]][j-1];
        end_times[current_sequence[0]][j] = start_times[current_sequence[0]][j] + jobs[current_sequence[0]].processing_times[j];
    }

    for (int i = 1; i < n; ++i) {
        int curr_job = current_sequence[i];
        int prev_job = current_sequence[i-1];

        start_times[curr_job][0] = end_times[prev_job][0];
        end_times[curr_job][0] = start_times[curr_job][0] + jobs[curr_job].processing_times[0];

        for (int j = 1; j < m; ++j) {
            start_times[curr_job][j] = max(end_times[prev_job][j], end_times[curr_job][j-1]);
            end_times[curr_job][j] = start_times[curr_job][j] + jobs[curr_job].processing_times[j];
        }
    }

    int final_cmax = end_times[current_sequence.back()][m-1];

    cout << "\n==================================================" << endl;
    cout << "RAPORT KONCOWY (Heurystyka NEH) DLA: " << filename << endl;
    cout << "==================================================" << endl;
    cout << "Liczba zadan: " << n << " | Liczba maszyn: " << m << endl;
    cout << "Czas obliczen: " << fixed << setprecision(6) << duration << " s" << endl;
    cout << "Osiagniete Cmax: " << final_cmax << endl;
    cout << "--------------------------------------------------" << endl;

    cout << left << setw(10) << "Kolejnosc" << setw(15) << "Nazwa" << "Harmonogram (Start-Koniec)" << endl;
    for (int idx = 0; idx < current_sequence.size(); idx++) {
        int job_id = current_sequence[idx];
        cout << left << setw(10) << idx + 1 << setw(15) << "Zadanie_" + to_string(job_id);
        for (int j = 0; j < m; j++) {
            cout << "M" << j << ":[" << start_times[job_id][j] << "-" << end_times[job_id][j] << "] ";
        }
        cout << endl;
    }

    printGanttChart(n, m, jobs, start_times, end_times, current_sequence);
    cout << "==================================================" << endl;

    // ZAPIS DO PLIKU CSV
    ofstream csvFile("wyniki.csv", ios::app);
    csvFile.seekp(0, ios::end);
    if (csvFile.tellp() == 0) {
        csvFile << "Algorytm,Instancja,N,M,Cmax,Czas_s\n";
    }
    csvFile << "NEH," << filename << "," << n << "," << m << "," << final_cmax << ","
            << fixed << setprecision(6) << duration << "\n";
    csvFile.close();
}

void generateRandomInstance(const string& filename, int n, int m, int min_time, int max_time) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Blad podczas tworzenia pliku!" << endl;
        return;
    }

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(min_time, max_time);

    file << n << " " << m << "\n";
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            file << dist(gen) << (j == m - 1 ? "" : " ");
        }
        file << "\n";
    }

    file.close();
    cout << "Wygenerowano plik: " << filename << " (" << n << " zadan, " << m << " maszyn)\n";
}

bool loadFile(const string& filename, vector<Job>& jobs, int& n, int& m) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Plik " << filename << " nie zostal znaleziony!" << endl;
        return false;
    }

    string temp;
    while (file >> temp) {
        try {
            n = stoi(temp);
            break;
        } catch (...) {
            // Ignorujemy tekst przed liczbami
        }
    }

    file >> m;
    jobs.clear();
    jobs.resize(n);

    for (int i = 0; i < n; ++i) {
        jobs[i].id = i;
        int sum_time = 0;
        for (int j = 0; j < m; ++j) {
            int t;
            file >> t;
            jobs[i].processing_times.push_back(t);
            sum_time += t;
        }
        jobs[i].total_time = sum_time;
    }

    file.close();
    return true;
}

int main() {
    int choice;
    string filename;
    vector<Job> jobs;
    int n = 0, m = 0;

    while (true) {
        cout << "\n--- MENU FLOW SHOP (HEURYSTYKA NEH) ---" << endl;
        cout << "1. Wczytaj pojedynczy plik" << endl;
        cout << "2. Wygeneruj losowe dane" << endl;
        cout << "3. Uruchom testy wsadowe (Batch Run - zapis do CSV)" << endl;
        cout << "4. Wyjscie" << endl;
        cout << "Twoj wybor: ";
        cin >> choice;

        if (choice == 1) {
            cout << "Podaj nazwe pliku (np. car5.txt): ";
            cin >> filename;
            if (loadFile(filename, jobs, n, m)) {
                nehAlgorithm(jobs, m, filename);
            }
        }
        else if (choice == 2) {
            int gen_n, gen_m, min_t, max_t;
            cout << "Podaj nazwe dla nowego pliku: ";
            cin >> filename;
            cout << "Liczba zadan (n): ";
            cin >> gen_n;
            cout << "Liczba maszyn (m): ";
            cin >> gen_m;
            cout << "Minimalny czas operacji: ";
            cin >> min_t;
            cout << "Maksymalny czas operacji: ";
            cin >> max_t;
            generateRandomInstance(filename, gen_n, gen_m, min_t, max_t);
        }
        else if (choice == 3) {
            // Lista plikow do przetworzenia w Batch Run
            vector<string> test_files = {
                "dane.txt", "car1.txt", "car2.txt", "car3.txt", "car4.txt", "car5.txt",
                "tai10x5.txt", "tai10x10.txt", "tai20x5.txt", "nowy.txt", "test_30_10.txt", "test_100_20.txt", "test_100_50.txt"
            };

            cout << "\nRozpoczynam testy wsadowe..." << endl;
            for (const string& file : test_files) {
                if (loadFile(file, jobs, n, m)) {
                    cout << ">>> Przetwarzanie: " << file << " <<<" << endl;
                    nehAlgorithm(jobs, m, file);
                } else {
                    cout << "Pominiecie pliku: " << file << endl;
                }
            }
            cout << "\nTESTY ZAKONCZONE! Wyniki zapisano w pliku wyniki.csv" << endl;
        }
        else if (choice == 4) {
            cout << "Wyjscie z programu..." << endl;
            break;
        }
        else {
            cout << "Nieprawidlowy wybor. Sprobuj ponownie." << endl;
        }
    }

    return 0;
}