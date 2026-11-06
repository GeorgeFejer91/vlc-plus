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
    res.setHeader("Content-Type", ({ ".html": "text/html", ".js": "text/javascript", ".mjs": "text/javascript", ".css": "text/css", ".woff2": "font/woff2" })[extname(path)] || "application/octet-stream");
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
  await page.exposeFunction("fixtureInvoke", async (command, args) => {
    calls.push({ command, args });
    if (command === "start_recording_and_play" || command === "start_vlc_and_record") {
      if (fail) throw new Error("VLC+ is not listening on the selected port.");
      phase = "recording";
    }
    if (command === "stop_recording") phase = "idle";
    return { revision: 0, controlRevision: 0, participantId: "UITest", outputDirectory: "C:\\Recordings",
      selectAllStreams: true, keyboardMarkers: false, mouseMarkers: false, externalPages: [],
      externalPagesInitialized: true, externalPagesRevision: 0, viewer: {}, streams: [], markers: [],
      recording: { phase, bytesWritten: 0 }, remote: { phase: "idle", active: false, route: "unknown", approval: "waiting" } };
  });
  await page.addInitScript(() => { window.__TAURI__ = { core: { invoke: (command, args = {}) => window.fixtureInvoke(command, args) } }; });
  await page.goto(`http://127.0.0.1:${server.address().port}/web/index.html`);
  const start = page.getByRole("button", { name: "Record and play VLC+", exact: true });
  const launch = page.getByRole("button", { name: "Start VLC+ and record", exact: true });
  await start.waitFor();
  await launch.click();
  assert.equal(await page.locator("#vlc-dialog").evaluate(el => el.open), true);
  assert.equal(calls.some(call => call.command === "start_vlc_and_record"), false);
  await page.getByLabel("Video path for Start VLC+ and record", { exact: true }).fill("C:\\Videos\\clip with spaces.mp4");
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
  await start.click();
  await page.waitForFunction(() => document.querySelector("#record-phase").textContent === "Recording").catch(async error => {
    console.error(JSON.stringify({ calls, status: await page.locator("#system-status").textContent(), port: await page.locator("#vlc-port").inputValue() }));
    throw error;
  });
  assert.equal(await start.isDisabled(), true);
  const startIndex = calls.findIndex(call => call.command === "start_recording_and_play");
  assert.equal(calls[startIndex].args.port, 42124);
  assert.equal(calls[startIndex - 1].command, "configure_session");
  await page.getByRole("button", { name: "Stop recording", exact: true }).click();
  await start.waitFor({ state: "visible" });
  await page.waitForFunction(() => !document.querySelector("#record-play").disabled);
  fail = true;
  await start.click();
  await page.waitForFunction(() => document.querySelector("#system-status").textContent.includes("not listening"));
  assert.equal(await start.isEnabled(), true);
  async function checkLibrary() {
    const seen = new Map();
    const category = page.getByLabel("Category", { exact: true });
    const names = await category.locator("option").evaluateAll(options => options.map(option => option.value));
    assert.equal(names.length, 8);
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
  for (const [width, height] of [[320, 600], [320, 900], [720, 600], [1280, 600], [1280, 900], [1920, 1080]]) {
    await page.setViewportSize({ width, height });
    await page.evaluate(() => document.fonts.ready);
    assert.equal(await start.evaluate(el => el.scrollWidth <= el.clientWidth + 1 && el.scrollHeight <= el.clientHeight + 1), true);
    assert.equal(await launch.evaluate(el => el.scrollWidth <= el.clientWidth + 1 && el.scrollHeight <= el.clientHeight + 1), true);
    await page.getByRole("button", { name: "VLC+ setup", exact: true }).click();
    assert.equal(await page.locator("#vlc-dialog").evaluate(el => el.scrollWidth <= el.clientWidth + 1 && el.scrollHeight <= el.clientHeight + 1), true);
    await page.getByRole("button", { name: "Hotkeys", exact: true }).click();
    await checkLibrary();
    await page.screenshot({ path: resolve(output, `library-${width}-${height}.png`), fullPage: true });
    await page.getByLabel("Category", { exact: true }).selectOption("Flubber");
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
  await page.addStyleTag({ content: "html {font-size:200% !important} #record-play {line-height:1.5 !important;letter-spacing:.12em !important;word-spacing:.16em !important}" });
  assert.equal(await start.evaluate(el => el.scrollWidth <= el.clientWidth + 1 && el.scrollHeight <= el.clientHeight + 1), true);
  await page.screenshot({ path: resolve(output, "recorder-large-text.png"), fullPage: true });
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
  assert.equal(await page.getByLabel("Video path for Start VLC+ and record", { exact: true }).evaluate(input => input === document.activeElement), true);
  await page.getByRole("button", { name: "Close VLC+ setup" }).click();
  await page.goto(`http://127.0.0.1:${server.address().port}/companion/vlc.html`);
  await page.locator("#vlc-hotkeys dt").first().waitFor();
  assert.equal(await page.locator("#vlc-hotkeys dt").count(), 64);
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
  console.log("PASS: launch/record/play, session ordering, video path/port, active lock, failure recovery, 64 hotkey groups, bounded category/page navigation, Pretext, keyboard focus and 200% text (mock native IPC).");
} finally { await browser.close(); await new Promise(done => server.close(done)); }
