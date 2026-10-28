"""Generate the portable scalar Manual reservation composition from current source.

Reuses the existing Manual lifecycle generator and regression vectors. Only the
ordinary scalar predicate is varied for rollback controls; producer payload,
original-step admission, dt, body ownership and native completion remain separate.
This is offline software evidence, not native Manual physics acceptance.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess

VARIANTS=('current','rollback-no-reservation','rollback-admitted-only','rollback-first-down-mask')

def generate(root,variant='current'):
    if variant not in VARIANTS:raise ValueError('unknown control')
    TREE=root
    has_ground_step=(TREE/'Extension/ArcadeMode/arcade_ground_motion_step.h').exists()
    if not has_ground_step:raise ValueError('observation-bound scalar consumer dependency is absent')
    generator=TREE/'Extension/ArcadeMode/Test/manual_lifecycle/generate.py'
    spec=importlib.util.spec_from_file_location('manual_source_generator',generator)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    generated=module.generated(TREE)
    source=generated['current.cpp']
    mode=(TREE/'Extension/ArcadeMode/arcade_mode.cpp').read_text()
    reverse_mask=module.between(mode,'        if (result.suppress_reverse) left_y = 0;',
                               '        // The inspected guide stance')
    drive=module.between(mode,'        const bool rotation_enabled = mode_enabled.load(std::memory_order_acquire) && game_input && !offboard;',
                         '        const auto rotation_native = arcade_input.native_context(now);')
    correction=module.between(mode,'        const auto configured_balance=arcade_input.configured_balance(now);',
                              '        observe_grind_pad(user, now,')
    manual_publication=module.between(mode,'            const auto contact = arcade_input.balance_admission(now,pose == SkatePose::nose_manual ?',
                                      '            const auto& spin_proof = rotation_native;')
    native_current=module.function(mode,'bool native_manual_contact_current(')
    noclip=(TREE/'Extension/Skater/client_noclip.cpp').read_text()
    resistance=module.between(noclip,'        const bool braking = (requests & 4u) != 0 || ((requests & 2u) != 0 &&\n',
                             '        a.ground_output=a.ground_yaw.step(')
    resistance_current=resistance
    resistance=resistance.replace('        // A real Manual RS holder reserves vertical input before balance proof\n        // catches up. Reservation only withdraws ordinary scalar assistance.\n','')
    resistance=resistance.replace('controls_current && (balancing || !intent.ground_controls.manual_vertical_reserved) &&\n            (physics', 'controls_current && (physics')
    if resistance==resistance_current:
        raise ValueError('current scalar reservation consumer is absent')
    commit=module.between(noclip,'        const auto commit_motion = [&](bool linear,bool angular) {',
                          '        const auto release_turn = ')

    def once(old,new):
        nonlocal source
        assert source.count(old)==1,old
        source=source.replace(old,new,1)

    once('        const auto& action = result.action;',
         ('' if variant=='rollback-first-down-mask' else reverse_mask)+'        const auto& action = result.action;')

    once('#include "Extension/Skater/no_bail.h"','#include "Extension/Skater/no_bail.h"\n#include "Extension/Skater/native_bail_request.h"')
    if has_ground_step:
     once('#include "Extension/ArcadeMode/arcade_air_motion.h"','#include "Extension/ArcadeMode/arcade_air_motion.h"\n#include "Extension/ArcadeMode/arcade_ground_motion_step.h"\n#include "Extension/ArcadeMode/arcade_ground_writer_observation.h"')
    once('bool auto_kick_enabled(){return true;}','bool automatic_kick=true;bool auto_kick_enabled(){return automatic_kick;}')
    once('namespace dingosdk {void cancel_balance_bail()noexcept{}std::uint64_t request_balance_bail(const NoBailSkater&)noexcept{return 1;}}',r'''
    // Native service boundary only: record the production request; no impact or solver.
    NativeBailRequest<NativeInputOwner> bail_receipt;bool protected_no_bail{};
    unsigned bail_requests{},bail_vetoes{};
    namespace dingosdk {
    void cancel_balance_bail()noexcept{bail_receipt.cancel();}
    std::uint64_t request_balance_bail(const NoBailSkater& owner)noexcept{
     ++bail_requests;if(protected_no_bail){++bail_vetoes;return 0;}
     return bail_receipt.queue(input_owner(owner),GetTickCount64());
    }}
    ''')
    once('void set_arcade_push_intent(bool){}',r'''
    bool delivered_drive{},delivered_crouch{};
    void set_arcade_push_intent(bool drive,bool crouched=false){delivered_drive=drive;delivered_crouch=crouched;}
    ''')
    signatures=(
        'NativeGesture::Output remap(std::uint16_t physical_buttons=0,short physical_y=0){',
        'NativeGesture::Output remap(std::uint16_t physical_buttons=0,short physical_y=0,short physical_x=0){')
    declarations=(
        'const auto buttons=physical_buttons;short left_x=0,left_y=physical_y,right_x=0,right_y=0;unsigned char left_trigger=0,right_trigger=0;',
        'const auto buttons=physical_buttons;short left_x=physical_x,left_y=physical_y,right_x=0,right_y=0;unsigned char left_trigger=0,right_trigger=0;')
    signature_counts=[source.count(value) for value in signatures]
    declaration_counts=[source.count(value) for value in declarations]
    if sum(signature_counts)!=1 or sum(declaration_counts)!=1:
        raise ValueError('unknown or ambiguous remap fixture form')
    remap_form=signature_counts.index(1)
    if declaration_counts.index(1)!=remap_form:
        raise ValueError('mismatched remap fixture signature and axis declaration')
    once(signatures[remap_form],r'''
    struct PadResult {short x{},y{},rx{},ry{};unsigned buttons{},controls{};bool correction{},drive{};}last_pad;
    bool competing_left_owner{};
    NativeGesture::Output remap(std::uint16_t physical_buttons=0,short physical_y=0,short physical_x=0,
                               unsigned char lt=0,unsigned char rt=0){''')
    once(declarations[remap_form],
         'const auto buttons=physical_buttons;short left_x=physical_x,left_y=physical_y,right_x=0,right_y=0;unsigned char left_trigger=lt,right_trigger=rt;')
    once('const bool reference=false;const unsigned char physical_right_trigger=0;',
         'const bool reference=false;const unsigned char physical_left_trigger=lt,physical_right_trigger=rt;')
    once('may_push=false,may_toggle_board=false','may_push=admission.push,may_toggle_board=false')
    once('struct PadResult {short x{},y{},rx{},ry{};',
         'NativeManualContact writer_manual;AirMotionIntent writer_intent;bool writer_manual_hold{};\nstruct PadResult {short x{},y{},rx{},ry{};')
    # Deliberately grant outer drive eligibility to stress the real inner balance/
    # command/pose veto; no fake native propulsion occurs at this boundary.
    once('raw.state==100&&raw.crouch_mode==1,false,false,false,',
         'raw.state==100&&raw.crouch_mode==1,true,false,true,')
    old=' if(releasing_manual&&!result.authored_playback&&!action.owned)result.right_x=result.right_y=0;\n return result;'
    new=r'''
     if(releasing_manual&&!result.authored_playback&&!action.owned)result.right_x=result.right_y=0;
     struct RotationBoundary{bool owns_left;short x{1111},y{2222};}rotation{competing_left_owner};
     const auto rail_pad=arcade_input.sample_native_rail(result.rail,now,game_input&&!offboard&&!releasing_manual&&!reference&&!action.owned);
     movement_eligible_until.store(now+150);
     DRIVE
     const auto rotation_native=arcade_input.native_context(now);
     MANUAL_PUBLICATION
     writer_manual=manual;writer_manual_hold=result.manual.owns||releasing_manual;
     writer_intent={};writer_intent.enabled=rotation_enabled;writer_intent.sampled_at=now;
     writer_intent.ground_controls=ground_controls;writer_intent.angular_proof=NativeAirSpinProof::from_frame(rotation_native);
     if(rotation.owns_left){left_x=rotation.x;left_y=rotation.y;}
     CORRECTION
     last_pad={left_x,left_y,result.right_x,result.right_y,result.buttons,ground_controls.encode(),manual_correction.owns_y,delivered_drive};
     return result;'''.replace('DRIVE',drive).replace('CORRECTION',correction).replace('MANUAL_PUBLICATION',manual_publication)
    once(old,new)

    scalar_template=r'''
    namespace dingosdk::arcade_mode { NATIVE_CURRENT }
    struct CrouchBoundary{std::atomic<bool> arcade_crouched{};GroundDrive arcade_drive;};CrouchBoundary& push_speed(){static CrouchBoundary value;return value;}
    void source_require(bool value,const char* message){if(!value){std::cerr<<message<<'\n';std::abort();}}
    unsigned linear_writes{},dirty_writes{};
    float scalar_seconds{1.f/60.f};
    void body_write(std::uintptr_t,const std::array<float,3>&){++linear_writes;}
    void body_write(std::uintptr_t,std::uint32_t flags){source_require(flags==8,"linear batch preserves native dirty-bit contract");++dirty_writes;}
    namespace dingosdk {bool no_bail_skater(NoBailSkater& value)noexcept{value=::owner;return true;}}
    struct ScalarWorld{GroundResistance ground_resistance;GroundMotionStep ground_steps;};
    float FUNCTION(ScalarWorld& a){
     auto& intent=writer_intent;auto& manual=writer_manual;auto& native_frame=latest;
     const auto now=clock_ms;const auto physics=raw.state,mode=raw.crouch_mode,contacts=raw.words[1]&0xc000u,requests=raw.words[0];
     const bool ordinary=true;
     struct Bodies{std::array<float,3> forward{0,0,1};std::array<std::uintptr_t,2>parts{1,2};std::uintptr_t context{4};float seconds{scalar_seconds};}bodies;
     struct Reader{template<class T>T value(std::uintptr_t,std::uintptr_t){return T{};}};
     std::array<std::array<float,3>,2>velocities{{{0,0,5},{1,2,4}}},spins{};std::array<std::uint32_t,2>flags{};
     Reader reader;float resistance_change{};linear_writes=dirty_writes=0;
     GroundWriterObservation writer_observation{{},+[](const GroundWriterEvidence&)noexcept{}};
     AngularWriterObservation angular_observation{{},+[](const AngularWriterEvidence&)noexcept{}};
     auto& writer_row=writer_observation.row;
     COMMIT
     RESISTANCE
     commit_motion(resistance_change<0,false);
     return resistance_change;
    }
    '''
    # A local class cannot have a member template; the reader is a platform seam.
    scalar_template=scalar_template.replace('struct ScalarWorld{', 'struct ScalarReader{void verify(){}template<class T>T value(std::uintptr_t,std::uintptr_t){return T{};}};\nstruct ScalarWorld{')
    scalar_template=scalar_template.replace(' struct Reader{template<class T>T value(std::uintptr_t,std::uintptr_t){return T{};}};','')
    scalar_template=scalar_template.replace(' Reader reader;',' ScalarReader reader;')
    if not has_ground_step:scalar_template=scalar_template.replace('GroundMotionStep ground_steps;','')
    baseline_scalar=scalar_template.replace('NATIVE_CURRENT',native_current).replace('FUNCTION','before_scalar').replace('RESISTANCE',resistance).replace('COMMIT',commit)
    # The positive candidate uses the exact current consumer and encoded
    # publisher payload. Controls remove only its ordinary scalar reservation.
    candidate_resistance=resistance_current
    if variant=='rollback-no-reservation':candidate_resistance=resistance
    if variant=='rollback-admitted-only':
     candidate_resistance=candidate_resistance.replace('!intent.ground_controls.manual_vertical_reserved','!manual.admitted')
    candidate_scalar=scalar_template[scalar_template.index('float FUNCTION'):].replace('FUNCTION','current_scalar').replace('RESISTANCE',candidate_resistance).replace('COMMIT',commit)

    cases=r'''
    unsigned coupling_checks{},source_step{};
    void expect(bool value,const char* why){++coupling_checks;check(value,why);}
    void physical_step(unsigned hz,unsigned contacts,unsigned state=100){
     clock_ms=1000+static_cast<std::uint64_t>(std::llround(1000.0*++source_step/hz));
     const float seconds=1.f/static_cast<float>(hz);scalar_seconds=seconds;observe(seconds,contacts,state);client(seconds,history());
    }
    void enter_ground(bool nose,unsigned hz){
     reset_case();source_step=0;delivered_drive=delivered_crouch=false;automatic_kick=true;competing_left_owner=false;
     protected_no_bail=false;bail_requests=bail_vetoes=0;bail_receipt={};
     physical_step(hz,0xc000);remap();
     physical_step(hz,0xc000);remap(nose?2u:1u);
     physical_step(hz,0xc000);const auto request=remap(nose?1u:2u);
     expect(request.manual.owns,"ground preparation must deliver actual partial RS before testing correction coupling");
     physical_step(hz,nose?0x8000u:0x4000u);const auto entered=remap();
     expect(manual_balance_admitted.load()&&entered.manual.owns,"fresh matching native contact must independently admit the emulated balance");
     physical_step(hz,nose?0x8000u:0x4000u);remap();
    }
    void trace(const char* label,unsigned hz,bool nose,short physical_y){
     std::cout<<"{\"case\":\""<<label<<"\",\"hz\":"<<hz<<",\"nose\":"<<nose
     <<",\"physical_y\":"<<physical_y<<",\"output_x\":"<<last_pad.x<<",\"output_y\":"<<last_pad.y
     <<",\"rs_y\":"<<last_pad.ry<<",\"correction_owned\":"<<last_pad.correction<<",\"drive\":"<<last_pad.drive
     <<",\"native_x_push\":"<<bool(last_pad.buttons&0x4000u)<<",\"control_down\":"<<GroundControl::decode(last_pad.controls).down<<"}\n";
    }
    void coupling_cases(){
     for(unsigned hz:{30u,60u})for(bool nose:{false,true}){
      const unsigned contact=nose?0x8000u:0x4000u;
      const short inward=nose?-20000:20000,outward=static_cast<short>(-inward);
      enter_ground(nose,hz);
      // New physical correction cannot borrow an earlier post-integration decision.
      remap(0,inward,15000);expect(last_pad.correction&&last_pad.y==0,"new physical direction waits for its own original-step model decision");
      physical_step(hz,contact);remap(0,inward,15000);
      expect(last_pad.correction&&last_pad.y==(nose?-32768:32767),"actual protected model correction becomes full-scale native LSY");
      expect(last_pad.ry==(nose?20000:-20000)&&last_pad.x==15000,"partial RS and independent horizontal steering coexist with projected LSY");
      expect(!last_pad.drive&&!(last_pad.buttons&0x4000u),"admitted Manual cannot acquire SDK drive or automatic native X push despite granted outer drive permission");
      trace("effective-inward",hz,nose,inward);
      for(unsigned poll=0;poll<4;++poll){++clock_ms;remap(0,inward,15000);expect(last_pad.y==(nose?-32768:32767)&&!last_pad.drive,"repeated input polls reuse force decision without creating drive authority");}
      remap(0,0,15000);expect(last_pad.correction&&last_pad.y==0&&last_pad.ry==(nose?20000:-20000),"physical neutral withdraws LSY while preserving native manual hold");
      remap(0,outward,15000);expect(last_pad.correction&&last_pad.y==0,"opposite physical input cannot reuse old inward force");
      competing_left_owner=true;remap(0,inward);expect(!last_pad.correction&&last_pad.x==1111&&last_pad.y==2222,"manual projection yields to actual competing left-stick owner");
      competing_left_owner=false;

      enter_ground(nose,hz);remap(0,outward,15000);physical_step(hz,contact);remap(0,outward,15000);
      expect(last_pad.correction&&last_pad.y==0&&last_pad.ry==(nose?20000:-20000),"first-second outward protection suppresses projection while retaining RS hold");
      trace("protected-outward",hz,nose,outward);
      // Trigger-only input is permitted to retain the established Manual RS hold,
      // but makes native-correction compatibility false in the actual mode block.
      remap(0,outward,15000,31,0);
      expect(!last_pad.correction&&last_pad.y==outward&&last_pad.ry==(nose?20000:-20000),"trigger veto exposes raw physical LSY alongside retained native RS hold");
      expect(!last_pad.drive&&!(last_pad.buttons&0x4000u),"trigger veto still cannot acquire SDK drive/native X push");
      trace("trigger-veto-raw-y",hz,nose,outward);
      remap(0,outward,15000);
      expect(last_pad.correction&&last_pad.y==0,"removing trigger restores protected zero ownership on the same physical interval");

      // A newer physical contact generation can revoke old correction until the
      // next client publication, rather than making a cached force current.
      observe(1.f/static_cast<float>(hz),0xc000);remap(0,inward,15000);
      expect(!last_pad.correction&&last_pad.y==inward,"unrepublished contact change revokes projection and exposes raw LSY");
      trace("contact-generation-gap",hz,nose,inward);

      // Safe and unsafe model endpoints use different production release paths.
      enter_ground(nose,hz);bool putdown=false;
      remap(0,inward);
      for(unsigned i=0;i<10*hz;++i){physical_step(hz,contact);remap(0,inward);if(combo_session.manual_putdown()){putdown=true;break;}}
      expect(putdown&&manual_release.pending()&&!last_pad.ry&&!last_pad.drive&&!last_pad.correction,
             "safe endpoint owns neutral RS and withdraws force/drive until observed physical completion");
      expect(writer_intent.ground_controls.manual_vertical_reserved,"current production carrier reserves owned neutral release");
      expect(writer_manual_hold&&last_pad.y==inward,
             "owned neutral-RS release remains a separate reservation while raw physical LSY is visible");
      trace("safe-putdown",hz,nose,inward);
      physical_step(hz,0xc000);remap();expect(!manual_release.pending(),"fresh actual two-wheel completion retires putdown ownership");

      for(bool protected_skater:{false,true}){
       enter_ground(nose,hz);protected_no_bail=protected_skater;
       bool failed=false;
       for(unsigned i=0;i<10*hz;++i){physical_step(hz,contact);remap();if(combo_session.failed_section()){failed=true;break;}}
       expect(failed&&bail_requests>0&&!last_pad.ry&&!last_pad.drive,"unsafe model endpoint withdraws partial RS and drive while requesting native failure");
       expect(!manual_release.pending(),"unsafe endpoint cannot silently substitute safe putdown");
       if(protected_skater)expect(bail_vetoes>0&&bail_receipt.status().serial==0,"controlled native NoBail veto yields no issued bail ticket");
       else{
        auto ticket=bail_receipt.ticket();expect(bool(ticket),"unprotected native service boundary queues the original owner ticket");
        expect(ticket&&bail_receipt.take(*ticket,input_owner(owner),clock_ms,false),"actual SDK ticket distinguishes consumed delivery from queue");
        const auto serial=bail_receipt.status().serial;bail_receipt.finish(serial,true,300);
        expect(bail_receipt.status().result==NativeBailResult::observed_wipeout,"actual SDK receipt needs observed native300 to report failure completion");
       }
       physical_step(hz,0,300);remap();expect(!manual_balance_admitted.load()&&!last_pad.ry,"positive native bail retires balance and RS ownership");
      }
      // Real source brake contamination at the physics/client handoff. These
      // controlled observed velocities do not model the native Skate solver.
      // Establish this coasting/exit case with a new matching history edge.
      // First attributable contact alone retains the original bounded request.
      enter_ground(nose,hz);rows.push_back({91,92,nose?2u:1u});
      remap(0,-20000);physical_step(hz,contact);remap(0,-20000);
      expect(writer_intent.ground_controls.manual_vertical_reserved,"current production carrier reserves the delivered Manual owner");
      expect(writer_manual.admitted&&writer_manual_hold,"baseline writer snapshot belongs to a real admitted Manual RS owner");
      ScalarWorld before,after,turning;expect(before_scalar(before)==0&&current_scalar(after)==0&&before_scalar(turning)==0,"scalar episodes prime on qualified Manual contact");
      ++source_step;clock_ms=1000+static_cast<std::uint64_t>(std::llround(1000.0*source_step/hz));observe(1.f/static_cast<float>(hz),0xc000);
      const float old_loss=before_scalar(before);expect(linear_writes==2&&dirty_writes==2,"ordinary brake staging reaches the exact production linear commit transaction");
      const float withdrawn=current_scalar(after);expect(!linear_writes&&!dirty_writes,"production reservation grants no linear write during unresolved Manual hold");
      expect(old_loss<-.3f&&withdrawn==0,"new two-wheel contact before client publication must not reinterpret held balance Down as ordinary SDK braking");
      std::cout<<"{\"case\":\"physics-before-client\",\"hz\":"<<hz<<",\"nose\":"<<nose<<",\"manual_marker\":"<<writer_manual.admitted<<",\"rs_owned\":"<<writer_manual_hold<<",\"before_sdk_delta\":"<<old_loss<<",\"production_reservation_delta\":"<<withdrawn<<"}\n";
      remap(0,-20000,15000);const float turn_loss=before_scalar(turning);
      expect(turn_loss>-.1f&&writer_manual_hold,"Down plus horizontal turning preserves the distinct sharp-turn/no-brake source policy");
      std::cout<<"{\"case\":\"turning-down-handoff\",\"hz\":"<<hz<<",\"nose\":"<<nose<<",\"before_sdk_delta\":"<<turn_loss<<"}\n";
      remap(0,-20000);
      expect(!writer_manual.admitted&&writer_manual_hold,"an intervening input poll can lose balance proof while retaining the real RS owner");
      ++source_step;clock_ms=1000+static_cast<std::uint64_t>(std::llround(1000.0*source_step/hz));observe(1.f/static_cast<float>(hz),0xc000);
      const float input_gap_loss=before_scalar(before),input_withdrawal=current_scalar(after);
      expect(input_gap_loss<-.3f&&input_withdrawal==0,"ordinary brake budget remains unsafe after a new input poll until balance publication catches up");
      std::cout<<"{\"case\":\"input-before-client\",\"hz\":"<<hz<<",\"nose\":"<<nose<<",\"manual_marker\":"<<writer_manual.admitted<<",\"rs_owned\":"<<writer_manual_hold<<",\"before_sdk_delta\":"<<input_gap_loss<<",\"production_reservation_delta\":"<<input_withdrawal<<"}\n";
      client(1.f/static_cast<float>(hz),history());remap(0,-20000);
      expect(writer_manual.admitted,"fresh client publication restores admitted short two-wheel Manual continuity");
      ++source_step;clock_ms=1000+static_cast<std::uint64_t>(std::llround(1000.0*source_step/hz));observe(1.f/static_cast<float>(hz),0xc000);
      const float qualified_loss=before_scalar(before),qualified_control=current_scalar(after);
      expect(qualified_loss>-.1f&&qualified_control>-.1f,"qualified Manual coasting does not select ordinary Down brake acceleration");
      std::cout<<"{\"case\":\"qualified-manual-coast\",\"hz\":"<<hz<<",\"nose\":"<<nose<<",\"before_sdk_delta\":"<<qualified_loss<<",\"production_reservation_delta\":"<<qualified_control<<"}\n";
      physical_step(hz,0xc000);remap(0,-20000);
      const float continued_loss=before_scalar(before),continued_control=current_scalar(after);
      expect(continued_loss<0&&std::abs(continued_loss-continued_control)<.00001f,
             "after rebaselining the reservation preserves independently admitted Manual coasting");
      // Reservation starts with actual RS delivery, before a successful contact
      // admission. Reusing the stronger admission bit loses this distinction.
      reset_case();source_step=0;physical_step(hz,0xc000);remap();
      ScalarWorld acquiring,acquiring_control;before_scalar(acquiring);current_scalar(acquiring_control);
      physical_step(hz,0xc000);remap(nose?2u:1u);before_scalar(acquiring);current_scalar(acquiring_control);
      physical_step(hz,0xc000);const auto preparing=remap(nose?1u:2u);remap(2u);
      expect(writer_intent.ground_controls.manual_vertical_reserved,"current production carrier reserves unadmitted acquisition");
      expect(preparing.manual.owns&&writer_manual_hold&&!writer_manual.admitted,
             "delivered partial RS reserves entry input before any matching native Manual admission");
      const float acquisition_loss=before_scalar(acquiring),acquisition_withdrawal=current_scalar(acquiring_control);
      expect(acquisition_loss<-.3f&&acquisition_withdrawal==0,
             "new acquisition RS ownership must withdraw ordinary Down brake before balance admission");
      expect(!last_pad.drive&&!(last_pad.buttons&0x4000u),"entry reservation has no SDK drive or synthetic X push");
      std::cout<<"{\"case\":\"manual-acquisition-down\",\"hz\":"<<hz<<",\"nose\":"<<nose<<",\"manual_marker\":"<<writer_manual.admitted<<",\"rs_owned\":"<<writer_manual_hold<<",\"before_sdk_delta\":"<<acquisition_loss<<",\"production_reservation_delta\":"<<acquisition_withdrawal<<"}\n";
     }
     // Ordinary riding is the independent positive push control: the fixture's
     // granted drive permission really can drive when Manual does not own it.
     reset_case();source_step=0;automatic_kick=true;protected_no_bail=false;competing_left_owner=false;
     physical_step(60,0xc000);remap();physical_step(60,0xc000);remap(0,25000);
     expect(last_pad.drive&&bool(last_pad.buttons&0x4000u),"ordinary physical forward remains capable of SDK drive/native X push");
     for(unsigned hz:{30u,60u}){
      reset_case();source_step=0;physical_step(hz,0xc000);remap();physical_step(hz,0xc000);remap(0,-20000);
      expect(!writer_manual_hold&&!last_pad.drive,"ordinary Down has no Manual reservation or push budget");
      ScalarWorld before,after;before_scalar(before);current_scalar(after);
      ++source_step;clock_ms=1000+static_cast<std::uint64_t>(std::llround(1000.0*source_step/hz));observe(1.f/static_cast<float>(hz),0xc000);
      const float ordinary=before_scalar(before),preserved=current_scalar(after);
      expect(ordinary<-.3f&&std::abs(ordinary-preserved)<.00001f,"production reservation preserves independently qualified ordinary ground braking");
      expect(current_scalar(after)==0&&!linear_writes&&!dirty_writes,
             "a repeated consumer cannot spend a second original-step resistance budget");
      writer_intent.ground_controls.manual_vertical_reserved=true;
      expect(current_scalar(after)==0&&!linear_writes&&!dirty_writes,
             "reservation withdrawal cannot grant a force on a spent original observation");
      writer_intent.ground_controls.manual_vertical_reserved=false;
      expect(current_scalar(after)==0&&!linear_writes&&!dirty_writes,
             "reservation recovery preserves the consumed original-step watermark");
      const auto saved=writer_intent;
      physical_step(hz,0xc000);
      writer_intent.ground_controls.known=false;
      expect(current_scalar(after)==0&&!linear_writes&&!dirty_writes,
             "unknown physical controls cannot gain an ordinary resistance budget");
      writer_intent=saved;++writer_intent.angular_proof.owner_generation;
      expect(current_scalar(after)==0&&!linear_writes&&!dirty_writes,
             "a mismatched owner generation cannot select scalar resistance");
      writer_intent=saved;clock_ms+=151;observe(1.f/static_cast<float>(hz),0xc000);
      expect(current_scalar(after)==0&&!linear_writes&&!dirty_writes,
             "a fresh physical observation cannot renew stale input controls");
     }
     // First Down reserves returned LSY only. The current physical carrier
     // remains Down for scalar braking and cannot acquire automatic push.
     for(unsigned hz:{30u,60u})for(unsigned age:{0u,250u,275u,276u}){
      reset_case();source_step=0;automatic_kick=true;competing_left_owner=false;
      physical_step(hz,0xc000);remap();
      physical_step(hz,0xc000);const auto first_at=clock_ms;remap(0,-20000);
      ScalarWorld scalar;current_scalar(scalar);
      const auto end=first_at+age;float delta{};
      while(clock_ms<end){
       clock_ms=std::min<std::uint64_t>(end,clock_ms+(hz==30?34u:17u));
       observe(1.f/static_cast<float>(hz),0xc000);client(1.f/static_cast<float>(hz),history());remap(0,-20000);
       delta=current_scalar(scalar);
      }
      const auto controls=writer_intent.ground_controls;
      expect(last_pad.y==(age<=275?0:-20000),"actual first-Down return mask ends only after275ms");
      expect(controls.known&&controls.down&&controls.trying_to_brake(5.f,1.f)&&!controls.can_kick(5.f,false),
             "returned-axis reservation preserves physical Down braking and push veto");
      expect(!writer_manual_hold&&!controls.manual_vertical_reserved&&!last_pad.drive&&!(last_pad.buttons&0x4000u),
             "a first Down is not Manual RS ownership and cannot acquire drive or push");
      if(age){
       expect(delta<-.3f&&linear_writes==2&&dirty_writes==2,
              "qualified original-step Down resistance remains eligible during and after the return mask");
      }
      remap(0,-20000,15000);
      expect(last_pad.x==15000&&writer_intent.ground_controls.down&&!last_pad.drive,
             "independent horizontal steering and physical Down survive returned-axis reservation");
     }
     for(bool nose:{false,true})for(unsigned hz:{30u,60u}){
      // A historyless first touch and subsequent full-contact riding are still
      // the same provisional request, not the established exit tested above.
      enter_ground(nose,hz);const auto began=remap().manual.request_at;
      const auto loss_at=clock_ms;
      while(clock_ms-loss_at<200){physical_step(hz,0xc000);remap(0,-20000);}
      expect(rows.empty()&&!manual_balance_admitted.load()&&writer_manual_hold&&
             writer_intent.ground_controls.manual_vertical_reserved,
             "firsttouch loss retains only the original provisional Manual reservation");
      ScalarWorld before,after;before_scalar(before);current_scalar(after);
      physical_step(hz,0xc000);const auto request=remap(0,-20000);
      const auto baseline=before_scalar(before),withdrawn=current_scalar(after);
      expect(request.manual.owns&&request.manual.request_at==began&&baseline<0&&withdrawn==0&&
             !linear_writes&&!dirty_writes&&!last_pad.drive,
             "a provisional original request withdraws ordinary resistance without a second balance admission");
     }
     std::cout<<coupling_checks<<" additional production output-coupling checks passed\n";
     reset_case();
    }
    '''
    once('int main(){','void coupling_cases();\nint main(){\n coupling_cases();')
    # Insert exact native guard and scalar blocks before the new test main, after
    # their ordinary fixture services and owner variables have been declared.
    once('void coupling_cases();',baseline_scalar+'\n'+candidate_scalar+'\nvoid coupling_cases();')

    # Use the current carrier statements without changing production publication.
    def statement(marker):
        begin=module.unique(noclip,marker)
        return noclip[begin:noclip.index(';',begin)+1]
    store=statement('    a.ground_controls.store(')
    load=statement('    candidate.ground_controls = arcade_mode::GroundControl::decode(')
    if store.count('active && ground_controls')!=1:
        raise ValueError('unexpected carrier activation expression')
    fixture_store=store.replace('active && ground_controls','fixture_transport_active && ground_controls')
    transport=r'''
    GroundControl transported(const GroundControl& published){
     const GroundControl* ground_controls=&published;const bool fixture_transport_active=true;
     struct{std::atomic<unsigned> ground_controls{};}a;
     struct{GroundControl ground_controls{};}candidate;
     STORE
     LOAD
     return candidate.ground_controls;
    }
    '''.replace('STORE',fixture_store).replace('LOAD',load)
    once('NativeManualContact writer_manual;',transport+'\nNativeManualContact writer_manual;')
    source=source.replace('writer_intent.ground_controls=ground_controls;','writer_intent.ground_controls=transported(ground_controls);')

    source+='\n'+cases

    return source, {'base_chunks':json.loads(generated['correspondence.json']),
     'variant':variant,
     'production_chunks_sha256':{name:hashlib.sha256(value.encode()).hexdigest() for name,value in {
      'reverse_mask':reverse_mask,'drive_and_reservation_publication':drive,'correction':correction,'manual_publication':manual_publication,
      'native_manual_current':native_current,'resistance_current':resistance_current,
      'resistance_before':resistance,'commit_motion':commit,'carrier_store':store,'carrier_load':load,
      'fixture_carrier_store':fixture_store}.items()},
     'fixture_binding_map':{'active':'fixture_transport_active'},'remap_fixture_form':remap_form,
     'fixture_sha256':hashlib.sha256(source.encode()).hexdigest(),
     'boundaries':'Existing production lifecycle fixture clocks/probes and nonactuating writer/NoBail sink. Exact current producer/carrier/consumer blocks; no native solver or Windows hook execution.'}

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root',type=Path)
    parser.add_argument('--output',type=Path)
    parser.add_argument('--variant',choices=VARIANTS,default='current')
    parser.add_argument('--check',action='store_true')
    parser.add_argument('--verify-control',choices=VARIANTS[1:])
    parser.add_argument('--binary',type=Path)
    args=parser.parse_args()
    if args.verify_control:
        if not args.binary:parser.error('--verify-control requires --binary')
        expected={'rollback-no-reservation':'production reservation grants no linear write during unresolved Manual hold',
                  'rollback-admitted-only':'new acquisition RS ownership must withdraw ordinary Down brake before balance admission',
                  'rollback-first-down-mask':'actual first-Down return mask ends only after275ms'}[args.verify_control]
        result=subprocess.run([str(args.binary.resolve())],capture_output=True,text=True)
        if result.returncode!=1 or result.stderr.strip()!=expected:
            raise ValueError('rollback did not fail at its intended assertion: '+result.stderr)
        print('Manual scalar rollback assertion verified: '+args.verify_control)
        return
    if not args.root or not args.output:parser.error('--root and --output are required for generation')
    source,manifest=generate(args.root.resolve(),args.variant)
    if args.check:
        if args.output.read_text()!=source:raise ValueError('stale scalar source composition')
        print('Manual scalar source correspondence verified')
    else:
        args.output.write_text(source)
        args.output.with_suffix('.json').write_text(json.dumps(manifest,indent=2)+'\n')

if __name__=='__main__':main()
