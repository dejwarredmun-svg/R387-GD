# R387 — Art Agent for Geometry Dash (Geode)

Select blocks in the editor, press the pink **R387** button, describe the art in any language,
and it is drawn right in the editor in the shape of your selection.

- **Attach** (or Ctrl+V) a reference picture. Ctrl+V pastes a clipboard image on Windows, or text anywhere
  (pasting is the reliable way to type non-Latin text, since GD's own input fields are Latin-only).
- Ask for changes in the chat box, or press **Another variant**.
- **Undo** removes what this window generated. Your original blocks are never touched; art is placed on the top z-layer.
- Generations come in packs; the balance is shown top-right of the window.
- Art is built from deco blocks recolored through color triggers (channels 850+ by default), merged into rectangles to save objects.

## Build the mod
Easiest: push this folder to GitHub and run the **Build R387** action. Download the `.geode` artifact.
Local: install the Geode CLI + SDK, then `geode build` in this folder.
Targets Geode 4.x / GD 2.2081.

## Run the server
```
cd server
pip install -r requirements.txt
export ANTHROPIC_API_KEY=...
export R387_ADMIN=pick-a-secret
python server.py            # add R387_FREE=1 for unlimited dev mode
```
Sell/give packs (starter 10, creator 50, studio 200):
```
curl -X POST localhost:8387/admin/grant -H "X-Admin: pick-a-secret" -H "Content-Type: application/json" -d '{"pack":"creator"}'
```
Give the returned key to the player. In GD: Geode → R387 → settings → set **Server URL** and **License key**.
Failed generations are never charged.
