# Time Trial Racer

A racing game for the TI-84 Plus CE. Drive around six tracks as fast as you can and try to beat your own best time.

## Install

You need TI Connect CE on your computer.

1. Connect your calculator and send these two files, in this order:
   - `clibs.8xg` (send this first)
   - `RACER.8xp`
   - The libraries are the `clibs.8xg` included here (its exact version could not be read from the file; it came from the CE-Programming libraries "latest" release). If the calculator says the libraries are too old, send the included `clibs.8xg` again.
   - Archive `RACER.8xp` (in TI Connect CE, or on the calculator via `2nd` `mem` > Archive). It then survives a RAM reset and needs less free RAM to start.
2. Keep your calculator on OS 5.4. Do not update it. Newer versions (5.5 and up) block programs like this one.

## Run

1. Press `prgm`.
2. Pick `RACER`.
3. Press `enter`.

## Controls

| Key | What it does |
|-----|--------------|
| ← / → | Steer left / right |
| 2nd or ↑ | Gas |
| alpha or ↓ | Brake (hold it when you are stopped to reverse) |
| mode | Restart the track right away |
| clear | Go back to the menu (in the menu, quits the game) |
| graph | Show or hide the frame rate (fps) |
| ↑ / ↓ (menu) | Pick a track |
| ← / → (menu) | Turn the ghost car on or off |
| 2nd or enter (menu) | Start the track |

Tracks: Warmup, Rolling Hills, Hairpin Alley, Canyon Run, Twin Peaks, The Gauntlet.

## Best times and ghosts

- Each track saves your best time.
- When you beat your best, the game saves it, along with a ghost. The ghost is a faint car that copies your best run, so you can race against yourself.
- While saving, the calculator may ask "Garbage Collect?". Answer Yes.
- Saves are kept even when the calculator's memory is cleared.
- To reset the bests, delete the saved files `RCRSV0` to `RCRSV5` from the calculator's memory.

## If you see ERR:MEMORY when starting

The game needs about 40 KB of free RAM to start. Archive `RACER.8xp` (see above), and archive or delete other programs you don't need. You can check free RAM with `2nd` `+` (mem) > 2: Mem Management/Delete.
