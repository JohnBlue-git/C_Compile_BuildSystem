// Meyers singleton holding nested maps (company -> branch -> employee).
#include <iostream>
#include <map>
#include <string>

using EmployeeList = std::map<unsigned int, std::string>;  // employee ID -> name
using BranchList = std::map<unsigned int, EmployeeList>;   // branch ID -> employees
using CompanyList = std::map<std::string, BranchList>;     // company name -> branches

class Cache {
public:
    static Cache& instance() {
        static Cache cache;  // initialized once, thread-safe since C++11
        return cache;
    }

    Cache(const Cache&) = delete;
    Cache& operator=(const Cache&) = delete;

    // Note: the member functions below are NOT synchronized. Add a mutex if
    // several threads use the cache.
    void updateEmployee(const std::string& company, unsigned int branchId,
                        unsigned int employeeId, const std::string& name) {
        BranchList& branches = companies_[company];  // operator[] inserts if missing
        EmployeeList& employees = branches[branchId];
        employees[employeeId] = name;
    }

    void updateBranch(const std::string& company, unsigned int branchId,
                      const EmployeeList& employees) {
        companies_[company][branchId] = employees;
    }

    void updateCompany(const std::string& company, const BranchList& branches) {
        companies_[company] = branches;
    }

    void print() const {
        for (const auto& [company, branches] : companies_) {
            std::cout << "Company: " << company << '\n';
            for (const auto& [branchId, employees] : branches) {
                std::cout << "  Branch ID: " << branchId << '\n';
                for (const auto& [employeeId, name] : employees) {
                    std::cout << "    Employee ID: " << employeeId << ", Name: " << name << '\n';
                }
            }
        }
    }

private:
    Cache() = default;
    CompanyList companies_;
};

int main() {
    Cache& cache = Cache::instance();

    cache.updateEmployee("Acme", 101, 1, "Alice");
    cache.updateEmployee("Acme", 101, 2, "Bob");
    cache.updateBranch("Acme", 102, {{3, "Charlie"}, {4, "Diana"}});
    cache.updateCompany("Globex", {{201, {{5, "Eve"}, {6, "Frank"}}}, {202, {{7, "Grace"}}}});

    Cache::instance().print();  // same object as `cache`
}
