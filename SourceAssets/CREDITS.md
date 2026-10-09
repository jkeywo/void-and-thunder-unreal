# Asset credits

Placeholder art for the Void & Thunder client. All third-party assets here are
redistributable under their stated licences.

## Ship models — `models/*.glb`, `models/textures/*.png`

**Ultimate Spaceships Pack** by **Quaternius** — https://quaternius.com/packs/ultimatespaceships.html

- Licence: **CC0 1.0** (public domain; no attribution required — credited here anyway).
- The glTF hulls were converted to `.glb`, re-oriented bow-along-+X and up-along-+Z
  for V&T's Z-up world, and normalised to ~44-unit length (see the conversion in
  the project history). Each faction uses one hull plus the pack's matching colour
  variant:

  | Faction | Hull | Texture variant |
  |---|---|---|
  | Corsairs | Executioner | Green |
  | Houses | Imperial | Red |
  | Janissariat | Bob | Orange |
  | Guild | Dispatcher | Blue |
  | Freebooters | Challenger | Purple |

## Skybox — `skybox/phoenix_space_cubemap.png`

Authored for the sibling project *project-phoenix-v2* (same author); reused here.

## Shaders — `shaders/star_surface.wgsl`, `shaders/star_halo.wgsl`

Authored for *project-phoenix-v2* (same author); ported to V&T's Z-up camera.

Native WAV files are offline renders of the MIT-licensed source synthesizers in vt_client/src/audio.rs. Niagara burst content derives from the installed Unreal Engine Niagara DirectionalBurst and FountainLightweight templates; Epic-provided content retains its Unreal Engine license.

Original HUD metal/CRT artwork: exported from the MIT-licensed `vt_client/assets/ui/hud.html`; see `ui/SOURCE.md`. The native composite font packages Droid Sans Mono from the installed Unreal Engine font distribution (Android Open Source Project font); its original third-party license applies.

Intro portraits: original AI-generated art created for this project on 2026-10-09 using image_gen. Source images and generation prompts are preserved in `ArtSource/Intro/README.md`. The engineer and enemy captain are role labels, not additions to the vendored setting.
