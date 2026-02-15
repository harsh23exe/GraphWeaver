# Example Run

This document shows a minimal example run of scrape-llm and what to expect.

## Prerequisites

- Build the project: `cd build && cmake .. && make scrape-llm`
- Set `GEMINI_API_KEY` (or use `--base-url` with an OpenAI-compatible endpoint and the corresponding API key in that environment).

## Basic example

```bash
export GEMINI_API_KEY=your_key_here

./build/scrape-llm \
  --url "https://example.com" \
  --schema "Extract a list of products with fields: name (string), price_usd (number), rating (number), url (string). Include source_url." \
  --out ./out \
  --format jsonl \
  --max-pages 30 \
  --max-depth 2 \
  --keep-pages 10
```

## What happens

1. **Schema inference**  
   The LLM turns your natural-language schema into a JSON Schema and writes `./out/schema.json`.

2. **Crawl**  
   BFS from the start URL on the same host. Fetched HTML is cached under `./out/cache/pages/`. Rate limiting and robots.txt are applied.

3. **Relevance**  
   For each crawled page a short digest (URL, title, headings, text preview) is sent to the LLM. It returns KEEP or SKIP. Up to `--keep-pages` pages are kept for parsing.

4. **Extraction and parsing**  
   For each kept page, main text and tables are extracted. The LLM returns JSON records that conform to the inferred schema. `source_url` is added if missing.

5. **Validation and repair**  
   Each record is validated against the schema. If invalid, one repair attempt is made via the LLM; if still invalid, the record is dropped and logged.

6. **Deduplication**  
   Records are deduped using the schema’s dedupe key (if any) or a hash of the normalized JSON.

7. **Output**  
   Records are written to `./out/records.jsonl`. Optional `./out/records.json` and `./out/records.csv` (when schema is flat and `--csv` is set). `./out/report.json` and `./out/report.md` summarize the run.

## Dry run

To crawl and run relevance only (no parsing, no LLM record extraction):

```bash
./build/scrape-llm \
  --url "https://example.com" \
  --schema "List of article titles and links." \
  --out ./out \
  --dry-run
```

Output: same crawl cache and report layout, but no `records.jsonl` and no parsing cost.

## Fixtures for unit tests

Unit tests use a mock LLM client (no network):

- **test_schema_infer**: Valid JSON schema response is parsed; null/invalid/missing fields yield the fallback schema.
- **test_main_text**: `main_text()` strips script and style; `extract_content` returns title and main text.
- **test_validator_repair**: Invalid record fails validation; mock repair returns a valid record that passes.

Run: `./build/tests/test_schema_infer`, `./build/tests/test_main_text`, `./build/tests/test_validator_repair`, or `ctest` from the build directory.
