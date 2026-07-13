$files = Get-ChildItem -Path src, headers -Include *.c, *.h -Recurse
foreach ($file in $files) {
    $content = Get-Content $file.FullName -Raw
    if ($content -cnotmatch '\bdatatype\b') { continue }
    $modified = $content -creplace '\bdatatype\b', 'DataType'
    if ($content -cne $modified) {
        Set-Content $file.FullName -Value $modified -NoNewline
    }
}
