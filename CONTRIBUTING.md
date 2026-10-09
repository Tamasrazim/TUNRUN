# Contributing to TUNRUN

## Repository workflow

TUNRUN intentionally uses **one branch: `main`**. Changes are committed directly to `main`; this project does not use topic branches or pull requests. Do not create a branch or open a pull request for routine project work.

Only the repository owner and explicitly trusted maintainers should have write access. Anyone without write access can use issues for non-sensitive bug reports and feature discussion. Never post vulnerability details in a public issue; follow [SECURITY.md](SECURITY.md).

Direct-to-main does not mean untested. Before committing a change:
1. Run the relevant checks locally.
2. Inspect the diff for secrets, generated files, unrelated changes, and license/provenance problems.
3. Update documentation and tests when behavior or requirements change.
4. Push the commit to `main`, then inspect the resulting GitHub Actions run and fix failures with a follow-up commit on `main`.

## License and contributions

The project uses the proprietary, all-rights-reserved notice in [`LICENSE`](LICENSE); it is not an open-source contribution program. Do not submit third-party code or assets unless their license and redistribution rights have been verified and the repository owner has explicitly approved their inclusion. The repository owner must confirm permission and attribution before incorporating externally authored material.

## Project status

The current repository is a specification baseline. Do not describe a feature, executable, installer, security scanner, or release as working until there is implementation and test evidence.

## Code and content standards

- Prefer small, understandable modules with explicit ownership of resources and state.
- Treat files, seeds, replay data, settings, and imported assets as untrusted input.
- Keep gameplay simulation deterministic; isolate cosmetic randomness.
- Keep credentials and signing keys out of source control and build logs.
- Do not add a dependency or asset until its source, version, license, and redistribution rights are recorded.
- Avoid telemetry and network access by default. Any exception needs a documented purpose and privacy/security review.
- Report bugs with reproduction steps, expected and actual behavior, environment, and logs with personal data removed.
