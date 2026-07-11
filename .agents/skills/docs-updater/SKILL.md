---
name: docs-updater
description: Updates the README.md and other documentation in Enfusion docs
---

---
name: doc-updater
description: Update Enfusion Script (Enforce) Doxygen documentation in .c files and keep project markdown docs in sync with code changes. Use this skill whenever the user makes code changes to .c script files, adds/removes/modifies classes or methods, asks to "update docs", or prepares a PR for an Arma Reforger/DayZ mod project. Proactively offer to update Doxygen comments after any change that adds, removes, or modifies method signatures, class declarations, or member variables in .c files.
---

# Enfusion Script Documentation Updater

## Purpose

Keep two types of documentation in sync for Enfusion Script (Enforce) projects:

1. **Inline Doxygen docs in `.c` files** - the `//!` comment blocks above classes and methods, following Bohemia Interactive's official conventions
2. **Project markdown docs** - `README.md` and files in `docs/` that describe the mod's architecture, setup, and usage

## When to act

Proactively suggest a documentation update after:
- Adding, removing, or renaming methods in `.c` files
- Adding, removing, or renaming classes
- Changing method signatures (parameters, return types)
- Adding or removing member variables
- Changing class inheritance (extends)
- Modifying setup steps, build instructions, or dependencies

Also act when the user explicitly asks to update docs.

## Part 1: Inline Doxygen Docs in .c Files

### Reference: Enfusion Script conventions

These are the official Bohemia Interactive conventions for Enforce Script documentation.
All Doxygen updates must follow these rules exactly.

**Method documentation format:**
- Use `//!` prefix for Doxygen comments (not `///` or `/**`)
- Brief description on the first `//!` line after the separator
- `//! \param paramName Description of the parameter` for each parameter
- `//! \return Description of return value` if the method returns something
- Parameters use `camelCase` naming; methods use `PascalCase`

**Method separator:**
- Every method must be preceded by: `//------------------------------------------------------------------------------------------------`
- That is two slashes followed by exactly 96 dashes

**Class documentation format:**
- A `//!` comment line directly above the class declaration describing its purpose

**Method ordering (top to bottom within a class):**
1. General methods
2. EOnFrame
3. EOnInit
4. Constructor (method named after the class)
5. Destructor (`~ClassName`)

### Step 1: Identify changed .c files

Run `git --no-pager diff --name-only` against the base branch to find changed `.c` files.
Also scan for any `.c` files with missing or stale Doxygen comments.

### Step 2: Analyze each class in changed files

For each `.c` file, read the full content and identify:
- Class declarations and their inheritance (`class TAG_MyClass : ParentClass`)
- All methods with their signatures (return type, name, parameters)
- Existing `//!` documentation blocks
- Missing documentation (methods without `//!` comments)
- Stale documentation (comments that don't match the actual signature)

### Step 3: Update or add Doxygen comments

For each method, write or update the `//!` block in this exact format:

```enfusion
//------------------------------------------------------------------------------------------------
//! Brief one-line description of what the method does
//! \param paramName1 Description of first parameter
//! \param paramName2 Description of second parameter
//! \return Description of what is returned
protected int GetHealth(string unitName, bool includeArmor)
```

Rules:
- If a method has no return value (`void`), omit `\return`
- If a method has no parameters, only write the description line
- Parameter descriptions should explain what the value represents, not just restate the type
- Keep descriptions concise but informative - one sentence or phrase per line
- The separator `//------------------------------------------------------------------------------------------------` goes BEFORE the `//!` comment block
- If a method already has documentation but the signature changed, update the existing block rather than replacing it entirely when possible

### Step 4: Verify class-level docs

Every class should have at least one `//!` comment above it:

```enfusion
//! Handles player health, damage, and regeneration systems
class TAG_HealthComponent : ScriptComponent
{
```

Add missing class descriptions. Update stale ones if the class's role has changed.

### Step 5: Check method ordering

If you added new methods, verify they are in the correct position following the ordering convention:
General methods first, then EOnFrame, EOnInit, Constructor, Destructor.
Suggest reordering if methods are out of place (but tell the user before moving code).

## Part 2: Markdown Docs

### Step 1: Find relevant .md files

Scan the project root and `docs/` directory for markdown files:
- `README.md` at the project root
- Any `.md` files in `docs/` or similar directories

### Step 2: Map code changes to doc sections

For each code change, find the corresponding section in the markdown docs that describes that behavior.
If a new feature or class has no documentation section, plan where to add it.

### Step 3: Update markdown docs

- Preserve existing document structure, heading hierarchy, and formatting style
- Only change sections affected by the code changes
- Keep code examples accurate: verify method names, parameter counts, and class references
- Remove references to removed classes, methods, or features
- Add new sections only when the feature warrants it, matching the existing doc style
- Keep lines at or under 120 characters

## Summary output

After updating, provide a brief Markdown summary of all changes:
- Which `.c` files had Doxygen comments added or updated, and for which methods
- Which `.md` files were modified and which code changes drove each update
- Any sections the user should manually verify
- If method ordering needs adjustment, note it separately so it can be reviewed
