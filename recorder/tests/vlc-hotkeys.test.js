import assert from "node:assert/strict";
import test from "node:test";
import { vlcHotkeys } from "../web/vlc-hotkeys.js";

test("the shared guide keeps playback controls and lists the shared settings and representation popups", () => {
  assert.deepEqual(vlcHotkeys.map(([keys]) => keys), [
    "Alt+F", "Left / Right", "Down / Up", "Alt+G / Alt+Shift+G", "Alt+H / Alt+Shift+H",
    "Alt+W / A / S / D", "Alt+plus / minus", "Alt+J", "Ctrl+Shift+S", "Ctrl+Shift+1 / 2 / 3", "Escape",
  ]);
  const guide = new Map(vlcHotkeys.map(([keys, action]) => [keys, action]));
  assert.match(guide.get("Ctrl+Shift+S"), /shared arrow hold time.*JSON/);
  assert.match(guide.get("Ctrl+Shift+1 / 2 / 3"), /Flubber, 2D grid or Face morph.*preview/);
  assert.match(guide.get("Alt+J"), /snapshot.*restores.*launch/);
});
