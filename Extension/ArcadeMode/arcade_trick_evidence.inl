// RC3's report explicitly leaves full gameplay acceptance open. It is not a verified receipt.
inline constexpr TrickEvidence rc3_evidence{
    "9906af3b5d4816c15d4506430d51afb7d860b970", "Pinned 20260929; RC3 candidate; see individual captures",
    "contrib/arcade-mode/research/rc3-gameplay-status.md",
    "Full native action, exit/landing, scoring and combo acceptance remains open."};
inline constexpr TrickEvidence planned_evidence{
    "9906af3b5d4816c15d4506430d51afb7d860b970", "Pinned 20260929; design only",
    "contrib/arcade-mode/research/trickpedia-implementation.md",
    "No executable Arcade binding is available for this trick in this build."};
// Add independently reviewed receipts by canonical ID here. Changes to this
// evidence-only file do not change the gameplay source fingerprint.
constexpr TrickEvidence apply_trick_evidence(ArcadeTrick,TrickEvidence fallback) noexcept {return fallback;}
