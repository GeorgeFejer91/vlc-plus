export function showVlcHotkeys(list) {
  const shortcuts = [
    ["Alt+F", "Toggle affect arrows. While on, VLC arrow shortcuts are reserved."],
    ["Left / Right", "Decrease / increase valence."],
    ["Down / Up", "Decrease / increase arousal."],
    ["Alt+G / Alt+Shift+G", "Switch representation: Flubber, 2D grid, affective face morph map. One is visible at a time."],
    ["Alt+H / Alt+Shift+H", "Next / previous face character (9 total). Selects face morph."],
    ["Ctrl+Shift+1 / 2 / 3", "Select Flubber / grid / face directly."],
    ["Ctrl+Alt+1 / 2 / 3", "Select Flubber / grid / face directly (alternate keys)."],
    ["Ctrl+Alt+arrows", "With affect arrows on, move the main feedback. With them off, VLC keeps these keys."],
    ["Ctrl+Alt+= / -", "Increase / decrease feedback size."],
    ["Ctrl+Alt+[ / ]", "Decrease / increase affect step."],
    ["Ctrl+Alt+, / .", "Slower / faster face transitions."],
    ["Ctrl+Alt+PageUp / PageDown", "Increase / decrease grid density."],
    ["Ctrl+Shift+PageUp / PageDown", "Increase / decrease panel height."],
    ["Ctrl+Shift+S", "Open detailed Flubber settings."],
    ["Escape", "Release mouse rating capture."],
  ];
  for (const [keys, action] of shortcuts) {
    const term = document.createElement("dt");
    const description = document.createElement("dd");
    term.textContent = keys;
    description.textContent = action;
    list.append(term, description);
  }
}
