# Pac-Man tutorial — working notes

This folder contains the tutorial, not the game.

- `tutorial/` — a Zensical site with the introduction and chapters.
- `AGENTS.md` — these working notes.
- There are intentionally **no C++ source files, headers, or Pac-Man build target** in this folder. The student creates them.

The tutorial is the product. Complete, runnable code lives inside the chapter pages as listings. Do not "helpfully" add a reference implementation to the repository.

## Current status

- Written and published: `tutorial/index.md` (introduction), `tutorial/01-window.md` (Chapter 1), and `tutorial/02-maze.md` (Chapter 2).
- The tutorial itself ships no game source. The reader creates `apps/pacman/main.cpp` and the `pacman` target in Chapter 1. An in-progress source file and target currently exist in this working copy from outside the tutorial workflow; leave them alone and do not treat them as reference code.
- The complete listings for Chapters 1 and 2 have been compiled and smoke-tested in disposable projects outside this repository.
- Planned but **not written**: Chapters 3–9. Do not create placeholder pages for them. Add a page only when its chapter is written.
- The next chapter to write is Chapter 3, "Pac-Man Moves", unless the user asks for something else.

## Version control

This project autocommits and pushes. When a coherent, verified change is complete — a new chapter, a correction found while testing, or a site configuration change — commit it with a short imperative message and push to `origin/main`. Do not wait for a separate instruction. Never commit generated `site/` or `.cache/` output. If a push is rejected because the remote moved, fetch and rebase, then push again.

## Audience

The reader:

- knows basic C++: variables, functions, loops, classes, and `std::vector`;
- has never used SDL3, CMake, or a game loop (assume no game-development experience);
- is on macOS and will use Homebrew, LLVM, CMake, and SDL3;
- wants to understand the code, not paste a finished game.

Write for that person. Define jargon the first time it appears. Explain *why* before *how*. A little repetition is fine; unexplained cleverness is not.

## Hard technical constraints

- **No source code in this repository.** Do not create `main.cpp`, headers, or a game-level `CMakeLists.txt` under `apps/pacman`, and do not add a `pacman` target to the root `CMakeLists.txt`. The reader writes those files. The only files we add are tutorial pages, site configuration, documentation, and clearly labelled illustrations.
- **Platform:** macOS. Apple Silicon and Intel Homebrew prefixes must both work.
- **Language:** C++23, built with Homebrew LLVM. The root `CMakeLists.txt` selects the compiler for the reader's target; do not duplicate that logic in the tutorial.
- **Graphics and input:** SDL3 only. The reader's `pacman` target should link `SDL3::SDL3` and nothing else. Do not link the Metal frameworks used by `tetris`.
- **No extra game dependencies or assets** without explicit approval. That means no SDL_ttf, SDL_image, SDL_mixer, font files, sound files, or sprite sheets. Draw everything with SDL primitives; use `SDL_RenderDebugText` or a tiny custom glyph routine for HUD text. Decorative tutorial illustrations are fine; keep them in `tutorial/assets/` and label them clearly.
- **One translation unit for the reader:** the game stays in the reader-created `apps/pacman/main.cpp` through Chapter 8. Chapter 9 may discuss splitting it up, but do not do the refactor in this tutorial. The single file is deliberate.
- **Warnings:** the full listing must compile cleanly with the root flags: `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion`.
- **Ownership:** no raw `new`/`delete`, no owning raw pointers, no manual cleanup on multiple `return` paths. Use RAII and `std::unique_ptr` with custom deleters, as Chapter 1 demonstrates.
- **No reading of uninitialized state:** initialize locals, initialize structs, and validate tile lookups.
- **Every checkpoint runs.** Never publish a chapter whose complete listing fails to compile or crashes at startup.

## Game design decisions

These choices keep the game small and the chapters coherent. Change them only if the change is important, and update this file and every affected chapter when you do.

**Window and maze.** 800 × 600 logical pixels. The maze is 28 columns × 21 rows of 24 px tiles, drawn with its top-left tile at (64, 56). That leaves roughly 56 px of HUD space above the maze and 40 px below it. Chapter 2 must keep these numbers unless it deliberately revises the layout everywhere.

**Maze data.** Store an original 28 × 21 layout as a small array of strings. Suggested legend:

| Character | Meaning |
| --------- | ------- |
| `#` | wall |
| `.` | pellet |
| `o` | power pellet |
| `=` | ghost-house door |
| space | empty walkable floor |

Do not reproduce the arcade maze or use ripped assets. Spawn positions can be constants or dedicated map markers; document whichever the chapter uses.

**Movement.** Use grid-step movement, not continuous collision:

- An entity knows its current tile, its target tile, and a direction.
- It moves from tile center to tile center. A float `progress` in `[0, 1]` interpolates the render position.
- At a tile center, the entity picks its next tile. Walls block entry; tunnels wrap horizontally.
- Pac-Man may reverse immediately; other turns take effect at the next tile center and should accept a buffered input.
- Speeds are in tiles per second and multiplied by delta time. Cap delta time (for example, at 0.1 s) so a stalled window cannot teleport an entity through walls.

This model keeps collision simple and is close enough to how Pac-Man actually feels. Do not introduce continuous AABB collision in the tutorial.

**Ghost AI.** At each tile center, a ghost considers the legal directions (excluding an immediate reversal unless forced), and chooses the one that minimizes squared distance from the neighbouring tile to its current target tile. Use distinct targets:

- Red chases Pac-Man.
- Pink targets a few tiles ahead of Pac-Man.
- Cyan targets a point derived from Pac-Man and the red ghost.
- Orange chases from a distance and retreats when close.

Scatter mode sends each ghost to a corner; chase mode uses the targets above. Frightened mode makes ghosts slower and edible. A simplified scatter/chase timer is enough. No BFS, A*, or pathfinding library.

**Game flow.** The final game has a small state machine: Ready, Playing, Dying, GameOver, and Won. The player starts with three lives. Pellets score 10 points, power pellets 50, and eaten ghosts 200/400/800/1600. Clearing the maze wins. Pressing Enter or Space restarts after a win or loss. Pause is a small stretch goal, not a requirement.

**Scope.** One maze, one level, four ghosts, pellets, power pellets, score, lives, win/lose, restart. Out of scope: fruit, audio, cutscenes, multiple levels, persistent high scores, menus, sprite art, shaders, and mobile support. Chapter 9 can suggest them as next steps.

**Rendering.** Draw Pac-Man as a filled circle with a wedge-shaped mouth, ghosts as simple rounded bodies with eyes, walls as rectangles or lines, and pellets as circles. No textures. Use a limited palette defined near the other tunables.

## Syllabus

Each chapter is a runnable checkpoint. File names are flat and follow the `NN-topic.md` pattern.

| # | Title | File | Checkpoint |
| - | ----- | ---- | ---------- |
| — | Introduction | `index.md` | Reader knows the scope, setup, and roadmap |
| 1 | A Window Into the Maze | `01-window.md` | Student-created window; Esc/close exits cleanly |
| 2 | Drawing the Maze | `02-maze.md` | Maze, walls, pellets, and power pellets on screen |
| 3 | Pac-Man Moves | `03-pacman.md` | Pac-Man controllable, blocked by walls, uses tunnels, chomps |
| 4 | Pellets and Score | `04-pellets.md` | Pellets disappear, score updates, clearing the maze wins |
| 5 | Four Ghosts | `05-ghosts.md` | Four ghosts leave the house and move legally |
| 6 | Ghost Personalities | `06-ghost-ai.md` | Chase/scatter targeting differs per ghost |
| 7 | Danger and Lives | `07-lives.md` | Power pellets, frightened ghosts, death, lives, respawn |
| 8 | The Complete Game | `08-complete-game.md` | Full ready/play/die/win/lose/restart loop |
| 9 | Beyond the Maze | `09-next-steps.md` | Finished game; ideas and refactoring advice |

### Chapter contracts

**1. A Window Into the Maze (written).** The reader installs Homebrew LLVM, CMake, and SDL3; creates `apps/pacman/main.cpp` and adds the `pacman` target to the root `CMakeLists.txt`. The chapter explains CMake targets, the SDL window/renderer/event-loop model, RAII, `unique_ptr` custom deleters, VSync, and clean shutdown. End with a working window and a clean build in the reader's own checkout.

**2. Drawing the Maze (written).** Introduce the 28 × 21 text map, tile constants, coordinate conversion, and per-tile drawing. Draw walls, pellets, and power pellets. No Pac-Man or ghosts yet. The window and loop stay unchanged. End with the maze on screen.

**3. Pac-Man Moves.** Add a Pac-Man entity and keyboard input. Introduce delta time, direction buffering, grid-step movement, wall collision, tunnel wrapping, and mouth animation. Pac-Man should feel responsive, not square-by-square. No pellets are eaten yet.

**4. Pellets and Score.** Consume pellets from the map, count the remaining pellets, update the score, and draw a HUD. When the last pellet is gone, show a win message. Keep the code structured so ghosts can be added next.

**5. Four Ghosts.** Add a ghost house, four ghosts, release timers, and legal ghost movement. Reuse the movement model from Chapter 3 rather than inventing a second one. Draw the ghosts. They may wander or rudimentarily chase, but distinct personalities come next.

**6. Ghost Personalities.** Add per-ghost target tiles, the distance-based direction choice, and a scatter/chase schedule. Explain why "no immediate reversal" matters and what happens at intersections. The four ghosts should visibly behave differently.

**7. Danger and Lives.** Add power pellets, frightened mode, ghost-eating combos, collision with ghosts, a Pac-Man death sequence, remaining lives, and respawn. This is where collisions and timers start interacting; keep the state transitions explicit.

**8. The Complete Game.** Finish the game state machine: Ready, Playing, Dying, GameOver, Won. Add restart, the final HUD layout, and a polish pass. Fix integration bugs. This chapter must ship a genuinely playable game and a final complete listing.

**9. Beyond the Maze.** No new game features required. Discuss refactoring the single file into headers/sources, adding fruit, sound with SDL_mixer, multiple levels, high scores, and tests. Point to useful next projects. A short final recap is appropriate.

## Writing conventions

- Address the reader as "you". Use present tense. Be encouraging without being sugary.
- Open every chapter with a `!!! abstract "Chapter goal"` callout. State the concrete result, not a list of topics.
- Use short sections with descriptive headings. Keep paragraphs short. Prefer tables and small diagrams over walls of text.
- Introduce a concept immediately before the code that uses it. After the code, explain the non-obvious lines.
- Show only the part being changed in the body of the chapter; put the full, compilable file at the end under "The complete file".
- **The latest complete listing is the reference implementation.** There is no checked-in `main.cpp` to compare against. Extract the listing to a scratch project to compile or diff it.
- Do not link to a source file in the repository. Link to sections or pages. There is no `apps/pacman/main.cpp` to link to.
- Use `~~~` fenced code blocks with a language tag (as Chapter 1 does). Shell commands are run from the repository root after the reader has created their files; do not prefix them with `$`, because the copy button would copy the prompt too.
- Use relative Markdown links such as `01-window.md`. Link only pages that exist. Update the intro roadmap when a chapter is published.
- Use American English in prose to match the code: "color", "behavior", "initialize", "organize".
- Callouts available and encouraged: `!!! note`, `!!! tip`, `!!! warning`, `!!! abstract`. Use `??? example` for optional solutions.
- Every chapter ends with: a build/run section, a checkpoint task list, exercises (at least three; one should be a stretch), a recap, and a short "what's next".
- Include a short troubleshooting table only for problems a reader can plausibly hit in that chapter. Do not repeat the entire Chapter 1 table everywhere.
- No fake screenshots. If an image is added, it must show the actual game or be clearly decorative, and it goes in `tutorial/assets/`.
- Keep the tone practical. Avoid "simply", "just", and "obviously" for anything that is not obvious to a beginner.

## Workflow for adding the next chapter

1. Re-read this file and the previous chapter. Extract the previous chapter's complete listing into `/tmp/pacman-check/main.cpp` and confirm it still builds. That is the student's starting point.
2. Implement the next checkpoint in the scratch copy, never inside `apps/pacman`. Build and run it there before writing prose.
3. Write the chapter in `tutorial/NN-topic.md`. Follow the chapter contract and writing conventions.
4. Add the complete listing to the chapter. Extract it from the Markdown and diff it against `/tmp/pacman-check/main.cpp`; they must match. Rebuild the scratch copy.
5. Update `tutorial/zensical.toml` with the new nav entry. Add the chapter link to the roadmap in `tutorial/index.md`.
6. Update "Current status" in this file. Do not claim the code lives in the repository; it does not.
7. Build the site with `zensical build --strict` and fix every warning.
8. Run the scratch copy once more, press Esc, and confirm it exits cleanly.
9. Do not commit generated `site/` output; it is gitignored.

## Build and verify

### The reader's build

After the reader has created `apps/pacman/main.cpp` and added the two CMake lines from Chapter 1, they run from the repository root:

~~~bash
cmake -B build
cmake --build build --target pacman
./build/pacman
~~~

This repository's own `CMakeLists.txt` intentionally does **not** contain the `pacman` target. Do not add it to make a check pass.

### The agent's check

Use a disposable directory outside the repository, for example `/tmp/pacman-check`:

- Copy the chapter's complete listing into `/tmp/pacman-check/main.cpp`.
- Create a small `CMakeLists.txt` there that mirrors the root setup: force Homebrew LLVM before `project()`, request C++23, call `find_package(SDL3 CONFIG REQUIRED)`, build the source, and link `SDL3::SDL3`.
- Configure and build there, then run the executable briefly and press Esc.

Format the scratch source with Homebrew clang-format. The scratch directory has no `.clang-format` of its own, so either copy the repository's file there or point the tool at it; otherwise it falls back to LLVM's two-space style and flags valid code:

~~~bash
"$(brew --prefix llvm)/bin/clang-format" -i \
    --style=file:/path/to/cppgamearch/.clang-format /tmp/pacman-check/main.cpp
~~~

Build the tutorial site:

~~~bash
cd apps/pacman/tutorial
zensical build --strict
~~~

Preview while writing:

~~~bash
cd apps/pacman/tutorial
zensical serve
~~~

Then open the URL Zensical prints (usually around 8000).

## Do not

- **Do not add Pac-Man source code to this repository.** No `main.cpp`, no headers, no game-level `CMakeLists.txt`, and no `pacman` target. The student writes these; the tutorial teaches them.
- Do not add chapters 3–9 without being asked. This file describes the plan; it does not authorize jumping ahead.
- Do not add placeholder pages, empty nav entries, or links to unpublished chapters.
- Do not add a new library, game asset, or platform dependency without approval.
- Do not split the game into multiple files during the tutorial.
- Do not copy the original Pac-Man maze, sprites, sounds, or other assets. The fan-project note in the introduction is there for a reason.
- Do not give the reader a downloadable finished game or link to one. The point is to write it.
