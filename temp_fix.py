
import sys
with open(sys.argv[1], 'rb') as f:
    content = f.read()

marker = b'// Using hardcoded offsets from offsets.h directly'
idx = content.find(marker)
if idx < 0:
    print('ERROR: marker not found')
    sys.exit(1)

end = content.find(b'// Set UTF-8 encoding for cool characters', idx)
if end < 0:
    print('ERROR: end marker not found')
    sys.exit(1)

start = idx

replacement = (
    b'	// Using hardcoded offsets from offsets.h directly
'
    b'	printf("[38;5;45m   [»] [0m[38;5;118m'
    b'Using built-in offsets (version %s)[0m
", '
    b'Offsets::ClientVersion.c_str());
'
    b'
'
)

# Check: replacement should not contain literal ESC (0x1b)
assert b'' in replacement, 'missing text escape'
assert b'' not in replacement, 'has literal control char'

content = content[:start] + replacement + content[end:]
with open(sys.argv[1], 'wb') as f:
    f.write(content)
print('Fixed!')
