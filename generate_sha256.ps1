# Script pour générer le SHA256 d'un fichier firmware
param(
    [string]$firmwarePath = "firmware.bin"
)

if (-not (Test-Path $firmwarePath)) {
    Write-Host "Erreur: Fichier $firmwarePath non trouvé" -ForegroundColor Red
    exit 1
}

# Calculer SHA256
$hash = Get-FileHash -Path $firmwarePath -Algorithm SHA256
$sha256 = $hash.Hash.ToLower()

Write-Host "SHA256 du firmware:" -ForegroundColor Green
Write-Host $sha256 -ForegroundColor Yellow

# Mettre à jour le manifest.json
$manifestPath = "manifest.json"
if (Test-Path $manifestPath) {
    $manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json
    $manifest.sha256 = $sha256
    $manifest | ConvertTo-Json | Set-Content $manifestPath
    Write-Host "Manifest.json mis à jour" -ForegroundColor Green
} else {
    Write-Host "Création du manifest.json..." -ForegroundColor Yellow
    $manifest = @{
        version = "1.0.0"
        firmware_url = "https://github.com/votre_utilisateur/votre_repo/releases/download/v1.0.0/firmware.bin"
        sha256 = $sha256
        build_date = Get-Date -Format "yyyy-MM-dd"
        description = "Version initiale"
    }
    $manifest | ConvertTo-Json | Set-Content $manifestPath
}

Write-Host "`nCopiez cette valeur dans votre code ESP8266:" -ForegroundColor Cyan
Write-Host "const char* expectedSHA256 = `"$sha256`";" -ForegroundColor Yellow
