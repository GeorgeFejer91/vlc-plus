import assert from "node:assert/strict";
import { createRequire } from "node:module";
import { createServer } from "node:http";
import { readFile, mkdir, writeFile } from "node:fs/promises";
import { resolve, extname, sep } from "node:path";
const root = resolve(import.meta.dirname, "..");
const require = createRequire(resolve(root, "recorder/package.json"));
const { chromium } = require(process.env.PLAYWRIGHT_MODULE || "playwright");
const output = resolve(root, "build/site");
const evidence = resolve(root, "build/site-ui");
await mkdir(evidence, { recursive: true });
const server = createServer(async (req, res) => {
  try {
    let pathname = decodeURIComponent(new URL(req.url, "http://localhost").pathname);
    if (pathname.endsWith("/")) pathname += "index.html";
    const path = resolve(output, "." + pathname);
    if (!path.startsWith(output + sep)) throw new Error("Outside site");
    res.setHeader("Content-Type", ({ ".html": "text/html; charset=utf-8", ".js": "text/javascript", ".css": "text/css", ".woff2": "font/woff2" })[extname(path)] || "application/octet-stream");
    res.end(await readFile(path));
  } catch { res.writeHead(404); res.end(); }
});
await new Promise(done => server.listen(0, "127.0.0.1", done));
const browser = await chromium.launch({ channel: "msedge", headless: true });
try {
  const page = await browser.newPage();
  const errors = [];
  page.on("pageerror", error => errors.push(error.message));
  await page.goto(`http://127.0.0.1:${server.address().port}/`);
  await page.waitForFunction(() => [...document.querySelectorAll("[data-fit-text]")].every(label => label.dataset.fitState));
  assert.equal(await page.locator("#vlc-hotkeys dt").count(), 18);
  assert.match(await page.locator("#vlc-hotkeys").textContent(), /Alt\+H \/ Alt\+Shift\+H.*9 total/s);
  assert.match(await page.locator("#vlc-hotkeys").textContent(), /Alt\+W.*Alt\+J.*snapshot/s);
  for (const width of [320, 390, 720, 1280, 1920]) {
    await page.setViewportSize({ width, height: 900 });
    await page.evaluate(() => document.fonts.ready);
    assert.equal(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth + 1), true, `Site overflow at ${width}`);
    await page.screenshot({ path: resolve(evidence, `vlc-plus-${width}.png`), fullPage: true });
  }
  await page.setViewportSize({ width: 320, height: 568 });
  await page.evaluate(() => {
    document.documentElement.style.fontSize = "200%";
    for (const element of document.querySelectorAll("main *")) {
      element.style.lineHeight = "1.5";
      element.style.letterSpacing = ".12em";
      element.style.wordSpacing = ".16em";
      if (element.tagName === "P") element.style.marginBottom = "2em";
    }
  });
  assert.equal(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth + 1), true, "Enlarged text must reflow immediately");
  await page.screenshot({ path: resolve(evidence, "vlc-plus-large-text.png"), fullPage: true });
  await page.getByRole("link", { name: "Open remote LSL controller", exact: true }).click();
  await page.getByRole("heading", { name: "Join the recorder", exact: true }).waitFor();
  assert.equal(await page.evaluate(() => window.__TAURI__ === undefined), true);
  assert.equal(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth + 1), true, "Controller at phone width");
  assert.deepEqual(errors, []);
  await writeFile(resolve(evidence, "receipt.json"), JSON.stringify({ widths: [320,390,720,1280,1920], shortcuts: 18, textResize: "200% plus text spacing", controllerLoaded: true, browserNativePrivileges: false, pageErrors: errors }, null, 2));
  console.log("PASS: VLC+ page, actual Pretext, 18 hotkeys, 320–1920 px, enlarged text and bundled controller without native privileges.");
} finally { await browser.close(); await new Promise(done => server.close(done)); }
