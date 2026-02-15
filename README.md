# GraphWeaver

LLM-powered web scraper CLI: give it a starting URL and a natural-language description of the data you want. It crawls relevant pages, uses an LLM to decide what to keep and how to parse content, and outputs validated structured data (JSONL by default, with optional JSON/CSV).

**Language:** C++20 · **Build:** CMake · **Platforms:** macOS, Linux

---

## What it does

1. **Takes** a starting URL and a natural-language schema (e.g. "list of products with name, price_usd, rating, url").
2. **Crawls** the site with BFS, respecting robots.txt and rate limits, and caches pages to disk.
3. **Uses an LLM** to infer a strict JSON Schema, decide which pages are relevant, and parse page content into records.
4. **Validates** records against the schema, with one repair attempt for invalid output.
5. **Outputs** validated JSONL (default), plus optional JSON/CSV, plus a run report (stats, errors, cost estimates).

Defaults are deterministic and safe: rate limiting, robots respect, SSRF protection (no private networks or localhost unless explicitly allowed), and strict JSON validation.

---

## Requirements

- CMake 3.20+
- C++20 compiler (Clang 14+ or GCC 11+)
- libcurl (HTTP for LLM API)
- OpenSSL
- Gumbo (HTML parsing) or libxml2 as fallback

**macOS:**
```bash
brew install cmake openssl gumbo-parser
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get install cmake build-essential libcurl4-openssl-dev libssl-dev libgumbo-dev
```

---

## Build

```bash
git clone <repo-url>
cd GraphWeaver
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

Or use the script: `./scripts/build.sh --release`

The CLI binary is `scrape-llm`.

---

## Usage

### CLI contract

```text
scrape-llm \
  --url "https://example.com" \
  --schema "<natural-language description of desired structured output>" \
  --out ./out \
  [--format jsonl|json|csv] \
  [--max-pages N] \
  [--max-depth N] \
  [--keep-pages N] \
  [--rate-limit R] \
  [--respect-robots true|false] \
  [--allow-private-network false|true] \
  [--model MODEL] \
  [--base-url URL] \
  [--csv] \
  [--dry-run]
```

| Option | Description | Default |
|--------|-------------|---------|
| `--url` | Starting URL to crawl | (required) |
| `--schema` | Natural-language description of the data format to extract | (required) |
| `--out` | Output directory (schema, cache, records, report) | (required) |
| `--format` | Output format: jsonl, json, or csv | jsonl |
| `--max-pages` | Maximum number of pages to crawl | 30 |
| `--max-depth` | Maximum BFS depth from start URL | 2 |
| `--keep-pages` | Max pages sent to LLM for parsing (cost control) | 10 |
| `--rate-limit` | Requests per second per host | 1.0 |
| `--respect-robots` | Honor robots.txt | true |
| `--allow-private-network` | Allow localhost and private IP ranges | false |
| `--model` | LLM model name | (configurable) |
| `--base-url` | LLM API base URL (OpenAI-compatible; e.g. Gemini) | (configurable) |
| `--csv` | Also emit CSV when schema is flat | off |
| `--dry-run` | Crawl and select only; no parsing or output | off |

---

## Environment

- **GEMINI_API_KEY** — API key for the LLM (e.g. Gemini REST). Read at runtime.

LLM API is OpenAI-compatible; `--base-url` can point at Gemini or another provider.

---

## Examples

**Products from a store (JSONL):**
```bash
export GEMINI_API_KEY=your_key
scrape-llm \
  --url "https://example.com/store" \
  --schema "Extract a list of products with fields: name (string), price_usd (number), rating (number), url (string). Include source_url." \
  --out ./out \
  --max-pages 30 \
  --keep-pages 10
```

**Dry run (crawl + relevance only, no parsing):**
```bash
scrape-llm --url "https://example.com" --schema "List of article titles and links." --out ./out --dry-run
```

---

## Output layout

Under `--out`:

| Path | Description |
|------|-------------|
| `schema.json` | Inferred JSON Schema for one record + extraction_mode + hints |
| `records.jsonl` | One JSON object per line (default) |
| `records.json` | Optional; single JSON array |
| `records.csv` | Optional; when schema is flat and `--csv` used |
| `report.json` | Machine-readable run report |
| `report.md` | Human-readable run report |
| `cache/pages/<sha256(url)>.html` | Cached HTML |
| `cache/meta.json` | Cache metadata |

---

## Safety and robustness

- **SSRF:** Only `http`/`https` allowed. Localhost and private IP ranges are blocked unless `--allow-private-network` is set.
- **Rate limiting:** Configurable per-host limit (default 1.0 req/s).
- **Robots:** robots.txt is respected unless `--respect-robots false`.
- **Timeouts:** HTTP requests use timeouts to avoid hangs.
- **Validation:** All records are validated against the inferred JSON Schema; one repair attempt, then drop and log on failure.

---

## Project structure

```text
GraphWeaver/
├── CMakeLists.txt
├── README.md
├── docs/
│   ├── assumptions.md
│   ├── example_run.md
│   └── prompts.md
├── include/
│   ├── parse/          # html_parser, normalizer
│   ├── fetch/          # rate_limiter, robots
│   ├── utils/          # hash
│   └── scrape_llm/     # CLI, pipeline, LLM, extractor, validator, output, report
├── src/
├── tests/              # test_schema_infer, test_main_text, test_validator_repair
└── scripts/            # build.sh, test.sh
```

---

## Testing

Unit tests use a mock LLM client (no network):

- **test_schema_infer**: Valid JSON schema response is parsed; null/invalid/missing fields yield the fallback schema.
- **test_main_text**: `main_text()` strips script and style; `extract_content` returns title and main text.
- **test_validator_repair**: Invalid record fails validation; mock repair returns a valid record that passes.

Run: `./build/tests/test_schema_infer`, `./build/tests/test_main_text`, `./build/tests/test_validator_repair`, or `ctest` from the build directory. Or: `./scripts/test.sh`.

---

## License

MIT (or as specified in the repository).
