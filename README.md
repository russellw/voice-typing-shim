# voice-typing-shim

An editing layer over Windows voice typing.

## What it does

Opens a window with a large edit box. Whenever the edit box gains focus, it automatically triggers Windows voice typing (Win+H), so you can dictate directly into it.

Two buttons sit at the bottom of the window:

| Button | Shortcut | Action |
|--------|----------|--------|
| Fix up (F11) | F11 | Cleans up the text — lowercases words that are capitalised mid-sentence but are not proper nouns |
| Copy (F12) | F12 | Copies the full text to the clipboard |

## Building

Requires MSVC. Open a **Developer Command Prompt for Visual Studio** and run:

```
build.bat
```

This produces `main.exe`.

## Fix up dictionary

The Fix up function uses a compiled-in dictionary of ~9,000 English proper nouns (names, places, brands) derived from the [SCOWL](https://github.com/en-wl/wordlist) project at level ≤ 60. To regenerate the dictionary from a local SCOWL checkout:

1. Clone the SCOWL repo into `wordlist-2/` next to this project.
2. Run: `python gen_proper_nouns.py`
3. Rebuild.
