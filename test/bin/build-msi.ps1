<#
.SYNOPSIS
    Builds an MSI for mongosqd.exe and mongodrdl.exe.
.DESCRIPTION
    .
.PARAMETER ProgramFilesFolder
    The Program Files folder to use for installation
.PARAMETER ProjectName
    The name to use when referring to the project.
.PARAMETER VersionLabel
    The label to use when referring to this version of the
    project.
.PARAMETER WixPath
    The path to the WIX binaries.
.PARAMETER Arch
    The binary arcitecture we're building: 'x86' or 'x64'.
#>
Param(
  [string]$ProgramFilesFolder,
  [string]$ProjectName,
  [string]$VersionLabel,
  [string]$WixPath,
  [string]$Arch
)

$wixUiExt = "$WixPath\WixUIExtension.dll"
$sourceDir = pwd
$resourceDir = "$sourceDir\installers\msi\mongosql-auth\"
$artifactsDir = "$sourceDir\test\artifacts\"
$objDir = "$artifactsDir\build\"
$libDir = "$artifactsDir\build\"

if (-not ($VersionLabel -match "(\d\.\d).*")) {
    throw "invalid version specified: $VersionLabel"
}
$version = $matches[1]

# unlike the BI Connector itself, only one version of the plugin can be
# installed at a time.
$upgradeCode = "3f021824-c333-49f5-9cbf-d6de9b6adacc"

# compile wxs into .wixobjs
& $WixPath\candle.exe -wx `
    -dProgramFilesFolder="$ProgramFilesFolder" `
    -dProductId="*" `
    -dUpgradeCode="$upgradeCode" `
    -dVersion="$version" `
    -dVersionLabel="$VersionLabel" `
    -dProjectName="$ProjectName" `
    -dSourceDir="$sourceDir" `
    -dResourceDir="$resourceDir" `
    -dSslDir="$binDir" `
    -dLibraryDir="$libDir" `
    -dTargetDir="$objDir" `
    -dTargetExt=".msi" `
    -dTargetFileName="release" `
    -dOutDir="$objDir" `
    -dConfiguration="Release" `
    -arch "$Arch" `
    -out "$objDir" `
    -ext "$wixUiExt" `
    "$resourceDir\Product.wxs" `
    "$resourceDir\FeatureFragment.wxs" `
    "$resourceDir\LibraryFragment.wxs" `
    "$resourceDir\DocumentationFragment.wxs" `
    "$resourceDir\ConfigFragment.wxs" `
    "$resourceDir\UIFragment.wxs"

if(-not $?) {
    exit 1
}

# link wixobjs into an msi
& $WixPath\light.exe -wx `
    -cultures:en-us `
    -out "$artifactsDir\release.msi" `
    -ext "$wixUiExt" `
    $objDir\Product.wixobj `
    $objDir\FeatureFragment.wixobj `
    $objDir\LibraryFragment.wixobj `
    $objDir\DocumentationFragment.wixobj `
    $objDir\ConfigFragment.wixobj `
    $objDir\UIFragment.wixobj

if(-not $?) {
    exit 1
}
