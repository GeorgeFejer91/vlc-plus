import assert from "node:assert/strict";
import { mkdtemp, mkdir, cp, symlink, readFile, writeFile, rm } from "node:fs/promises";
import { resolve, sep } from "node:path";
import { spawnSync } from "node:child_process";
const root = resolve(import.meta.dirname, "..");
const base = resolve(root, "build");
await mkdir(base, { recursive: true });
const fixture = await mkdtemp(resolve(base, "site-safety-"));
assert.ok(fixture.startsWith(base + sep));
try {
  const project = resolve(fixture, "project");
  const external = resolve(fixture, "external");
  await mkdir(resolve(project, "scripts"), { recursive: true });
  await mkdir(resolve(external, "site"), { recursive: true });
  const sentinel = resolve(external, "site/keep.txt");
  await writeFile(sentinel, "outside the fixture checkout");
  await cp(resolve(root, "scripts/build-site.mjs"), resolve(project, "scripts/build-site.mjs"));
  await symlink(external, resolve(project, "build"), "junction");
  const run = spawnSync(process.execPath, [resolve(project, "scripts/build-site.mjs")], { encoding: "utf8" });
  assert.notEqual(run.status, 0);
  assert.match(run.stderr, /Linked build output/);
  assert.equal(await readFile(sentinel, "utf8"), "outside the fixture checkout");
  console.log("PASS: site builder refuses linked build parent and preserves external files.");
} finally { await rm(fixture, { recursive: true, force: true }); }
