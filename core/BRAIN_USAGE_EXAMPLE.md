# Using melvin_brain.h - Simple One-File Brain

## Everything is in `core/melvin_brain.h`

No more hunting through multiple files. Everything you need:

```cpp
#include "core/melvin_brain.h"

using namespace melvin::brain;

// Create brain
Brain brain;

// Load existing graph (optional)
brain.load_graph(existing_edges, existing_embeddings, word_to_id, id_to_word);

// Think about something
Brain::Result result = brain.think("what is intelligence?");

std::cout << "Answer: " << result.answer << "\n";
std::cout << "Confidence: " << result.confidence << "\n";
std::cout << "Concepts: ";
for (const auto& concept : result.concepts) {
    std::cout << concept << " ";
}

// Learn from feedback
brain.learn(true);  // or false if answer was wrong
```

## That's it!

The brain handles:
- ✅ Activation spreading
- ✅ Connection formation (need > cost)
- ✅ Connection pruning (cost > need)
- ✅ Answer generation
- ✅ Learning from feedback

## Replacing Old System

**Before (complex):**
```cpp
#include "core/unified_intelligence.h"
#include "core/cognitive/cognitive_engine.h"
#include "core/metacognition/reflection_controller_dynamic.h"
// ... 10+ more includes

UnifiedIntelligence intelligence;
intelligence.initialize(graph, embeddings, word_to_id, id_to_word);
UnifiedResult result = intelligence.reason(query);
// ... complex result with 15+ fields
```

**After (simple):**
```cpp
#include "core/melvin_brain.h"

Brain brain;
brain.load_graph(graph, embeddings, word_to_id, id_to_word);
Brain::Result result = brain.think(query);
std::cout << result.answer;  // Done!
```

## Need and Cost (Built In)

The `Graph` class automatically handles:
- **Need computation**: predictive value + utility + success rate + novelty
- **Cost computation**: energy + memory + computation
- **Connection updates**: `change = need - cost` happens in `update_connections()`

You don't need to think about it - it just works!

