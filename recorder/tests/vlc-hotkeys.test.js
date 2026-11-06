import assert from "node:assert/strict";
import test from "node:test";
import { vlcHotkeys } from "../web/vlc-hotkeys.js";

test("the shared guide retains the original controls and all dedicated settings", () => {
  const original = ["Alt+F", "Left / Right", "Down / Up", "Alt+G / Alt+Shift+G", "Alt+H / Alt+Shift+H", "Alt+W / A / S / D", "Alt+plus / minus", "Alt+J", "Ctrl+Shift+1 / 2 / 3", "Ctrl+Alt+1 / 2 / 3", "Ctrl+Alt+arrows", "Ctrl+Alt+= / -", "Ctrl+Alt+[ / ]", "Ctrl+Alt+, / .", "Ctrl+Alt+PageUp / PageDown", "Ctrl+Shift+PageUp / PageDown", "Ctrl+Shift+S", "Escape"];
  assert.deepEqual(vlcHotkeys.slice(0, 18).map(([keys]) => keys), original);
  assert.equal(vlcHotkeys.length, 64);
  assert.equal(new Set(vlcHotkeys.map(([keys]) => keys)).size, 64);
  const guide = new Map(vlcHotkeys.map(([keys, action]) => [keys, action]));
  for (const [key, setting, step] of [["T", "transparency", "0.05"], ["B", "outline width", "0.25"], ["W", "halo width", "5 percentage points"], ["E", "steepness", "0.1"], ["X", "columns", "2"], ["Y", "rows", "2"], ["J", "line width", "0.25"], ["P", "outline width", "0.25"], ["Z", "cursor size", "1"]]) {
    const action = guide.get(`Ctrl+Alt+${key} / Ctrl+Alt+Shift+${key}`);
    assert.ok(action?.includes(setting) && action.includes(`by ${step}`), key);
  }
  for (const [key, setting] of [["O", "outline"], ["H", "halo"], ["G", "gradient"], ["A", "axes / corners"], ["V", "grid outline"]]) {
    assert.ok(guide.get(`Ctrl+Alt+${key}`)?.includes(setting), key);
  }
  for (const [index, setting] of ["frequency", "edge smoothness", "projection amplitude", "pulse synchrony", "wave-size variation", "saturation"].entries()) {
    const step = index === 0 ? "0.1" : "0.05";
    assert.ok(guide.get(`Ctrl+Alt+F${index + 1} / Ctrl+Alt+Shift+F${index + 1}`)?.includes(`${setting} maximum by ${step}`));
    assert.ok(guide.get(`Ctrl+Alt+F${index + 7} / Ctrl+Alt+Shift+F${index + 7}`)?.includes(`${setting} minimum by ${step}`));
    assert.ok(guide.get(`Ctrl+Alt+${index + 4} / Ctrl+Alt+Shift+${index + 4}`)?.includes(`${setting} driver: x-axis, y-axis, angle, radius`));
    assert.ok(guide.get(`Ctrl+Shift+F${index + 1}`)?.includes(`${setting} mapping direction`));
  }
  for (const [key, setting] of [["U", "up"], ["D", "down"], ["L", "left"], ["R", "right"], ["I", "idle"], ["K", "outline"], ["Q", "halo"], ["C", "cursor"]]) {
    assert.ok(guide.get(`Ctrl+Alt+${key}`)?.startsWith(`Choose ${setting} color. Cancel preserves settings.`));
  }
  assert.match(guide.get("Alt+J"), /full v3.*affect step.*last successful.*automatically next launch.*v2 imports/s);
});
