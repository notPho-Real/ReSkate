// RC3's report explicitly leaves full gameplay acceptance open. It is not a verified receipt.
inline constexpr TrickEvidence rc3_evidence{
    "9906af3b5d4816c15d4506430d51afb7d860b970", "Pinned 20260929; RC3 candidate; see individual captures",
    "contrib/arcade-mode/research/rc3-gameplay-status.md",
    "Only attributable native trials below count toward this trick's applicable checklist. Unlisted criteria have not been established."};
inline constexpr TrickEvidence casper_evidence{
    "ab41aab56f67f5c546a084a5d41cb34c2a43675b", "Pinned 20260929; custom flatland on native Manual",
    "contrib/arcade-mode/UNIFIED_RC5.md",
    "Experimental animation on native Manual balance. Full foot contact and all outfits remain unverified; no custom native scoring or trick ID."};
inline constexpr TrickEvidence planned_evidence{
    "9906af3b5d4816c15d4506430d51afb7d860b970", "Pinned 20260929; design only",
    "contrib/arcade-mode/research/trickpedia-implementation.md",
    "No executable Arcade binding is available for this trick in this build."};
inline constexpr TrickEvidence triple_flip_evidence{
    "7abb02fabed9c96e0157fbd5691df003b29e2b11", "Pinned 20260929; existing held actor extended to two extra rotations",
    "Extension/ArcadeMode/arcade_held_flip.h",
    "Third-X binding implemented by extending the original owned double. Native identity, physical rotations, catch, landing and scoring acceptance remain unverified."};
inline constexpr TrickEvidence frontside_shove_evidence{
    "6d8dda8f9431aef21268c74455a5f1ec930df8d8", "Pinned 20260929; source catalog identities7/22",
    "contrib/arcade-mode/research/native-flip-source-catalog.json",
    "R3 plus Down plus X binding implemented using the original native flip route. Native identity, actor, rotations, catch, landing and scoring acceptance remain unverified."};
inline constexpr TrickEvidence frontside_360_shove_evidence{
    "6d8dda8f9431aef21268c74455a5f1ec930df8d8", "Pinned 20260929; source catalog identities13/25",
    "contrib/arcade-mode/research/native-flip-source-catalog.json",
    "R3 plus Up plus X binding implemented using the original native flip route. Native identity, actor, board rotation, catch, landing and scoring acceptance remain unverified."};
// Add independently reviewed receipts by canonical ID here. Changes to this
// evidence-only file do not change the gameplay source fingerprint.
#include "arcade_trick_capture_data.inl"
constexpr TrickEvidence apply_trick_evidence(ArcadeTrick id,TrickEvidence fallback) noexcept {
    fallback.trials=captured_trick_trials(id);
    return fallback;
}
