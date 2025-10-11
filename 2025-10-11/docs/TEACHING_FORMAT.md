# Melvin Teaching Format Specification

**`.tch` files - Human-writable structured knowledge for Melvin**

---

## Overview

The Melvin Teaching Format (`.tch`) is a simple, text-based markup language for teaching Melvin facts, associations, and rules. It's designed to be:

- **Human-writable** - Easy to create and edit
- **Diff-friendly** - Works well with Git
- **Robust** - Tolerant to formatting variations
- **Verifiable** - Built-in testing with QUERY/EXPECT blocks

---

## Quick Example

```
#FACT
cats ARE mammals

#FACT
mammals PRODUCE milk

#TEST CatsMilk
#QUERY
What do cats produce?
#EXPECT
milk
```

---

## File Structure

- UTF-8 text file
- Line-based blocks starting with `#TAG`
- Empty lines ignored
- Comments: `# ` (hash + space) at line start
- Blocks extend until next tag or EOF

---

## Tags Reference

### `#FACT` - Canonical Statement

Encodes a definite relationship: **SUBJECT RELATION OBJECT**

```
#FACT
cats ARE mammals

#FACT
fire PRODUCES heat

#FACT
water IS_A liquid
```

**Relations supported:**
- `ARE` / `IS_A` - Category membership
- `PRODUCES` - Creation/output
- `CAUSES` - Causation
- `HAS` / `HAVE` - Possession/property
- `REQUIRES` - Dependency
- `PART_OF` - Composition
- `USES` - Tool/method
- `MEANS` - Definition
- `ENABLES` - Capability

**Graph encoding:**
- Creates directed edge: `cats --ARE--> mammals`
- Weight: `meta.confidence` (default 1.0)
- Stored as: node(cats) → edge(ARE) → node(mammals)

---

### `#ASSOCIATION` - Loose Relationship

Symmetric or asymmetric connection:

```
#ASSOCIATION
fire PRODUCES heat

#ASSOCIATION
tea ↔ kettle
```

**Bidirectional markers:**
- `↔` or `<->` creates edges in both directions
- Without marker: single direction (like FACT but lower default weight)

**Weight:** 0.8× confidence (slightly weaker than FACT)

---

### `#RULE` - Schema Pattern

Template for inference:

```
#RULE
IF X ARE mammals THEN X PRODUCE milk

#RULE
IF X IS_A liquid THEN X HAS volume
```

**Syntax:** `IF pattern THEN implication`

**Encoding:** Stored as metadata for now (future: schema nodes)

---

### `#QUERY` - Test Question

Question to ask Melvin:

```
#QUERY
What do cats produce?
```

Must be followed by `#EXPECT` for verification.

---

### `#EXPECT` - Expected Answer

Acceptable answers (comma-separated for multiple):

```
#EXPECT
milk

#EXPECT
hydrogen, oxygen, H2O
```

**Matching:**
- Exact match (after normalization)
- Fuzzy match (Jaro-Winkler ≥ 0.85)
- Substring match (for longer answers)

---

### `#TEST` - Named Test Case

Combines QUERY + EXPECT with a name:

```
#TEST CatsMilk
#QUERY
What do cats produce?
#EXPECT
milk
```

**Benefits:**
- Named tests for reporting
- Clearer organization
- Easier to reference in logs

---

### `#EXPLAIN` - Rationale

Freeform explanation (stored as metadata):

```
#EXPLAIN
Mammals are defined by milk production for offspring.
This is a defining characteristic of the class.

#FACT
mammals PRODUCE milk
```

---

### `#SOURCE` - Provenance

Track where knowledge came from:

```
#SOURCE
Wikipedia: Mammal article, 2025-10-11

#FACT
mammals ARE vertebrates
```

Stored in `edge.meta.source` for traceability.

---

### `#WEIGHT` - Metadata Hint

Confidence and temporal info:

```
#WEIGHT
{confidence:0.9, temporal:"2025-10-11"}

#FACT
electrons HAVE negative_charge
```

**Format:** JSON-ish, tolerates spaces

**Fields:**
- `confidence` - Float 0.0-1.0 (default 1.0)
- `temporal` - Timestamp string
- `source` - Short source note

---

## Syntax Details

### Relation Parsing

General form: `SUBJECT RELATION OBJECT`

**Rules:**
- First token = SUBJECT
- Last token = OBJECT
- Middle tokens = RELATION (can be multi-word)

**Examples:**
```
cats ARE mammals            → subj:cats, rel:ARE, obj:mammals
fire PRODUCES heat          → subj:fire, rel:PRODUCES, obj:heat
birds PART_OF ecosystem     → subj:birds, rel:PART_OF, obj:ecosystem
```

### Arrow Notation

Optional for clarity:

```
cats ARE→ mammals
fire PRODUCES→ heat
X ↔ Y  (bidirectional)
```

Arrows are normalized away during parsing.

### Normalization

Before matching:
1. Lowercase
2. Trim whitespace
3. Remove punctuation (except in EXPLAIN/SOURCE)
4. Collapse multiple spaces

---

## Complete Example

File: `lessons/03_weather.tch`

```
# Lesson 03: Weather and Climate

#SOURCE
Elementary Science Textbook, Chapter 8

#FACT
clouds PRODUCE rain

#FACT
sun CAUSES evaporation

#ASSOCIATION
evaporation LEADS_TO clouds

#RULE
IF sun HEATS water THEN water BECOMES vapor

#EXPLAIN
The water cycle demonstrates energy transfer and phase changes.

#TEST RainSource
#QUERY
What produces rain?
#EXPECT
clouds

#TEST SunEffect
#QUERY
What does the sun cause?
#EXPECT
evaporation

#WEIGHT
{confidence:0.95, temporal:"2025-10-11"}

#FACT
water_vapor IS_A gas

#TEST VaporType
#QUERY
What is water vapor?
#EXPECT
gas
```

---

## Parsing Behavior

### Tolerant

- Extra whitespace ignored
- Case insensitive for tags
- Missing punctuation OK
- Empty lines skip

### Strict

- Unknown tags → warning (not error)
- QUERY without EXPECT → warning
- Malformed relation → error (skips block)
- Invalid #WEIGHT JSON → warning (uses defaults)

### Error Handling

Errors reported with file:line context:
```
Error: Line 42: Failed to parse FACT relation
Warning: Line 15: QUERY without EXPECT
```

---

## Verification Logic

### Query Answering

Simple graph traversal:
1. Find nodes mentioned in question
2. Traverse edges from those nodes
3. Score destination nodes by edge weight
4. Return highest-scoring node's text

### Answer Matching

Given actual answer and expected answers:
1. Normalize both (lowercase, punctuation removal)
2. Check exact match
3. Check fuzzy match (similarity ≥ threshold)
4. Check substring (actual contains expected)

**Pass if any expected answer matches.**

### Success Criteria

- Test passes: Any expected answer matches actual
- File passes: ≥50% tests pass
- Curriculum passes: All files pass (or --continue mode)

---

## Metrics Tracked

### Ingestion Metrics

- `nodes_added` - New concepts learned
- `edges_added` - New relationships created
- `edges_updated` - Existing edges strengthened
- `duplicates_skipped` - Redundant teachings
- `rules_added` - Schema patterns stored

### Verification Metrics

- `tests_total` - Number of tests run
- `tests_passed` - Tests that matched expected
- `pass_rate` - Passed / Total
- `avg_confidence` - Mean confidence (placeholder)

### Derived Metrics

- `retention_score` = pass_rate
- `growth_efficiency` = edges_added / blocks_processed

### Teaching Session

Complete record:
- Filepath, timestamp, duration
- Graph state before/after (nodes, edges)
- All ingestion and verification metrics
- Logged to `logs/teaching/YYYY-MM-DD.jsonl`

---

## Usage Examples

### Teach Single File

```bash
make teach FILE=lessons/00_basics.tch
```

Output:
```
[INGEST] FACT: cats ARE mammals
[INGEST] FACT: mammals PRODUCE milk
...
✅ Passed: cats ARE mammals | Expected: milk | Got: milk
...
🎉 Teaching session complete!
```

### Teach Curriculum (Directory)

```bash
make teach_dir DIR=lessons/
```

Processes files in lexical order:
- 00_basics.tch
- 01_animals.tch
- 02_science.tch

Stops on first failure (unless `--continue`).

### Verify Without Snapshot

```bash
make teach_verify FILE=lessons/00_basics.tch
```

Runs tests only, doesn't save snapshot.

### Reset Brain

```bash
make teach_reset
```

Deletes `melvin_brain.bin` - Melvin starts fresh.

---

## Idempotency

### Deduplication Strategy

**Edge Key:** `normalize(subj) + "|" + normalize(rel) + "|" + normalize(obj)`

**Behavior:**
- First teaching: Creates edge, `edges_added++`
- Re-teaching same lesson: Updates weight, `duplicates_skipped++`
- Different lesson, same fact: Updates weight, `edges_updated++`

**Result:** Re-teaching doesn't inflate counts incorrectly.

---

## File Organization

### Recommended Structure

```
lessons/
├── 00_basics.tch       # Foundational concepts
├── 01_animals.tch      # Domain knowledge
├── 02_science.tch      # Scientific facts
├── 03_weather.tch      # Specialized domain
└── README.md           # Curriculum guide
```

### Naming Convention

- `##_topic.tch` - Number prefix for ordering
- Lowercase topic name
- `.tch` extension

### Lesson Design

**Progressive complexity:**
1. Start with basic facts (IS_A, ARE)
2. Add relationships (PRODUCES, CAUSES)
3. Introduce rules (IF...THEN...)
4. Build domain knowledge layer by layer

**Each lesson should:**
- Build on previous lessons
- Include verification tests (≥5)
- Have clear learning objectives
- Be re-teachable (idempotent)

---

## Best Practices

### DO ✅

- Use clear, simple language
- One fact per #FACT block
- Include tests for every concept
- Provide multiple acceptable answers when appropriate
- Use #SOURCE for controversial or temporal facts
- Group related facts together
- Number lessons for ordering

### DON'T ❌

- Write ambiguous relations
- Skip verification tests
- Mix multiple relations in one FACT
- Use complex nested logic (save for #RULE)
- Create circular definitions without care
- Forget to test edge cases

---

## Advanced Features

### Multi-Word Relations

```
#FACT
birds PART_OF ecosystem

#FACT
plants CONTRIBUTE_TO oxygen_cycle
```

### Bidirectional Associations

```
#ASSOCIATION
parent ↔ child

#ASSOCIATION
teacher ↔ student
```

### Confidence Weighting

```
#WEIGHT
{confidence:0.6}

#FACT
dark_matter EXISTS_IN universe
```

Lower confidence for uncertain facts.

### Temporal Facts

```
#WEIGHT
{confidence:1.0, temporal:"2025-10-11"}

#FACT
president IS Biden
```

Mark facts that may change over time.

---

## Verification Examples

### Simple Test

```
#FACT
2 PLUS 2 EQUALS 4

#TEST TwoPlusTwo
#QUERY
What is 2 plus 2?
#EXPECT
4, four
```

### Complex Test

```
#FACT
photosynthesis CONVERTS light_energy

#FACT
photosynthesis PRODUCES glucose

#TEST PhotosynthesisOutput
#QUERY
What does photosynthesis produce?
#EXPECT
glucose, sugar, energy
```

Multiple acceptable answers increase pass rate.

---

## Integration with Learning System

### How Teaching Feeds Learning

1. **Parse** `.tch` file → AST
2. **Ingest** AST → create/update nodes & edges
3. **Snapshot** → save `melvin_brain.bin`
4. **Verify** → run tests, measure retention
5. **Log** → metrics to JSONL and database

### Growth Ledger

During ingestion, you'll see:
```
[GROWTH] +nodes: 5 (cats, dogs, mammals, milk, oxygen)
[GROWTH] +edges: 8
[SNAPSHOT] Saved brain state
```

### Metrics Output

```
📊 TEACHING METRICS:
  Retention score: 0.900 (9/10 tests passed)
  Growth efficiency: 0.8 edges/block
  Snapshot saved: ✅ Yes
```

---

## JSONL Log Format

File: `logs/teaching/2025-10-11.jsonl`

```json
{"timestamp":"2025-10-11T15:30:00","filepath":"lessons/00_basics.tch","duration_ms":245,"nodes_before":0,"nodes_after":15,"edges_before":0,"edges_after":12,"nodes_added":15,"edges_added":12,"edges_updated":0,"duplicates_skipped":0,"rules_added":0,"tests_total":10,"tests_passed":9,"tests_failed":1,"pass_rate":0.9,"retention_score":0.9,"growth_efficiency":0.8}
```

One JSON object per line (append-only).

---

## Command Reference

```bash
# Teach single file
make teach FILE=lessons/00_basics.tch

# Teach entire curriculum
make teach_dir DIR=lessons/

# Verify without changing brain
make teach_verify FILE=lessons/01_animals.tch

# Reset Melvin's brain
make teach_reset

# View teaching logs
make teach_report
ls logs/teaching/
```

---

## Curriculum Design Guide

### Lesson 00: Basics

Foundation concepts:
- Basic categories (mammals, liquids, etc.)
- Simple properties (HAVE, ARE)
- Physical relationships (PRODUCES)

**Goal:** Establish vocabulary and basic graph structure

### Lesson 01: Domain Knowledge

Expand a domain (e.g., animals):
- Specific instances (cats, dogs)
- Category hierarchies (canines ARE mammals)
- Domain-specific relations (CARE_FOR, BUILD)

**Goal:** Create dense subgraph in one domain

### Lesson 02: Cross-Domain

Connect domains:
- Physics + Chemistry
- Biology + Environment
- Abstract + Concrete

**Goal:** Enable multi-hop reasoning

### Lesson 03+: Specialization

Deep dive into specific topics:
- Weather systems
- Human anatomy
- Technology concepts

**Goal:** Build expert knowledge in narrow domains

---

## Testing Strategy

### Coverage

Aim for:
- ≥1 test per 2 facts
- Test simple facts directly
- Test complex facts via multi-hop queries
- Include edge cases

### Test Design

**Good test:**
```
#FACT
plants USE sunlight

#TEST PlantEnergy
#QUERY
What do plants use?
#EXPECT
sunlight, light, sun
```

Multiple acceptable answers.

**Bad test:**
```
#QUERY
Explain photosynthesis
#EXPECT
complex chemical process converting light to glucose via chlorophyll
```

Too complex, too many acceptable phrasings.

---

## Troubleshooting

### Low Pass Rate

If retention score < 0.5:
1. Check query phrasing matches fact structure
2. Ensure graph has necessary edges
3. Verify normalization isn't too aggressive
4. Add more facts to connect concepts

### No Growth

If `edges_added` = 0:
1. Check for duplicates (already taught)
2. Verify parse succeeded
3. Check relation format (SUBJ REL OBJ)
4. Ensure facts aren't comments

### Verification Fails

If tests fail unexpectedly:
1. Check answer traversal logic
2. Verify expected answers are realistic
3. Test manually with simpler query
4. Check for missing intermediate nodes

---

## Future Extensions

### Planned Features

- **Confidence decay** - Reduce confidence over time unless reinforced
- **Spaced repetition** - Re-teach based on test failures
- **Active learning** - Ask Melvin for uncertain facts
- **Multi-modal** - Images, audio in teaching files
- **Explanation generation** - Use #EXPLAIN for answer justification

### Format Extensions

- `#IMAGE path/to/img.png` - Visual teaching
- `#DIALOG` - Conversational exchanges
- `#PROCEDURE` - Step-by-step processes
- `#ANALOGY` - Comparative teaching

---

## API Reference

### TeachingParser

```cpp
// Parse a .tch file
ParseResult result = TeachingParser::parse_file(
    "lessons/00_basics.tch",
    ParseOptions{.verbose = true}
);

// Check for errors
if (!result.success) {
    for (const auto& err : result.doc.errors) {
        std::cerr << err << "\n";
    }
}
```

### TeachingIngestor

```cpp
// Ingest parsed document
IngestResult ingest = TeachingIngestor::ingest(
    doc, G_nodes, G_edges,
    IngestOptions{.snapshot_after = true}
);

std::cout << "Added " << ingest.nodes_added << " nodes, "
          << ingest.edges_added << " edges\n";
```

### TeachingVerifier

```cpp
// Verify retention
VerifyResult verify = TeachingVerifier::verify(
    doc, G_nodes, G_edges,
    VerifyOptions{.fuzzy_threshold = 0.85}
);

std::cout << "Pass rate: " << (verify.pass_rate * 100) << "%\n";
```

---

## Performance

### File Sizes

- Small lesson: ~500 bytes, 10 facts
- Medium lesson: ~2 KB, 50 facts
- Large lesson: ~10 KB, 200+ facts

### Processing Speed

- Parse: ~1ms per KB
- Ingest: ~0.5ms per fact
- Verify: ~2ms per test
- Snapshot: ~10ms for 100 nodes

**Total:** ~250ms for a medium lesson with verification and snapshot.

### Memory

- AST: ~1 KB per 100 blocks
- Ingestion: Minimal (uses existing graph)
- No accumulation between files

---

## Version History

- **v1.0** (2025-10-11) - Initial specification
  - Basic tags (FACT, ASSOCIATION, QUERY, EXPECT, TEST)
  - Parser with normalization
  - Ingestion with deduplication
  - Verification with fuzzy matching

---

**Created:** October 11, 2025  
**Status:** Production Ready  
**Format Version:** 1.0  

---

**Teach Melvin. Verify retention. Measure growth. Repeat.** 🧠

