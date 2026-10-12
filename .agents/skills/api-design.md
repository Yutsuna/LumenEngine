---
name: api-design
description: Workflow for designing public interfaces in Lumen.
---

# API Design

Use this skill when adding or changing a public class, function, or module surface.

## Principles

1. **Small surface:** expose only what callers need.
2. **Clear contracts:** document preconditions, postconditions, invariants, ownership, and thread-safety.
3. **Stable names:** prefer names that describe intent, not implementation details.
4. **Backend agnostic:** public engine APIs must not expose Vulkan, OpenGL, X11, or SDL details.

## Checklist

- [ ] Is the function/class necessary? Can it be internal?
- [ ] Are parameter types appropriate (`std::span` instead of raw pointer + size, `std::expected` for fallible operations)?
- [ ] Is ownership explicit (return values, out-parameters, observers)?
- [ ] Are lifetimes documented?
- [ ] Is thread-safety documented if the type may be used concurrently?
- [ ] Are errors propagated, not logged and swallowed?
- [ ] Is the API consistent with the surrounding module?

## What must not be done

- Do not expose Vulkan, OpenGL, X11, or SDL details in public engine APIs unless explicitly justified and documented behind an abstraction layer.
- Do not introduce unstable names that describe implementation details rather than intent.
- Do not design a public API without documenting ownership, lifetime, and thread-safety contracts.
- Do not add to the public surface without confirming it is necessary (checklist item: "Is the function/class necessary? Can it be internal?").
