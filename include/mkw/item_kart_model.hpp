#pragma once
#include "mkw/item_karts.hpp"
#include <algorithm>
#include <array>

namespace mkw {
// Host-test stand-in for the Kinoko kart. It applies the same retail rules as the patched
// KartAction::start, KartMove::activateStar/activateMega and applyLightningEffect to the racer
// snapshots a test supplies, so item logic can be exercised without a course.
class ItemKartModel final : public ItemKarts {
public:
    static constexpr unsigned Capacity = 12;
    struct Kart {
        int action = -1;
        uint16_t actionFrames = 0, star = 0, mega = 0, shock = 0;
        bool cloud = false, mushroom = false, bullet = false, powTarget = false;
        float powLift = 0;
        ItemKartLoss loss{};
        unsigned starts = 0, mushrooms = 0;
    };
    /// Racers the model reads its gates from; RaceItems binds its own copy each step.
    /// A test that changes a racer's damageAction overrides the model's own action, standing in
    /// for an action the native kart starts or ends by itself.
    void bind(const ItemRacer *racers, unsigned count) override {
        racers_ = racers;
        count_ = std::min(count, Capacity);
        for (unsigned i = 0; i < count_; ++i) {
            if (racers[i].damageAction != supplied_[i]) {
                supplied_[i] = racers[i].damageAction;
                karts_[i].action = racers[i].damageAction;
                karts_[i].actionFrames = 0;
            }
        }
    }
    void resetKarts() override { karts_ = {}; supplied_.fill(-1); powWindow_ = -1; }
    const Kart &kart(unsigned player) const { return karts_[player < Capacity ? player : 0]; }
    Kart &kart(unsigned player) { return karts_[player < Capacity ? player : 0]; }

    int startAction(unsigned player, int action, bool force, unsigned /*attacker*/,
            Vec3 /*hitDepth*/) override {
        if (player >= count_ || action < 0 || action >= KartAction::Count) return KartAction::None;
        const ItemRacer &r = racers_[player];
        auto &k = karts_[player];
        // RMCU 0x8056325c gates.
        if (r.damageProtected || r.unavailable || r.inCannon || k.star || r.star || r.bullet) return KartAction::None;
        if (!force && (k.mega || r.mega)) return KartAction::None;
        if (r.zipperStick && action >= KartAction::Launch && action <= KartAction::BulletHit) {
            action = KartAction::SpinSlow;
        }
        const int current = k.action >= 0 ? k.action : r.damageAction;
        if (current >= 0 && kartActionPriority(action) <= kartActionPriority(current)) return KartAction::None;
        k.action = action;
        k.actionFrames = 0;
        ++k.starts;
        const ItemKartLoss loss = kartActionLoss(action);
        k.loss.strong |= loss.strong;
        k.loss.tethered |= loss.tethered;
        k.loss.trailOnly |= loss.trailOnly;
        return action;
    }
    bool activateStar(unsigned player) override {
        if (player >= count_) return false;
        const ItemRacer &r = racers_[player];
        auto &k = karts_[player];
        if (k.action >= 0 || r.damageAction >= 0 || r.bullet) return false;
        k.star = 450;
        return true;
    }
    void activateMega(unsigned player) override {
        if (player >= count_) return;
        karts_[player].mega = 450;
        karts_[player].shock = 0;
    }
    void activateMushroom(unsigned player) override {
        if (player < count_) ++karts_[player].mushrooms;
    }
    bool lightning(unsigned player, uint16_t frames, bool spinShrink, bool cloud) override {
        if (player >= count_) return false;
        const ItemRacer &r = racers_[player];
        auto &k = karts_[player];
        if (r.damageProtected || r.unavailable || r.inCannon || k.star || r.star || r.bullet) return false;
        if (k.mega || r.mega) {
            k.mega = 0;
            return true;
        }
        startAction(player, spinShrink ? KartAction::SpinShrink :
                cloud ? KartAction::CloudStrike : KartAction::Lightning, false, 12, {});
        k.loss.strong = true;
        k.shock = std::max(k.shock, frames);
        return true;
    }
    void setThunderCloud(unsigned player, bool active) override {
        if (player < count_) karts_[player].cloud = active;
    }
    void setBullet(unsigned player, bool active) override {
        if (player < count_) karts_[player].bullet = active;
    }
    void setPowTarget(unsigned player, bool target) override {
        if (player < count_) karts_[player].powTarget = target;
    }
    /// The model counts a grounded racer as airborne for at most 20 frames.
    void powHop(unsigned player, float lift) override {
        if (player < count_ && racers_[player].grounded) karts_[player].powLift += lift;
    }
    void openPowWindow() override { powWindow_ = 15; }
    bool powWindowOpen() const { return powWindow_ >= 0; }
    ItemKartLoss takeLoss(unsigned player) override {
        if (player >= count_) return {};
        const ItemKartLoss out = karts_[player].loss;
        karts_[player].loss = {};
        return out;
    }
    /// The model approximates the vehicle hull by the racer's collision sphere.
    bool touches(unsigned player, Vec3 center, float radius) const override {
        if (player >= count_) return false;
        const Vec3 d = racers_[player].position - center;
        const float reach = racers_[player].radius + radius;
        return dot(d, d) < reach * reach;
    }
    /// Model actions finish after a fixed 60 frames; tests that need native action
    /// lifetimes run against Kinoko instead.
    void advance() override {
        for (unsigned i = 0; i < count_; ++i) {
            auto &k = karts_[i];
            // KartMove 0x8057a844: a POW target on the ground during the window is hit (the
            // model has no trick input, so no dodge).
            if (powWindow_ >= 0 && k.powTarget && racers_[i].grounded) startAction(i, KartAction::Pow, false, 12, {});
            if (k.action >= 0 && ++k.actionFrames > 60) k.action = -1;
            if (k.star) --k.star;
            if (k.mega) --k.mega;
            if (k.shock) --k.shock;
        }
        if (powWindow_ >= 0) --powWindow_; // 0x80563214 after every kart
    }
    void overlay(unsigned player, ItemRacer &racer) const override {
        if (player >= Capacity) return;
        const auto &k = karts_[player];
        if (k.action >= 0) racer.damageAction = int8_t(k.action);
        racer.inAction = racer.damageAction >= 0;
        racer.star = std::max(racer.star, k.star);
        racer.mega = std::max(racer.mega, k.mega);
        racer.shocked = racer.shocked || k.shock;
        racer.bullet = racer.bullet || k.bullet;
    }

private:
    const ItemRacer *racers_ = nullptr;
    unsigned count_ = 0;
    std::array<Kart, Capacity> karts_{};
    std::array<int8_t, Capacity> supplied_{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
    int powWindow_ = -1;
};
}
