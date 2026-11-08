import assert from "node:assert/strict";
import test from "node:test";
import { vlcHotkeys } from "../web/vlc-hotkeys.js";

test("the shared guide keeps playback controls and sends tuning to one settings shortcut", () => {
  assert.deepEqual(vlcHotkeys.map(([keys]) => keys), [
    "Alt+F", "Left / Right", "Down / Up", "Alt+G / Alt+Shift+G", "Alt+H / Alt+Shift+H",
    "Alt+W / A / S / D", "Alt+plus / minus", "Alt+J", "Ctrl+Shift+S", "Escape",
  ]);
  const guide = new Map(vlcHotkeys.map(([keys, action]) => [keys, action]));
  assert.match(guide.get("Ctrl+Shift+S"), /colors.*transparency.*tiles or continuous.*hold time/);
  assert.match(guide.get("Alt+J"), /snapshot.*restores.*launch/);
});
