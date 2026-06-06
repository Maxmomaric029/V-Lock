$content = Get-Content -Path src\sdk\offsets.h
$ns = ''
$out = ''
foreach ($line in $content) {
    if ($line -match 'namespace\s+([A-Za-z0-9_]+)') {
        if ($matches[1] -ne 'Offsets') {
            $ns = $matches[1]
        }
    } elseif ($line -match 'inline\s+uintptr_t\s+([A-Za-z0-9_]+)') {
        $var = $matches[1]
        $out += "UPDATE_OFFSET($ns, $var);`n"
    }
}
Set-Content -Path temp_update_offsets.txt -Value $out
