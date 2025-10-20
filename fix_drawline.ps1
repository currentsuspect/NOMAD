# Fix drawLine calls in NUIRendererGL.cpp
$content = Get-Content 'NomadUI/Graphics/OpenGL/NUIRendererGL.cpp' -Raw

# Fix the double NUIPoint constructor and parameter order
$content = $content -replace 'drawLine\(NUIPoint\(NUIPoint\(([^)]+)\)\), NUIPoint\(NUIPoint\(([^)]+)\)\), color, lineWidth\)', 'drawLine(NUIPoint($1), NUIPoint($2), lineWidth, color)'

# Fix single NUIPoint calls with wrong parameter order
$content = $content -replace 'drawLine\(NUIPoint\(([^)]+)\), NUIPoint\(([^)]+)\), color, lineWidth\)', 'drawLine(NUIPoint($1), NUIPoint($2), lineWidth, color)'

Set-Content 'NomadUI/Graphics/OpenGL/NUIRendererGL.cpp' -Value $content
Write-Host "Fixed drawLine calls"
