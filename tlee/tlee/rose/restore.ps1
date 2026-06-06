$shell = New-Object -ComObject Shell.Application
$recycleBin = $shell.Namespace(0x0a)
$items = $recycleBin.Items() | Where-Object { $_.Name -eq 'sdk' }
foreach ($item in $items) {
    $item.InvokeVerb("undelete")
}
