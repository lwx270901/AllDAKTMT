#include <iostream>
#include <thread>
#include <chrono>




class FlagChanger {
public:
    FlagChanger() ;
    ~FlagChanger();
    bool getFlag() const;
private:
    bool flag_;
    std::thread thread_;
    void changeFlag();
};