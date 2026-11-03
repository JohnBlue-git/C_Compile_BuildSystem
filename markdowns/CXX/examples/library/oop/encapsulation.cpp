// Access specifiers, static members, const members, explicit, and friend.
#include <iostream>

class Inspector;  // forward declaration for the friend below

class Account {
public:
    explicit Account(int balance = 0) : balance_(balance), id_(nextId_++) {}
    virtual ~Account() = default;

    void deposit(int amount) { balance_ += amount; }       // public interface
    int id() const { return id_; }
    static int created() { return nextId_ - 1; }            // no `this`: cannot touch balance_

protected:
    void audit() const {}  // usable by Account, derived classes, and friends

private:
    int balance_;      // only Account and its friends can touch it
    const int id_;     // const member: must be set in the member initializer list
    inline static int nextId_ = 1;  // C++17: in-class definition of a static member
    static constexpr int kMaxBalance = 1'000'000;  // compile-time class constant

    friend class Inspector;  // grants Inspector access to private members
};

class Inspector {
public:
    static int balanceOf(const Account& a) { return a.balance_; }  // OK: friend
    static int limit() { return Account::kMaxBalance; }
};

int main() {
    Account a(3);
    // Account b = 3;  // error: explicit constructor blocks implicit conversion
    a.deposit(4);
    // a.balance_ = 0; // error: private

    Account b(10);
    std::cout << "a.id = " << a.id() << ", b.id = " << b.id() << '\n';
    std::cout << "accounts created: " << Account::created() << '\n';
    std::cout << "a balance (via friend): " << Inspector::balanceOf(a) << '\n';
    std::cout << "limit: " << Inspector::limit() << '\n';
}
