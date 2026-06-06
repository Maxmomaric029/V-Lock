import os

def fix_mutex():
    for root, dirs, files in os.walk('src'):
        for f in files:
            if f.endswith('.cpp') or f.endswith('.h'):
                path = os.path.join(root, f)
                with open(path, 'r', encoding='utf-8', errors='ignore') as file:
                    content = file.read()
                
                new_content = content.replace('std::lock_guard<std::mutex>', 'std::lock_guard<std::recursive_mutex>')
                
                if new_content != content:
                    with open(path, 'w', encoding='utf-8') as file:
                        file.write(new_content)
                    print(f"Updated {path}")

fix_mutex()
