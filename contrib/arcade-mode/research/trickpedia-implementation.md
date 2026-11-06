# Arcade Trickpedia development candidate

This is an isolated feature on `arcade-stage-trickpedia`, based on published RC3
`9906af3b5d4816c15d4506430d51afb7d860b970`. The accepted development branch was
`a7548cf5384132d98750287c4aa769cba6139fe2` at inspection. RC3 is explicitly an
incomplete gameplay candidate, not a newly accepted stable release. RC4
`5925ac58bd35594310376e9453b8b321aac8741e` is not merged here. Wallplant remains
Planned until its actual implementation and evidence are integrated deliberately.

## Source audit

The source-first audit covered the current Arcade recognizer, InputSession,
native flip mapping, native grab leases and actor observation, fingerflip CDB
routing, footplant preparation, ground grab controller, established rail changes,
the full available upstream source tree, and relevant UI changes in the existing
fork survey (`upstream-method-survey.json`). No equivalent dynamic Arcade guide
was found in those sources. This is a missing presentation/metadata connection;
it does not justify replacing native gameplay execution.

The pinned 20260929 game's original level manifest identifies:

| Resource | Owner | Encoded SHA-1 |
| --- | --- | --- |
| 36627 | SkatepediaPageViewModel | 0f5cf9fd3526720fdd1c6513f6ea5c137c4e5db7 |
| 35947 | PageViewModel | 7dcde397fefdaf7d50fab42628fd2f645a5564b1 |
| 36000 | SkatepediaTrickEntryDataModel | e9086ff73c2d7af62054bebe061857df55e4e953 |
| 36739 | SkatepediaDataResource | 823252aa8a4cebc50f51aee1d705493dc37c3b57 |
| 37768 | Skatepedia_InfoCard_Widget | 378558e95e56fb1cba1ebf528ec6c114d46c5f04 |

These bounded source assets were read without game input or process modification,
hash checked and decoded locally. The original page schema is 4004900296;
MainPage is field 1244125067 and uses the existing PageViewModel schema
0x5844be65. The entry model independently owns TrickName, TrickCategory,
Descriptions, SelectedDescriptionDelegate, SubCategoryId and TrickInputs.
Original entries have stance-dependent descriptions and native previews; those
assets are not evidence of Arcade execution.

## Registry and adapters

`arcade_trick_registry.h` owns canonical records, typed alternative bindings,
native flip identities, native grab poses, fingerflip identities/source families/
destination families, and established-rail button pairs. Existing enum values are
retained for runtime compatibility. Alternate poses resolve to their existing
canonical record. Native animation slots are explicit and remain in the original
observer order even when the guide sorts the catalog.

`arcade_trick_table.h`, `arcade_native_mapping.h`, `arcade_native_grab_lease.h`
and `arcade_fingerflip.h` expose derived compatibility views. They do not carry
second copies of migrated names, bindings or native identity maps. Complex
execution remains in InputSession and the existing native adapters. Shared input
policy supplies timing; footplant, ground-grab, extended-grab, repeated-flip and
established-rail selectors consume the registry or its descriptors.

The catalog distinguishes original ordinary flip IDs from fingerflip IDs and
native grab details. Recognized but unsupported legacy rail-approach bindings
are shown as proposed, even where the same destination has an executable
established-rail transition. Those legacy grouped rail selectors still describe
geometry-dependent alternatives; they are not evidence that both members work.
Prototype contextual pressure/lip adapters without native producers are Planned.

## Native integration and fallback

`native_trickpedia.inl` discovers the original Skatepedia page under the existing
native model lock. When Arcade is enabled and that page has focus, it retains an
engine-owned typed copy of MainPage, substitutes a copy of ReSkate's existing
page shell, and preserves the original Back action. The existing Hub overlay
renders section 3 from the registry, with controller focus, search, category and
status filters, scrolling, selection and live R3+B pose choice.

Disabling Arcade restores the retained original page in the same session.
Level transitions restore before native assets unload. Manager replacement
discards obsolete handles; every live access revalidates handle types. No
localization override is installed: `text_overrides::release` cannot restore
shipped strings until restart and is unsuitable for this feature.

The separately accessible fallback is Pause → Mod Options → Player → Arcade
Trickpedia (also the TRICKS tab), visible only with Arcade enabled. If the
native page schema/focus checks fail, the normal Skatepedia is left alone and
the fallback remains available. The source-supported interception is the
Skatepedia MainPage model, not a blanket replacement of all navigation commands.
Actual native page publication, Back and focus behavior require hardware
acceptance; source inspection and portable tests do not establish them.

## Status and acceptance

Every record resolves to exactly Verified, Experimental or Planned. Executable
RC3 paths remain Experimental because the cited RC3 report leaves full acceptance
open. No family-wide Verified claim is made. Verified requires complete action,
physics/contact, recognition, exit/landing and scoring/combo coverage, a capture
reference and context, matching game build and exact source revision, and no
invalidation. Missing or stale evidence falls back to Experimental; no handler
means Planned. The current UI intentionally supplies no verified candidate
revision and therefore cannot accidentally inherit a previous build's acceptance.

Independent hardware acceptance must establish installed and mapped DLL identity,
original Skatepedia navigation with Arcade OFF, Arcade ON replacement, controller
search/filter/scroll/Back operation, a representative binding's real action and
landing, and OFF restoration without restart. Preserve the original-page state
and include OFF/ON/OFF, level reload, native menu close/reopen and focus-loss
negative controls. Serialize all input and console work under the established
shared device lock, with a separate exclusive handoff from RC3/RC4 work before
any installation or controller trial.

## Installation and rollback

Use only this branch's development workflow artifacts, never the rolling accepted
channel. The Windows artifact carries `ReSkate.dll`, `commit.txt` and
`ReSkate.dll.sha256`; the Deck job packages the same source and DLL after its
required gates. Record the final source and DLL checksum with the test receipt.
Close game and launcher before installing through the existing transactional
installer. Retain the previous DLL, launcher settings and installation receipt;
restore those exact files with both processes closed to roll back. Preserve all
stock, RC2, RC3 and RC4 rollback artifacts. This draft does not authorize a merge
or release-channel promotion.
