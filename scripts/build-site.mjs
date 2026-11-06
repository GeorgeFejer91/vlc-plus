import { cp, mkdir, readdir, readFile, rm, lstat, realpath, writeFile } from "node:fs/promises";
import { resolve, dirname, sep, relative } from "node:path";
import { createHash } from "node:crypto";
const root = await realpath(resolve(import.meta.dirname, ".."));
const build = resolve(root, "build");
const buildState = await lstat(build).catch(error => { if (error.code !== "ENOENT") throw error; });
if (buildState?.isSymbolicLink()) throw new Error("Linked build output is unsupported");
const output = resolve(root, "build/site");
if (!output.startsWith(root + sep) || dirname(output) !== build) throw new Error("Unexpected site output");
const previous = await lstat(output).catch(error => { if (error.code !== "ENOENT") throw error; });
if (previous?.isSymbolicLink()) throw new Error("Linked site output is unsupported");
await rm(output, { recursive: true, force: true });
await mkdir(output, { recursive: true });
await cp(resolve(root, "site"), output, { recursive: true });
await cp(resolve(root, "recorder/companion"), resolve(output, "remote"), { recursive: true });
await writeFile(resolve(output, ".nojekyll"), "");
const files = {};
async function inventory(folder) {
  for (const entry of await readdir(folder, { withFileTypes: true })) {
    const path = resolve(folder, entry.name);
    if (entry.isSymbolicLink()) throw new Error("Linked website assets are unsupported");
    if (entry.isDirectory()) await inventory(path);
    else files[relative(output, path).replaceAll("\\", "/")] = createHash("sha256").update(await readFile(path)).digest("hex");
  }
}
await inventory(output);
await writeFile(resolve(root, "build/site-receipt.json"), JSON.stringify({ schema: "vlc-plus-site/v1", files }, null, 2) + "\n");
console.log(`Built VLC+ page and self-contained controller: ${Object.keys(files).length} public assets.`);
