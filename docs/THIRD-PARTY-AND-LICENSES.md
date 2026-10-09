# Third-Party Code, Assets, and Licensing

Status: inventory policy. The game source and asset inventory have not been added yet.

## Before adding any third-party material

Record each dependency or redistributable asset with:

| Field | Required information |
|---|---|
| Name and version | Exact project, asset, or package name and release/version |
| Origin | Canonical source URL and retrieval date |
| License | Exact license name and link to its text |
| Changes | Whether the file was modified, converted, cropped, or bundled |
| Distribution | Whether the license allows redistribution in the game and installer |
| Notice | Required attribution, notice, source offer, or accompanying files |
| Verification | Reviewer and date the rights were checked |

The register must cover libraries, transitive dependencies, fonts, textures, models, shaders, sound effects, music, voice, icons, tools bundled with the game, and generated content based on external references.

## Dependency rules

- Prefer maintained upstream releases from canonical sources.
- Pin versions or immutable revisions used for a build; do not silently track a floating branch.
- Review the license and update notes before upgrading.
- Avoid adding dependencies solely for convenience when the platform or standard library already provides the capability.
- Include required license texts and notices in source distributions and release packages.
- Do not copy assets from games, websites, repositories, or stock catalogs without documented rights.

## TUNRUN's own license

There is currently no root `LICENSE` file. Until the owner chooses and adds one, do not imply that the source code or assets are freely reusable or redistributable. A public GitHub repository is not itself a license grant. Pick a license deliberately before inviting third-party contributions or publishing reusable game assets.
