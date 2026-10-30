<#
.SYNOPSIS
清理本目录 CMake / Ninja / CLion 工程的编译产物：双击BAT直接清理并保留固件，直接运行PS1默认预览。
.DESCRIPTION
清理 build 下经缓存确认的构建子目录，以及根目录 cmake-build-*。
保留 CMakeLists.txt、CMakePresets.json、cmake、generated、启动/链接文件、
驱动库、IDE设置、源码和串口工具。清理前请关闭正在编译或调试的 IDE。
.PARAMETER Clean
实际执行删除。未指定时只显示清单。
.PARAMETER KeepFirmware
保留各构建目录顶层的本项目 .elf、.axf、.hex、.bin 文件。
.EXAMPLE
.\cmakeclean.bat
.EXAMPLE
.\cmakeclean.bat -Preview
.EXAMPLE
.\cmakeclean.bat -Clean
.EXAMPLE
.\cmakeclean.bat -Clean -KeepFirmware
#>
[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'Medium')]
param([switch]$Clean, [switch]$KeepFirmware)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath($PSScriptRoot).TrimEnd('\')
$rootPrefix = $projectRoot + '\'
if ((Get-Item -LiteralPath $projectRoot -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {
    throw '工程根目录是链接或目录联接，已停止清理。'
}

function Assert-InProject([string]$Path) {
    $full = [IO.Path]::GetFullPath($Path)
    if (-not $full.StartsWith($rootPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "目标不在当前工程内：$full"
    }
    return $full
}
function Assert-SafeTree([string]$Path) {
    $full = Assert-InProject $Path
    $item = Get-Item -LiteralPath $full -Force
    if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
        throw "拒绝清理链接或目录联接：$full"
    }
    if ($item.PSIsContainer) {
        foreach ($child in Get-ChildItem -LiteralPath $full -Recurse -Force) {
            $null = Assert-InProject $child.FullName
            if ($child.Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw "目录中存在链接或目录联接，已停止：$($child.FullName)"
            }
        }
    }
}
function Show-Plan($Files, $Folders) {
    $total = [long]0
    foreach ($file in $Files) {
        Write-Host ('  文件  ' + $file.FullName.Substring($rootPrefix.Length))
        $total += $file.Length
    }
    foreach ($folder in $Folders) {
        Write-Host ('  范围  ' + $folder.Substring($rootPrefix.Length))
    }
    Write-Host ('计划删除：{0} 个文件，涉及 {1} 个目录范围，约 {2:N2} MB。' -f $Files.Count, $Folders.Count, ($total / 1MB))
    if (-not $Clean) {
        Write-Host '当前仅预览，没有删除文件。加 -Clean 执行；加 -KeepFirmware 保留固件。' -ForegroundColor Cyan
    }
}

$cmakeLists = Join-Path $projectRoot 'CMakeLists.txt'
if (-not (Test-Path -LiteralPath $cmakeLists -PathType Leaf)) {
    throw '当前目录没有 CMakeLists.txt。'
}
$content = Get-Content -LiteralPath $cmakeLists -Raw -Encoding UTF8
$projectMatch = [regex]::Match($content, '(?im)^\s*project\s*\(\s*([A-Za-z0-9_.+-]+)')
if (-not $projectMatch.Success) { throw '无法确定 CMake 工程名，已停止清理。' }
$projectName = $projectMatch.Groups[1].Value
$firmwareExtensions = @('.axf', '.elf', '.hex', '.bin')

function Test-CMakeOutput([string]$Directory) {
    Assert-SafeTree $Directory
    if (Test-Path -LiteralPath (Join-Path $Directory 'CMakeLists.txt')) {
        Write-Warning "目录含有 CMakeLists.txt，按源码目录保留：$Directory"
        return $false
    }
    $cache = Join-Path $Directory 'CMakeCache.txt'
    if (Test-Path -LiteralPath $cache -PathType Leaf) {
        $cacheText = Get-Content -LiteralPath $cache -Raw
        $nameMatch = [regex]::Match($cacheText, '(?m)^CMAKE_PROJECT_NAME:[^=]+=(.+)$')
        if ($nameMatch.Success -and $nameMatch.Groups[1].Value.Trim() -eq $projectName) {
            return $true
        }
        Write-Warning "缓存不属于本项目，保留：$Directory"
        return $false
    }
    $allFiles = @(Get-ChildItem -LiteralPath $Directory -Recurse -File -Force)
    # 允许重复清理“仅剩本项目固件”的目录，以及空目录。
    if (@($allFiles | Where-Object {
        $_.DirectoryName -ne $Directory -or $_.BaseName -ne $projectName -or
        $_.Extension.ToLowerInvariant() -notin $firmwareExtensions
    }).Count -eq 0) { return $true }
    Write-Warning "目录缺少可确认的 CMake 缓存，保留：$Directory"
    return $false
}
$candidates = @{}
$buildContainer = Join-Path $projectRoot 'build'
if (Test-Path -LiteralPath $buildContainer -PathType Container) {
    Assert-SafeTree $buildContainer
    # 兼容 cmake -B build 与 build/<presetName> 两种布局。
    if (Test-Path -LiteralPath (Join-Path $buildContainer 'CMakeCache.txt') -PathType Leaf) {
        $candidates[$buildContainer] = $true
    } else {
        foreach ($folder in Get-ChildItem -LiteralPath $buildContainer -Directory -Force) {
            $candidates[$folder.FullName] = $true
        }
    }
}
foreach ($folder in Get-ChildItem -LiteralPath $projectRoot -Directory -Force) {
    if ($folder.Name -like 'cmake-build-*') { $candidates[$folder.FullName] = $true }
}
$folders = @()
foreach ($directory in @($candidates.Keys | Sort-Object)) {
    if (Test-CMakeOutput $directory) { $folders += $directory }
}
$files = @()
$keep = @{}
foreach ($directory in $folders) {
    foreach ($file in Get-ChildItem -LiteralPath $directory -Recurse -File -Force) {
        if ($KeepFirmware -and $file.DirectoryName -eq $directory -and
            $file.BaseName -eq $projectName -and
            $file.Extension.ToLowerInvariant() -in $firmwareExtensions) {
            $keep[$file.FullName] = $true
        } else { $files += $file }
    }
}
Write-Host ("CMake 工程：{0}，名称：{1}" -f $projectRoot, $projectName)
if ($KeepFirmware) { Write-Host '保留固件模式：仅保留各构建目录顶层的本项目 AXF / ELF / HEX / BIN。' }
Show-Plan @($files | Sort-Object FullName) $folders
if (-not $Clean) { return }

foreach ($directory in $folders) { Assert-SafeTree $directory }
if (-not $KeepFirmware) {
    foreach ($directory in $folders) {
        # 使用已验证的绝对路径；不拼接 shell 删除命令。
        $null = Assert-InProject $directory
        Assert-SafeTree $directory
        if ($PSCmdlet.ShouldProcess($directory, '删除完整 CMake 构建输出目录')) {
            Remove-Item -LiteralPath $directory -Recurse -Force
        }
    }
} else {
    foreach ($file in $files) {
        Assert-SafeTree $file.FullName
        if ($PSCmdlet.ShouldProcess($file.FullName, '删除 CMake 编译产物或缓存')) {
            Remove-Item -LiteralPath $file.FullName -Force
        }
    }
    foreach ($directory in $folders) {
        $emptyCandidates = @(Get-ChildItem -LiteralPath $directory -Directory -Recurse -Force |
            Sort-Object { $_.FullName.Length } -Descending)
        $emptyCandidates += Get-Item -LiteralPath $directory
        foreach ($folder in $emptyCandidates) {
            if ((Test-Path -LiteralPath $folder.FullName) -and
                @(Get-ChildItem -LiteralPath $folder.FullName -Force).Count -eq 0 -and
                $PSCmdlet.ShouldProcess($folder.FullName, '删除空构建目录')) {
                $null = Assert-InProject $folder.FullName
                Remove-Item -LiteralPath $folder.FullName -Force
            }
        }
    }
}
if ((Test-Path -LiteralPath $buildContainer -PathType Container) -and
    @(Get-ChildItem -LiteralPath $buildContainer -Force).Count -eq 0 -and
    $PSCmdlet.ShouldProcess($buildContainer, '删除空 build 目录')) {
    $null = Assert-InProject $buildContainer
    Remove-Item -LiteralPath $buildContainer -Force
}
if ($WhatIfPreference) { Write-Host '模拟结束，未执行删除。' }
else { Write-Host 'CMake 清理结束。下次构建前请重新运行 CMake 配置或让 CLion 重新加载。' -ForegroundColor Green }
