#include "FlagChanger.h"


FlagChanger::FlagChanger() {
    flag_ = false;
    thread_ = std::thread(&FlagChanger::changeFlag, this);
}

FlagChanger::~FlagChanger (){
    thread_.join();
}

bool FlagChanger::getFlag() const {
    return flag_;
}

void FlagChanger::changeFlag() {
    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(5));
        flag_ = !flag_;
    }
}