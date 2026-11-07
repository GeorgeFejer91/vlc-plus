import assert from "node:assert/strict";
import test from "node:test";
import { readAffectScores } from "../web/affect-scores.js";

test("received affect scores follow channel labels and never invent missing values", () => {
  const stream = {
    selected: true, connected: true, isMarker: false,
    channels: [{ label: " Arousal ", unit: "normalized" }, { label: "Valence", unit: "normalized" }],
    preview: [{ values: [-0.5, 0.25] }, { values: [0.75, -1] }],
  };
  assert.deepEqual(readAffectScores(stream), [
    { label: "Valence", value: -1, unit: "normalized" },
    { label: "Arousal", value: 0.75, unit: "normalized" },
  ]);
  assert.equal(readAffectScores({ ...stream, selected: false }), null);
  assert.equal(readAffectScores({ ...stream, isMarker: true }), null);
  assert.equal(readAffectScores({ ...stream, channels: [{ label: "heart rate" }] }), null);
  assert.deepEqual(readAffectScores({ ...stream, connected: false }).map(axis => axis.value), [null, null]);
  assert.deepEqual(readAffectScores({ ...stream, preview: [] }).map(axis => axis.value), [null, null]);
  assert.deepEqual(readAffectScores({ ...stream, preview: [{ values: [Number.NaN, 0] }] }).map(axis => axis.value), [0, null]);
  assert.deepEqual(readAffectScores({ ...stream, preview: [{ values: [0] }] }).map(axis => axis.value), [null, 0]);
});
