# Prompt Templates

Templates used by the scrape-llm pipeline. Placeholders are denoted by `{{NAME}}`. The implementation must substitute these before sending the request to the LLM.

---

## schema_infer

**Purpose:** Turn the user's natural-language schema into a strict JSON Schema for one record, extraction mode, and optional hints.

**Input placeholders:**
- `{{USER_SCHEMA}}` — The user-provided `--schema` string (natural-language description of desired output).

**Instruction:** The LLM must respond with a single JSON object only, no markdown or explanation. Required fields:
- `json_schema` (object): Valid JSON Schema for one record (e.g. type, properties, required).
- `extraction_mode` (string): Either `"single"` (one record per page) or `"list"` (array of records per page).
- `hints` (object, optional): May include:
  - `item_selector_hint` (string): Optional hint for where to find items in the page.
  - `key_fields` (array of strings): Field names that identify a record.
  - `dedupe_key` (string or array of strings): Field(s) to use for deduplication.

**Template:**

```
You are a schema inference assistant. Given a natural-language description of the data to extract from web pages, output a strict JSON object (and nothing else) with this exact structure:

{
  "json_schema": { ... },   // JSON Schema for ONE record (type: "object", properties, required, etc.)
  "extraction_mode": "single" or "list",
  "hints": {
    "item_selector_hint": "optional CSS or description",
    "key_fields": ["field1", "field2"],
    "dedupe_key": "field_name" or ["f1", "f2"]
  }
}

User description of desired data:
{{USER_SCHEMA}}

Output ONLY valid JSON. No markdown, no code fence, no explanation.
```

---

## relevance_keep

**Purpose:** Decide whether a crawled page is relevant to the extraction goal (KEEP or SKIP), with a short reason.

**Input placeholders:**
- `{{USER_SCHEMA}}` — Same natural-language schema (extraction goal).
- `{{PAGE_DIGEST}}` — Lightweight digest: url, title, h1/h2 headings, first N characters of main text.

**Instruction:** The LLM must respond with a single JSON object only:
- `decision` (string): `"KEEP"` or `"SKIP"`.
- `reason` (string): Brief reason for the decision.

**Template:**

```
You are a relevance filter for a web scraper. Given the extraction goal and a short digest of a page, decide if the page should be kept for extraction (KEEP) or skipped (SKIP).

Extraction goal:
{{USER_SCHEMA}}

Page digest:
{{PAGE_DIGEST}}

Respond with a single JSON object only (no markdown, no explanation):
{ "decision": "KEEP" or "SKIP", "reason": "brief reason" }
```

---

## parse_records

**Purpose:** Extract structured records from page content (main text + tables + metadata) that conform to the given JSON Schema.

**Input placeholders:**
- `{{JSON_SCHEMA}}` — The inferred JSON Schema (as string or pretty-printed).
- `{{EXTRACTION_MODE}}` — `"single"` or `"list"`.
- `{{PAGE_CONTENT}}` — Concatenated main text, optional table data (e.g. TSV-like), and metadata (title, description).
- `{{SOURCE_URL}}` — The page URL (to inject into each record as source_url).

**Instruction:** The LLM must output either a single JSON object (extraction_mode "single") or a JSON array of objects ("list"). Every record must include `source_url` set to `{{SOURCE_URL}}`. No other text or markdown.

**Template:**

```
You are a structured data extractor. Extract records from the following page content so they conform to the given JSON Schema. Every record MUST include the field "source_url" with value exactly: {{SOURCE_URL}}

JSON Schema for one record:
{{JSON_SCHEMA}}

Extraction mode: {{EXTRACTION_MODE}}

Page content:
---
{{PAGE_CONTENT}}
---

If extraction_mode is "single", output a single JSON object. If "list", output a JSON array of objects. Output ONLY valid JSON. No markdown, no code fence, no explanation. Include source_url in every record.
```

---

## repair_to_schema

**Purpose:** Fix an invalid record so it conforms to the JSON Schema, given the validation error.

**Input placeholders:**
- `{{JSON_SCHEMA}}` — The JSON Schema (as string).
- `{{INVALID_RECORD}}` — The record that failed validation (JSON string).
- `{{VALIDATION_ERROR}}` — The error message from the validator.

**Instruction:** The LLM must output only the corrected JSON object (one record). No explanation, no markdown.

**Template:**

```
You are a JSON repair assistant. The following record failed JSON Schema validation. Output the corrected record as a single JSON object only. Do not output anything else (no markdown, no explanation).

JSON Schema:
{{JSON_SCHEMA}}

Invalid record:
{{INVALID_RECORD}}

Validation error:
{{VALIDATION_ERROR}}

Output ONLY the corrected JSON object.
```
