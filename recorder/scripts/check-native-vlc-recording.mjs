import assert from "node:assert/strict";
import { createRequire } from "node:module";
import { execFileSync, spawn } from "node:child_process";
import { createServer, createConnection } from "node:net";
import { readFile, writeFile, mkdir, stat, unlink } from "node:fs/promises";
import { resolve, join } from "node:path";
import { setTimeout as delay } from "node:timers/promises";

const require = createRequire(import.meta.url);
const { chromium } = require(process.env.PLAYWRIGHT_MODULE || "playwright");
const root = resolve(import.meta.dirname, "..");
const output = resolve(root, ".for-ai-local/native-vlc");
await mkdir(output, { recursive: true });
const inputDone = join(output, "input-done");
await unlink(inputDone).catch(error => { if (error.code !== "ENOENT") throw error; });
const playingInputDone = join(output, "playing-input-done");
await unlink(playingInputDone).catch(error => { if (error.code !== "ENOENT") throw error; });
const vlcExe = join(process.env.LOCALAPPDATA, "Programs/VLCPlus/vlc.exe");
const video = resolve(process.env.VLC_TEST_MEDIA || "../examples/media/dictator-3-study.mp4");
const recorderExe = process.env.RECORDER_TEST_EXE || resolve(root, "src-tauri/target/release/remote-lsl-recorder.exe");
for (const file of [vlcExe, video, recorderExe]) assert.equal((await stat(file)).isFile(), true);
const config = join(process.env.APPDATA, "dev.georgefejer.remotelslrecorder/workspace.json");
const running = execFileSync("powershell.exe", ["-NoProfile", "-Command",
  "@(Get-Process -Name vlc,remote-lsl-recorder,LabRecorderCLI -ErrorAction SilentlyContinue).Id -join ','"],
{ encoding: "utf8", windowsHide: true, timeout: 10000 }).trim();
assert.equal(running, "", "Close existing player/Recorder processes before native qualification");
const backupPath = join(output, "workspace-backup.json");
const priorBackup = await readFile(backupPath, "utf8").then(JSON.parse).catch(error => {
  if (error.code !== "ENOENT") throw error;
  return null;
});
if (priorBackup?.pending) {
  assert.equal(priorBackup.path, config, "Pending workspace backup belongs to another profile");
  if (priorBackup.existed) await writeFile(config, Buffer.from(priorBackup.content, "base64"));
  else await unlink(config).catch(error => { if (error.code !== "ENOENT") throw error; });
}
const previous = await readFile(config).catch(error => { if (error.code !== "ENOENT") throw error; return null; });
const workspaceBackup = { path: config, pending: true,
  existed: previous !== null, content: previous?.toString("base64") ?? null };
await writeFile(backupPath, JSON.stringify(workspaceBackup, null, 2));
async function freePort() {
  const server = createServer();
  await new Promise(done => server.listen(0, "127.0.0.1", done));
  const port = server.address().port;
  await new Promise(done => server.close(done));
  return port;
}
const rcPort = await freePort();
const debugPort = await freePort();
let vlcPid, affectSource, markerSource;
function ownsListener() {
  if (!Number.isSafeInteger(vlcPid) || vlcPid <= 0) return false;
  try {
    return execFileSync("powershell.exe", ["-NoProfile", "-Command",
      `@(Get-NetTCPConnection -LocalPort ${rcPort} -State Listen -ErrorAction SilentlyContinue).OwningProcess -join ','`],
    { encoding: "utf8", windowsHide: true, timeout: 10000 }).trim() === String(vlcPid);
  } catch { return false; }
}
const recorder = spawn(recorderExe, [], { windowsHide: false, stdio: "ignore",
  env: { ...process.env, WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS: `--remote-debugging-port=${debugPort} --remote-debugging-address=127.0.0.1` } });
await writeFile(join(output, "processes.json"), JSON.stringify({ recorderPid: recorder.pid, rcPort, debugPort, vlcExe, video }, null, 2));
async function rc(command, acknowledgement = command) {
  assert.ok(ownsListener(), "RC commands require the exact launched player's current listener");
  return new Promise((done, reject) => {
    const socket = createConnection({ host: "127.0.0.1", port: rcPort });
    const timer = setTimeout(() => { socket.destroy(); reject(new Error(`RC timeout: ${command}`)); }, 5000);
    let response = "";
    socket.on("error", error => { clearTimeout(timer); reject(error); });
    socket.on("connect", () => socket.write(command + "\n"));
    socket.on("data", chunk => {
      response += chunk.toString();
      if (response.includes(`${acknowledgement}: returned 0 `)) { clearTimeout(timer); socket.destroy(); done(response); }
    });
  });
}
let browser;
let stopped = false;
async function waitForMarker(page, source, suffix) {
  const deadline = Date.now() + 20000;
  while (Date.now() < deadline) {
    const state = await page.evaluate(() => window.__TAURI__.core.invoke("get_snapshot"));
    if (state.markers.some(marker => marker.value.endsWith(suffix)
      && state.streams.some(stream => stream.id === marker.streamId && stream.sourceId === source))) return;
    await delay(100);
  }
  throw new Error(`Exact player marker ${suffix} was not observed`);
}
try {
  const deadline = Date.now() + 30000;
  while (Date.now() < deadline) {
    try { if ((await fetch(`http://127.0.0.1:${debugPort}/json/version`)).ok) break; } catch {}
    await delay(200);
  }
  browser = await chromium.connectOverCDP(`http://127.0.0.1:${debugPort}`);
  let page;
  const pageDeadline = Date.now() + 20000;
  while (Date.now() < pageDeadline && !page) {
    page = browser.contexts().flatMap(context => context.pages()).find(page => /tauri|localhost/.test(page.url()));
    if (!page) await delay(100);
  }
  if (!page) console.error(browser.contexts().flatMap(context => context.pages()).map(page => page.url()));
  assert.ok(page, "Recorder's packaged WebView was not found");
  page.setDefaultTimeout(20000);
  await page.getByLabel("Participant ID", { exact: true }).fill("VLC_UI_TEST");
  await page.getByLabel("Recording folder", { exact: true }).fill(output);
  await page.getByRole("button", { name: "Apply session", exact: true }).click();
  const recordAll = page.getByRole("checkbox", { name: "Record all available streams", exact: true });
  // Checking then clearing also clears individually restored selections when
  // the saved workspace already had record-all off. Only the synthetic
  // player's automatically selected pair may enter this test recording.
  await recordAll.check();
  await recordAll.uncheck();
  await page.getByRole("button", { name: "VLC+ setup", exact: true }).click();
  await page.getByLabel("VLC+ control port").fill(String(rcPort));
  await page.getByLabel("Video path for Start VLC+ and record", { exact: true }).fill(video);
  await page.getByRole("button", { name: "Close VLC+ setup" }).click();
  await page.getByRole("button", { name: "Start VLC+ and record", exact: true }).click();
  await page.waitForFunction(() => document.querySelector("#record-phase").textContent === "Recording"
    && document.querySelector("#system-status").textContent === "Ready").catch(async error => {
      console.error(JSON.stringify({ status: await page.locator("#system-status").textContent(),
        snapshot: await page.evaluate(() => window.__TAURI__.core.invoke("get_snapshot")) }));
      throw error;
    });
  const before = await page.evaluate(() => window.__TAURI__.core.invoke("get_snapshot"));
  const launched = before.streams.find(stream => stream.selected && /^vlc-flubber-\d+-[\w-]+-affect$/.test(stream.sourceId));
  assert.ok(launched, "The launched player's affect stream must be automatically selected");
  vlcPid = Number(launched.sourceId.match(/^vlc-flubber-(\d+)-/)[1]);
  affectSource = launched.sourceId;
  markerSource = affectSource.replace(/-affect$/, "-markers");
  assert.ok(before.streams.some(stream => stream.selected && stream.sourceId === markerSource));
  await waitForMarker(page, markerSource, "_Start");
  // Start away from time zero so unintended backward seeks cannot be hidden
  // by VLC's lower bound, and let the audio output become active.
  await delay(5000);
  const volumeBefore = await rc("volume");
  await rc("pause");
  const transportBefore = await rc("get_time\nstatus", "status");
  await writeFile(join(output, "processes.json"), JSON.stringify({ vlcPid, affectSource, markerSource, recorderPid: recorder.pid, rcPort, debugPort, vlcExe, video }, null, 2));
  console.log(`READY_FOR_KEYS VLC_PID=${vlcPid} RECORDER_PID=${recorder.pid}`);
  const inputDeadline = Date.now() + 120000;
  while (Date.now() < inputDeadline) {
    if (await stat(inputDone).then(() => true, () => false)) break;
    await delay(200);
  }
  assert.equal(await stat(inputDone).then(() => true, () => false), true, "Native keyboard phase was not completed");
  await delay(1500);
  const after = await page.evaluate(() => window.__TAURI__.core.invoke("get_snapshot"));
  const affect = after.streams.find(stream => stream.sourceId === affectSource);
  assert.ok(affect?.preview.some(sample => sample.values[0] !== 0 && sample.values[1] !== 0), "Both rating axes must change in the real LSL preview");
  assert.ok(after.markers.some(marker => /Start|Resume/.test(marker.value)), "Playback marker missing");
  const transportAfter = await rc("get_time\nstatus", "status");
  const number = text => text.split(/\r?\n/).map(line => line.replace(/^> /, "").trim()).find(line => /^\d+$/.test(line));
  assert.ok(number(transportBefore) !== undefined, "Transport assertion requires actual media time");
  assert.equal(number(transportAfter), number(transportBefore), "Affect arrows must not seek the paused video");
  await rc("pause");
  await delay(1200);
  // VLC's RC interface refuses volume queries while paused. Query on either
  // side of the paused key phase, while audio is active, instead.
  const volumeAfter = await rc("volume");
  await writeFile(join(output, "transport.json"), JSON.stringify({ transportBefore, transportAfter, volumeBefore, volumeAfter }, null, 2));
  const volume = text => text.match(/audio volume: (\d+)/)?.[1];
  assert.ok(volume(volumeBefore) !== undefined, "Volume assertion requires an actual VLC volume value");
  assert.equal(volume(volumeAfter), volume(volumeBefore), "Affect arrows must preserve VLC volume");
  console.log(`READY_FOR_PLAYING_KEYS VLC_PID=${vlcPid}: emulate both axes again during playback so CSV and XDF capture the trajectory.`);
  const playingDeadline = Date.now() + 120000;
  while (Date.now() < playingDeadline) {
    if (await stat(playingInputDone).then(() => true, () => false)) break;
    await delay(200);
  }
  assert.equal(await stat(playingInputDone).then(() => true, () => false), true, "Playing keyboard phase was not completed");
  const playingAfter = await page.evaluate(() => window.__TAURI__.core.invoke("get_snapshot"));
  assert.ok((await rc("status")).includes("play state: 3"), "Video must remain playing during emulated ratings");
  await rc("stop");
  await waitForMarker(page, markerSource, "_Stop");
  await page.screenshot({ path: join(output, "native-recording.png") });
  await page.getByRole("button", { name: "Stop recording", exact: true }).click();
  await page.waitForFunction(() => document.querySelector("#record-phase").textContent === "Complete");
  const snapshot = await page.evaluate(() => window.__TAURI__.core.invoke("get_snapshot"));
  stopped = true;
  await writeFile(join(output, "receipt.json"), JSON.stringify({ before, after, playingAfter, stopped: snapshot, transportBefore, transportAfter, volumeBefore, volumeAfter, vlcPid, affectSource, markerSource, physicalInput: false }, null, 2));
  console.log(`XDF_READY ${snapshot.recording.outputFile}`);
  console.log("CLOSE_RECORDER: close the owned Recorder window to finish the native test.");
  await new Promise((done, reject) => {
    const timer = setTimeout(() => reject(new Error("Recorder did not close gracefully")), 120000);
    recorder.once("exit", () => { clearTimeout(timer); done(); });
  });
  console.log("PASS: native WebView button -> Rust Recorder -> VLC playback -> changing LSL ratings -> clean XDF stop.");
} finally {
  if (browser && !stopped) {
    for (const page of browser.contexts().flatMap(context => context.pages())) {
      await page.evaluate(async () => {
        const snapshot = await window.__TAURI__.core.invoke("get_snapshot");
        if (snapshot.recording.phase === "recording") await window.__TAURI__.core.invoke("stop_recording");
      }).catch(error => console.error(`Recording cleanup needs attention: ${error.message}`));
    }
  }
  if (ownsListener()) await new Promise(done => {
    const socket = createConnection({ host: "127.0.0.1", port: rcPort });
    const timer = setTimeout(() => { socket.destroy(); done(); }, 5000);
    let response = "";
    const finish = () => { clearTimeout(timer); socket.destroy(); done(); };
    socket.on("error", finish);
    socket.on("end", finish);
    socket.on("connect", () => socket.write("quit\n"));
    socket.on("data", chunk => {
      response += chunk.toString();
      if (/quit: returned 0 /.test(response)) finish();
    });
  });
  await browser?.close();
  const exitDeadline = Date.now() + 5000;
  while (recorder.exitCode === null && Date.now() < exitDeadline) await delay(100);
  if (recorder.exitCode !== null) {
    if (previous) await writeFile(config, previous);
    else await unlink(config).catch(error => { if (error.code !== "ENOENT") throw error; });
    workspaceBackup.pending = false;
    await writeFile(backupPath, JSON.stringify(workspaceBackup, null, 2));
  } else console.error("Owned Recorder remains open; close it before restoring the saved workspace.");
}
