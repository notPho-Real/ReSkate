#pragma once
#include "arcade_air_motion.h"
#include "arcade_input_types.h"

namespace dingosdk::arcade_mode {
struct GroundYawOutput {
    bool ready{},angular{},velocity_owned{};
    float rate{},delta_x{},delta_z{};
    NativeInputFrame frame{};
    GroundControl controls{};
    std::uint64_t left_pressed_at{},right_pressed_at{},input_epoch{};
    float angle_delta{};
    GroundTurnCommand command{};
};
// Ground angular policy shares the existing guarded articulated-body and
// physical motion-target adapters. A display transform is a separate interface.
class GroundYaw {
public:
    void reset() noexcept { active_=turn_active_=false; turn_output_={}; elapsed_=angle_=0; }
    void cancel(const GroundTurnCommand* turn=nullptr) noexcept { retire(turn); }
    static float selected_rate(const GroundControl& controls,float speed,std::uint64_t now,
                               std::uint64_t left_at,std::uint64_t right_at) noexcept {
        if(!controls.known || !std::isfinite(speed) || speed<0 || (!controls.left&&!controls.right))return 0;
        const auto pressed=controls.left ? left_at : right_at;
        if(now<pressed)return 0;
        float rate=controls.down ? 3.6f : 1.8f;
        if(controls.down && speed<10*GameplayProfile::metres_per_inch)
            rate*=static_cast<float>(std::min<std::uint64_t>(now-pressed,600))/600;
        // Source Left wins; preserve the existing native local-Y handedness.
        return controls.left ? -rate : rate;
    }
    GroundYawOutput step(const AirMotionIntent& intent,const NativeInputFrame& frame,
            std::uint64_t now,float dt,const std::array<float,3>& velocity,bool ordinary,bool balance_or_other_owner,
            const GroundTurnCommand* turn=nullptr) noexcept {
        const bool current=intent.enabled && intent.angular_allowed && intent.spin.physical_armed &&
            now>=intent.sampled_at && now-intent.sampled_at<=150 &&
            intent.angular_proof.same_owner_observation(frame,now) &&
            frame.contact_generation && frame.action_generation &&
            (frame.state==100 || frame.state==103) && frame.mode<=2 && frame.contacts==0xc000u &&
            ordinary && !balance_or_other_owner && intent.ground_controls.known &&
            std::isfinite(dt)&&dt>0&&dt<=.1f;
        if(!current){retire(turn);return {};}
        for(float value:velocity)if(!std::isfinite(value)||std::abs(value)>=1000){retire(turn);return {};}
        const auto& controls=intent.ground_controls;
        if((controls.left&&now<intent.spin.left_pressed_at)||(controls.right&&now<intent.spin.right_pressed_at)){retire(turn);return {};}
        const auto same_context=[&](const GroundTurnCommand& command) {
            return command.generations==InputGenerations{frame.owner_generation,frame.contact_generation,frame.action_generation} &&
                command.physics_step && command.physics_step<=frame.physics_step && command.sampled_at<=frame.sampled_at &&
                now>=command.requested_at && now>=command.sampled_at;
        };
        if(turn_active_ && (!turn || *turn!=turn_output_.command || !same_context(*turn) ||
            frame.owner!=turn_output_.frame.owner || frame.physics_step<turn_output_.frame.physics_step)){
            retire(turn);return {};
        }
        if(turn && turn->serial>spent_turn_){
            spent_turn_=turn->serial;
            if(!turn_active_ && same_context(*turn) && now-turn->requested_at<=150 && now-turn->sampled_at<=150){
                turn_active_=true;elapsed_=angle_=0;
                turn_output_={true,true,false,0,0,0,frame,controls,intent.spin.left_pressed_at,intent.spin.right_pressed_at,intent.spin.epoch,0,*turn};
                active_=true;return turn_output_;
            }
        }
        // Repeated reads of one physical step are an immutable publication,
        // not another integration interval (including its final clipped step).
        if(turn && turn_output_.command.serial && *turn==turn_output_.command &&
            frame.physics_step==turn_output_.frame.physics_step && same_context(*turn) &&
            frame.owner==turn_output_.frame.owner)return turn_output_;
        if(turn_active_){
            elapsed_=std::min(.3,elapsed_+static_cast<double>(dt));
            const double desired=TapSpin::pi*elapsed_/.3;
            const float delta=static_cast<float>(desired-angle_);angle_=desired;
            turn_output_={true,true,false,delta/dt,0,0,frame,controls,intent.spin.left_pressed_at,intent.spin.right_pressed_at,intent.spin.epoch,delta,*turn};
            turn_active_=elapsed_<.3;active_=true;return turn_output_;
        }
        const float speed=std::hypot(std::hypot(velocity[0],velocity[2]),velocity[1]);
        const float rate=selected_rate(controls,speed,now,intent.spin.left_pressed_at,intent.spin.right_pressed_at);
        const bool requested=controls.left||controls.right;
        GroundYawOutput output{true,requested||active_,false,rate,0,0,frame,controls,intent.spin.left_pressed_at,intent.spin.right_pressed_at,intent.spin.epoch};
        output.angle_delta=rate*dt;
        active_=requested;
        // Unknown is not unlocked. It leaves native velocity direction intact;
        // physical angular selection remains independently supported.
        if(requested && controls.velocity_lock==GroundVelocityLock::unlocked){
            const float angle=rate*dt,c=std::cos(angle),s=std::sin(angle);
            output.velocity_owned=true;
            output.delta_x=c*velocity[0]+s*velocity[2]-velocity[0];
            output.delta_z=-s*velocity[0]+c*velocity[2]-velocity[2];
        }
        return output;
    }
    static bool target_current(const GroundYawOutput& output,const AirMotionIntent& intent,
                               const NativeInputFrame& current,std::uint64_t now,bool balance_or_other_owner=false,
                               const GroundTurnCommand* turn=nullptr) noexcept {
        const auto& original=output.frame;
        return !balance_or_other_owner&&output.ready&&output.angular&&intent.enabled&&intent.angular_allowed&&intent.spin.physical_armed&&
            now>=intent.sampled_at&&now-intent.sampled_at<=150&&
            intent.angular_proof.same_owner_observation(current,now)&&
            original.valid&&original.owner.valid()&&original.owner==current.owner&&
            original.owner_generation==current.owner_generation&&original.contact_generation==current.contact_generation&&
            original.action_generation==current.action_generation&&original.physics_step&&original.physics_step<=current.physics_step&&
            now>=original.sampled_at&&now-original.sampled_at<=150&&original.sampled_at<=current.sampled_at&&
            (current.state==100||current.state==103)&&current.mode<=2&&current.contacts==0xc000u&&
            output.input_epoch==intent.spin.epoch&&
            (output.command.serial ? turn&&*turn==output.command&&original.physics_step==current.physics_step :
                output.controls.encode()==intent.ground_controls.encode()&&
                output.left_pressed_at==intent.spin.left_pressed_at&&output.right_pressed_at==intent.spin.right_pressed_at);
    }
    static AirMotionOutput body_output(const GroundYawOutput& ground) noexcept {
        AirMotionOutput out{};out.ready=ground.ready;out.angular=ground.angular;out.spin_y=ground.rate;
        out.horizontal=ground.velocity_owned;out.delta_x=ground.delta_x;out.delta_z=ground.delta_z;return out;
    }
private:
    void retire(const GroundTurnCommand* turn) noexcept {
        if(turn)spent_turn_=std::max(spent_turn_,turn->serial);
        reset(); // Deliberately preserve the once-only command watermark.
    }
    bool active_{};
    bool turn_active_{};
    std::uint64_t spent_turn_{};
    double elapsed_{},angle_{};
    GroundYawOutput turn_output_{};
};
}
