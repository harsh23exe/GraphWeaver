# Assumptions

This document records default choices and assumptions for the scrape-llm CLI. The tool avoids interactive prompts; when something is ambiguous or missing, it uses these defaults and logs when relevant.

## Environment and API

- **API key:** Read from `GEMINI_API_KEY` only. No `OPENAI_API_KEY` or other env vars are used for the LLM. If the key is missing at run time, the tool exits with a clear error.
- **Base URL:** Default is the project-configured OpenAI-compatible endpoint (e.g. Gemini REST). Override with `--base-url` for a different provider.
- **Model:** Default model string is set in code (e.g. `gpt-4.1-mini` or Gemini equivalent). Any string is accepted via `--model`; no validation of model name is performed.

## Crawl scope

- **Same host:** Crawl is limited to the same host (scheme + authority) as the start URL. Links to other hosts are not followed.
- **Protocols:** Only `http` and `https` are allowed. `file://`, `ftp://`, and other schemes are rejected.
- **Depth:** BFS depth is measured as the number of link hops from the start URL. The start URL is depth 0.
- **Normalization:** URLs are normalized (lowercase scheme/host, no fragment, default port omitted, sorted query) before deduplication and cache keying.
- **Content type:** Only responses that look like HTML (by Content-Type or sniffing) are parsed. Other content types are skipped and not cached as parseable pages.

## Safety

- **SSRF:** By default, requests to localhost, loopback, and private IP ranges (RFC 1918, etc.) are blocked. Use `--allow-private-network` to permit them in trusted environments.
- **Robots:** robots.txt is fetched and respected per host unless `--respect-robots false`.
- **Rate limit:** Applied per host. Default 1.0 request per second; no global rate limit.
- **Timeouts:** HTTP client uses a fixed timeout (e.g. 30s) for connect and read. No separate timeout for “total crawl” unless added in future.

## Schema and extraction

- **Schema inference:** The LLM is asked to return a single JSON object with `json_schema`, `extraction_mode`, and optional `hints`. If the LLM response cannot be parsed or is missing required fields, a fallback schema `{"source_url": "string", "content": "string"}` is used and a warning is logged.
- **Extraction mode:** Only `"single"` and `"list"` are supported. Any other value is treated as `"list"`.
- **source_url:** Every emitted record is required to include `source_url`. If the LLM omits it, the pipeline injects it from the page URL.
- **Deduplication:** If the inferred schema includes `dedupe_key` (or equivalent in hints), those fields are used for dedupe. Otherwise, a hash of the normalized JSON (stable key order) is used.

## Validation and repair

- **JSON Schema:** Validation is strict (types must match). No automatic type coercion before validation.
- **Repair:** At most one repair attempt per invalid record. The LLM is given the error, the record, and the schema and asked to output corrected JSON only. If the result still fails validation, the record is dropped and details are logged.
- **Schema format:** Inferred and fallback schemas are expected to be valid JSON Schema (draft-07 or compatible). The validator does not implement the full draft; it supports the subset used by the inferred schema (e.g. type, properties, required).

## Output and report

- **Output directory:** Must exist or be creatable. No overwrite confirmation; files under `--out` may be overwritten.
- **Cache:** Fetched HTML is stored under `out/cache/pages/` keyed by SHA-256 of the normalized URL. Cache is used when the same URL is seen again in the same run; no cross-run cache reuse is guaranteed unless the implementation explicitly does so.
- **Report:** Report fields (e.g. pages_crawled, pages_kept, records_emitted, validation_failures, tokens_estimate, timings) are best-effort. Token counts may be estimates when the API does not return usage.

## Cost and limits

- **keep-pages:** Caps how many pages are sent to the LLM for parsing. This is the main cost control; schema inference and relevance routing also consume tokens but are not capped by keep-pages.
- **max_tokens:** A per-request cap is applied to LLM calls to avoid runaway output. The exact value is set in code and may be overridable in future.
- **Retries:** 429 and 5xx responses trigger exponential backoff and a limited number of retries (e.g. 3). No retry for 4xx (other than 429).

## Platform and build

- **Platforms:** macOS and Linux are supported. Windows is not in scope.
- **Compiler:** C++20. CMake 3.20+.
- **HTTP for crawl:** Uses the existing project HTTP client (e.g. cpp-httplib) for fetching pages. libcurl is used for the LLM API only.
