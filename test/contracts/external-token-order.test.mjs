import assert from "node:assert/strict";
import { readFile } from "node:fs/promises";
import test from "node:test";

const grammar = JSON.parse(
  await readFile(new URL("../../src/grammar.json", import.meta.url), "utf8"),
);

const expected = [
  "_optional_semi",
  "interpolation_regular_start",
  "interpolation_verbatim_start",
  "interpolation_raw_start",
  "interpolation_start_quote",
  "interpolation_end_quote",
  "interpolation_open_brace",
  "interpolation_close_brace",
  "interpolation_string_content",
  "raw_string_start",
  "raw_string_end",
  "raw_string_content",
  "_html_attribute_tail_text",
  "_start_tag_name",
  "_void_tag_name",
  "_script_start_tag_name",
  "_style_start_tag_name",
  "_end_tag_name",
  "_self_closing_tag_delimiter",
  "_raw_text",
  "_razor_marker",
  "_razor_implicit_end",
];

test("preserves complete external token order", () => {
  assert.deepEqual(
    grammar.externals.map(({ name }) => name),
    expected,
  );
});
