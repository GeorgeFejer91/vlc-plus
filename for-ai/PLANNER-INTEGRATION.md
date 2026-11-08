# Planner and Runner integration

Read this when changing JSON readers, protocol execution, sampling, markers,
input, saved settings or cross-project behavior. Product format support is
listed in [JSON formats](../docs/experiment-formats.md); this file owns the
integration goal and agent acceptance rules.

## Project boundaries

| Project | Authority |
| --- | --- |
| Affect Research Experiment Planner | Author and validate studies; export exact versioned recipes and referenced assets |
| Affect Research Experiment Runner | Resolve participant/language/variant, execute the complete authored protocol and own its session evidence |
| VLC+ | Native VLC playback, feedback and its own acquired input/sample/marker/CSV evidence; future strict recipe consumer |
| Flubbercorder / Remote LSL Recorder | Independent stream subscription, recording and separately authorized controls |

The desired result is one Planner recipe usable by Runner and VLC+ when each
supports every requested feature. Separate GitHub projects do not authorize
separate interpretations of the same field. Compatibility is based on schema,
version, content identity and capability, never matching filenames alone.

## Upstream sources and contract ownership

The extraction manifest pins Affect Research at
`16106a70b57136c7243e85a5a05d83b0789bdd2c`. At that revision Planner's exporter
is `site/src/research/vlc-flubber-appearance.js`, with its test at
`test/research-vlc-flubber-appearance.test.js`. The original modules generated
`examples/planner-appearance.v1.json`; it is an appearance-only fixture.

The continuing repository is
[affect-tracker-research](https://github.com/GeorgeFejer91/affect-tracker-research).
Consult its current `AGENTS.md`, compatibility map, strict JS/Rust readers and
P7 recipe owner before choosing a full-master support version. Its paths and
saved generations have changed since the extraction revision. Pin the exact
Planner and Runner commits used by each new correspondence fixture; do not
infer support from an older README or overwrite historical schema meanings.

## Planner compartments and VLC+ support today

| Planner owner | Authored material | VLC+ status |
| --- | --- | --- |
| P1 | Study/workspace, exact video identity and geometry | Opens local files; no complete P1 recipe/asset verification reader |
| P2 | Language tree, questionnaires, routes and asset hashes | Unsupported |
| P3 | Variants, exact video/ISI sequence and marker contract | Unsupported as a Planner protocol |
| P4 | Authored screen/video/feedback geometry | Native player settings exist; no P4 semantic correspondence |
| P5 | Feedback, mappings, response and input | Standalone appearance subset imports; this does not import bindings or acquisition policy |
| P6 | Optional XR layout | Unsupported |
| P7 | Complete master, policy, integrity and save/reopen | Complete master intake unsupported |

`vlc-feedback-settings/v4` and earlier versions are VLC-local settings. They do
not replace a P7 master or import the Planner response policy.
Flubbercorder's `flubbercorder-experiment/v1` belongs to that companion and is
not the common Planner experiment format.

## Next milestone: exact common-recipe intake

1. Select an actual Planner/Runner revision pair and supported saved
   generation. Keep their complete authoritative schema; do not introduce a
   VLC-only experiment schema or hand-picked field reader.
2. Put a strict reader and capability report at VLC+'s intake boundary.
   Reject unsupported features before playing, opening outputs or emitting
   attempt markers. Preserve original JSON bytes and hash-bound assets.
3. Resolve the same participant/language/variant into the same ordered step
   plan as Runner. Requested questionnaires/ISIs, rate, input, layout, resets,
   markers, timestamps and recovery must each have an explicit consumer.
4. Declare real backend differences. Current VLC+ nominal 30 Hz output and
   filename-based markers do not equal Runner's authored cadence and semantic
   event contracts. Browser Runner also has a different capability boundary.
5. Implement only a declared executable subset until the complete set has
   consumers. A recipe outside that subset must fail with a precise reason.
6. Test one exact recipe and asset tree independently in Planner, Runner and
   VLC+. Compare identity, derived plans and semantic event meaning. Then
   qualify installed video, physical input, timing and independent XDF
   reconstruction against the exact changed artifacts.

No runtime compatibility adapter is added by the extraction milestone. A
future shared contract package is justified only after two concrete consumers
need it; avoid copied validators drifting across three repositories.
