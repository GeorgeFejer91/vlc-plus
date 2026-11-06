import { measureLineStats, measureNaturalWidth, prepareWithSegments } from "./vendor/pretext/layout.js";

export const vlcHotkeys = [
  ["Alt+F", "Toggle affect arrows. While on, VLC arrow shortcuts are reserved.", "Player"],
  ["Left / Right", "Decrease / increase valence.", "Player"],
  ["Down / Up", "Decrease / increase arousal.", "Player"],
  ["Alt+G / Alt+Shift+G", "Switch representation: Flubber, 2D grid, affective face morph map. One is visible at a time.", "Player"],
  ["Alt+H / Alt+Shift+H", "Next / previous face character (9 total). Selects face morph.", "Player"],
  ["Alt+W / A / S / D", "Move the selected feedback up / left / down / right, in either input mode.", "Layout"],
  ["Alt+plus / minus", "Increase / decrease selected feedback size. Alt+= and numpad keys also work.", "Layout"],
  ["Alt+J", "Save a full v3 settings JSON snapshot, including affect step. The last successful snapshot loads automatically next launch. v2 imports remain supported.", "Player"],
  ["Ctrl+Shift+1 / 2 / 3", "Select Flubber / grid / face directly.", "Player"],
  ["Ctrl+Alt+1 / 2 / 3", "Select Flubber / grid / face directly (alternate keys).", "Player"],
  ["Ctrl+Alt+arrows", "With affect arrows on, move the main feedback. With them off, VLC keeps these keys.", "Layout"],
  ["Ctrl+Alt+= / -", "Increase / decrease feedback size.", "Layout"],
  ["Ctrl+Alt+[ / ]", "Decrease / increase affect step.", "Player"],
  ["Ctrl+Alt+, / .", "Slower / faster face transitions.", "Player"],
  ["Ctrl+Alt+PageUp / PageDown", "Increase / decrease grid density.", "Grid"],
  ["Ctrl+Shift+PageUp / PageDown", "Increase / decrease panel height.", "Layout"],
  ["Ctrl+Shift+S", "Open detailed Flubber settings.", "Player"],
  ["Escape", "Release mouse rating capture.", "Player"],
  ["Ctrl+Alt+T / Ctrl+Alt+Shift+T", "Increase / decrease Flubber transparency by 0.05. Selects Flubber.", "Flubber"],
  ["Ctrl+Alt+O", "Toggle the Flubber outline. Selects Flubber.", "Flubber"],
  ["Ctrl+Alt+B / Ctrl+Alt+Shift+B", "Increase / decrease Flubber outline width by 0.25. Selects Flubber.", "Flubber"],
  ["Ctrl+Alt+H", "Toggle the Flubber halo. Selects Flubber.", "Flubber"],
  ["Ctrl+Alt+W / Ctrl+Alt+Shift+W", "Increase / decrease Flubber halo width by 5 percentage points. Selects Flubber.", "Flubber"],
  ["Ctrl+Alt+G", "Toggle the Flubber halo gradient. Selects Flubber.", "Flubber"],
  ["Ctrl+Alt+E / Ctrl+Alt+Shift+E", "Increase / decrease Flubber halo steepness by 0.1. Selects Flubber.", "Flubber"],
  ["Ctrl+Alt+A", "Switch Flubber axes / corners. Selects Flubber.", "Flubber"],
  ["Ctrl+Alt+X / Ctrl+Alt+Shift+X", "Increase / decrease grid columns by 2, independently of rows. Selects grid.", "Grid"],
  ["Ctrl+Alt+Y / Ctrl+Alt+Shift+Y", "Increase / decrease grid rows by 2, independently of columns. Selects grid.", "Grid"],
  ["Ctrl+Alt+J / Ctrl+Alt+Shift+J", "Increase / decrease grid line width by 0.25. Selects grid.", "Grid"],
  ["Ctrl+Alt+V", "Toggle the grid outline. Selects grid.", "Grid"],
  ["Ctrl+Alt+P / Ctrl+Alt+Shift+P", "Increase / decrease grid outline width by 0.25. Selects grid.", "Grid"],
  ["Ctrl+Alt+Z / Ctrl+Alt+Shift+Z", "Increase / decrease cursor size by 1. Selects grid.", "Grid"],
  ...["frequency", "edge smoothness", "projection amplitude", "pulse synchrony", "wave-size variation", "saturation"].flatMap((name, index) => {
    const step = index === 0 ? "0.1" : "0.05";
    return [
      [`Ctrl+Alt+F${index + 1} / Ctrl+Alt+Shift+F${index + 1}`, `Increase / decrease ${name} maximum by ${step}. Selects Flubber.`, "Mappings"],
      [`Ctrl+Alt+F${index + 7} / Ctrl+Alt+Shift+F${index + 7}`, `Increase / decrease ${name} minimum by ${step}. Selects Flubber.`, "Mappings"],
      [`Ctrl+Alt+${index + 4} / Ctrl+Alt+Shift+${index + 4}`, `Next / previous ${name} driver: x-axis, y-axis, angle, radius. Selects Flubber.`, "Drivers"],
      [`Ctrl+Shift+F${index + 1}`, `Toggle ${name} mapping direction. Selects Flubber.`, "Direction"],
    ];
  }),
  ...[["U", "up"], ["D", "down"], ["L", "left"], ["R", "right"], ["I", "idle"], ["K", "outline"], ["Q", "halo"], ["C", "cursor"]].map(([key, color]) => [
    `Ctrl+Alt+${key}`, `Choose ${color} color. Cancel preserves settings. Selects Flubber.`, "Colors",
  ]),
];

export function showVlcHotkeys(list, shortcuts = vlcHotkeys) {
  list.replaceChildren();
  for (const [keys, action] of shortcuts) {
    const term = document.createElement("dt");
    const description = document.createElement("dd");
    term.textContent = keys;
    description.textContent = action;
    list.append(term, description);
  }
}

export function mountVlcHotkeyLibrary(list, { category, previous, next, status }) {
  const view = list.ownerDocument.defaultView;
  const textContext = list.ownerDocument.createElement("canvas").getContext("2d");
  let page = 0;
  let pageSize = 6;
  let frame = 0;
  for (const name of new Set(vlcHotkeys.map((shortcut) => shortcut[2]))) {
    const option = document.createElement("option");
    option.value = name;
    option.textContent = name;
    category.append(option);
  }
  function render() {
    delete list.dataset.layout;
    const shortcuts = vlcHotkeys.filter((shortcut) => shortcut[2] === category.value);
    const pages = Math.ceil(shortcuts.length / pageSize);
    page = Math.min(page, pages - 1);
    showVlcHotkeys(list, shortcuts.slice(page * pageSize, (page + 1) * pageSize));
    for (const term of [...list.querySelectorAll("dt")]) {
      const entry = document.createElement("div");
      entry.className = "vlc-hotkey-entry";
      entry.append(term, term.nextElementSibling);
      list.append(entry);
    }
    status.textContent = `${category.value}: ${page + 1} of ${pages} · ${shortcuts.length} shortcuts`;
    previous.disabled = page === 0;
    next.disabled = page === pages - 1;
  }
  function measure() {
    frame = 0;
    if (!list.clientWidth || !list.clientHeight) return;
    const oldStart = page * pageSize;
    const count = vlcHotkeys.filter((shortcut) => shortcut[2] === category.value).length;
    pageSize = 6;
    for (; pageSize > 1; pageSize -= 1) {
      let fits = true;
      for (page = 0; page < Math.ceil(count / pageSize); page += 1) {
        render();
        if (list.scrollHeight > list.clientHeight + 1) { fits = false; break; }
      }
      if (fits) break;
    }
    page = Math.floor(oldStart / pageSize);
    render();
    let measured = true;
    const labels = [...list.querySelectorAll("dt, dd"), ...list.parentElement.querySelectorAll("label, p, select, button")];
    for (const label of labels) {
      const style = view.getComputedStyle(label);
      const bounds = label.getBoundingClientRect();
      const select = label.tagName === "SELECT";
      const text = select ? label.selectedOptions[0].textContent : label.textContent;
      const width = bounds.width - parseFloat(style.borderLeftWidth) - parseFloat(style.borderRightWidth)
        - parseFloat(style.paddingLeft) - parseFloat(style.paddingRight) - (select ? 24 : 0);
      const height = bounds.height - parseFloat(style.borderTopWidth) - parseFloat(style.borderBottomWidth)
        - parseFloat(style.paddingTop) - parseFloat(style.paddingBottom);
      try {
        const font = `${style.fontStyle} ${style.fontWeight} ${style.fontSize} ${style.fontFamily}`;
        const prepared = prepareWithSegments(text, font, {
          whiteSpace: "normal", wordBreak: "normal", letterSpacing: style.letterSpacing === "normal" ? 0 : parseFloat(style.letterSpacing),
        });
        const stats = measureLineStats(prepared, width);
        let lineHeight = parseFloat(style.lineHeight);
        if (select) {
          textContext.font = font;
          const metrics = textContext.measureText(text);
          lineHeight = metrics.fontBoundingBoxAscent + metrics.fontBoundingBoxDescent;
        }
        label.dataset.hotkeyFit = stats.maxLineWidth <= width + 1
          && stats.lineCount * lineHeight <= height + 1 && (!select || stats.lineCount === 1)
          && (stats.lineCount !== 1 || measureNaturalWidth(prepared) <= width + 1) ? "fit" : "no-fit";
      } catch {
        label.dataset.hotkeyFit = "unavailable";
        measured = false;
      }
    }
    list.dataset.measurement = measured ? "pretext" : "unavailable";
    list.dataset.layout = list.scrollHeight <= list.clientHeight + 1 ? "fit" : "no-fit";
    list.dataset.pageSize = String(pageSize);
    if (list.dataset.layout === "no-fit") {
      const message = document.createElement("p");
      message.textContent = "Make this window wider or taller to read this shortcut at your text size.";
      list.replaceChildren(message);
    }
  }
  function schedule() {
    if (!frame) frame = view.requestAnimationFrame(measure);
  }
  category.addEventListener("change", () => { page = 0; render(); schedule(); });
  previous.addEventListener("click", () => { page -= 1; render(); schedule(); });
  next.addEventListener("click", () => { page += 1; render(); schedule(); });
  if (view.ResizeObserver) new view.ResizeObserver(schedule).observe(list);
  view.addEventListener("resize", schedule);
  list.ownerDocument.fonts.addEventListener?.("loadingdone", schedule);
  list.ownerDocument.fonts.ready.then(schedule);
  render();
}
