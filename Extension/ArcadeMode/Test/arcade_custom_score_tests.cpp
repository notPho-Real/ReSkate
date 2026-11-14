#include "Extension/ArcadeMode/arcade_combo_session.h"
#include "Extension/ArcadeMode/arcade_input_telemetry.h"
#include "Extension/ArcadeMode/arcade_custom_score_display.h"
#include <cstdlib>
#include <iostream>
using namespace dingosdk;
using namespace dingosdk::arcade_mode;
void check(bool ok,const char* why){if(!ok){std::cerr<<why<<'\n';std::exit(1);}}
int main(){
    CustomScoreEvidence telemetry{1000,1,7,250,0,static_cast<unsigned>(ArcadeTrick::casper),2};
    check(client_telemetry_valid(telemetry),"performed custom score fits the detached telemetry contract");
    telemetry.kind=1;check(!client_telemetry_valid(telemetry),"a terminal cannot carry performed action fields");

    const NativeInputOwner owner{1,2,3,4,5};
    Model model;CustomTrickScoring score;
    CustomTrickExecution e{owner,1,7,1,1,1000,ArcadeTrick::casper,true,0};
    check(!score.observe(model,e,owner,1,1000),"no score outside a live combo");
    model.begin_balance(BalanceKind::manual,.4f);
    const auto balance=model.state().balance;
    check(score.observe(model,e,owner,1,1000),"acknowledged held entry scores");
    check(model.state().custom_points==250 && model.state().multiplier==2 &&
        model.state().balance==balance && model.state().balance_kind==BalanceKind::manual,
        "entry adds one action without resetting the physical balance");
    check(!score.observe(model,e,owner,1,1000),"duplicate entry cannot score");
    e.performed=false;e.held_seconds=1.f/60;
    for(unsigned i=0;i<60;++i){++e.step;e.at+=17;check(score.observe(model,e,owner,1,e.at),"native hold advances");}
    check(model.state().custom_points==350 && model.state().multiplier==2,"one second earns 100 points once");
    auto bad=e;++bad.step;
    check(!score.observe(model,bad,{1,99,3,4,5},1,bad.at),"wrong owner refused");
    check(!score.observe(model,bad,owner,2,bad.at),"wrong generation refused");
    check(!score.observe(model,bad,owner,1,bad.at+151),"stale evidence refused");
    bad.held_seconds=1;check(!score.observe(model,bad,owner,1,bad.at),"time jump refused");
    bad=e;++bad.step;++bad.occurrence;bad.performed=false;
    check(!score.observe(model,bad,owner,1,bad.at),"hold cannot invent a new trick");
    e.trick=ArcadeTrick::casper_flip;e.performed=true;e.held_seconds=0;++e.occurrence;++e.step;
    check(score.observe(model,e,owner,1,e.at),"completed flip scores");
    check(model.state().custom_points==850 && model.state().multiplier==3 &&
        model.state().balance==balance,"flip is a scored branch in the same balance");
    ++e.step;check(!score.observe(model,e,owner,1,e.at),"replayed completed flip refused");
    ScoreSettlement native;
    check(!native.begin(model,{11,7,3,e.at,0,0,0,0,true,true},0) &&
        model.state().custom_points==850,"custom bonus cannot become a native score attestation");
    CustomScoreDisplay display;tricks::ScoreHud hud;hud.rows_read=true;
    hud.rows={{2,"Fakie Manual"},{1,"Kickflip"}};hud.line_score=70;hud.line_shown=100;
    const auto line=display.update(model,score,hud,e.at);
    check(line && line->tricks==std::vector<std::string>{"Kickflip","Fakie Manual","Casper","Casper Flip"},
        "native rows retain order alongside distinct custom identities");
    check(line->points==2620 && model.state().raw_points==0,"native display is passed through without manufacturing native evidence");
    model.manual_airborne();model.trick(0);model.landing();model.tick(.5f);
    check(model.last_settlement().kind==SettlementKind::bank &&
        model.last_settlement().custom_points==3400,"clean landing banks bonus with continuing combo multiplier");
    check(model.take_banked_points()==3400 && model.take_banked_points()==0,"bank drains once");
    const auto closed=display.update(model,score,hud,e.at+500);
    check(closed && closed->closed && closed->line==3500,"banked bonus remains in summary");
    model.begin_balance(BalanceKind::manual);score.clear();
    e={owner,1,8,3,90,3000,ArcadeTrick::casper,true,0};
    check(score.observe(model,e,owner,1,e.at),"fresh combo can score new occurrence");
    model.bail();
    check(model.take_banked_points()==0 && !model.state().custom_points,"bail loses provisional points");
    check(!display.update(model,score,hud,3001),"bail removes custom score summary");
    // A terminal and fresh balance can occur between two UI polls.
    Model rollover;CustomTrickScoring rollover_score;CustomScoreDisplay rollover_display;
    rollover.begin_balance(BalanceKind::manual);
    CustomTrickExecution first{owner,1,20,10,100,4000,ArcadeTrick::casper,true,0};
    check(rollover_score.observe(rollover,first,owner,1,4000),"rollover entry scored");
    check(rollover_display.update(rollover,rollover_score,hud,4000).has_value(),"scored combo shown");
    rollover.bank();rollover.begin_balance(BalanceKind::manual);
    check(!rollover_display.update(rollover,rollover_score,hud,4100),"prior custom rows and bank never reopen in a new native combo");
    auto stale=first;stale.trick=ArcadeTrick::casper_flip;++stale.occurrence;++stale.step;stale.at=4100;
    check(!rollover_score.observe(rollover,stale,owner,1,4100),"prior episode flip cannot score after settlement");
    first.episode=21;first.occurrence=12;first.step=102;first.at=4200;
    check(rollover_score.observe(rollover,first,owner,1,4200),"fresh episode starts its own custom score");
    hud.rows_read=false;
    check(!rollover_display.update(rollover,rollover_score,hud,4200),"missing native history keeps original presentation visible");
    hud.rows_read=true;
    for(unsigned hz:{30u,60u,120u}) {
        Model rate_model;CustomTrickScoring rate_score;rate_model.begin_balance(BalanceKind::manual);
        CustomTrickExecution rate{owner,1,1,1,1,1000,ArcadeTrick::casper,true,0};
        check(rate_score.observe(rate_model,rate,owner,1,rate.at),"rate entry admitted");
        rate.performed=false;rate.held_seconds=1.f/static_cast<float>(hz);
        for(unsigned i=0;i<hz;++i) {
            ++rate.step;rate.at=1000+(i+1)*1000/hz;
            check(rate_score.observe(rate_model,rate,owner,1,rate.at),"original clock hold admitted");
        }
        check(rate_model.state().custom_points==350,"hold points independent of physics rate");
    }
    std::cout<<"custom score identity, continuity, settlement and display passed\n";
}
