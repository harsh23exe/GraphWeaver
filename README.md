# doc-scraper (GraphWeaver)

A production-grade, concurrent web crawler designed to scrape technical documentation websites and convert them into clean Markdown suitable for LLM training and RAG (Retrieval Augmented Generation) systems.

## Features

- **Configuration-driven crawling**: Define sites in `config.yaml` with domain scopes, CSS selectors, and rate limits
- **Concurrent crawling**: Multi-threaded workers fetch and process pages in parallel
- **Smart content extraction**: Auto-detects documentation frameworks (Docusaurus, Sphinx, MkDocs, etc.)
- **State management**: Uses RocksDB for persistence, enabling resume functionality
- **Multiple output formats**: Markdown files, TSV mappings, YAML metadata, and JSONL for RAG pipelines
- **LLM-guided discovery**: Optional AI-powered URL prioritization for efficient crawling
- **Structured data extraction**: Schema-driven JSON extraction from documentation

## Requirements

- CMake 3.20+
- C++20 compiler (Clang 14+ or GCC 11+)
- RocksDB
- OpenSSL

### macOS
```bash
brew install cmake rocksdb openssl
```

### Linux (Ubuntu/Debian)
```bash
sudo apt-get install cmake build-essential librocksdb-dev libssl-dev
```

## Building

```bash
# Clone the repository
git clone https://github.com/yourusername/doc-scraper-cpp.git
cd doc-scraper-cpp

# Build (Debug mode)
./scripts/build.sh

# Build (Release mode)
./scripts/build.sh --release

# Build with sanitizers (for development)
./scripts/build.sh --sanitizers
```

## Usage

### Basic Crawl
```bash
./build/crawler crawl --site python_docs --loglevel info
```

### Resume Interrupted Crawl
```bash
./build/crawler resume --site python_docs
```

### Crawl All Sites
```bash
./build/crawler crawl --all-sites
```

### Validate Configuration
```bash
./build/crawler validate --config config.yaml
```

### List Configured Sites
```bash
./build/crawler list-sites
```

### With LLM-Guided Discovery
```bash
export OPENAI_API_KEY=sk-...
./build/crawler crawl --site python_docs --llm-guide
```

## Configuration

Edit `config.yaml` to configure crawl settings. See the file for detailed options.

### Key Configuration Options

| Option | Description | Default |
|--------|-------------|---------|
| `default_delay_per_host` | Delay between requests to same host | 500ms |
| `num_workers` | Number of concurrent crawl workers | 8 |
| `max_depth` | Maximum crawl depth (0 = unlimited) | 0 |
| `content_selector` | CSS selector for content extraction | "auto" |
| `skip_images` | Skip image downloading | false |

## Project Structure

```
doc-scraper-cpp/
├── include/              # Public headers
│   ├── crawler/          # Main crawler logic
│   ├── fetch/            # HTTP client, rate limiting
│   ├── process/          # Content processing
│   ├── storage/          # RocksDB interface
│   └── ...
├── src/                  # Implementation files
├── tests/                # Unit and integration tests
├── scripts/              # Build and utility scripts
├── config.yaml           # Sample configuration
└── CMakeLists.txt        # Build configuration
```

## Testing

```bash
./scripts/test.sh
```

## License

MIT License

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run tests
5. Submit a pull request