param(
    [string]$Glslc = "",
    [string]$OutFile = ""
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
if ($Glslc -eq "") {
    $sdk = $env:VULKAN_SDK
    if ($sdk -and (Test-Path (Join-Path $sdk "Bin\glslc.exe"))) {
        $Glslc = Join-Path $sdk "Bin\glslc.exe"
    } else {
        $Glslc = "glslc"
    }
}
if ($OutFile -eq "") {
    $OutFile = Join-Path $scriptDir "..\vkshaders.h"
}

$tmpDir = Join-Path ([System.IO.Path]::GetTempPath()) ("vkrend_spirv_" + [Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Path $tmpDir | Out-Null

try {
    & $Glslc --target-env=vulkan1.0 -mfmt=c -O -o (Join-Path $tmpDir "present.vert.h") (Join-Path $scriptDir "present.vert")
    if ($LASTEXITCODE -ne 0) { throw "glslc failed for present.vert" }
    & $Glslc --target-env=vulkan1.0 -mfmt=c -O -o (Join-Path $tmpDir "present.frag.h") (Join-Path $scriptDir "present.frag")
    if ($LASTEXITCODE -ne 0) { throw "glslc failed for present.frag" }

    $vert = Get-Content -Raw (Join-Path $tmpDir "present.vert.h")
    $frag = Get-Content -Raw (Join-Path $tmpDir "present.frag.h")

    $vert = $vert -replace '^\s*\{\s*', ''
    $vert = $vert -replace '\s*\}\s*$', ''
    $frag = $frag -replace '^\s*\{\s*', ''
    $frag = $frag -replace '\s*\}\s*$', ''

    $header = @"
#ifndef _VKSHADERS_H_
#define _VKSHADERS_H_

#include <stdint.h>

static const uint32_t VKREND_PRESENT_VERT_SPV[] = {
$vert
};

static const uint32_t VKREND_PRESENT_FRAG_SPV[] = {
$frag
};

#endif
"@
    Set-Content -Path $OutFile -Value $header -Encoding ASCII
    Write-Output "Wrote $OutFile"
} finally {
    Remove-Item -Recurse -Force $tmpDir
}
