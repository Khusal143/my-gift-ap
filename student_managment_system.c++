#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <limits>
#include <functional> // For std::hash
#include <unistd.h>
#include <termios.h>

using namespace std;

// ==========================================
// UTILITY FUNCTIONS
// ==========================================

// Clears the input buffer to prevent infinite loops on bad input
void clearInputBuffer() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// Safely gets an integer from the user
int getIntInput(const string& prompt) {
    int value;
    while (true) {
        cout << prompt;
        if (cin >> value) {
            clearInputBuffer();
            return value;
        }
        cout << "[Error] Invalid input. Please enter a valid number.\n";
        clearInputBuffer();
    }
}

// Safely gets a float from the user
float getFloatInput(const string& prompt) {
    float value;
    while (true) {
        cout << prompt;
        if (cin >> value) {
            clearInputBuffer();
            return value;
        }
        cout << "[Error] Invalid input. Please enter a valid decimal number.\n";
        clearInputBuffer();
    }
}

// Masked password utility for POSIX terminals
string getPasswordInput() {
    string password;
    struct termios oldt, newt;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~ECHO; // Disable echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    getline(cin, password);

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    cout << endl;

    return password;
}

// ==========================================
// MODELS
// ==========================================

class Student {
private:
    int rollNumber;
    string name;
    float gpa;
    string course;

public:
    Student(int roll, string n, float g, string c) 
        : rollNumber(roll), name(std::move(n)), gpa(g), course(std::move(c)) {}

    [[nodiscard]] int getRollNumber() const { return rollNumber; }
    [[nodiscard]] string getName() const { return name; }
    [[nodiscard]] float getGPA() const { return gpa; }
    [[nodiscard]] string getCourse() const { return course; }

    void setName(const string& n) { name = n; }
    void setGPA(float g) { gpa = g; }
    void setCourse(const string& c) { course = c; }

    void displayRow() const {
        cout << left << setw(12) << rollNumber
             << setw(25) << name
             << setw(12) << fixed << setprecision(2) << gpa
             << setw(15) << course << endl;
    }
};

// ==========================================
// MANAGERS
// ==========================================

class AuthManager {
private:
    const string authFile = "users.txt";

    // Basic hashing for demonstration. 
    // In production, use a secure cryptographic hash (e.g., bcrypt/Argon2).
    string hashPassword(const string& password) const {
        size_t hashValue = std::hash<string>{}(password);
        return to_string(hashValue);
    }

public:
    void registerUser() {
        string username, password;
        
        cout << "\n--- REGISTER NEW ADMIN ---\n";
        cout << "Enter Username: ";
        getline(cin, username);

        // Check if user exists
        ifstream inFile(authFile);
        string fileUser, filePass;
        if (inFile) {
            while (inFile >> fileUser >> filePass) {
                if (fileUser == username) {
                    cout << "[Error] Username already exists! Please log in.\n";
                    return;
                }
            }
            inFile.close();
        }

        cout << "Enter Password: ";
        password = getPasswordInput();
        string hashedPass = hashPassword(password);

        ofstream outFile(authFile, ios::app);
        if (outFile) {
            outFile << username << " " << hashedPass << "\n";
            cout << "[Success] Account created successfully!\n";
        } else {
            cout << "[Error] Could not access database.\n";
        }
    }

    bool loginUser() {
        string username, password;
        
        cout << "\n--- ADMIN LOGIN ---\n";
        cout << "Enter Username: ";
        getline(cin, username);
        cout << "Enter Password: ";
        password = getPasswordInput();
        
        string hashedPass = hashPassword(password);

        ifstream inFile(authFile);
        if (!inFile) {
            cout << "[Error] No accounts found. Please register first.\n";
            return false;
        }

        string fileUser, filePass;
        while (inFile >> fileUser >> filePass) {
            if (fileUser == username && filePass == hashedPass) {
                cout << "[Success] Authentication verified.\n";
                return true;
            }
        }

        cout << "[Access Denied] Invalid username or password.\n";
        return false;
    }
};

class StudentManager {
private:
    vector<Student> students;
    const string fileName = "students.txt";

    void saveToFile() const {
        ofstream outFile(fileName, ios::trunc);
        if (!outFile) {
            cout << "\n[Error] Could not open file for writing!\n";
            return;
        }
        for (const auto& s : students) {
            outFile << s.getRollNumber() << "\n"
                    << s.getName() << "\n"
                    << s.getGPA() << "\n"
                    << s.getCourse() << "\n";
        }
    }

    void loadFromFile() {
        ifstream inFile(fileName);
        if (!inFile) return;

        int roll;
        string name, course;
        float gpa;

        while (inFile >> roll) {
            inFile >> ws; // Consume whitespace before getline
            getline(inFile, name);
            inFile >> gpa;
            inFile >> ws; // Consume whitespace before getline
            getline(inFile, course);

            students.emplace_back(roll, name, gpa, course);
        }
    }

public:
    StudentManager() {
        loadFromFile();
    }

    void addStudent() {
        cout << "\n--- ADD STUDENT ---\n";
        int roll = getIntInput("Enter Roll Number: ");

        // Check for duplicates
        for (const auto& s : students) {
            if (s.getRollNumber() == roll) {
                cout << "[Error] Student with Roll Number " << roll << " already exists!\n";
                return;
            }
        }

        string name, course;
        cout << "Enter Full Name: ";
        getline(cin, name);
        
        float gpa = getFloatInput("Enter GPA (0.0 - 4.0): ");
        
        cout << "Enter Course/Major: ";
        getline(cin, course);

        students.emplace_back(roll, name, gpa, course);
        saveToFile();
        cout << "[Success] Student record added!\n";
    }

    void displayAll() const {
        if (students.empty()) {
            cout << "\n[Info] Database is empty.\n";
            return;
        }

        cout << "\n---------------------------------------------------------------\n";
        cout << left << setw(12) << "Roll No" << setw(25) << "Name" << setw(12) << "GPA" << setw(15) << "Course" << endl;
        cout << "---------------------------------------------------------------\n";
        for (const auto& s : students) {
            s.displayRow();
        }
        cout << "---------------------------------------------------------------\n";
    }

    void searchStudent() const {
        if (students.empty()) {
            cout << "\n[Info] Database is empty.\n";
            return;
        }

        int roll = getIntInput("\nEnter Roll Number to search: ");

        for (const auto& s : students) {
            if (s.getRollNumber() == roll) {
                cout << "\n[Record Found]\n";
                cout << "---------------------------------------------------------------\n";
                cout << left << setw(12) << "Roll No" << setw(25) << "Name" << setw(12) << "GPA" << setw(15) << "Course" << endl;
                cout << "---------------------------------------------------------------\n";
                s.displayRow();
                cout << "---------------------------------------------------------------\n";
                return;
            }
        }
        cout << "\n[Error] Student not found.\n";
    }

    void updateStudent() {
        int roll = getIntInput("\nEnter Roll Number to update: ");

        for (auto& s : students) {
            if (s.getRollNumber() == roll) {
                string newName, newCourse;
                
                cout << "Enter New Full Name (Current: " << s.getName() << "): ";
                getline(cin, newName);
                
                float newGpa = getFloatInput("Enter New GPA (Current: " + to_string(s.getGPA()) + "): ");
                
                cout << "Enter New Course (Current: " << s.getCourse() << "): ";
                getline(cin, newCourse);

                s.setName(newName);
                s.setGPA(newGpa);
                s.setCourse(newCourse);

                saveToFile();
                cout << "[Success] Record updated successfully!\n";
                return;
            }
        }
        cout << "\n[Error] Student not found.\n";
    }

    void deleteStudent() {
        int roll = getIntInput("\nEnter Roll Number to delete: ");

        for (auto it = students.begin(); it != students.end(); ++it) {
            if (it->getRollNumber() == roll) {
                students.erase(it);
                saveToFile();
                cout << "[Success] Student record deleted.\n";
                return;
            }
        }
        cout << "\n[Error] Student not found.\n";
    }
};

// ==========================================
// MAIN APPLICATION ENTRY
// ==========================================

void runDashboard() {
    StudentManager sm;
    bool running = true;

    while (running) {
        cout << "\n=========================================\n";
        cout << "         STUDENT MANAGEMENT SYSTEM       \n";
        cout << "=========================================\n";
        cout << "1. Add Student\n";
        cout << "2. View All Students\n";
        cout << "3. Search Student\n";
        cout << "4. Update Student\n";
        cout << "5. Delete Student\n";
        cout << "6. Logout\n";
        cout << "=========================================\n";
        
        int choice = getIntInput("Select Option: ");

        switch (choice) {
            case 1: sm.addStudent(); break;
            case 2: sm.displayAll(); break;
            case 3: sm.searchStudent(); break;
            case 4: sm.updateStudent(); break;
            case 5: sm.deleteStudent(); break;
            case 6: 
                cout << "\nLogging out...\n";
                running = false; 
                break;
            default: cout << "[Error] Invalid option. Choose 1-6.\n";
        }
    }
}

int main() {
    AuthManager auth;
    bool systemActive = true;

    while (systemActive) {
        cout << "\n=========================================\n";
        cout << "              PORTAL LOGIN               \n";
        cout << "=========================================\n";
        cout << "1. Sign In\n";
        cout << "2. Sign Up\n";
        cout << "3. Exit System\n";
        cout << "=========================================\n";
        
        int choice = getIntInput("Select Option: ");

        switch (choice) {
            case 1:
                if (auth.loginUser()) {
                    runDashboard(); // Launch the management system on success
                }
                break;
            case 2:
                auth.registerUser();
                break;
            case 3:
                cout << "\nShutting down system. Goodbye!\n";
                systemActive = false;
                break;
            default:
                cout << "[Error] Invalid option. Choose 1-3.\n";
        }
    }

    return 0;
}