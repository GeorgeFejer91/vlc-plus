# Decision log

Record only durable choices whose rationale future agents would otherwise have
to rediscover. Source and tests remain the authority for implementation facts.

## D-0001 — Minimal control plane before product scaffolding

- Date: 2026-11-04
- Status: Accepted
- Context: The project needs a predictable AI entrypoint without choosing an
  application stack or inventing product structure prematurely.
- Decision: Use a short root `AGENTS.md` that routes to a lowercase `for-ai/`
  control plane. Keep product output outside that folder and add project files,
  skills, and protocols only for current requirements.
- Consequences: New agents get a reliable map and readiness gates. The first
  product task must still choose the smallest suitable output structure.
- Supersedes: None

## Record format

For later decisions, add one compact entry with:

- identifier and title;
- date and status (`Proposed`, `Accepted`, `Superseded`, or `Rejected`);
- context;
- decision;
- consequences;
- supersession link when applicable.

Do not rewrite accepted history to hide a changed direction. Add a superseding
decision and link both entries.

## D-0002 — Standalone Qt player, private first

- Date: 2026-11-04
- Status: Accepted
- Decision: Extract the current VLC Qt implementation into `vlc-plus`, using
  the authenticated owner's private GitHub repository. Keep recorders and the
  Research Planner/Runner as independent projects. Private is the For-AI
  default because visibility was unspecified.
- Consequences: Retain exact source provenance and notices; adapt packaging
  paths and installer identity, with no runtime semantic change.

## D-0003 — One experiment contract, explicit compatibility gaps

- Date: 2026-11-04
- Status: Accepted
- Decision: Target the same versioned Planner recipe for Runner and VLC+.
  Existing standalone appearance and VLC settings JSON remain separate formats.
- Consequences: Full recipe intake requires strict versioned readers,
  capability rejection and independent correspondence evidence. Extraction
  does not imply questionnaire, sequence, timing or recovery support.

## D-0004 — Public repository and collaborator access

- Date: 2026-11-04
- Status: Accepted
- Decision: The user explicitly authorized public visibility for VLC+ and
  Affect Research, and collaborator access for Antonin Fourcade (`afourcade`).
  Grant write access, preserving owner/admin authority.
- Evidence: GitHub reported both repositories public; write invitations for
  `afourcade` were pending acceptance on both at the time of the change.
- Supersedes: D-0002's initial private-visibility choice only. Independent
  project boundaries, provenance and research evidence requirements remain.

## D-0005 — Experimental Windows installer and bridge lifetime

- Date: 2026-11-04
- Status: Accepted
- Decision: Ship a per-user standalone installer with its own portable profile,
  immutable payload manifest, actual toolchain receipt and matching dependency
  notices/sources. Validate the exact fresh install in a clean Windows account.
- Runtime fix: retain the Rust/LSL DLL mapping through process exit using
  QLibrary PreventUnloadHint. Still stop the GUI timer, join owned workers and
  drain CSV before releasing the bridge. Immediate DLL unload caused an
  observed access violation from process-lifetime code/TLS.
- Consequences: the experimental release qualifies its named offscreen scope.
  Preserve the existing marker stream but disclose VLC 3's terminal End
  ambiguity and initial CSV acquisition gap. Scientific completion/onset and
  the full shared Planner recipe need separate contracts and qualification.
