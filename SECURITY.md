# Security Policy

## Reporting a vulnerability

**Do not publish suspected vulnerabilities, exploit details, private files, or credentials in a public issue.**

Use GitHub's **Private vulnerability reporting** feature on this repository's Security tab when it is available. If it is not enabled or available, contact the repository owner through a private GitHub communication channel and ask for a secure reporting route. Do not attach real player saves or personal information unless the maintainer specifically requests a sanitized reproduction.

Please include the affected commit or version, Windows version, impact, and minimal reproduction steps. Redact usernames, local paths, tokens, and other private data. A proof-of-concept should be minimal and should not damage data or affect other people.

## Scope and response

TUNRUN is an actively developed native C++20 gameplay prototype. The repository includes procedural course and obstacle generation, collision and flight logic, a bounded profile parser with local save/recovery handling, automated tests, and Windows build/package automation. It is not a finished game, and there is no public release installer, updater, or online service. Temporary CI development artifacts may be produced by the build workflow. Reports about the C++ code, untrusted save/profile data, build workflow, package contents, dependency provenance, and release integrity are in scope.

The maintainer will acknowledge reports when reasonably possible, assess severity and reproducibility, and fix confirmed issues before announcing technical details where practical. No fixed response or resolution time is promised.

## Security status

A policy document is not proof that every GitHub security feature is enabled. Repository settings, automated scanners, build hardening, and release signing must be verified independently as implementation progresses.
