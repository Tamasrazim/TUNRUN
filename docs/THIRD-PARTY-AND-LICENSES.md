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

The root [`LICENSE`](../LICENSE) applies a project-specific proprietary, all-rights-reserved notice to original project content. TUNRUN is publicly viewable but is not released under an open-source license. Access, viewing, and forking on GitHub remain subject to GitHub's Terms of Service; that platform permission is not a general license to republish or reuse project content outside the permissions granted by GitHub and applicable law.

This notice is not a standard SPDX-recognized open-source license. Third-party dependencies and assets are not relicensed by it: identify each separately, include required notices, and verify redistribution rights before shipping. Obtain appropriate legal review before a commercial release or broader licensing change.
