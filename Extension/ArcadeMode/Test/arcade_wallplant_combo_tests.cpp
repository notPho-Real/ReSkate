#include "Extension/ArcadeMode/arcade_combo_session.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk::arcade_mode;
namespace {
unsigned checks{};
void check(bool value,const char* message) {
    ++checks;if(!value){std::cerr<<message<<'\n';std::exit(1);}
}
void advance(ComboSession& combo,SkatePose pose,unsigned count,bool held=false) {
    for(unsigned i=0;i<count;++i)combo.step(pose,.02f,0,false,nullptr,false,false,held);
}
}
int main() {
    ComboSession plant,ordinary;
    advance(plant,SkatePose::air,1);advance(ordinary,SkatePose::air,1);
    advance(plant,SkatePose::riding,8,true);advance(ordinary,SkatePose::riding,8);
    check(plant.model().state().combo_active,"plant retains existing combo during authenticated contact");
    advance(plant,SkatePose::air,60);advance(ordinary,SkatePose::riding,60);
    check(plant.model().state().combo_active,"release continues original airborne combo");
    check(!ordinary.model().state().combo_active,"unowned ground observation still settles normally");
    check(!plant.take_score_settlement(),"plant lifecycle cannot manufacture score authority");
    advance(plant,SkatePose::riding,60);
    check(!plant.model().state().combo_active,"real post-release landing settles");
    ComboSession idle;advance(idle,SkatePose::riding,8,true);
    check(!idle.model().state().combo_active,"hold flag cannot create combo on ground");
    ComboSession bailed;advance(bailed,SkatePose::air,1);advance(bailed,SkatePose::bail,1,true);
    check(!bailed.model().state().combo_active,"real bail overrides plant preservation");
    std::cout<<checks<<" wallplant combo checks passed\n";
}
