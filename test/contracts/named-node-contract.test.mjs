import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import test from "node:test";

const nodeTypes = JSON.parse(
  await readFile(new URL("../../src/node-types.json", import.meta.url), "utf8"),
);

const required = [
  "element",
  "razor_block",
  "razor_explicit_expression",
  "razor_implicit_expression",
  "razor_page_directive",
  "razor_using_directive",
  "razor_namespace_directive",
  "razor_rendermode_directive",
];

test("preserves Julie-facing named nodes", () => {
  for (const kind of required) {
    assert.ok(nodeTypes.some((node) => node.type === kind && node.named), kind);
  }
});
