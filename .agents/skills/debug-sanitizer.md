---
name: Debug Sanitizers
description: Investigate crashes/UB/sanitizer reports in Lumen using the ASan/UBSan debug build and valgrind
---

Build a clean sanitized debug tree and reproduce (commands detail in
`.agents/commands/build-debug.md`):

1. Setup:
   - existing `Build/`: `meson setup Build --buildtype=debug -Db_sanitize=address,undefined -Denable_testing=true --reconfigure`
   - fresh: same without `--reconfigure`.
   then `ninja -C Build`.
2. Reproduce under tests: `meson test -C Build` (exit **84** = failure, not
   1) or run one binary directly: `Build/Source/Meson/Tests/<Module>Tests`,
   `Build/Source/Meson/Benchmarks/<Module>Benchmarks`, or the `Launch`
   binary (`Build/Source/Launch/Launch`).
3. Read sanitizer stacks top-down; engine frames are the directories
   under `Source/`; ignore sanitizer/plumbing frames. Map any `0x…` address
   to a file:line via debug info (builds are unstripped, `-Og`).
4. Frequent findings in this codebase and where they usually come from:
   - use-after-free / dangling `FStringView`/`TSpan`: lifetime doubts in
     `Scene` actor payloads — that is why mailbox queues exist (`Core/Thread/SPMCQueue.hpp`);
     check `Scene/Actor` ownership before editing.
   - double-free: manual `delete`/mismatched ownership — engine policy is
     `Memory/UniquePtr.hpp`/`SharedPtr.hpp` and no raw `new` in engine code.
   - UBSan integer UB: shift/overflow/cast issues — engine uses
     `Int32/UInt32/USize` typedefs; unsigned wrap is not "fine", fix the range.
   - data race (TSan needs a separate build `-Db_sanitize=thread`): actor
     state must not be shared; only mailbox messages cross actors (`modules/Scene.md`).
5. If sanitizer stacks don't explain it, valgrind is installed in the
   devshell (`nix develop`): `valgrind --tool=memcheck
   Build/Source/Launch/Launch` (or the test binary). Slower but no rebuild.
6. Remember: fixes follow `rules/core-first.md`,
   `rules/coding-style.md` (noexcept everywhere, Lumen error handling).

Do not "fix" sanitizer noise by disabling instrumentation or adding
suppressions; patch the root cause or document why the pattern is safe.
