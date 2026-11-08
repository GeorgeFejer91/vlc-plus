import assert from "node:assert/strict";
import { createRequire } from "node:module";
import { createServer } from "node:http";
import { readFile, mkdir } from "node:fs/promises";
import { resolve, extname } from "node:path";
import { vlcHotkeys } from "../web/vlc-hotkeys.js";

const require = createRequire(import.meta.url);
const { chromium } = require(process.env.PLAYWRIGHT_MODULE || "playwright");
const root = resolve(import.meta.dirname, "..");
const output = resolve(root, ".for-ai-local/record-and-play");
await mkdir(output, { recursive: true });
const server = createServer(async (req, res) => {
  try {
    const path = resolve(root, `.${new URL(req.url, "http://localhost").pathname}`);
    if (!path.startsWith(root + "/") && !path.startsWith(root + "\\")) throw new Error("Outside root");
    const bytes = await readFile(path);
    res.setHeader("Content-Type", ({ ".html": "text/html", ".js": "text/javascript", ".mjs": "text/javascript", ".css": "text/css", ".woff2": "font/woff2", ".svg": "image/svg+xml" })[extname(path)] || "application/octet-stream");
    res.end(bytes);
  } catch { res.writeHead(404); res.end(); }
});
await new Promise(done => server.listen(0, "127.0.0.1", done));
const browser = await chromium.launch({ channel: "msedge", headless: true });
try {
  const page = await browser.newPage();
  page.on("pageerror", error => console.error(error.message));
  page.setDefaultTimeout(2500);
  page.setDefaultNavigationTimeout(30000);
  const calls = [];
  let fail = false;
  let phase = "idle";
  let streams = [];
  await page.exposeFunction("fixtureInvoke", async (command, args) => {
    calls.push({ command, args });
    if (command === "start_vlc_and_record") {
      if (fail) throw new Error("VLC+ is not listening on the selected port.");
      phase = "recording";
    }
    if (command === "stop_recording") phase = "idle";
    return { revision: 0, controlRevision: 0, participantId: "UITest", outputDirectory: "C:\\Recordings",
      selectAllStreams: true, keyboardMarkers: false, mouseMarkers: false, externalPages: [],
      externalPagesInitialized: true, externalPagesRevision: 0, viewer: {}, streams, markers: [],
      recording: { phase, bytesWritten: 0 }, remote: { phase: "idle", active: false, route: "unknown", approval: "waiting" } };
  });
  await page.addInitScript(() => { window.__TAURI__ = { core: { invoke: (command, args = {}) => window.fixtureInvoke(command, args) } }; });
  await page.goto(`http://127.0.0.1:${server.address().port}/web/index.html`);
  const launch = page.getByRole("button", { name: "Start VLC and record", exact: true });
  await launch.waitFor();
  assert.equal(await page.locator(".vlc-record-action").count(), 1);
  assert.equal(await page.locator("#record-play").count(), 0);
  assert.equal(await launch.locator('svg[aria-hidden="true"]').count(), 1);
  assert.equal(await launch.locator("img").count(), 0);
  const outlineColors = await launch.locator(".vlc-cone-outlines use")
    .evaluateAll(outlines => outlines.map(outline => getComputedStyle(outline).stroke));
  assert.deepEqual(outlineColors, ["rgb(255, 255, 255)", "rgb(255, 255, 255)"],
    "Each inward-facing cone edge has a white outline");
  await launch.click();
  assert.equal(await page.locator("#vlc-dialog").evaluate(el => el.open), true);
  assert.equal(calls.some(call => call.command === "start_vlc_and_record"), false);
  await page.getByLabel("Video path", { exact: true }).fill("C:\\Videos\\clip with spaces.mp4");
  await page.getByRole("button", { name: "Close VLC+ setup" }).click();
  await launch.click();
  await page.waitForFunction(() => document.querySelector("#record-phase").textContent === "Recording");
  const launched = calls.find(call => call.command === "start_vlc_and_record");
  assert.deepEqual(launched.args, { port: 42123, videoPath: "C:\\Videos\\clip with spaces.mp4" });
  assert.equal(await launch.isDisabled(), true);
  await page.getByRole("button", { name: "Stop recording", exact: true }).click();
  await page.waitForFunction(() => !document.querySelector("#vlc-launch-record").disabled);
  await page.getByRole("button", { name: "VLC+ setup", exact: true }).click();
  await page.getByLabel("VLC+ control port").fill("42124");
  await page.getByRole("button", { name: "Close VLC+ setup" }).click();
  await launch.focus();
  await page.keyboard.press("Shift+Tab");
  await page.keyboard.press("Tab");
  assert.equal(await launch.evaluate(el => el.matches(":focus-visible") && getComputedStyle(el).outlineStyle !== "none"), true);
  await page.keyboard.press("Enter");
  await page.waitForFunction(() => document.querySelector("#record-phase").textContent === "Recording").catch(async error => {
    console.error(JSON.stringify({ calls, status: await page.locator("#system-status").textContent(), port: await page.locator("#vlc-port").inputValue() }));
    throw error;
  });
  assert.equal(await launch.isDisabled(), true);
  const startIndex = calls.findLastIndex(call => call.command === "start_vlc_and_record");
  assert.equal(calls[startIndex].args.port, 42124);
  assert.equal(calls[startIndex - 1].command, "configure_session");
  await page.getByRole("button", { name: "Stop recording", exact: true }).click();
  await launch.waitFor({ state: "visible" });
  await page.waitForFunction(() => !document.querySelector("#vlc-launch-record").disabled);
  fail = true;
  await launch.click();
  await page.waitForFunction(() => document.querySelector("#system-status").textContent.includes("not listening"));
  assert.equal(await launch.isEnabled(), true);
  streams = [{ id: "vlc-affect", name: "VLC_Flubber_Affect", streamType: "Affect", hostname: "Test PC",
    sourceId: "vlc-flubber-fixture-affect", channelCount: 2, nominalRate: 30, selected: true,
    connected: true, isMarker: false, channels: [{ label: "valence", unit: "normalized" }, { label: "arousal", unit: "normalized" }],
    preview: [{ timestamp: 1, values: [0, 0] }, { timestamp: 2, values: [0.25, -0.5] }] }];
  const scores = page.locator(".affect-score-number");
  await page.waitForFunction(() => document.querySelector(".affect-score-number")?.textContent === "+0.25");
  assert.deepEqual(await scores.allTextContents(), ["+0.25", "-0.50"]);
  streams[0].preview.push({ timestamp: 3, values: [-1, 0.75] });
  await page.waitForFunction(() => document.querySelector(".affect-score-number")?.textContent === "-1.00");
  assert.deepEqual(await scores.allTextContents(), ["-1.00", "+0.75"]);
  streams[0].connected = false;
  await page.waitForFunction(() => document.querySelector(".affect-score-status")?.textContent === "Disconnected");
  assert.deepEqual(await scores.allTextContents(), ["—", "—"]);
  streams[0].connected = true;
  streams[0].preview = [];
  await page.waitForFunction(() => document.querySelector(".affect-score-status")?.textContent === "Waiting for samples…");
  assert.deepEqual(await scores.allTextContents(), ["—", "—"]);
  streams[0].selected = false;
  await page.waitForFunction(() => document.querySelector("#affect-score-list").hidden);
  streams[0].selected = true;
  streams[0].preview = [{ timestamp: 4, values: [0, 0.5] }, { timestamp: 5, values: [-0.25, 0.75] }];
  await page.waitForFunction(() => document.querySelector(".affect-score-number")?.textContent === "-0.25");
  async function checkLibrary() {
    const seen = new Map();
    const category = page.getByLabel("Category", { exact: true });
    const names = await category.locator("option").evaluateAll(options => options.map(option => option.value));
    assert.deepEqual(names, ["Player", "Layout"]);
    for (const name of names) {
      await category.selectOption(name);
      for (;;) {
        await page.waitForFunction(() => document.querySelector("#vlc-hotkeys").dataset.layout === "fit").catch(async error => {
          await page.screenshot({ path: resolve(output, "library-failure.png"), fullPage: true });
          console.error(await page.locator("#vlc-hotkeys").evaluate(list => ({ category: document.querySelector("#vlc-hotkey-category").value, state: list.dataset.layout, height: list.clientHeight, contentHeight: list.scrollHeight, text: list.textContent })));
          throw error;
        });
        const labels = await page.locator("#vlc-hotkeys dt").allTextContents();
        const actions = await page.locator("#vlc-hotkeys dd").allTextContents();
        assert.ok(labels.length > 0 && labels.length <= 6);
        labels.forEach((keys, index) => { assert.equal(seen.has(keys), false, `Repeated ${keys}`); seen.set(keys, actions[index]); });
        const overflow = await page.locator("#vlc-dialog, #vlc-library, #vlc-hotkeys, .vlc-library-footer").evaluateAll(regions => regions.filter(region => region.scrollWidth > region.clientWidth + 1 || region.scrollHeight > region.clientHeight + 1).map(region => region.id || region.className));
        assert.deepEqual(overflow, [], `Panel overflow: ${name}`);
        assert.equal(await page.locator("#vlc-hotkeys").getAttribute("data-measurement"), "pretext");
        assert.deepEqual(await page.locator('#vlc-library [data-hotkey-fit="no-fit"], #vlc-library [data-hotkey-fit="unavailable"]').evaluateAll(labels => labels.map(label => ({ text: label.textContent, width: label.clientWidth, height: label.clientHeight, lineHeight: getComputedStyle(label).lineHeight, fontSize: getComputedStyle(label).fontSize }))), []);
        if (await page.getByRole("button", { name: "Next", exact: true }).isDisabled()) break;
        await page.getByRole("button", { name: "Next", exact: true }).click();
      }
    }
    assert.deepEqual([...seen].sort(), vlcHotkeys.map(([keys, action]) => [keys, action]).sort());
  }
  async function checkRecordButton() {
    assert.equal(await launch.evaluate(el => el.scrollWidth <= el.clientWidth + 1 && el.scrollHeight <= el.clientHeight + 1), true);
    await page.waitForFunction(() => [...document.querySelectorAll(".vlc-record-action [data-fit-text]")].every(label => ["fit", "no-fit"].includes(label.dataset.fitState)));
    const { cones, body, bounds, text, labelFits } = await launch.evaluate(button => {
      // Clip-path shapes expose their geometry in SVG coordinates.
      const paintedBounds = shape => {
        const { x, y, width, height } = shape.getBBox();
        const transform = shape.getScreenCTM();
        const start = new DOMPoint(x, y).matrixTransform(transform);
        const end = new DOMPoint(x + width, y + height).matrixTransform(transform);
        return { left: start.x, top: start.y, right: end.x, bottom: end.y, width, height };
      };
      const label = button.querySelector("[data-fit-text]");
      return {
        cones: [...button.querySelectorAll(".vlc-button-cone")].map(paintedBounds),
        body: paintedBounds(button.querySelector(".vlc-button-body")),
        bounds: button.getBoundingClientRect().toJSON(),
        text: label.getBoundingClientRect().toJSON(),
        labelFits: label.scrollWidth <= label.clientWidth + 1 && label.scrollHeight <= label.clientHeight + 1,
      };
    });
    assert.equal(cones.length, 2, "The orange button has one cone at each end");
    const [left, right] = cones;
    assert.ok(left.width > 0 && right.width > 0 && body.width > 0 && body.height > 0);
    assert.ok(left.left >= bounds.left && left.right < text.left && text.right < right.left && right.right <= bounds.right,
      "The full label fits between the cones");
    assert.ok(Math.abs(left.bottom - body.bottom) < 1 && Math.abs(right.bottom - body.bottom) < 1,
      "Both cone bases align with the orange body");
    assert.ok(text.top >= body.top && text.bottom <= body.bottom, "The orange body contains the label vertically");
    assert.equal(labelFits, true);
  }
  for (const [width, height] of [[320, 600], [320, 900], [720, 600], [1280, 600], [1280, 900], [1920, 1080]]) {
    await page.setViewportSize({ width, height });
    await page.evaluate(() => document.fonts.ready);
    await checkRecordButton();
    assert.equal(await page.locator("#affect-score-list").evaluate(region => region.scrollWidth <= region.clientWidth + 1), true);
    assert.deepEqual(await scores.allTextContents(), ["-0.25", "+0.75"]);
    await page.getByRole("button", { name: "VLC+ setup", exact: true }).click();
    assert.equal(await page.locator("#vlc-dialog").evaluate(el => el.scrollWidth <= el.clientWidth + 1 && el.scrollHeight <= el.clientHeight + 1), true);
    await page.getByRole("button", { name: "Hotkeys", exact: true }).click();
    await checkLibrary();
    await page.screenshot({ path: resolve(output, `library-${width}-${height}.png`), fullPage: true });
    await page.getByLabel("Category", { exact: true }).selectOption("Player");
    await page.waitForFunction(() => document.querySelector("#vlc-hotkeys").dataset.layout === "fit");
    await page.getByRole("button", { name: "Next", exact: true }).focus();
    await page.keyboard.press("Enter");
    await page.waitForFunction(() => !document.querySelector("#vlc-hotkey-previous").disabled);
    await page.getByRole("button", { name: "Previous", exact: true }).focus();
    await page.keyboard.press("Shift+Tab");
    const focus = await page.evaluate(() => ({ id: document.activeElement.id, visible: document.activeElement.matches(":focus-visible"), outline: getComputedStyle(document.activeElement).outlineStyle }));
    assert.equal(focus.visible, true, JSON.stringify(focus));
    assert.notEqual(focus.outline, "none");
    await page.getByRole("button", { name: "Setup", exact: true }).click();
    await page.getByRole("button", { name: "Close VLC+ setup" }).click();
    await page.screenshot({ path: resolve(output, `recorder-${width}.png`), fullPage: true });
  }
  await page.setViewportSize({ width: 1280, height: 900 });
  await launch.screenshot({ path: resolve(output, "record-button.png") });
  await page.addStyleTag({ content: "html {font-size:200% !important} #vlc-launch-record span {line-height:1.5 !important;letter-spacing:.12em !important;word-spacing:.16em !important}" });
  await checkRecordButton();
  assert.equal(await page.locator(".setup-panel").evaluate(panel => panel.scrollWidth <= panel.clientWidth + 1 && panel.scrollHeight <= panel.clientHeight + 1), true, "Enlarged controls and the session form must remain visible");
  await page.screenshot({ path: resolve(output, "recorder-large-text.png"), fullPage: true });
  await page.setViewportSize({ width: 320, height: 900 });
  await checkRecordButton();
  await page.setViewportSize({ width: 1280, height: 900 });
  await page.getByRole("button", { name: "VLC+ setup", exact: true }).click();
  assert.equal(await page.locator("#vlc-settings").isVisible(), true);
  assert.equal(await page.locator("#vlc-dialog").evaluate(el => el.scrollWidth <= el.clientWidth + 1 && el.scrollHeight <= el.clientHeight + 1), true);
  await page.getByRole("button", { name: "Hotkeys", exact: true }).click();
  assert.equal(await page.evaluate(() => document.fonts.check('400 16px "Noto Sans"') && document.fonts.check('600 16px "Noto Sans"')), true);
  await checkLibrary();
  await page.addStyleTag({ content: '#vlc-dialog p, #vlc-dialog dt, #vlc-dialog dd, #vlc-dialog label, #vlc-dialog button, #vlc-dialog select {line-height:1.5 !important;letter-spacing:.12em !important;word-spacing:.16em !important} #vlc-dialog p {margin-bottom:2em !important}' });
  await checkLibrary();
  await page.screenshot({ path: resolve(output, "library-large-text.png"), fullPage: true });
  await page.getByRole("button", { name: "Close VLC+ setup" }).click();
  await page.locator("#vlc-video").evaluate(input => { input.value = ""; });
  await launch.click();
  assert.equal(await page.locator("#vlc-settings").isVisible(), true);
  assert.equal(await page.getByLabel("Video path", { exact: true }).evaluate(input => input === document.activeElement), true);
  await page.getByRole("button", { name: "Close VLC+ setup" }).click();
  await page.goto(`http://127.0.0.1:${server.address().port}/companion/vlc.html`);
  await page.locator("#vlc-hotkeys dt").first().waitFor();
  assert.equal(await page.locator("#vlc-hotkeys dt").count(), vlcHotkeys.length);
  assert.deepEqual(await page.locator("#vlc-hotkeys dt").allTextContents(), vlcHotkeys.map(([keys]) => keys));
  assert.match(await page.locator("#vlc-hotkeys").textContent(), /Alt\+H \/ Alt\+Shift\+H.*9 total/s);
  assert.match(await page.locator("#vlc-hotkeys").textContent(), /Alt\+W.*Alt\+J.*snapshot/s);
  for (const width of [320, 390, 720, 1280, 1920]) {
    await page.setViewportSize({ width, height: 800 });
    await page.evaluate(() => document.fonts.ready);
    assert.equal(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth + 1), true, `Hotkey page overflow at ${width}`);
    await page.screenshot({ path: resolve(output, `hotkeys-${width}.png`), fullPage: true });
  }
  await page.setViewportSize({ width: 320, height: 800 });
  await page.evaluate(() => {
    document.documentElement.style.fontSize = "200%";
    for (const element of document.querySelectorAll("body *")) {
      element.style.lineHeight = "1.5";
      element.style.letterSpacing = ".12em";
      element.style.wordSpacing = ".16em";
    }
  });
  assert.equal(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth + 1), true);
  console.log("PASS: aligned SVG cones, received valence/arousal values, missing/disconnected/selection states, launch/record/play, keyboard, 320–1920px, 200% text and bounded hotkey library (mock native IPC).");
} finally { await browser.close(); await new Promise(done => server.close(done)); }
