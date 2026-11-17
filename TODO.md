# TODO

## Bugs

- [x] **Component-less archetypes corrupt `entity_list` on delete.** Now prevented: an assert in
  `archetype_manager::create_archetype` rejects empty signatures. Revisit if per-entity
  `add_component`/`remove_component` is added, since removing the last component would need
  to delete the entity (or `world::delete_entity` should decide the swap from `entity_list`).
- [x] **Spawning or deleting inside a `for_each` callback is UB.** Now asserted (debug builds
  only): `world::_iterating` counts running `for_each` calls, and `spawn_entity`, `build()` and
  `delete_entity` assert it is zero. Add a deferred command buffer only if a system ever
  needs to spawn while iterating.

## Design

- [ ] **Entity ids have no generation counter.** `id_manager` recycles ids immediately, so a
  stale id held after `delete_entity` silently refers to whatever entity gets that id next.

## Minor

- [ ] `entity_builder`: an entity that never calls `build()`, or whose `build()` throws on an
  unregistered component, leaks its id.
- [ ] `spawn_entity` with the same component type twice pushes two values into one column,
  so the columns fall out of step.
- [ ] `particle_system::bounded(0)` divides by zero (`color_count == 0`).
- [ ] `entity_record::global_id` holds an `entity_id` but is typed `archetype_id`, and
  `is_active` compares it with `NULL_ARCHETYPE_ID`.
- [ ] `[[maybe_unused]]` on `entity_builder::_world` is unnecessary; it is used.
- [ ] `fixed.hpp` uses `__int128` directly, which GCC rejects under `-Wpedantic -Werror`
  (clang accepts it). Only matters for GCC builds; fix is an `__extension__` typedef.

## Comments

- [ ] `archetype::array_for`: the comment explaining why the unchecked `static_cast` is safe was
  removed. Restore a short version (the id comes from a per-type counter and
  `register_component<C>` installs `component_array<C>` under it).
- [ ] `signature::hash`: the "Rework this to use an inline bitset" TODO is no longer needed for
  correctness now that the hash is order-independent. Drop it, or keep it only for speed.
- [ ] `draw_system`: the note about "a skipped tick" doesn't apply; an `UNCAPPED` system never
  skips.
- [ ] `world::register_system` TODO says the draw system redraws "several times per frame";
  it draws once per frame, and the waste is redrawing when physics didn't tick.

## Performance / architecture (from review)

Estimates from reading the code, not measured. Ranked within each group.

### Cheap, high-value

- [ ] **Faster `isqrt`** (`fixed.hpp`, ~8 lines). Inputs are at most 2^31, but it loops on
  `unsigned __int128` starting from `x = n`: ~15-20 Newton steps, each a 128-bit division
  libcall. Add a `uint64_t` overload starting at `1ull << ((std::bit_width(n) + 1) / 2)`.
  Results are bit-identical, so determinism is unaffected.
- [ ] **Entity records in a vector indexed by id** (`world.hpp`, ~15 lines). Replaces
  `unordered_map<entity_id, entity_record>`: removes a node allocation per spawn, a free per
  delete and rehash spikes. Drop `global_id` (closes the Minor item about its type); a `gen`
  field can go here later (see Design).
- [ ] **Flat column index in `archetype`** (~8 lines). Replace `_componentMap` with a
  `std::vector<uint8_t>` indexed by `component_id` (0xFF = absent). Removes a hash lookup per
  component per spawn and per archetype per query.
- [ ] **`uint64_t` bitmask signature** (~25 lines). Archetype lookup, query cache keys and
  `contains_all` become word operations; removes the heap-allocated sets. Limit of 64
  component types (assert in `insert`). Makes archetype migration targets trivial.
- [ ] **Stop drawing identical frames** (`draw_system.hpp`, 1 line). `k_target_fps = 999`
  against 240 Hz physics redraws ~3 of 4 frames unchanged. Set it to the physics rate (or use
  vsync). Catch: spawn is once per frame in `main.cpp`, so this also lowers the spawn rate
  unless spawning moves into the fixed tick.
- [ ] **Chunk-level query + entity ids in the archetype.** Expose
  `for_each_chunk<Cs...>(f(size_t n, Cs*...))` and build `for_each_with_components` on it
  (~10 lines; enables batched draw / SIMD). Move `world::_archetypeEntities` into `archetype`
  (~15 lines) so queries can yield `entity_id`, which no system can get today.

### Bigger, later

- [ ] **Deferred command buffer**: queue spawn/delete during iteration, flush FIFO after
  `for_each`. Needs entity ids in queries. Replaces the `_iterating` assert with real support.
- [ ] **Add/remove components on live entities** (archetype migration): a `move_row_to`
  virtual on `component_array_base`, plus the dense records, bitmask signature and in-archetype
  entity ids. `entity_builder` could become "spawn + add", removing the `std::any` path.
- [ ] **Compile-time ids per component pack**, indexing per-world vectors of archetype ids and
  query cache entries. Removes signature hashing from spawn and query entirely. Only if
  profiling batch spawns still shows it after the bitmask.
- [ ] **Type-erased raw columns** (the "manual mem offset" TODO). No iteration gain; only
  worth it once migration is hot.
- [ ] **System dependency DAG with dirty-marking.** Premature with two systems; a per-system
  tick counter covers the current need.

### Leave as is

`fixed` multiply/divide in `update` (cheap, and changing it shifts trajectories), column
virtuals (not on hot paths), `id_manager`, the query cache design, the CRTP
`component_system`, and `entity_builder` (debug/test path).
