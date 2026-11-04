# JSON interoperability

| Format | Producer / consumer | Current meaning |
| --- | --- | --- |
| `vlc-flubber-appearance/v1` | Planner export ↔ VLC+ native settings | Flubber colors, transparency, outline/halo, color anchors and six visual mappings |
| `vlc-feedback-settings/v2` | VLC+ settings export ↔ VLC+ settings import | Appearance plus panel height, primary layer, input mode, layer positions/visibility, grid and face settings |
| `affect-research-planner-recipe` with explicit version | Planner → Experiment Runner | Complete study recipe; VLC+ intake is not implemented |
| `flubbercorder-experiment/v1` | Separate Flubbercorder console | Companion configuration; not a Planner master and not loaded as a full study by VLC+ |

Use **Flubber → Settings…** to import/export appearance or player settings.
For startup, `--flubber-settings=<absolute JSON path>` loads supported settings;
see [the native guide](../vlc-qt/README.md) for overrides and RC commands.

[The example](../examples/planner-appearance.v1.json) is emitted by the original
Planner exporter at the source commit in [provenance](provenance.md). It
contains no participant, questionnaire, media path, input binding, sampling
rate, sequence or timing authority. Importing it is appearance compatibility.

Native readers reject duplicate/unknown keys and invalid values. Preserve the
current strict size limits and validate before replacing live settings. Future
master support must dispatch on explicit schema/version and retain every
run-affecting semantic; see [the integration gate](../for-ai/PLANNER-INTEGRATION.md).
