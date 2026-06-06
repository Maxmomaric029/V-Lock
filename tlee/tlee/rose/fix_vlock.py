import os
import shutil

src_dir = r"C:\Users\Mxzzy\Downloads\vertex up no weird shit\tlee\tlee\rose\src"
dst_dir = r"C:\Users\Mxzzy\AppData\Local\Temp\V-Lock\src"

files = [
    r"runtime\memory.cpp",
    r"main.cpp"
]

for f in files:
    src = os.path.join(src_dir, f)
    dst = os.path.join(dst_dir, f)
    shutil.copy2(src, dst)
    print(f"Copied {f}")

# Verify
for f in files:
    dst = os.path.join(dst_dir, f)
    if not os.path.exists(dst):
        print(f"ERROR: {f} not found!")
        continue
    size = os.path.getsize(dst)
    print(f"{f}: {size} bytes")

print("\nDone!")
