[CmdletBinding()]
param(
    [string]$BuildDirectory = "build/Release",
    [string]$OutputDirectory = "build/package",
    [string]$Version = "0.1.0-dev",
    [string]$CommitSha = $env:GITHUB_SHA
)

$ErrorActionPreference = "Stop"

$repositoryRoot = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$separator = [System.IO.Path]::DirectorySeparatorChar
$rootPrefix = $repositoryRoot + $separator
$buildPath = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot $BuildDirectory))
$outputPath = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot $OutputDirectory))
$buildRoot = [System.IO.Path]::GetFullPath((Join-Path $repositoryRoot "build")) + $separator

if (-not $buildPath.StartsWith($rootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Build directory must be inside the repository."
}
if (-not $outputPath.StartsWith($buildRoot, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Output directory must be contained in the repository's build directory."
}
if ($outputPath -eq $buildPath -or $outputPath.StartsWith($buildPath + $separator, [System.StringComparison]::OrdinalIgnoreCase)) {
    throw "Output directory must not be inside the build input directory."
}

$executable = Join-Path $buildPath "tunrun.exe"
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
    throw "Missing build output: $executable"
}

$stagePath = Join-Path $outputPath "TUNRUN-Windows-x64-Portable"
$zipPath = Join-Path $outputPath "TUNRUN-Windows-x64-Portable.zip"
New-Item -ItemType Directory -Path $outputPath -Force | Out-Null
if (Test-Path -LiteralPath $stagePath) {
    Remove-Item -LiteralPath $stagePath -Recurse -Force
}
if (Test-Path -LiteralPath $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}
New-Item -ItemType Directory -Path $stagePath -Force | Out-Null

$requiredFiles = @(
    "README.md",
    "LICENSE",
    "THIRD-PARTY-NOTICES.md",
    "docs/BUILD-AND-RELEASE.md",
    "docs/PROCEDURAL-HAZARDS.md"
)
foreach ($relativePath in $requiredFiles) {
    $source = Join-Path $repositoryRoot $relativePath
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
        throw "Missing required distribution file: $relativePath"
    }
    $destination = Join-Path $stagePath $relativePath
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination -Force
}
Copy-Item -LiteralPath $executable -Destination (Join-Path $stagePath "tunrun.exe") -Force

# Future content packs are included automatically when present. This development
# package does not fabricate assets or pad itself with unused data.
$includedContent = @()
foreach ($contentDirectory in @("assets", "resources")) {
    $source = Join-Path $repositoryRoot $contentDirectory
    if (Test-Path -LiteralPath $source -PathType Container) {
        Copy-Item -LiteralPath $source -Destination (Join-Path $stagePath $contentDirectory) -Recurse -Force
        $includedContent += $contentDirectory
    }
}

if ([string]::IsNullOrWhiteSpace($CommitSha)) {
    $CommitSha = "local-build"
}
$buildInfo = [ordered]@{
    product = "TUNRUN"
    version = $Version
    buildType = "portable-development"
    platform = "windows-x64"
    sourceCommit = $CommitSha
    builtAtUtc = [DateTime]::UtcNow.ToString("o")
    contentDirectoriesIncluded = @($includedContent)
}
$buildInfoPath = Join-Path $stagePath "build-info.json"
$buildInfo | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $buildInfoPath -Encoding utf8

$manifest = Get-ChildItem -LiteralPath $stagePath -File -Recurse | ForEach-Object {
    $relative = $_.FullName.Substring($stagePath.Length).TrimStart([char[]]@('\', '/')).Replace('\', '/')
    [ordered]@{
        path = $relative
        bytes = $_.Length
        sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}
$manifestPath = Join-Path $stagePath "package-manifest.json"
ConvertTo-Json -InputObject @($manifest) -Depth 5 | Set-Content -LiteralPath $manifestPath -Encoding utf8

Compress-Archive -Path (Join-Path $stagePath "*") -DestinationPath $zipPath -CompressionLevel Optimal -Force
$archive = Get-Item -LiteralPath $zipPath
Write-Host ("Created {0} ({1:N0} bytes)" -f $archive.FullName, $archive.Length)
